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

#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <settings/settings.h>

#if defined(CONFIG_DK_LIBRARY)
#include <dk_buttons_and_leds.h>
#endif

#include <drivers/gpio.h>

#include <logging/log.h>

#include "events/ble_event.h"
#include "events/thingy_event.h"
#include "events/wdt_event.h"
#include "thingy_module.h"

#define MODULE thingy_module
LOG_MODULE_REGISTER(MODULE, CONFIG_LOG_DEFAULT_LEVEL);

#define THINGY_NAME CONFIG_THINGY_NAME

#define LED_1 DK_LED1
#define LED_2 DK_LED2
#define LED_3 DK_LED3
#define LED_4 DK_LED4

static K_SEM_DEFINE(ble_ready, 0, 1);

static K_SEM_DEFINE(temperature_received, 0, 1);
static K_SEM_DEFINE(humidity_received, 0, 1);
static K_SEM_DEFINE(air_pressure_received, 0, 1);

static struct k_work_delayable scan_cycle;

//Connection parameters to decrease power consumption
static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(0x140, 0x140, 0, 100);
	//Interval = N * 1.25 ms = 0x140 * 1.25 ms = 400 ms , timeout = N * 10 ms = 1 s

bool configured = false;

// data_array = {<temperature integer>, <temperature decimals>, <humidity percentage}
uint8_t data_array[3];

int32_t pressure_int;
int32_t pressure_float;
uint8_t battery_charge;

struct bt_conn *thingy_conn;
bool thingy_scanning = false;

/* ------------------------- on received notifications ---------------------------------*/
static uint8_t on_received_temperature(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	if (length > 0) {	
		if(configured){
			data_array[0] = ((uint8_t *)data)[0];
			data_array[1] = ((uint8_t *)data)[1];
			k_sem_give(&temperature_received);
			LOG_DBG("K give temperature");
		}
		LOG_INF("Temperature [Celsius]: %d,%d, \n", ((uint8_t *)data)[0], ((uint8_t *)data)[1]);

	} else {
		LOG_WRN("Temperature notification with 0 length\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t on_received_humidity(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	if (length > 0) {
		if(configured){
			data_array[2] = ((uint8_t *)data)[0];
			k_sem_give(&humidity_received);
			LOG_DBG("K give humidity");
		}
		LOG_INF("Relative humidity [%%]: %d \n", ((uint8_t *)data)[0]);

	} else {
		LOG_WRN("Humidity notification with 0 length\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t on_received_air_pressure(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	if (length > 0) {
		LOG_DBG("Air pressure data, length: %i", length);
		if(configured){
			/* Copying the 4 first bytes into an int32_t */
			memcpy(&pressure_int, data, 4);
			LOG_DBG("Air pressure int: %i", pressure_int);
			int32_t pressure_float_long;

			/* Copying the 4 next bytes into another int32_t */
			memcpy(&pressure_float_long, data + 4, 4);
			LOG_DBG("Air pressure float: %i", pressure_float);

			pressure_float = (uint8_t)((float)pressure_float_long/10000);
			LOG_DBG("Air pressure float_8: %i", pressure_float);

			k_sem_give(&air_pressure_received);
			LOG_DBG("K give air");
		}
		LOG_INF("Air Pressure [hPa]: %i,%i \n", pressure_int, pressure_float);
	} else {
		LOG_WRN("Air Pressure notification with 0 length\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t on_received_battery(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	if (length > 0) {
		LOG_INF("on_received_battery(): Battery charge: %i %%\n", ((uint8_t *)data)[0]);
		battery_charge = ((uint8_t *)data)[0];

	} else {
		LOG_WRN("on_received_battery(): Battery notification with 0 length\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

/* Callback for manually reading the battery charge on first connection.
 * Subsequent battery charge data is read through the 
 * on_received_battery(), on notification from Thingy:52. */
uint8_t read_cb (struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length){
						
	if (length > 0){
		LOG_INF("read_cb(): Battery charge: %d %%. \n ", ((uint8_t *)data)[0]);
		battery_charge = ((uint8_t *)data)[0];
		LOG_DBG("read_cb(): Data length: %d. \n", length);
		/* Can't remember why this is here, is it even entered? */
		if(length == 2){
			LOG_INF("read_cb(): Battery charge: %d %%. \n ", ((uint8_t *)data)[1]);
			battery_charge = ((uint8_t *)data)[1];
		}
	} else {
		LOG_WRN("read_cb(): Read data with 0 length. \n");
	}
	return BT_GATT_ITER_CONTINUE;	
}

/* ----------------------- Write to Thingy callbacks -------------------------*/
void write_cb (struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params){
	LOG_INF("Write callback started, %i, length: %i, offset: %i, handle: %i", err, params->length, params->offset, params->handle);

	/* Write function has finished, the Thingy:52 has been configured and will start
	 * to send data. */
	configured = true;
	
	#if defined(CONFIG_THINGY53)
	const struct device *gpio0_dev;
	gpio0_dev = device_get_binding("GPIO_0");
	err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_LOW); //3V3 ENABLE (LOW means on)
	const struct device *gpio1_dev;
	gpio1_dev = device_get_binding("GPIO_1");
	err = gpio_pin_configure(gpio1_dev, 6, GPIO_OUTPUT_HIGH); //Green led	
	
	k_sleep(K_MSEC(500));
	
	err = gpio_pin_configure(gpio1_dev, 6, GPIO_OUTPUT_LOW);  //Green led	
	err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_HIGH); //3V3 ENABLE (HIGH means off)
	#endif
	/* Signal that the Thingy module is done to allow the next module to run.
	 * (Currently the peripheral module.) */
    struct ble_event *thingy_ready = new_ble_event();

	thingy_ready->type = THINGY_READY;

	APP_EVENT_SUBMIT(thingy_ready);

}

struct bt_gatt_write_params params;
const struct bt_gatt_dm_attr *chrc;
const struct bt_gatt_dm_attr *desc;

// Write to Thingy to update configuration
static void discovery_write_completed(struct bt_gatt_dm *disc, void *ctx){
	int err;

	char delay[1] = {THINGY_SAMPLE_RATE};
	
	LOG_INF("%i", delay[0]);


	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_WRITE);
	if (!chrc) {
		LOG_WRN("Missing Thingy configuration characteristic\n");
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_WRITE);
	if (!desc) {
	 	LOG_WRN("Missing Thingy configuration char value descriptor\n");
	}

	params.func = write_cb;
	params.data = delay;
	params.handle = desc->handle;
	params.offset = 0;
	params.length = 1;
	
	err = bt_gatt_write(thingy_conn, &params);
	
	LOG_INF("Releasing write discovery\n");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_ERR("Could not release humidity discovery data, err: %d\n", err);
	}

}
static void discovery_write_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_WRN("Thingy write service not found!\n");
	
}

static void discovery_write_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_ERR("The write discovery procedure failed, err %d\n", err);
}

static void write_to_characteristic_gattp(struct bt_conn *conn){
	int err;

	// ----------------------- Write to Thingy ---------------------------
    LOG_INF("Entering TES service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_BEE, &discovery_write_cb, NULL);
	if (err) {
		LOG_ERR("Could not start write service discovery, err %d\n", err);
	}
}

/* Function for subscribing to all the Thingy:52's sensor characteristics. */
static void discovery_bee_completed(struct bt_gatt_dm *disc, void *ctx)
{
	int err;
	LOG_DBG("Discovery_bee_completed");
	/* Must be statically allocated */
	/* Subscribing to temperature characteristic */
	static struct bt_gatt_subscribe_params temperature_param = {
		.notify = on_received_temperature,
	};
	temperature_param.value = BT_GATT_CCC_NOTIFY;

	const struct bt_gatt_dm_attr *chrc;
	const struct bt_gatt_dm_attr *desc;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_TEMP_SENSOR);
	if (!chrc) {
		LOG_WRN("Missing Thingy temperature characteristic\n");
		goto release;
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_TEMP_SENSOR);
	if (!desc) {
		LOG_WRN("Missing Thingy temperature char value descriptor");
		goto release;
	}

	LOG_DBG("temp_param.value handle: %x", desc->handle);
	temperature_param.value_handle = desc->handle,

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_WRN("Missing Thingy temperature char CCC descriptor");
		goto release;
	}

	temperature_param.ccc_handle = desc->handle;
	LOG_DBG("temp_param.ccc handle: %x", desc->handle);

	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &temperature_param);
	if (err) {
		LOG_ERR("Subscribe to temperature service failed (err %d)\n", err);
	}

	LOG_INF("Temperature discovery completed");
	
	/* Must be statically allocated */
	/* Subscribing to humidity characteristic */
	static struct bt_gatt_subscribe_params humidity_param = {
		.notify = on_received_humidity,
	};
	humidity_param.value = BT_GATT_CCC_NOTIFY;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_HUMID_SENSOR);
	if (!chrc) {
		LOG_WRN("Missing Thingy humidity characteristic\n");
		goto release;
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_HUMID_SENSOR);
	if (!desc) {
		LOG_WRN("Missing Thingy humidity char value descriptor\n");
		goto release;
	}

	LOG_DBG("humidity_param.value handle: %x", desc->handle);
	humidity_param.value_handle = desc->handle,

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_WRN("Missing Thingy humidity char CCC descriptor\n");
		goto release;
	}

	LOG_DBG("humidity_param.ccc handle: %x", desc->handle);
	humidity_param.ccc_handle = desc->handle;

    err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &humidity_param);
    if (err) {
        LOG_WRN("Subscribe to humidity service failed (err %d)\n", err);
    }
	LOG_INF("humidity discovery completed\n");
	
	/* Must be statically allocated */
	/* Subscribing to air_pressure characteristic */
	static struct bt_gatt_subscribe_params air_pres_param = {
		.notify = on_received_air_pressure,
	};
	air_pres_param.value = BT_GATT_CCC_NOTIFY;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_PRESSURE_SENSOR);
	if (!chrc) {
		LOG_WRN("Missing Thingy air pressrue characteristic\n");
		goto release;
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_PRESSURE_SENSOR);
	if (!desc) {
		LOG_WRN("Missing Thingy air pressure char value descriptor\n");
		goto release;
	}

	air_pres_param.value_handle = desc->handle,

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_WRN("Missing Thingy air pressure char CCC descriptor\n");
		goto release;
	}

	LOG_DBG("air_pres_param.ccc handle: %x", desc->handle);
	air_pres_param.ccc_handle = desc->handle;

    err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &air_pres_param);
    if (err) {
        LOG_ERR("Subscribe to air pressure service failed (err %d)\n", err);
    }

	LOG_INF("Air pressure discovery completed\n");
	
	/* Must be statically allocated */
	/* Subscribing to battery characteristic */

	static struct bt_gatt_subscribe_params battery_param = {
		.notify = on_received_battery,
	};

	
	const struct bt_gatt_dm_attr *chrc_bat;
	const struct bt_gatt_dm_attr *desc_bat;

	battery_param.value = BT_GATT_CCC_NOTIFY;

	chrc_bat = bt_gatt_dm_char_by_uuid(disc, BT_UUID_BATTERY);
	if (!chrc_bat) {
		LOG_WRN("Missing Thingy battery characteristic");
		goto release;
	}

	desc_bat = bt_gatt_dm_desc_by_uuid(disc, chrc_bat, BT_UUID_BATTERY);
	if (!desc_bat) {
		LOG_WRN("Missing Thingy battery char value desc_batriptor");
		goto release;
	}
	
	static struct bt_gatt_read_params read_params = {
		.func = read_cb,
		.handle_count = 1,
		.single.offset = 0,
	};

	LOG_DBG("read handle: %x", desc_bat->handle);
	battery_param.value_handle = desc_bat->handle,
	read_params.single.handle = desc->handle+2,

	desc_bat = bt_gatt_dm_desc_by_uuid(disc, chrc_bat, BT_UUID_GATT_CCC);
	if (!desc_bat) {
		LOG_WRN("Missing Thingy battery char CCC desc_batriptor");
		goto release;
	}
	LOG_DBG("desc_bat.ccc handle: %x", desc_bat->handle);

	battery_param.ccc_handle = desc_bat->handle;

	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &battery_param);
	if (err) {
		LOG_ERR("Subscribe to battery service failed (err %d)", err);
	}

	LOG_INF("Battery discovery completed");

	LOG_INF("Reading initial battery charge");
	err = bt_gatt_read(bt_gatt_dm_conn_get(disc), &read_params);
	if(err){
		LOG_ERR("bt_gatt_read, Error: %i", err);
	}
	
release:
	
	LOG_DBG("Releasing battery discovery");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_ERR("Could not release battery discovery data, err: %d", err);
	}
	
	write_to_characteristic_gattp(bt_gatt_dm_conn_get(disc));
}

static void discovery_bee_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_WRN("Thingy battery service not found!");
}

static void discovery_bee_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_ERR("The battery discovery procedure failed, err %d", err);
}

static void discover_thingy_characteristics_gattp(struct bt_conn *conn)
{
	int err;

    LOG_INF("Entering bee service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_BEE, &discovery_bee_cb, NULL);
	if (err) {
		LOG_INF("Could not start bee service discovery, err %d\n", err);
	}
	LOG_INF("Gatt bee DM started with code: %i\n", err);
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
		
    if(conn == thingy_conn){
		#if defined(CONFIG_THINGY53)
		const struct device *gpio0_dev;
		gpio0_dev = device_get_binding("GPIO_0");
		err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_LOW); //3V3 ENABLE (LOW means on)
		const struct device *gpio1_dev;
		gpio1_dev = device_get_binding("GPIO_1");
		err = gpio_pin_configure(gpio1_dev, 7, GPIO_OUTPUT_HIGH); //Blue led	
		if(err){
			sys_reboot(SYS_REBOOT_COLD);
		}

		k_sleep(K_MSEC(500));
		err = gpio_pin_configure(gpio1_dev, 7, GPIO_OUTPUT_LOW);  //Blue led	
		if(err){
			sys_reboot(SYS_REBOOT_COLD);
		}
		err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_HIGH); //3V3 ENABLE (HIGH means off)
		#endif
        
		LOG_INF("connected(): Thingy:52	Connected.");
        
		/* Thingy:52 is connected, stop trying to scan for it. */
		k_work_cancel_delayable(&scan_cycle);
		thingy_scanning = false;
        
		thingy_conn = bt_conn_ref(conn);
		
		#if defined(CONFIG_DK_LIBRARY)
        LOG_DBG("Setting LED 1 Status for successful connection with T:52.");
        dk_set_led_on(LED_1);
		
		#endif
        
		LOG_DBG("connected(): Starting Thingy:52 service discovery chain.");
        /* Starts the service discovery chain*/
        discover_thingy_characteristics_gattp(thingy_conn);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if(conn==thingy_conn){
		#if defined(CONFIG_DK_LIBRARY)
        LOG_INF("LED 1 toggled off. Thingy:52  disconnected. \n");
        dk_set_led_off(LED_1);
		#endif
        bt_conn_unref(thingy_conn);
        thingy_conn = NULL;

		/* Start scanning for the Thingy:52 periodically. */
        struct ble_event *thingy_scan = new_ble_event();

        thingy_scan->type = SCAN_START;
        thingy_scan->scan_name = THINGY_NAME;
        thingy_scan->len = strlen(THINGY_NAME);

        APP_EVENT_SUBMIT(thingy_scan);

        thingy_scanning = true;

		k_work_reschedule(&scan_cycle, K_MINUTES(1));
    }
}
#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u\n", log_strdup(addr),
			level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d\n", log_strdup(addr),
			level, err);
	}
}
#endif

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif
};

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", log_strdup(addr));
}


static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", log_strdup(addr),
		bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing failed conn: %s, reason %d", log_strdup(addr),
		reason);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

/* Schedulable function for periodically scanning for the Thingy:52. */
static void scan_cycle_fn(struct k_work *work){
	if(thingy_conn){
		LOG_INF("Thingy:52 allready connected, canceling scan cycle.");
		return;
	}
	struct ble_event *thingy_scan = new_ble_event();

    thingy_scan->type = SCAN_START;
    thingy_scan->scan_name = THINGY_NAME;
    thingy_scan->len = strlen(THINGY_NAME);

    APP_EVENT_SUBMIT(thingy_scan);

    thingy_scanning = true;

	
	k_work_reschedule(&scan_cycle, K_MINUTES(1));
	LOG_INF("Queing scan in 1 minute.");
}

void thingy_module_thread_fn(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);
	
	LOG_INF("thingy_module_thread_fn(): Waiting for sem ble_ready, K_FOREVER. \n");
    k_sem_take(&ble_ready, K_FOREVER);
    LOG_INF("thingy_module_thread_fn(): ble_ready. \n");

    struct ble_event *thingy_scan = new_ble_event();

    thingy_scan->type = SCAN_START;
    thingy_scan->scan_name = THINGY_NAME;
    thingy_scan->len = strlen(THINGY_NAME);

    APP_EVENT_SUBMIT(thingy_scan);

    thingy_scanning = true;

	k_work_init_delayable(&scan_cycle, scan_cycle_fn);

	for(;;){
		LOG_INF("Waiting for k_sem_take Thingy measurments");
		k_sem_take(&temperature_received, K_FOREVER);
		k_sem_take(&humidity_received, K_FOREVER);
		k_sem_take(&air_pressure_received, K_FOREVER);
	
		/* Send Thingy:52 data to the peripheral module, where it is processed. */
		struct thingy_event *thingy_send = new_thingy_event();

		thingy_send->data_array[0] = data_array[0];
		thingy_send->data_array[1] = data_array[1];
		thingy_send->data_array[2] = data_array[2];
		thingy_send->pressure_int = pressure_int;
		thingy_send->pressure_float = pressure_float;
		thingy_send->battery_charge = battery_charge;
 
		APP_EVENT_SUBMIT(thingy_send);
		LOG_INF("thingy_module_thread_fn(): thingy_send event submitted. \n");
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_ble_event(eh)) {
		struct ble_event *event = cast_ble_event(eh);
		if(event->type==BLE_READY){
			LOG_INF("event_handler(): BLE ready");
			k_sem_give(&ble_ready);
			return false;
		}
		if(event->type==SCAN_SUCCES){
			int err;
			if(thingy_scanning){
				/* Thingy:52 found, establishing connection. */
				struct bt_conn_le_create_param *conn_params;		
				conn_params = BT_CONN_LE_CREATE_PARAM(
						BT_CONN_LE_OPT_CODED | BT_CONN_LE_OPT_NO_1M,
						BT_GAP_SCAN_FAST_INTERVAL,
						BT_GAP_SCAN_FAST_INTERVAL);

				err = bt_conn_le_create(event->addr, conn_params,
					BT_LE_CONN_PARAM_DEFAULT,
					&thingy_conn);

				if (err) {
					LOG_ERR("Create conn failed (err %d)\n", err);
					//TODO: Schedule scan
					return false;
				}
			}
			return false;
		}
		return false;
	}
	return false;
}

K_THREAD_DEFINE(thingy_module_thread, 1024,
		thingy_module_thread_fn, NULL, NULL, NULL,
		K_HIGHEST_APPLICATION_THREAD_PRIO + 1, 0, 0);

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_event);