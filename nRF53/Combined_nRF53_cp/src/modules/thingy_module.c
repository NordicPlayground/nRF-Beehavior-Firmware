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
#include <sys/printk.h>

#include <event_manager.h>

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

#include "events/ble_event.h"
#include "events/thingy_event.h"
// #include "events/bm_w_event.h"

// #include "ble.h"

static K_SEM_DEFINE(ble_ready, 0, 1);
static K_SEM_DEFINE(temperature_received, 0, 1);
static K_SEM_DEFINE(humidity_received, 0, 1);
static K_SEM_DEFINE(air_pressure_received, 0, 1);

bool configured = false;

uint8_t data_array[3];

int32_t pressure_int;
uint8_t pressure_float;

#define MODULE thingy_module
LOG_MODULE_REGISTER(MODULE, 4);

static struct bt_conn *thingy_conn;

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

				/* Thingy Environment User Interface UUID */
#define BT_UUID_UIS                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x03, 0x68, 0xEF)

				/* Thingy Environment LED UUID */
#define BT_UUID_LED                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x01, 0x03, 0x68, 0xEF)

struct ble_tes_color_config_t
{
    uint8_t  led_red;
    uint8_t  led_green;
    uint8_t  led_blue;
};

struct ble_tes_config_t
{
    uint16_t                temperature_interval_ms;
    uint16_t                pressure_interval_ms;
    uint16_t                humidity_interval_ms;
    uint16_t                color_interval_ms;
    uint8_t                 gas_interval_mode;
    struct ble_tes_color_config_t  color_config;
};

bool first = false;

/* -------------- Temperature headers and cb -------------------- */
static void discovery_temperature_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_temperature_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_temperature_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_temperature_cb = {
	.completed = discovery_temperature_completed,
	.service_not_found = discovery_temperature_service_not_found,
	.error_found = discovery_temperature_error_found,
};

/* -------------- Humidity headers and cb -------------------- */
static void discovery_humidity_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_humidity_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_humidity_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_humidity_cb = {
	.completed = discovery_humidity_completed,
	.service_not_found = discovery_humidity_service_not_found,
	.error_found = discovery_humidity_error_found,
};

/* -------------- Air pressure discovery headers and cb --------------------*/
static void discovery_air_pressure_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_air_pressure_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_air_pressure_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_air_pressure_cb = {
	.completed = discovery_air_pressure_completed,
	.service_not_found = discovery_air_pressure_service_not_found,
	.error_found = discovery_air_pressure_error_found,
};

/* ------------------ Orientation headers and cb --------------------
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


/* ------------------------ Declaration of connection and gattp functions ----------------------*/
static void connected(struct bt_conn *conn, uint8_t conn_err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);

static void discover_temperature_gattp(struct bt_conn *conn);
static void discover_humidity_gattp(struct bt_conn *conn);
static void discover_air_pressure_gattp(struct bt_conn *conn);
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
		if(configured){
			data_array[0] = ((uint8_t *)data)[0];
			data_array[1] = ((uint8_t *)data)[1];
			k_sem_give(&temperature_received);
		}
		LOG_INF("Temperature [Celsius]: %d,%d, \n", ((uint8_t *)data)[0], ((uint8_t *)data)[1]);

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
		if(configured){
			data_array[2] = ((uint8_t *)data)[0];
			k_sem_give(&humidity_received);
		}
		LOG_INF("Relative humidity [%%]: %d \n", ((uint8_t *)data)[0]);

	} else {
		LOG_INF("Humidity notification with 0 length\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t on_received_air_pressure(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	if (length > 0) {
		if(configured){
			pressure_int = ((int32_t *)data)[0];
			pressure_float = ((uint8_t *)data)[1];
			k_sem_give(&air_pressure_received);
		}
		LOG_INF("Air Pressure [hPa]: %d,%d \n", ((int32_t *)data)[0], ((uint8_t *)data)[1]);
		LOG_INF("Air Pressure [hPa]: %d,%d \n", pressure_int, pressure_float);

	} else {
		LOG_INF("Air Pressure notification with 0 length\n");
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

void write_to_led_cb (struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params){
	LOG_INF("Write callback started, %i, length: %i, offset: %i, handle: %i", err, params->length, params->offset, params->handle);

	configured = true;

    struct ble_event *thingy_ready = new_ble_event();

	thingy_ready->type = THINGY_READY;

	EVENT_SUBMIT(thingy_ready);
	// char *test = params->data;
	// LOG_INF("%.12s", log_strdup(test));
}

static void discovery_write_to_led_completed(struct bt_gatt_dm *disc, void *ctx){
	

	uint8_t data[1] = {0x00};

	const struct bt_gatt_dm_attr *chrc;
	const struct bt_gatt_dm_attr *desc;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_LED);
	if (!chrc) {
		LOG_INF("Missing Thingy configuration characteristic\n");
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_LED);
	if (!desc) {
	 	LOG_INF("Missing Thingy configuration char value descriptor\n");
	}
	struct bt_gatt_write_params params;
	params.func = write_to_led_cb;
	params.data = data;
	uint16_t test = 0;
	params.handle = desc->handle;
	LOG_INF("Handle: %i", desc->handle);
	params.offset = 0;
	params.length = 1;
	int err = bt_gatt_write(bt_gatt_dm_conn_get(disc), &params);
	//int err = bt_gatt_write_without_response(bt_gatt_dm_conn_get(disc), desc->handle, data, 12, 0);
	LOG_INF("Error: %i", err);
	
	LOG_INF("Releasing write discovery\n");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_INF("Could not release write to led discovery data, err: %d\n", err);
	}

}
static void discovery_write_to_led_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_INF("Thingy write service not found!\n");
}

static void discovery_write_to_led_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_INF("The write discovery procedure failed, err %d\n", err);
}

static struct bt_gatt_dm_cb discovery_write_to_led_cb = {
	.completed = discovery_write_to_led_completed,
	.service_not_found = discovery_write_to_led_service_not_found,
	.error_found = discovery_write_to_led_error_found,
};

static void write_to_led_gattp(struct bt_conn *conn){
	int err;

	// ----------------------- Write to Thingy ---------------------------
    LOG_INF("Entering TES service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_UIS, &discovery_write_to_led_cb, NULL);
	if (err) {
		LOG_INF("Could not start write  to led service discovery, err %d\n", err);
	}
	LOG_INF("Gatt write DM started with code: %i\n", err);
}


/* ----------------------- Write to Thingy callbacks -------------------------*/
void write_cb (struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params){
	LOG_INF("Write callback started, %i, length: %i, offset: %i, handle: %i", err, params->length, params->offset, params->handle);

}

static void discovery_write_completed(struct bt_gatt_dm *disc, void *ctx){
	//Write to Thingy to update configuration

	char data[12] = { 0x60,0xEA,0x60,0xEA,0x60,0xEA,0x10,0x27,0x03,0xFF,0x00,0x00};

	LOG_INF("%d", data[0]);

	const struct bt_gatt_dm_attr *chrc;
	const struct bt_gatt_dm_attr *desc;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_TECC);
	if (!chrc) {
		LOG_INF("Missing Thingy configuration characteristic\n");
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_TECC);
	if (!desc) {
	 	LOG_INF("Missing Thingy configuration char value descriptor\n");
	}

	struct bt_gatt_write_params params;
	params.func = write_cb;
	params.data = data;
	uint16_t test = 0;
	params.handle = desc->handle;
	LOG_INF("Handle: %i", desc->handle);
	params.offset = 0;
	params.length = 12;
	int err = bt_gatt_write(bt_gatt_dm_conn_get(disc), &params);
	
	LOG_INF("Error: %i", err);
	
	LOG_INF("Releasing write discovery\n");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_INF("Could not release humidity discovery data, err: %d\n", err);
	}
	write_to_led_gattp(bt_gatt_dm_conn_get(disc));

}
static void discovery_write_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_INF("Thingy write service not found!\n");
}

static void discovery_write_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_INF("The write discovery procedure failed, err %d\n", err);
}

static struct bt_gatt_dm_cb discovery_write_cb = {
	.completed = discovery_write_completed,
	.service_not_found = discovery_write_service_not_found,
	.error_found = discovery_write_error_found,
};

static void write_to_characteristic_gattp(struct bt_conn *conn){
	int err;

	// ----------------------- Write to Thingy ---------------------------
    LOG_INF("Entering TES service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_TES, &discovery_write_cb, NULL);
	if (err) {
		LOG_INF("Could not start write service discovery, err %d\n", err);
	}
	LOG_INF("Gatt write DM started with code: %i\n", err);
}

/****************************** Discovery functions *******************/

/* -------------------- Temperature discovery functions ---------------------*/
static void discovery_temperature_completed(struct bt_gatt_dm *disc, void *ctx)
{
	int err;

	/* Must be statically allocated */
	static struct bt_gatt_subscribe_params param = {
		.notify = on_received_temperature,
	};
	param.value = BT_GATT_CCC_NOTIFY;

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

	// if(!resubscribe){
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


/* ---------- Humidity discovery functions ------------------- */
static void discovery_humidity_completed(struct bt_gatt_dm *disc, void *ctx)
{
	int err;

	/* Must be statically allocated */
	static struct bt_gatt_subscribe_params param = {
		.notify = on_received_humidity,
	};
	param.value = BT_GATT_CCC_NOTIFY;

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

	// if(!resubscribe){
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
	discover_air_pressure_gattp(bt_gatt_dm_conn_get(disc));
}

static void discovery_humidity_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_INF("Thingy humidity service not found!\n");
}

static void discovery_humidity_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_INF("The humidity discovery procedure failed, err %d\n", err);
}

/* -------------------- Air Pressure discovery functions  ------------------------*/
static void discovery_air_pressure_completed(struct bt_gatt_dm *disc, void *ctx)
{
	int err;

	/* Must be statically allocated */
	static struct bt_gatt_subscribe_params param = {
		.notify = on_received_air_pressure,
	};
	param.value = BT_GATT_CCC_NOTIFY;

	const struct bt_gatt_dm_attr *chrc;
	const struct bt_gatt_dm_attr *desc;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_TPC);
	if (!chrc) {
		LOG_INF("Missing Thingy air pressrue characteristic\n");
		goto release;
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_TPC);
	if (!desc) {
		LOG_INF("Missing Thingy air pressure char value descriptor\n");
		goto release;
	}

	param.value_handle = desc->handle,

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_INF("Missing Thingy air pressure char CCC descriptor\n");
		goto release;
	}

	param.ccc_handle = desc->handle;

	// if(!resubscribe){
		err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param);
		if (err) {
			LOG_INF("Subscribe to air pressure service failed (err %d)\n", err);
		}

	LOG_INF("Air pressure discovery completed\n");
release:
	LOG_INF("Releasing air pressure discovery\n");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_INF("Could not release air pressure discovery data, err: %d\n", err);
	}
	discover_orientation_gattp(bt_gatt_dm_conn_get(disc));
}

static void discovery_air_pressure_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_INF("Thingy air pressure service not found!\n");
}

static void discovery_air_pressure_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_INF("The air pressure discovery procedure failed, err %d\n", err);
}

/* ---------- Orientation discovery functions */
static void discovery_orientation_completed(struct bt_gatt_dm *disc, void *ctx)
{
	int err;

	/* Must be statically allocated */
	static struct bt_gatt_subscribe_params param = {
		.notify = on_received_orientation,
	};
	param.value = BT_GATT_CCC_NOTIFY;

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

	// if(!resubscribe){
		err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param);
		if (err) {
			LOG_INF("Subscribe to orientation service failed (err %d)\n", err);
		}
		// resubscribe = true;
	// }
	// else{
	// 	err = bt_gatt_resubscribe(0, bt_gatt_dm_conn_get(disc), &param);
	// 	if (err) {
	// 		LOG_INF("Resubscribe to orientation service failed (err %d)\n", err);
	// 	}
	// }

	LOG_INF("Orientation discovery completed\n");

release:
	LOG_INF("Releasing orientation discovery\n");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_INF("Could not release orientation discovery data, err: %d\n", err);
	}
	write_to_characteristic_gattp(bt_gatt_dm_conn_get(disc));
}

static void discovery_orientation_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_INF("Thingy orientation service not found!\n");
}

static void discovery_orientation_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_INF("The orientation discovery procedure failed, err %d\n", err);
}

/* ----------------------- gattp functions ------------------------*/ 
static void discover_temperature_gattp(struct bt_conn *conn)
{

	int err;

    LOG_INF("Entering TES service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_TES, &discovery_temperature_cb, NULL);
	if (err) {
		LOG_INF("Could not start temperature service discovery, err %d\n", err);
	}
	LOG_INF("Gatt temperature DM started with code: %i\n", err);
}

static void discover_humidity_gattp(struct bt_conn *conn)
{
	int err;

    LOG_INF("Entering TES service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_TES, &discovery_humidity_cb, NULL);
	if (err) {
		LOG_INF("Could not start humidity service discovery, err %d\n", err);
	}
	LOG_INF("Gatt humidity DM started with code: %i\n", err);
}

static void discover_air_pressure_gattp(struct bt_conn *conn)
{
	int err;

    LOG_INF("Entering TES service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_TES, &discovery_air_pressure_cb, NULL);
	if (err) {
		LOG_INF("Could not start air pressure service discovery, err %d\n", err);
	}
	LOG_INF("Gatt air pressure DM started with code: %i\n", err);
}

static void discover_orientation_gattp(struct bt_conn *conn)
{
	int err;

    LOG_INF("Entering TMS service bt_gatt_dm_start;\n");
	err = bt_gatt_dm_start(conn, BT_UUID_TMS, &discovery_orientation_cb, NULL);
	if (err) {
		LOG_INF("Could not start motion service discovery, err %d\n", err);
	}
	LOG_INF("Gatt motion DM started with code: %i\n", err);
}

/*------------------------- Connectivity, scanning and pairing functions -------------------------- */
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if(!strncmp(addr, "06:09:16:57:01:FD", 17)){
		LOG_INF("NO! Not for you!");
		int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		LOG_INF("Bluetooth disconnected with error code: %i", err);
		return;
	}

	if (conn_err) {
		LOG_INF("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	struct bt_conn_info conn_info;
	
	bt_conn_get_info(conn, &conn_info);

	LOG_INF("Type: %i, Role: %i, Id: %i", conn_info.type, conn_info.role, conn_info.id);

	if(!conn_info.role){
		LOG_INF("Thingy:52 Connected");
		thingy_conn = bt_conn_ref(conn);
	}
	else{
		LOG_INF("Connected to central hub");
	}

	LOG_INF("Connected: %.17s", log_strdup(addr));
	LOG_INF("Starting Thingy:52 service discovery chain. \n");
	LOG_INF("Discovering temperature service:"); /* Starts the service discovery chain*/
	discover_temperature_gattp(conn);
}

static int scan_init(bool first);

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if(!strncmp(addr, "06:09:16:57:01:FD", 17)){
		
		struct ble_event *bm_w_read = new_ble_event();

		bm_w_read->type = BM_W_READ;

		EVENT_SUBMIT(bm_w_read);
		return;
	}

	LOG_INF("Disconnected: %s (reason %u)\n", log_strdup(addr),	reason);

	err = bt_gatt_disconnected(conn);
	LOG_INF("Gatt cleared: %i", err);

	scan_init(false);

	bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);

	if (thingy_conn != conn) {
		LOG_INF("Central hub disconnected");
		return;
	}

	bt_conn_unref(thingy_conn);
	thingy_conn = NULL;
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
}

static int scan_init(bool first)
{
	int err;
	if(first){
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, "T52And2");
		if (err) {
			LOG_ERR("Scanning filters cannot be set (err %d)\n", err);
			return err;
		}
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

	LOG_INF("Pairing failed conn: %s, reason %d", log_strdup(addr),
		reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

/* ------------------------ main() -----------------------*/
void thingy_module_thread_fn(void)
{
	int err;
	
	LOG_INF("STOP!... (Wait for BLE to be ready)\n");
    k_sem_take(&ble_ready, K_FOREVER);
    LOG_INF("HAMMERTIME! (BLE ready.\n");
	
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization callbacks.");
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	err = scan_init(true);
	if(err){
		LOG_INF("Failed to initialize scan: %i", err);
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_INF("Scanning successfully started");
	LOG_INF("Scanning for Thingy:52 with name: %d", BT_SCAN_FILTER_TYPE_NAME);

	for(;;){
		k_sem_take(&temperature_received, K_FOREVER);
		k_sem_take(&humidity_received, K_FOREVER);
		k_sem_take(&air_pressure_received, K_FOREVER);
	
		struct thingy_event *thingy_send = new_thingy_event();

		thingy_send->data_array[0] = data_array[0];
		thingy_send->data_array[1] = data_array[1];
		thingy_send->data_array[2] = data_array[2];
		thingy_send->pressure_int = pressure_int;
		thingy_send->pressure_float = pressure_float;

		EVENT_SUBMIT(thingy_send);
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_ble_event(eh)) {

		struct ble_event *event = cast_ble_event(eh);
		if(event->type==BLE_READY){
			LOG_INF("BLE ready");
			k_sem_give(&ble_ready);
			return false;
		}
		return false;
	}
    
	return false;
}

K_THREAD_DEFINE(thingy_module_thread, 1024,
		thingy_module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);


EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, ble_event);
EVENT_SUBSCRIBE(MODULE, thingy_event);
// EVENT_SUBSCRIBE(MODULE, bm_w_event);