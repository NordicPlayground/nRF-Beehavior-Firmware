
#include <errno.h>
#include <sys/reboot.h>
#include <zephyr.h>
#include <stdio.h>

#include <app_event_manager.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <settings/settings.h>
#if defined(CONFIG_DK_LIBRARY)
#include <dk_buttons_and_leds.h>
#endif

#include <logging/log.h>

#include "events/ble_event.h"
#include "events/bee_count_event.h"


#define MODULE bee_counter_module
LOG_MODULE_REGISTER(MODULE, CONFIG_LOG_DEFAULT_LEVEL);

#define BEE_COUNTER_NAME CONFIG_BEE_COUNTER_NAME

static K_SEM_DEFINE(peripheral_done, 0, 1);

static struct bt_conn *bee_count_conn;

static bool bee_count_scanning = false;

static struct bt_nus_client nus_client; //Handles communication for the bee_conn

static void bee_discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	struct bt_nus_client *nus = context;
	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	/* Subscribe to the NUS_RX and NUS_TX to receive and send messages over Bluetooth. */
	bt_nus_handles_assign(dm, nus);
	bt_nus_subscribe_receive(nus);

	bt_gatt_dm_data_release(dm);

}

static void bee_discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_WRN("Service not found");
}

static void bee_discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_ERR("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb bee_discovery_cb = {
	.completed         = bee_discovery_complete,
	.service_not_found = bee_discovery_service_not_found,
	.error_found       = bee_discovery_error,
};

/* -------------- Gatt discover for the Bee Counter -------------------- */
static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != bee_count_conn) {
		return;
	}

	/* Attempt to subscribe to the NUS service. */
	err = bt_gatt_dm_start(conn,
			       BT_UUID_NUS_SERVICE,
			       &bee_discovery_cb,
			       &nus_client);
	if (err) {
		LOG_ERR("could not start the discovery procedure, error "
			"code: %d", err);
	}
}

static uint8_t ble_data_received(const uint8_t *const data, uint16_t len)
{
	LOG_INF("Received data from the Bee Counter");
	
	char out_arr[2];
	char in_arr[2];
	for (uint8_t i = 0; i < 4; i++){
		out_arr[i] = data[i];
		in_arr[i] = data[i+2];	
	}
	uint16_t totalOut;
	uint16_t totalIn;

	memcpy(&totalOut, out_arr, sizeof(totalOut));
	memcpy(&totalIn, in_arr, sizeof(totalIn));

	LOG_INF("Total out: %d, Total in: %d", totalOut, totalIn);

	/* Send the data to the peripheral module, so that it can be sent to the nRF9160. */
	struct bee_count_event *bc_send = new_bee_count_event();

	bc_send->out = totalOut;
	bc_send->in = totalIn;
	
	APP_EVENT_SUBMIT(bc_send);

	return BT_GATT_ITER_CONTINUE;
}

static int nus_client_init(void)
{
	int err;
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
		}
	};

	err = bt_nus_client_init(&nus_client, &init);
	if (err) {
		LOG_ERR("NUS Client initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("NUS Client module initialized");
	return err;
}

/*------------------------- Connectivity, scanning and pairing functions -------------------------- */
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;

	char addr[BT_ADDR_LE_STR_LEN];
	
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_ERR("connected(): Failed to connect to %s (%u).", addr, conn_err);
		return;
	}

	struct bt_conn_info conn_info;
	
	bt_conn_get_info(conn, &conn_info);

	LOG_DBG("connected(): Type: %i, Role: %i, Id: %i.", conn_info.type, conn_info.role, conn_info.id);
		
    if(conn == bee_count_conn){
        LOG_INF("connected(): Bee Counter Connected.");
	
		bee_count_scanning = false;

        bee_count_conn = bt_conn_ref(conn);
        gatt_discover(conn);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	/* Check if the disconnected unit is the bee counter. */
    if(conn==bee_count_conn){

        LOG_INF("Bee counter disconnected.");
        bt_conn_unref(bee_count_conn);
        bee_count_conn = NULL;

		/* Tell the scan module to start scanning for the bee counter. */
        struct ble_event *bee_count_scan = new_ble_event();

        bee_count_scan->type = SCAN_START;
        bee_count_scan->scan_name = BEE_COUNTER_NAME;
        bee_count_scan->len = strlen(BEE_COUNTER_NAME);

        APP_EVENT_SUBMIT(bee_count_scan);

        bee_count_scanning = true;
    }
}
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", log_strdup(addr),
			level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", log_strdup(addr),
			level, err);
	}
}
static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param){
	LOG_DBG("%d, %d",param->interval_min,param->interval_max);
	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout){
	LOG_DBG("Param updated %d, %d, %d",interval,latency, timeout);			
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

void bee_counter_module_thread_fn(void)
{
	int err;
	
	LOG_DBG("bee_counter_module_thread_fn(): Waiting for sem peripheral_done, 120 seconds.");
	err = k_sem_take(&peripheral_done, K_SECONDS(120));
    LOG_DBG("bee_counter_module_thread_fn(): peripheral done advertising.");

	bt_conn_cb_register(&conn_callbacks);
	
   	/* Tell the scan module to start scanning for the bee counter. */
	struct ble_event *bee_scan = new_ble_event();

    bee_scan->type = SCAN_START;
    bee_scan->scan_name = BEE_COUNTER_NAME;
    bee_scan->len = strlen(BEE_COUNTER_NAME);

    APP_EVENT_SUBMIT(bee_scan);

    bee_count_scanning = true;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_ble_event(eh)) {

		struct ble_event *event = cast_ble_event(eh);
		if(event->type==HUB_CONNECTED){
			LOG_INF("event_handler(): Thingy connected");
			k_sem_give(&peripheral_done);
			return false;
		}
		int err;
		
		if(event->type==SCAN_SUCCES){
			if(bee_count_scanning){
				/* Bee counter found, creating a connection. */
				struct bt_conn_le_create_param *conn_params;		
				conn_params = BT_CONN_LE_CREATE_PARAM(
						BT_CONN_LE_OPT_CODED | BT_CONN_LE_OPT_NO_1M,
						BT_GAP_SCAN_FAST_INTERVAL,
						BT_GAP_SCAN_FAST_INTERVAL);

				err = bt_conn_le_create(event->addr, conn_params,
					BT_LE_CONN_PARAM_DEFAULT,
					&bee_count_conn);

				if (err) {
					LOG_ERR("Create conn failed (err %d)\n", err);
					//Schedule scan
					return false;
				}
			}
			return false;
		}
		return false;
	}
    
	return false;
}

K_THREAD_DEFINE(bee_counter_module_thread, 1024,
		bee_counter_module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO + 3, 0, 0);

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_event);