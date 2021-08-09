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

#include <dk_buttons_and_leds.h>

#include <drivers/uart.h>

#include <logging/log.h>

#include "events/ble_event.h"
#include "central_module.h"
#include "led/led.h"
#if defined(CONFIG_THINGY_ENABLE)
#include "events/thingy_event.h"
#endif
#if defined(CONFIG_BEE_COUNTER_ENABLE)
#include "events/bee_count_event.h"
#endif
#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
#include "events/bm_w_event.h"
#endif




// #define MODULE central_module
// LOG_MODULE_REGISTER(MODULE, 4);

// /* ----------------------- Thingy declaration and initialization -------------------------
// This could probably be put in a central_module.h

// */
// static K_SEM_DEFINE(ble_ready, 0, 1);
// static K_SEM_DEFINE(peripheral_done, 0, 1);
// #if defined(CONFIG_BEE_COUNTER_ENABLE)
// static K_SEM_DEFINE(bee_count_done, 0, 1);
// #endif
// #if defined(CONFIG_THINGY_ENABLE)
// static K_SEM_DEFINE(temperature_received, 0, 1);
// static K_SEM_DEFINE(humidity_received, 0, 1);
// static K_SEM_DEFINE(air_pressure_received, 0, 1);

// bool configured = false;

// uint8_t data_array[3];

// int32_t pressure_int;
// uint8_t pressure_float;

// static struct bt_conn *thingy_conn;
// #endif

// #if defined(CONFIG_BEE_COUNTER_ENABLE)
// #define BEE_COUNTER CONFIG_BEE_COUNTER_NAME

// static struct bt_conn *bee_conn;

// static struct bt_nus_client nus_client; //Handles communication for the bee_conn
// #endif

// K_SEM_DEFINE(service_ready, 0, 1)

// #if defined(CONFIG_THINGY_ENABLE)
// #define THINGY CONFIG_THINGY_NAME
// /* Thinghy advertisement UUID */
// #define BT_UUID_THINGY                                                         \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x01, 0x68, 0xEF)

// /* Thingy Motion service UUID */
// #define BT_UUID_TMS                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x04, 0x68, 0xEF)

// /* Thingy Orientation characteristic UUID */
// #define BT_UUID_TOC                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x03, 0x04, 0x68, 0xEF)

// /* Thingy environment service - EF68xxxx-9B35-4933-9B10-52FFA9740042 */
// 					/* Thingy Environment service UUID */
// #define BT_UUID_TES                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x02, 0x68, 0xEF)

// /*				Thingy Temperature characteristic UUID		*/
// #define BT_UUID_TTC                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x01, 0x02, 0x68, 0xEF)


// /*				Thingy Pressure characteristic UUID - 12 bytes			*/
// #define BT_UUID_TPC                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x02, 0x02, 0x68, 0xEF)

// 		/*		Thingy Humidity characteristic UUID		*/
// #define BT_UUID_THC                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x03, 0x02, 0x68, 0xEF)


// 		/*		Thingy Environment Configuration characteristic UUID - 12 bytes */
// #define BT_UUID_TECC                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x06, 0x02, 0x68, 0xEF)

// 				/* Thingy Environment User Interface UUID */
// #define BT_UUID_UIS                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x03, 0x68, 0xEF)

// 				/* Thingy Environment LED UUID */
// #define BT_UUID_LED                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x01, 0x03, 0x68, 0xEF)

// #define BT_UUID_BTRY                                                            \
// 	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
// 			    0x33, 0x49, 0x35, 0x9B, 0x0F, 0x18, 0x68, 0xEF)

// struct ble_tes_color_config_t
// {
//     uint8_t  led_red;
//     uint8_t  led_green;
//     uint8_t  led_blue;
// };

// struct ble_tes_config_t
// {
//     uint16_t                temperature_interval_ms;
//     uint16_t                pressure_interval_ms;
//     uint16_t                humidity_interval_ms;
//     uint16_t                color_interval_ms;
//     uint8_t                 gas_interval_mode;
//     struct ble_tes_color_config_t  color_config;
// };

// bool thingy_scan = true;
// #endif

#if defined(CONFIG_BEE_COUNTER_ENABLE)
static void bee_discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	struct bt_nus_client *nus = context;
	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	bt_nus_handles_assign(dm, nus);
	bt_nus_subscribe_receive(nus);

	bt_gatt_dm_data_release(dm);

	k_sem_give(&bee_count_done);

	struct ble_event *bc_ready = new_ble_event();

	bc_ready->type = BEE_COUNTER_READY;

	EVENT_SUBMIT(bc_ready);
}

static void bee_discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_INF("Service not found");
}

static void bee_discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

// struct bt_gatt_dm_cb bee_discovery_cb = {
// 	.completed         = bee_discovery_complete,
// 	.service_not_found = bee_discovery_service_not_found,
// 	.error_found       = bee_discovery_error,
// };

/* -------------- Gatt discover for the Bee Counter -------------------- */
static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != bee_conn) {
		return;
	}

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

	struct bee_count_event *bc_send = new_bee_count_event();

	bc_send->out = totalOut;
	bc_send->in = totalIn;
	
	EVENT_SUBMIT(bc_send);

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
#endif

// #if defined(CONFIG_THINGY_ENABLE)
// /* -------------- Temperature headers and cb -------------------- */
// static void discovery_temperature_completed(struct bt_gatt_dm *disc, void *ctx);
// static void discovery_temperature_service_not_found(struct bt_conn *conn, void *ctx);
// static void discovery_temperature_error_found(struct bt_conn *conn, int err, void *ctx);

// static struct bt_gatt_dm_cb discovery_temperature_cb = {
// 	.completed = discovery_temperature_completed,
// 	.service_not_found = discovery_temperature_service_not_found,
// 	.error_found = discovery_temperature_error_found,
// };

// /* -------------- Humidity headers and cb -------------------- */
// static void discovery_humidity_completed(struct bt_gatt_dm *disc, void *ctx);
// static void discovery_humidity_service_not_found(struct bt_conn *conn, void *ctx);
// static void discovery_humidity_error_found(struct bt_conn *conn, int err, void *ctx);

// static struct bt_gatt_dm_cb discovery_humidity_cb = {
// 	.completed = discovery_humidity_completed,
// 	.service_not_found = discovery_humidity_service_not_found,
// 	.error_found = discovery_humidity_error_found,
// };

// /* -------------- Air pressure discovery headers and cb --------------------*/
// static void discovery_air_pressure_completed(struct bt_gatt_dm *disc, void *ctx);
// static void discovery_air_pressure_service_not_found(struct bt_conn *conn, void *ctx);
// static void discovery_air_pressure_error_found(struct bt_conn *conn, int err, void *ctx);

// static struct bt_gatt_dm_cb discovery_air_pressure_cb = {
// 	.completed = discovery_air_pressure_completed,
// 	.service_not_found = discovery_air_pressure_service_not_found,
// 	.error_found = discovery_air_pressure_error_found,
// };

// /* ------------------ Orientation headers and cb --------------------
// 	This can be left out or used for alarm purposes, i.e "Notify when sensor is moving and trigger "hive has been
// 	toppled"- alarm.

// */
// static void discovery_orientation_completed(struct bt_gatt_dm *disc, void *ctx);
// static void discovery_orientation_service_not_found(struct bt_conn *conn, void *ctx);
// static void discovery_orientation_error_found(struct bt_conn *conn, int err, void *ctx);

// static struct bt_gatt_dm_cb discovery_orientation_cb = {
// 	.completed = discovery_orientation_completed,
// 	.service_not_found = discovery_orientation_service_not_found,
// 	.error_found = discovery_orientation_error_found,
// };

// /* ------------------------ Declaration of connection and gattp functions ----------------------*/
// static void discover_temperature_gattp(struct bt_conn *conn);
// static void discover_humidity_gattp(struct bt_conn *conn);
// static void discover_air_pressure_gattp(struct bt_conn *conn);
// static void discover_orientation_gattp(struct bt_conn *conn);
// // static void discover_battery_gattp(struct bt_conn *conn);
// #endif

// static void connected(struct bt_conn *conn, uint8_t conn_err);
// static void disconnected(struct bt_conn *conn, uint8_t reason);
// static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);


// // ------------------ Connected struct
// static struct bt_conn_cb conn_callbacks = {
// 	.connected = connected,
// 	.disconnected = disconnected,
// 	.security_changed = security_changed
// };

// #if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
// /* ----------------------- BM_W Initialization and declarations  -------------------------*/

// static struct k_work_delayable weight_interval;
// static struct k_work_delayable temperature_interval;

// // #define LOG_MODULE_NAME bm_w_module
// // LOG_MODULE_REGISTER(LOG_MODULE_NAME, 4);

// #define REAL_TIME_WEIGHT 0x16
// #define BROODMINDER_ADDR ((bt_addr_le_t[]) { { 0, \
// 			 { { 0xFD, 0x01, 0x57, 0x16, 0x09, 0x06 } } } })
// #define BROODMINDER_ADDR_TEMPERATURE ((bt_addr_le_t[]) { { 0, \
// 			 { { 0x93, 0x05, 0x47, 0x16, 0x09, 0x06 } } } })

// #endif
// // // #define USE_BMW;
// // // #define USE_TEMPERATURE;

// #if defined(CONFIG_THINGY_ENABLE)
// static int scan_init(bool first);
// #endif
// #if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
// static int scan_init_bm(bool first);
// #endif
// #if defined(CONFIG_BEE_COUNTER_ENABLE)
// static int bee_scan_init(bool first);
// #endif

#if defined(CONFIG_THINGY_ENABLE)
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
			LOG_INF("K give temperature");
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
			LOG_INF("K give humidity");
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
			LOG_INF("K give air");
		}
		LOG_INF("Air Pressure [hPa]: %d,%d \n", ((int32_t *)data)[0], ((uint8_t *)data)[1]);
		// LOG_INF("Air Pressure [hPa]: %d,%d \n", pressure_int, pressure_float);

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
	
	LOG_INF("Releasing write to led discovery\n");
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

	
	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param);
	if (err) {
		LOG_INF("Subscribe to orientation service failed (err %d)\n", err);
	}
	
	LOG_INF("Orientation discovery completed\n");

release:
	LOG_INF("Releasing orientation discovery\n");
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_INF("Could not release orientation discovery data, err: %d\n", err);
	}
	write_to_characteristic_gattp(bt_gatt_dm_conn_get(disc));
	// discover_battery_gattp(bt_gatt_dm_conn_get(disc));
	
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

#endif

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		LOG_INF("MTU exchange done");
	} else {
		LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	}
}

/*------------------------- Connectivity, scanning and pairing functions -------------------------- */
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;

	char addr[BT_ADDR_LE_STR_LEN];
	
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
	if(!strncmp(addr, "06:09:16:57:01:FD", 17)){
		LOG_INF("connected(): Weight manually stopped from connecting");
		int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		LOG_INF("connected(): Bluetooth disconnected with error code: %i", err);
		return;
	}
	#endif

	if (conn_err) {
		LOG_INF("connected(): Failed to connect to %s (%u). \n", addr, conn_err);
		return;
	}

	struct bt_conn_info conn_info;
	
	bt_conn_get_info(conn, &conn_info);

	LOG_INF("connected(): Type: %i, Role: %i, Id: %i. \n", conn_info.type, conn_info.role, conn_info.id);
		
	LOG_INF("connected(): Connected: %.17s", log_strdup(addr));

	if(conn_info.type){
		#if defined(CONFIG_THINGY_ENABLE)
		if(thingy_scan){
			LOG_INF("connected(): Thingy:52	Connected.");
			thingy_scan = false;
			thingy_conn = bt_conn_ref(conn);
			LOG_INF("Setting LED 1 Status for successful connection with T:52. \n");
			dk_set_led_on(LED_1);
			LOG_INF("connected(): Starting Thingy:52 service discovery chain. \n");
			LOG_INF("connected(): Discovering temperature service: "); /* Starts the service discovery chain*/
			discover_temperature_gattp(conn);
		}
		else{
		#endif
		#if defined(CONFIG_BEE_COUNTER_ENABLE)
			LOG_INF("connected(): Bee Counter Connected.");
	
			static struct bt_gatt_exchange_params exchange_params;

			// exchange_params.func = exchange_func;
			// err = bt_gatt_exchange_mtu(conn, &exchange_params);
			// if (err) {
			// 	LOG_WRN("MTU exchange failed (err %d)", err);
			// }

			bee_conn = bt_conn_ref(conn);
			gatt_discover(conn);
		#endif
		#if defined(CONFIG_THINGY_ENABLE)
		}
		#endif
	}
	else{
		LOG_INF("connected(): Connected to central hub (91). \n");
		LOG_INF("Setting LED 2 for successful connection with 91. \n");
		dk_set_led_on(LED_2);
	}
}

// static int scan_init(bool first); Decleared at the top

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
	if(!strncmp(addr, "06:09:16:57:01:FD", 17)){
		
		struct ble_event *bm_w_read = new_ble_event();

		bm_w_read->type = BM_W_READ;

		EVENT_SUBMIT(bm_w_read);
		return;
	}
	#endif

	LOG_INF("disconnected(): Bluetooth disconnection occured: %s (reason %u)\n", log_strdup(addr),	reason);
	struct bt_conn_info conn_info;
	
	bt_conn_get_info(conn, &conn_info);
	// if(!strncmp(addr, "DF:91:24:65:5F:88", 17)){ // This must be change to not be hard coded
	if(!conn_info.role){
		#if defined(CONFIG_THINGY_ENABLE)
		if(conn==thingy_conn){
			LOG_INF("LED 1 toggled off. Thingy:52  disconnected. \n");
			dk_set_led_off(LED_1);
			bt_conn_unref(thingy_conn);
			thingy_conn = NULL;
			err = bt_gatt_disconnected(conn);
			LOG_INF("disconnected(): Gatt cleared: %i. \n", err);
			scan_init(false);
			//Start scan
		}
		#endif
		#if defined(CONFIG_BEE_COUNTER_ENABLE)
		if(conn==bee_conn){
			LOG_INF("Bee Counter disconnected.");
			bt_conn_unref(bee_conn);
			bee_conn = NULL;
			err = bt_gatt_disconnected(conn);
			LOG_INF("disconnected(): Gatt cleared: %i. \n", err);
			bee_scan_init(false);
			//Start scan
		}
		#endif
	}
	else{
		LOG_INF("Hub/nRF91 disconnected.");
		//Start adv
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
}

#if defined(CONFIG_THINGY_ENABLE)
static int scan_init(bool first)
/* 
Scan init (bool first); 
If its the first time the machine is running: Add the scan type through bt_scan_filter_add,
else: enable the filter.
 */
{
	int err;
	if(first){
		LOG_INF("scan_init(): Inside 'first' condition. \n");
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, THINGY);
		if (err) {
			LOG_ERR("scan_init(): Scanning filters cannot be set (err %d). \n", err);
			return err;
		}
	}
	err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
	if (err) {
		LOG_ERR("scan_init(): Filters cannot be turned on (err %d). \n", err);
		return err;
	}

	LOG_INF("scan_init(): Scan module initialized. \n");
	return err;
}
#endif

#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
static int scan_init_bm(bool first){
	int err = 0;

    LOG_INF("Changing filters. \n");

	if(first){
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, BROODMINDER_ADDR);
		if (err){
			LOG_INF("Filters cannot be set (err %d)\n", err);
			return err;
		}
	}

    LOG_INF("Checkpoint 1");
	
	err = bt_scan_filter_enable(BT_SCAN_ADDR_FILTER, false);
    if (err) {
        LOG_INF("Filters cannot be turned on (err %d)\n", err);
        return err;
	}

	//LOG_INF("Scan module initialized\n");
    return err;
}
#endif

#if defined(CONFIG_BEE_COUNTER_ENABLE)
static int bee_scan_init(bool first)
{
	int err;
	if(first){
		// err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, "T52And2");
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, BEE_COUNTER);
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
#endif

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

// static struct bt_conn_auth_cb conn_auth_callbacks = {
// 	.cancel = auth_cancel,
// 	.pairing_confirm = pairing_confirm,
// 	.pairing_complete = pairing_complete,
// 	.pairing_failed = pairing_failed
// };

#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
static void ble_scan_start_fn(struct k_work *work)
{
	LOG_INF("Scanning for bm_w starting.");
	LOG_INF("LED 3 toggled while scanning. \n");
	dk_set_led_on(LED_3);

	int err = scan_init_bm(false);
	if(err){
		LOG_INF("Scanning for bm_w failed to initialize");
		LOG_INF("LED 3 toggled off.\n");
		dk_set_led_off(LED_3);
		// return;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_INF("Scanning for bm_w failed to start (err %d)", err);
		LOG_INF("LED 3 toggled off.\n");
		dk_set_led_off(LED_3);
		// return;
	}
	// struct ble_event *bm_w_read = new_ble_event();

	// bm_w_read->type = BM_W_READ;

	// EVENT_SUBMIT(bm_w_read);
	k_work_reschedule(&weight_interval ,K_MINUTES(1));
}
#endif

/* ------------------------ main() -----------------------*/
void central_module_thread_fn(void)
{
	int err;
	
	LOG_INF("thingy_module_thread_fn(): Waiting for sem ble_ready, K_FOREVER. \n");
    k_sem_take(&ble_ready, K_FOREVER);
    LOG_INF("thingy_module_thread_fn(): ble_ready. \n");
	
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("thingy_module_thread_fn(): Failed to register authorization callbacks. \n");
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	
	#if defined(CONFIG_THINGY_ENABLE)
	err = scan_init(true);
	if(err){
		LOG_INF("thingy_module_thread_fn(): Failed to initialize scan: %i.  \n", err);
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("thingy_module_thread_fn(): Scanning failed to start (err %d). \n", err);
		return;
	}

	LOG_INF("thingy_module_thread_fn(): Scanning successfully started. \n");
	LOG_INF("thingy_module_thread_fn(): Scanning for Thingy:52: \n", strlen(THINGY), THINGY);
    // bm module thread fn sketch
	LOG_INF("Waiting for thingy_done semaphore.");
	#else
	struct ble_event *thingy_ready = new_ble_event();

	thingy_ready->type = THINGY_READY;

	EVENT_SUBMIT(thingy_ready);
	#endif

	k_sem_take(&peripheral_done, K_SECONDS(120));
		
	#if defined(CONFIG_BEE_COUNTER_ENABLE)
	LOG_INF("Starting scan for BeeCounter");
	err = bee_scan_init(true);
	if(err){
		LOG_INF("Failed to initialize bee_scan: %i", err);
	}

	nus_client_init();

	// bt_scan_stop();


	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_INF("STOP!... (Wait for bee_count_done semaphore)\n");
    k_sem_take(&bee_count_done, K_SECONDS(30));
	#endif
	
	#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
    LOG_INF("HAMMERTIME! (Bee counter is ready) \n");

	err = scan_init_bm(true);
	if(err){
		LOG_INF("Scanning failed to initialize\n");
		return;
	}
	
    LOG_INF("Checkpoint 2 \n");

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_INF("Scanning for bm_w failed to start (err %d)\n", err);
		LOG_INF("LED 3 toggled off.\n");
		dk_set_led_off(LED_3);
	}

	LOG_INF("Scanning for Broodminder succesfully started\n");
	LOG_INF("LED 3 toggled while scanning for BM_Weight. \n");
	dk_set_led_on(LED_3);
	
	k_work_init_delayable(&weight_interval, ble_scan_start_fn);

	k_work_reschedule(&weight_interval ,K_MINUTES(1));
	#endif

	
	#if defined(CONFIG_THINGY_ENABLE)
	for(;;){
		LOG_INF("Waiting for k_sem_take Thingy measurments");
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
		LOG_INF("thingy_module_thread_fn(): thingy_send event submitted. \n");

	}
	#endif
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

		if(event->type==HUB_CONNECTED){
			LOG_INF("event_handler(): Thingy connected\n");
			k_sem_give(&peripheral_done);
			return false;
		}
		return false;
	}
    
	return false;
}

K_THREAD_DEFINE(central_module_thread, 1024,
		central_module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);


EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, ble_event);
EVENT_SUBSCRIBE(MODULE, thingy_event);
// EVENT_SUBSCRIBE(MODULE, bm_w_event);