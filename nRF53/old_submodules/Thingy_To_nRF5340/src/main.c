/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <errno.h>
#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/printk.h>
#include <stdio.h>
#include <string.h>

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

#include <drivers/uart.h>

#include <logging/log.h>

// #include "ble.h"

#define LOG_MODULE_NAME central_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME, 4);

static struct bt_conn *default_conn;

K_SEM_DEFINE(service_ready, 0, 1)

/* Thinghy advertisement UUID */
#define BT_UUID_THINGY                                                         \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x01, 0x68, 0xEF)

/* Thingy Motion service UUID */
#define BT_UUID_TMS                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x04, 0x68, 0xEF)

/* Thingy Orientation characteristic UUID */
#define BT_UUID_TOC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x03, 0x04, 0x68, 0xEF)

// Thingy environment service - EF68xxxx-9B35-4933-9B10-52FFA9740042
					/* Thingy Environment service UUID */
#define BT_UUID_TES                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x02, 0x68, 0xEF)
			
				// Thingy Temperature characteristic UUID 
#define BT_UUID_TTC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x01, 0x02, 0x68, 0xEF)


				// Thingy Pressure characteristic UUID - 12 bytes
#define BT_UUID_TPC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x02, 0x02, 0x68, 0xEF)

				// Thingy Humidity characteristic UUID 
#define BT_UUID_THC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x03, 0x02, 0x68, 0xEF)


				// Thingy Environment Configuration characteristic UUID - 12 bytes
#define BT_UUID_TECC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x06, 0x02, 0x68, 0xEF)



/* -------------- Temperature stuff */
static void discovery_temperature_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_temperature_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_temperature_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_temperature_cb = {
	.completed = discovery_temperature_completed,
	.service_not_found = discovery_temperature_service_not_found,
	.error_found = discovery_temperature_error_found,
};

/* -------------- Humidity stuff */
static void discovery_humidity_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_humidity_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_humidity_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_humidity_cb = {
	.completed = discovery_humidity_completed,
	.service_not_found = discovery_humidity_service_not_found,
	.error_found = discovery_humidity_error_found,
};

/* ------------------ Orientation stuff --------------------
	This can be left out or used for alarm purposes, i.e "Notify when sensor is moving and trigger "hive has been
	toppled"- alarm.

*/
static void discovery_orientation_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_orientation_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_orientation_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_orientation_cb = {
	.completed = discovery_orientation_completed,
	.service_not_found = discovery_orientation_service_not_found,
	.error_found = discovery_orientation_error_found,
};


/* ------------------------ Connected stuff ----------------------*/
static void connected(struct bt_conn *conn, uint8_t conn_err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);

static void discover_temperature_gattp(struct bt_conn *conn);
static void discover_humidity_gattp(struct bt_conn *conn);
static void discover_orientation_gattp(struct bt_conn *conn);

// ------------------ Connected struct
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

/* ------------------------- on received notifications ---------------------------------*/
static uint8_t on_received_temperature(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	if (length > 0) {
		LOG_INF("Temperature: %d, %d, %s\n", ((uint8_t *)data)[0], ((uint8_t *)data)[1]," degrees Celsius.\n");

	} else {
		LOG_INF("Temperature notification with 0 length\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t on_received_humidity(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	if (length > 0) {
		LOG_INF("Humidity: %d percent\n", ((uint8_t *)data)[0]);

	} else {
		LOG_INF("Himidity notification with 0 length\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t on_received_orientation(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	if (length > 0) {
		LOG_INF("Orientation: %x\n", ((uint8_t *)data)[0]);

	} else {
		LOG_INF("Orientation notification with 0 length\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

/* ---------- Temperature discovery functions */
static void discovery_temperature_completed(struct bt_gatt_dm *disc, void *ctx)
{
	int err;
	/* Must be statically allocated */
	static struct bt_gatt_subscribe_params param = {
		.notify = on_received_temperature,
		.value = BT_GATT_CCC_NOTIFY,
	};

	const struct bt_gatt_dm_attr *chrc;
	const struct bt_gatt_dm_attr *desc;
	
	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_TTC);
	if (!chrc) {
		LOG_INF("Missing Thingy temperature characteristic\n");
		goto release;
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_TTC);
	if (!desc) {
		LOG_INF("Missing Thingy temperature char value descriptor\n");
		goto release;
	}

	param.value_handle = desc->handle,

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_INF("Missing Thingy temperature char CCC descriptor\n");
		goto release;
	}

	param.ccc_handle = desc->handle;

	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param);
	if (err) {
		LOG_INF("Subscribe to temperature service failed (err %d)\n", err);
	}

	LOG_INF("Temperature discovery completed\n");
release:
	LOG_INF("Releasing temperature discovery\n");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_INF("Could not release temperature discovery data, err: %d\n", err);
	}

	// Discover the other services here
	discover_humidity_gattp(bt_gatt_dm_conn_get(disc));
	
}


static void discovery_temperature_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_INF("Thingy temperature service not found!\n");
}

static void discovery_temperature_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_INF("The temperature discovery procedure failed, err %d\n", err);
}


/* ---------- Humidity discovery functions  */
static void discovery_humidity_completed(struct bt_gatt_dm *disc, void *ctx)
{
	int err;
	/* Must be statically allocated */
	static struct bt_gatt_subscribe_params param = {
		.notify = on_received_humidity,
		.value = BT_GATT_CCC_NOTIFY,
	};

	const struct bt_gatt_dm_attr *chrc;
	const struct bt_gatt_dm_attr *desc;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_THC);
	if (!chrc) {
		LOG_INF("Missing Thingy humidity characteristic\n");
		goto release;
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_THC);
	if (!desc) {
		LOG_INF("Missing Thingy humidity char value descriptor\n");
		goto release;
	}

	param.value_handle = desc->handle,

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_INF("Missing Thingy humidity char CCC descriptor\n");
		goto release;
	}

	param.ccc_handle = desc->handle;

	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param);
	if (err) {
		LOG_INF("Subscribe to humidity service failed (err %d)\n", err);
	}

	LOG_INF("humidity discovery completed\n");
release:
	LOG_INF("Releasing humidity discovery\n");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_INF("Could not release humidity discovery data, err: %d\n", err);
	}
	discover_orientation_gattp(bt_gatt_dm_conn_get(disc));
}

static void discovery_humidity_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_INF("Thingy orientation service not found!\n");
}

static void discovery_humidity_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_INF("The orientation discovery procedure failed, err %d\n", err);
}


/* ---------- Orientation discovery functions */
static void discovery_orientation_completed(struct bt_gatt_dm *disc, void *ctx)
{
	int err;
	/* Must be statically allocated */
	static struct bt_gatt_subscribe_params param = {
		.notify = on_received_orientation,
		.value = BT_GATT_CCC_NOTIFY,
	};

	const struct bt_gatt_dm_attr *chrc;
	const struct bt_gatt_dm_attr *desc;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_TOC);
	if (!chrc) {
		LOG_INF("Missing Thingy orientation characteristic\n");
		goto release;
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_TOC);
	if (!desc) {
		LOG_INF("Missing Thingy orientation char value descriptor\n");
		goto release;
	}

	param.value_handle = desc->handle,

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_INF("Missing Thingy orientation char CCC descriptor\n");
		goto release;
	}

	param.ccc_handle = desc->handle;

	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param);
	if (err) {
		LOG_INF("Subscribe to orientation service failed (err %d)\n", err);
	}

	LOG_INF("orientation discovery completed\n");
release:
	LOG_INF("Releasing orientation discovery\n");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_INF("Could not release orientation discovery data, err: %d\n", err);
	}
}

static void discovery_orientation_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_INF("Thingy orientation service not found!\n");
}

static void discovery_orientation_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_INF("The orientation discovery procedure failed, err %d\n", err);
}

// Commented due to declaration at top
// static struct bt_gatt_dm_cb discovery_cb = {
// 	.completed = discovery_completed,
// 	.service_not_found = discovery_service_not_found,
// 	.error_found = discovery_error_found,
// };

static void discover_temperature_gattp(struct bt_conn *conn)
{	
	// k_sem_give(&service_ready);
	int err;
	// char addr[BT_ADDR_LE_STR_LEN];

		// ----------------------- TES Service ---------------------------
    LOG_INF("Entering TES service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_TES, &discovery_temperature_cb, NULL);
	if (err) {
		LOG_INF("Could not start temperature service discovery, err %d\n", err);
	}
	LOG_INF("Gatt temperature DM started with code: %i\n", err);
}

static void discover_humidity_gattp(struct bt_conn *conn)
{	
	// k_sem_give(&service_ready);
	int err;
	// char addr[BT_ADDR_LE_STR_LEN];

		// ----------------------- TES Service ---------------------------
    LOG_INF("Entering TES service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_TES, &discovery_humidity_cb, NULL);
	if (err) {
		/* This might be unnecessary since bt_gatt_dm_start has already been done once 
		with discovery_temperature_cb?
		*/
		LOG_INF("Could not start humidityservice discovery, err %d\n", err);
	}
	LOG_INF("Gatt humidity DM started with code: %i\n", err);
}

static void discover_orientation_gattp(struct bt_conn *conn)
{	
	// k_sem_give(&service_ready);
	int err;
	// char addr[BT_ADDR_LE_STR_LEN];
	
	// ----------------------- TMS Service ---------------------------
    LOG_INF("Entering TMS service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_TMS, &discovery_orientation_cb, NULL);
	if (err) {
		LOG_INF("Could not start motion service discovery, err %d\n", err);
	}
	LOG_INF("Gatt motion DM started with code: %i\n", err);
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	// int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_INF("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}
	LOG_INF("Connected: %.17s\n", log_strdup(addr));

	LOG_INF("Discovering temperature service:");
	discover_temperature_gattp(conn);
	// discover_humidity_gattp(conn);
	// How to make sure temp service is released first?
	// if (k_sem_take(&service_ready, K_FOREVER) != 0) {
    //     printk("Input data not available!");
	LOG_INF("Discovering orientation service:");
	// discover_orientation_gattp(conn);

}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)\n", log_strdup(addr),
		reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)\n",
			err);
	}
}

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
	//gatt_discover(conn);
}

// Commented due to declaration at top
// static struct bt_conn_cb conn_callbacks = {
// 	.connected = connected,
// 	.disconnected = disconnected,
// 	.security_changed = security_changed
// };

// ----------------------------- Scanning and pairing -------------------------
static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d\n",
		log_strdup(addr), connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed\n");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static int scan_init(void)
{
	int err;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	// err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, "57:01:FD");
	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, "Thingy");
	if (err) {
		LOG_ERR("Scanning filters cannot be set (err %d)\n", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)\n", err);
		return err;
	}

	LOG_INF("Scan module initialized.\n");
	return err; 
}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", log_strdup(addr));
}


static void pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_auth_pairing_confirm(conn);

	LOG_INF("Pairing confirmed: %s", log_strdup(addr));
}


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

	LOG_WRN("Pairing failed conn: %s, reason %d", log_strdup(addr),
		reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

void main(void)
{
	int err;

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization callbacks.");
		return;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	// bt_conn_cb_register(&conn_temperature_callbacks);
	// bt_conn_cb_register(&conn_orientation_callbacks);

	bt_conn_cb_register(&conn_callbacks);

	int (*module_init[])(void) = {scan_init}; //uart_init, , nus_client_init
	for (size_t i = 0; i < ARRAY_SIZE(module_init); i++) {
		err = (*module_init[i])();
		if (err) {
			return;
		}
	}

	LOG_INF("Starting Bluetooth Central UART example\n");


	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_INF("Scanning successfully started");
	LOG_INF("Scanning for name: %d", BT_SCAN_FILTER_TYPE_NAME);

	/*for (;;) {
		Wait indefinitely for data to be sent over Bluetooth 
		struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data,
						     K_FOREVER);

		err = bt_nus_client_send(&nus_client, buf->data, buf->len);
		if (err) {
			LOG_WRN("Failed to send data over BLE connection"
				"(err %d)", err);
		}

		err = k_sem_take(&nus_write_sem, NUS_WRITE_TIMEOUT);
		if (err) {
			LOG_WRN("NUS send timeout");
		}
	}*/
}
