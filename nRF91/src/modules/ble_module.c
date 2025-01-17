/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <errno.h>
#include <zephyr.h>
#include <init.h>
#include <stdio.h>
#include <app_event_manager.h>
#include <sys/byteorder.h>
#include <sys/printk.h>

#define MODULE ble_module

#include "modules_common.h"
#include "events/ble_event.h"
#include "events/cloud_event.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <bluetooth/conn_ctx.h>
#include <stdlib.h>
#include <stdio.h>

#include <settings/settings.h>

#include <drivers/uart.h>

#include <device.h>
#include <soc.h>

#include <logging/log.h>

#if defined(CONFIG_DK_LIBRARY)
#include <dk_buttons_and_leds.h>
#endif

#include "cJSON.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_LOG_DEFAULT_LEVEL);

static K_SEM_DEFINE(sem_cloud_setup, 0, 1);

static struct k_work_delayable ble_scan_start;
static struct k_work_delayable ble_scan_stop;

#define STACKSIZE 2048

#define NUS_WRITE_TIMEOUT K_MSEC(500)

K_SEM_DEFINE(nus_write_sem, 0, 1);

static struct bt_conn *default_conn;

BT_CONN_CTX_DEF(conns, CONFIG_BT_MAX_CONN, sizeof(struct bt_nus_client));

/* UUID made for this project. */
#define BT_UUID_BEEHAVIOUR_MONITORING_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5b3, 0xf393, 0xe1a9, 0xe50e14dcea9e)

#define BT_UUID_BEEHAVIOUR_MONITORING_SERVICE   BT_UUID_DECLARE_128(BT_UUID_BEEHAVIOUR_MONITORING_VAL)

/* Array of addresses of connected peripherals. */
char address_array[CONFIG_BT_MAX_CONN][BT_ADDR_LE_STR_LEN]; 
/* Array of names of connected peripherals, correlates with address_array. */
char name_array[CONFIG_BT_MAX_CONN][20]; 
/* Name of newest connected peripheral, used in discovery_complete() to store name of connection in the name_array. */
char name_buffer[20];

/* Callback triggered when data is successfully sent over Bluetooth. */
static void ble_data_sent(struct bt_nus_client *nus, uint8_t err, const uint8_t *const data, uint16_t len)
{
	LOG_DBG("BLE Data sent");
	/* Give semaphore to signal that the sending was succesfull. */
	/* Note: bt_nus_send(...), should be followed by k_sem_take(&nus_write_sem, NUS_WRITE_TIMEOUT); */
	k_sem_give(&nus_write_sem);

	if (err) {
		LOG_WRN("ATT error code: 0x%02X", err);
	}
}

static uint8_t ble_data_received(struct bt_nus_client *nus, const uint8_t *const data, uint16_t len)
{
	LOG_DBG("ble_data_received");
	
	char addr[BT_ADDR_LE_STR_LEN];

	LOG_DBG("%.*s. Length: %i", len, data, len);

	/* All messages sent by the peripherals start with an * followed by 
	* the units given ID */
	if((char)data[0]=='*'){
		/* Get address from received ID */
		strcpy(addr, address_array[(uint8_t)data[1]]);

		// Currently you need an if for each data type received. 
		// Untested solution: 

		/*** START ***/
		
		/* struct ble_event *ble_event = new_ble_event(len-2);

		ble_event->type = BLE_RECEIVED;

		// Send the data without the * and ID to cloud_module

		memcpy(ble_event->dyndata.data, data + 2, len-2);

		memcpy(ble_event->address, log_strdup(addr), 17);
		
		// Get name from received ID
		memcpy(ble_event->name, log_strdup(name_array[(uint8_t)data[1]]), 20);

		APP_EVENT_SUBMIT(ble_event); */

		/*** END ***/

		// Length 3 means WDT data
		if(len==3){	
			
			struct ble_event *ble_event = new_ble_event(1); // 1 corresponds to number of bits of data.

			ble_event->type = BLE_RECEIVED;

			/* Send the data without the * and ID to cloud_module */
			uint8_t data_var = data[2];

			memcpy(ble_event->dyndata.data, data_var, 1);

			memcpy(ble_event->address, log_strdup(addr), 17);
			
			/* Get name from received ID */
			memcpy(ble_event->name, log_strdup(name_array[(uint8_t)data[1]]), 20);

			APP_EVENT_SUBMIT(ble_event);
		}
		/* Length 6 means data from the Bee Counter */
		if(len==6){	
			
			struct ble_event *ble_event = new_ble_event(4);

			ble_event->type = BLE_RECEIVED;

			/* Send the data without the * and ID to cloud_module */
			uint8_t data_array[4];
			for(int i=0; i<4; i++){
				data_array[i] = (uint8_t)data[i+2];
				LOG_DBG("%.02x", data_array[i]);
			}

			memcpy(ble_event->dyndata.data, data_array, 4);

			memcpy(ble_event->address, log_strdup(addr), 17);
			
			/* Get name from received ID */
			memcpy(ble_event->name, log_strdup(name_array[(uint8_t)data[1]]), 20);

			APP_EVENT_SUBMIT(ble_event);
		}
		/* Length 7 means data from the Bee Counter */
		if(len==7){	
			
			struct ble_event *ble_event = new_ble_event(5);

			ble_event->type = BLE_RECEIVED;

			/* Send the data without the * and ID to cloud_module */
			uint8_t data_array[5];
			for(int i=0; i<5; i++){
				data_array[i] = (uint8_t)data[i+2];
				LOG_DBG("%.02x", data_array[i]);
			}

			memcpy(ble_event->dyndata.data, data_array, 5);

			memcpy(ble_event->address, log_strdup(addr), 17);
			
			/* Get name from received ID */
			memcpy(ble_event->name, log_strdup(name_array[(uint8_t)data[1]]), 20);

			APP_EVENT_SUBMIT(ble_event);
		}
		/* Length 10 means data from BroodMinder weight */
		if(len==10){	
			
			struct ble_event *ble_event = new_ble_event(8);

			ble_event->type = BLE_RECEIVED;

			/* Send the data without the * and ID to cloud_module */
			uint8_t data_array[8];
			for(int i=0; i<8; i++){
				data_array[i] = (uint8_t)data[i+2];
				LOG_DBG("%.02x", data_array[i]);
			}

			strcpy(addr, address_array[(uint8_t)data[1]]);

			memcpy(ble_event->dyndata.data, data_array, 8);

			memcpy(ble_event->address, log_strdup(addr), 17);
			
			/* Get name from received ID */
			memcpy(ble_event->name, log_strdup(name_array[(uint8_t)data[1]]), 20);

			APP_EVENT_SUBMIT(ble_event);
		}
		/* Length 11 means data from the Thingy:52 */
		if(len==11){
			struct ble_event *ble_event = new_ble_event(9);

			ble_event->type = BLE_RECEIVED;

			/* Send the data without the * and ID to cloud_module */
			uint8_t data_array[9];
			for(int i=0; i<9; i++){
				data_array[i] = (uint8_t)data[i+2];
				LOG_DBG("%.02x", data_array[i]);
			}

			strcpy(addr, address_array[(uint8_t)data[1]]);

			memcpy(ble_event->dyndata.data, data_array, 9);

			memcpy(ble_event->address, log_strdup(addr), 17);

			/* Get name from received ID */
			memcpy(ble_event->name, log_strdup(name_array[(uint8_t)data[1]]), 20);

			APP_EVENT_SUBMIT(ble_event);
			
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	/* This function is taken from NordicMatt's multi-NUS
	* example: https://github.com/NordicMatt/multi-NUS */
	struct bt_nus_client *nus = context;
	LOG_DBG("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	bt_nus_handles_assign(dm, nus);
	bt_nus_subscribe_receive(nus);

	bt_gatt_dm_data_release(dm);

	/*	Send a message to the new NUS server informing it of its ID in this 
	*	mini-network
	*	The new NUS will have been added to the connection context library and so
	* 	will be the highest index as they are added incrementally upwards.
	*	This is a bit of a workaround because in this function, I don't know
	*	the ID of this connection which is the piece of info I want to transmit.
	*/
	size_t num_nus_conns = bt_conn_ctx_count(&conns_ctx_lib);
	size_t nus_index = 99;

	/*	This is a little inelegant but we must get the index of the device to
	* 	convey it. This bit sends the index number to the peripheral.
	*/
	int err; 
	for (size_t i = 0; i < num_nus_conns; i++) {
		const struct bt_conn_ctx *ctx = bt_conn_ctx_get_by_id(&conns_ctx_lib, i);
		if (ctx) {
			if (ctx->data == nus) {
				nus_index = i;
				char message[4];
				sprintf(message, "*%d", nus_index);
				message[3] = '\r';
				int length = 4;
				struct bt_nus_client *nus = ctx->data;
				char addr[BT_ADDR_LE_STR_LEN];
				bt_addr_le_to_str(bt_conn_get_dst(nus->conn), addr, sizeof(addr));
				strcpy(address_array[i], addr);
				strcpy(name_array[i], name_buffer);
				LOG_INF("Address %s, name %s added as number %i", log_strdup(address_array[i]), log_strdup(name_array[i]), i);
				err = bt_nus_client_send(nus, message, length);
				if (err) {
					LOG_WRN("Failed to send data over BLE connection (err %d)", err);
				} else {
					LOG_DBG("Sent to server %d: %s", nus_index, log_strdup(message));
				}

				bt_conn_ctx_release(&conns_ctx_lib, (void *)ctx->data);
				err = k_sem_take(&nus_write_sem, NUS_WRITE_TIMEOUT);
				if (err) {
					LOG_WRN("NUS send timeout");
				}
				break;
			} else {
				bt_conn_ctx_release(&conns_ctx_lib,
						    (void *)ctx->data);
			}
		}
	}
	/* Check if max number of peripherals are connected.
	* If max number of peripherals aren't reached, start scan */
	for(int i=0; i<CONFIG_BT_MAX_CONN; i++){
		if(address_array[i][0]=='\0'){		
			k_work_reschedule(&ble_scan_start, K_NO_WAIT);
			return;
		}
	}
	/* If max reached stop scan immediatly */
	k_work_reschedule(&ble_scan_stop, K_NO_WAIT);

}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_WRN("Service not found");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_ERR("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;
	LOG_DBG("gatt_discover");

	struct bt_nus_client *nus_client =
		bt_conn_ctx_get(&conns_ctx_lib, conn);

	if (!nus_client) {
		return;
	}

	err = bt_gatt_dm_start(conn,
			       BT_UUID_NUS_SERVICE,
			       &discovery_cb,
			       nus_client);
	if (err) {
		LOG_ERR("could not start the discovery procedure, error " "code: %d", err);
	}

	bt_conn_ctx_release(&conns_ctx_lib, (void *) nus_client);

}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected");
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_INF("Failed to connect to %s (%d)", log_strdup(addr), conn_err);

		if (default_conn == conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;

			k_work_reschedule(&ble_scan_start, K_NO_WAIT);
		}

		return;
	}

	#if defined(CONFIG_DK_LIBRARY)
	dk_set_led_off(DK_LED2);
	#endif
	LOG_INF("Connected: %s", log_strdup(addr));

	/*Allocate memory for this connection using the connection context library. For reference,
	this code was taken from hids.c
	*/
	struct bt_nus_client *nus_client = bt_conn_ctx_alloc(&conns_ctx_lib, conn);

	if (!nus_client) {
		LOG_WRN("There is no free memory to allocate the connection context");
	}

	memset(nus_client, 0, bt_conn_ctx_block_size_get(&conns_ctx_lib));
	
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
			.sent = ble_data_sent,
		}
	};

	err = bt_nus_client_init(nus_client, &init);

	bt_conn_ctx_release(&conns_ctx_lib, (void*)nus_client);
	
	if (err) {
		LOG_ERR("NUS Client initialization failed (err %d)", err);
	}else{
		LOG_DBG("NUS Client module initialized");
	}

	gatt_discover(conn);

	/*Stop scanning during the discovery*/
	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("disconnected");
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr), reason);

	err = bt_conn_ctx_free(&conns_ctx_lib, conn);

	if (err) {
		LOG_WRN("The memory was not allocated for the context of this connection.");
	}

	bt_conn_unref(conn);
	default_conn = NULL;

	/* Find out the ID of the disconnected unit */
	for(int i = 0; i<CONFIG_BT_MAX_CONN; i++){
		if(address_array[i][0]=='\0'){
			LOG_DBG("Array %i is empty", i);
			continue;
		}
	
		bool equal = true;
		for(int j=0; j<17; j++){
			if(addr[j]!=address_array[i][j]){
				equal = false;
				break;
			}
		}
		if(equal){
			/* Empty the address and name array to the disconnected unit */ 
			char empty[30] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
			strcpy(address_array[i], empty);
			char empty_name[20] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
			strcpy(name_array[i], empty_name);

			#if defined(CONFIG_DK_LIBRARY)
			dk_set_led_on(DK_LED2);
			#endif

			LOG_INF("Connection %i removed", i);
			/* Start scanning */
			k_work_reschedule(&ble_scan_start, K_NO_WAIT);
			return;
		}
	}
	LOG_ERR("The address was not registered");
}

static void security_changed(struct bt_conn *conn, bt_security_t level,

			     enum bt_security_err err)
{
	LOG_DBG("security_changed");
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", log_strdup(addr), level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", log_strdup(addr), level, err);
	}

	gatt_discover(conn);
}
static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param){
	LOG_DBG("%d, %d",param->interval_min,param->interval_max);
	/* Return true to accept new parameters, false to decline. */
	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout){
	LOG_INF("Param updated %d, %d, %d",interval,latency, timeout);			
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	uint8_t adv_data_type = net_buf_simple_pull_u8(device_info->adv_data);

	/* Get the name from the scan response */
	if (adv_data_type != BT_DATA_NAME_COMPLETE){
		LOG_DBG("Scan response name complete");
		char connection_name[20] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
		LOG_DBG("Length %i", device_info->adv_data->len);

		/* Device name starts at index 19 of the advertising data (Found through testing) */
		for(int i=19; i<device_info->adv_data->len; i++){
			LOG_DBG("%i: %c", i, device_info->adv_data->data[i]);
			connection_name[i-19] = device_info->adv_data->data[i];
		}

		LOG_INF("Filter matched unit with name: %.*s", device_info->adv_data->len - 19, log_strdup(connection_name));

		strcpy(name_buffer, connection_name);
		/* Alternative solution (Less ugly) */
		// name_buffer = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
		// memcpy(&name_buffer, &device_info->adv_data->data + 19, device_info->adv_data->len - 19);
	}

	LOG_INF("Filters matched. Address: %s connectable: %d", log_strdup(addr), connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	LOG_DBG("scan_connecting");
	default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

/* Function to stop BLE scanning */
static void ble_scan_stop_fn(struct k_work *work)
{
	int err;

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
	// Schedule scan if number of connected peripherals is less than CONFIG_BT_MAX_CONN.
	for(int i=0; i<CONFIG_BT_MAX_CONN; i++){
		if(address_array[i][0]=='\0'){	
			LOG_INF("Still missing connections, queing scan");	
			k_work_reschedule(&ble_scan_start, K_MINUTES(1));
			return;
		}
	}
}

void ble_scan_start_fn(){

	#if defined(CONFIG_DK_LIBRARY)
	dk_set_led_on(DK_LED1);
	#endif
	int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);

	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_INF("Scanning successfully started");

	/* Schedule the scan to be stopped in 10 seconds */
	k_work_reschedule(&ble_scan_stop, K_SECONDS(10));
}

static int scan_init(void)
{
	LOG_DBG("scan_init");
	int err;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	// Add filter for the project UUID that the nRF53 advetise. 
	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_BEEHAVIOUR_MONITORING_SERVICE);
	if (err) {
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);	
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	LOG_INF("Scan module initialized");

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
	LOG_DBG("pairing_confirm");
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_auth_pairing_confirm(conn);

	LOG_INF("Pairing confirmed: %s", log_strdup(addr));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_DBG("pairing_complete");
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", log_strdup(addr), bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_WRN("Pairing failed conn: %s, reason %d", log_strdup(addr), reason);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

static void ble_module_thread_fn(void){
    
	int err;

	// Wait for the cloud module to finish.
	k_sem_take(&sem_cloud_setup, K_FOREVER);
	LOG_INF("Bluetooth initializing");

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization callbacks.");
		return;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization info callbacks.\n");
		return;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_conn_cb_register(&conn_callbacks);

	err = scan_init();
	if (err) {
		LOG_ERR("Scan failed to initialize. Error: %i", err);
	}

	// Initialize the schedulable scan controls.
	k_work_init_delayable(&ble_scan_start, ble_scan_start_fn);
	k_work_init_delayable(&ble_scan_stop, ble_scan_stop_fn);

	k_work_reschedule(&ble_scan_start, K_NO_WAIT);

	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_INF("Scanning successfully started");

}
static bool event_handler(const struct app_event_header *eh)
{
	if (is_cloud_event_abbr(eh)) {

		struct cloud_event_abbr *event = cast_cloud_event_abbr(eh);
		if(event->type==CLOUD_SETUP_COMPLETE){
			LOG_DBG("Cloud connected");
			/* Start initialization of Bluetooth */
			k_sem_give(&sem_cloud_setup);
			return false;
		}
		if(event->type==CLOUD_RECEIVED){
			cJSON *obj = NULL;
			obj = cJSON_Parse(event->dyndata.data);
			cJSON *message = cJSON_GetObjectItem(obj, "message");
			if(message!=NULL){
				if(message->valuestring!=NULL){
					LOG_INF("JSON message: %s, Length: %i", message->valuestring, strlen(message->valuestring));
					/* Logic to send data received from cloud to one of the connected peripherals
					* PS. Not used for anything yet */
					if(message->valuestring[0]=='*'){
						LOG_DBG("Send to BLE");
						struct ble_event *ble_event_routed = new_ble_event(strlen(message->valuestring));

						ble_event_routed->type = BLE_SEND;
						
						char addr[2];
						addr[0]=message->valuestring[0];
						addr[1]=message->valuestring[1];
						
						memcpy(ble_event_routed->address, addr, 2);

						memcpy(ble_event_routed->dyndata.data, message->valuestring, strlen(message->valuestring));

						APP_EVENT_SUBMIT(ble_event_routed);
						return false;
					}
					/* Start scanning */
					if(!strcmp(message->valuestring, "StartScan")){
						k_work_reschedule(&ble_scan_start, K_NO_WAIT);
						return false;
					}
					/* Find number of connected and missing units and send to cloud_module */
					if(!strcmp(message->valuestring,"BLE_status")){
						char status[2];
						uint8_t connected = 0;
						for(int i=0; i < CONFIG_BT_MAX_CONN; i++){
							if(address_array[i][0]!='\0'){
								connected++;
							}
						}

						status[0] = '0' + (char)connected;

						status[1] = '0' + (char)(CONFIG_BT_MAX_CONN - connected);

						LOG_DBG("Connected: %i or %c, Missing %c", connected, status[0], status[1]);

						struct ble_event *ble_event = new_ble_event(strlen(status));
						
						ble_event->type = BLE_STATUS;
						
						memcpy(ble_event->address, "Placeholder", strlen("Placeholder"));

						memcpy(ble_event->dyndata.data, status, strlen(status));

						APP_EVENT_SUBMIT(ble_event);
						return false;
					}
				}
				return false;
			}
			else{
				LOG_INF("Message is null");
				return false;
			}
			LOG_DBG("Message: %.*s",event->dyndata.size, event->dyndata.data);

			return false;
		}
		return false;
	}
	if(is_ble_event(eh)){
		int err;
		struct ble_event *event = cast_ble_event(eh);
		/* Send to peripheral unit with corresponding ID */
		/* Not currently in used, but can be used by other modules to send messages over Bluetooth. */
		if(event->type==BLE_SEND){
			if(event->address[0]=='*'){
				int id = (int)event->address[1]-(int)'0';
				if(id>=CONFIG_BT_MAX_CONN){
					LOG_ERR("Id out of range");
					return false;
				}
				if(address_array[id][0]=='\0' || (int)address_array[id][0]==0 || address_array[id]==NULL){
					LOG_INF("Address associated with id is empty");
					return false;
				}
				LOG_INF("ID: %i, address: %.17s", id, address_array[id]);
				const struct bt_conn_ctx *ctx = bt_conn_ctx_get_by_id(&conns_ctx_lib, id);
				if (ctx) {
					struct bt_nus_client *nus = ctx->data;
					err = bt_nus_client_send(nus, event->dyndata.data, event->dyndata.size);
					if (err) {
						LOG_WRN("Failed to send data over BLE connection (err %d)", err);
					} else {
						LOG_INF("Sent to server %d: %.*s", id, event->dyndata.size, event->dyndata.data);
					}

					bt_conn_ctx_release(&conns_ctx_lib, (void *)ctx->data);
					err = k_sem_take(&nus_write_sem, NUS_WRITE_TIMEOUT);
					if (err) {
						LOG_WRN("NUS send timeout");
					}
				} 
				else {
					bt_conn_ctx_release(&conns_ctx_lib,
								(void *)ctx->data);
				}
			}
		}

	}
	
	return false;
}

K_THREAD_DEFINE(ble_module_thread, STACKSIZE,
		ble_module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_event);
APP_EVENT_SUBSCRIBE(MODULE, cloud_event_abbr);