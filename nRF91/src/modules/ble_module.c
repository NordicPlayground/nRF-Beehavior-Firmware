/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

//Når cloud og BLE scan kjører samtidig
//W: opcode 0x0000 pool id 3 pool 0x2001530c != &hci_cmd_pool 0x20015364

#include <errno.h>
#include <zephyr.h>
#include <init.h>
#include <stdio.h>
#include <event_manager.h>
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

#include <dk_buttons_and_leds.h>

#include "cJSON.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_LOG_DEFAULT_LEVEL);

static K_SEM_DEFINE(cloud_connected, 0, 1);

static struct k_work_delayable ble_scan_stop;

#define STACKSIZE 2048

/* UART payload buffer element size. */
#define UART_BUF_SIZE 20

#define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
#define KEY_PASSKEY_REJECT DK_BTN2_MSK

#define NUS_WRITE_TIMEOUT K_MSEC(500)
#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)
#define UART_RX_TIMEOUT 50

static const struct device *uart;
static struct k_delayed_work uart_work;

K_SEM_DEFINE(nus_write_sem, 0, 1);

struct uart_data_t {
	void *fifo_reserved;
	uint8_t  data[UART_BUF_SIZE];
	uint16_t len;
};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static struct bt_conn *default_conn;

BT_CONN_CTX_DEF(conns, CONFIG_BT_MAX_CONN, sizeof(struct bt_nus_client));
static bool routedMessage = false;
static bool messageStart = true;

#define BT_UUID_BEEHAVIOUR_MONITORING_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5b3, 0xf393, 0xe1a9, 0xe50e14dcea9e)

#define BT_UUID_BEEHAVIOUR_MONITORING_SERVICE   BT_UUID_DECLARE_128(BT_UUID_BEEHAVIOUR_MONITORING_VAL)

#define ROUTED_MESSAGE_CHAR '*'
#define BROADCAST_INDEX 99

char address_array[CONFIG_BT_MAX_CONN][BT_ADDR_LE_STR_LEN]; 

static void ble_scan_stop_fn(struct k_work *work)
{
	int err;

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}

	struct ble_event *ble_stopped_scanning = new_ble_event(strlen("Stopped scanning"));

	ble_stopped_scanning->type = BLE_DONE_SCANNING;
	
	memcpy(ble_stopped_scanning->address, "Placeholder", 17);

	memcpy(ble_stopped_scanning->dyndata.data, "Stopped scanning", strlen("Stopped scanning"));

	EVENT_SUBMIT(ble_stopped_scanning);
}

void scan_start(bool start){
	int err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);

	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_INF("Scanning successfully started");

	struct ble_event *ble_started_scanning = new_ble_event(strlen("Started scanning"));

	ble_started_scanning->type = BLE_SCANNING;
	
	memcpy(ble_started_scanning->address, "Placeholder", 17);

	memcpy(ble_started_scanning->dyndata.data, "Started scanning", strlen("Started scanning"));

	EVENT_SUBMIT(ble_started_scanning);

	if(start){
		k_work_reschedule(&ble_scan_stop, K_SECONDS(10));
	}
}

static void ble_data_sent(uint8_t err, const uint8_t *const data, uint16_t len)
{
	LOG_INF("BLE Data sent");
	k_sem_give(&nus_write_sem);

	if (err) {
		LOG_WRN("ATT error code: 0x%02X", err);
	}
}


static uint8_t ble_data_received(const uint8_t *const data, uint16_t len)
{
	LOG_INF("ble_data_received");
	char data_string[100];
	// char data_received[len];
	char addr[BT_ADDR_LE_STR_LEN];

	LOG_INF("%.*s", len, data);

	if((char)data[0]=='*'){
		strcpy(addr, address_array[(uint8_t)data[1]]);
		if(len==10){	
			LOG_INF("WeightR: %i,%i, WeightL: %i,%i, RealTimeWeight: %i,%i, Temperature: %i,%i, received from %s, ID: %i", \
					(uint8_t)data[2], (uint8_t)data[3], (uint8_t)data[4], (uint8_t)data[5], (uint8_t)data[6], (uint8_t)data[7],\
					(uint8_t)data[8], (uint8_t)data[9], log_strdup(addr), (uint8_t)data[1]);

			int err = snprintf(data_string, 100, "WeightR: %i,%i, WeightL: %i,%i, RealTimeWeight: %i,%i, Temperature: %i,%i, ID: %i", \
					 (uint8_t)data[2], (uint8_t)data[3], (uint8_t)data[4], (uint8_t)data[5], (uint8_t)data[6], (uint8_t)data[7], \
					 (uint8_t)data[8], (uint8_t)data[9], (uint8_t)data[1]);
			LOG_INF("Did it work? %i", err);
		}
		if(len==11){

		char temp[4];
		for (uint8_t i = 5; i <= 8; i++){
			temp[i-5] = data[i];
			/*printf("index of temp: %i\n", i-5);
			printf("Address of this element: %pn \n",&temp[i-5]);
			printf("Value of element: %X\n", temp[i-5]);*/

		}
		printf("\n"); 
		int32_t tempvar;//= (int32_t)temp;
		int32_t tempvar2;

		memcpy(&tempvar, temp, sizeof(tempvar));
		printf("The number is %X,%i \n",tempvar,tempvar);

		char reverse_temp[4];
		for (uint8_t i = 0; i <=3; i++){
			reverse_temp[i] = temp[3-i];
			printf("Index of reverse temp %i \n", i);
			printf("temp[i] after reversing: %X\n", reverse_temp[i]);
		}

		memcpy(&tempvar2, reverse_temp, sizeof(tempvar2));
		
    	printf("The number after reversing is %X,%i \n",tempvar2,tempvar2);

			LOG_INF("Temperature [C]: %i,%i, Humidity [%%]: %i, Air Pressure [hPa]: %i,%i, received from %s, ID: %i", \
						(uint8_t)data[2], (uint8_t)data[3], (uint8_t)data[4], (uint32_t)tempvar2,(uint8_t)data[9], log_strdup(addr), (uint8_t)data[1]);

			int err = snprintf(data_string, 100, "Temperature [C]: %i,%i, Humidity [%%]: %i, Air Pressure [hPa]: %i,%i, ID: %i", \
						(uint8_t)data[2], (uint8_t)data[3], (uint8_t)data[4],(uint32_t)tempvar2,(uint8_t)data[9], (uint8_t)data[1]);
			LOG_INF("Did it work? %i", err);
		}
	}
	// LOG_INF("Received: %s", log_strdup(data_received));
	// LOG_INF("From %s", log_strdup(addr));

	struct ble_event *ble_event = new_ble_event(100);

	ble_event->type = BLE_RECEIVED;
	
	memcpy(ble_event->address, log_strdup(addr), 17);

	memcpy(ble_event->dyndata.data, data_string, 100);

	EVENT_SUBMIT(ble_event);

	return BT_GATT_ITER_CONTINUE;
}

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	LOG_INF("discovery_complete");
	struct bt_nus_client *nus = context;
	LOG_INF("Service discovery completed");

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
				LOG_INF("Address %s, or %s added as number %i", log_strdup(address_array[i]), log_strdup(addr), i);
				err = bt_nus_client_send(nus, message, length);
				if (err) {
					LOG_WRN("Failed to send data over BLE connection (err %d)", err);
				} else {
					LOG_INF("Sent to server %d: %s",
						nus_index, log_strdup(message));
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
	for(int i=0; i<CONFIG_BT_MAX_CONN; i++){
		LOG_INF("%i",i);
		if(address_array[i][0]=='\0'){		
			scan_start(false);

			return;
		}
	}
	k_work_reschedule(&ble_scan_stop, K_NO_WAIT);

}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;
	LOG_INF("gatt_discover");

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
		LOG_ERR("could not start the discovery procedure, error "
			"code: %d", err);
	}

	bt_conn_ctx_release(&conns_ctx_lib, (void *) nus_client);

}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_INF("connected");
	char addr[BT_ADDR_LE_STR_LEN];
	int err;
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
			.sent = ble_data_sent,
		}
	};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_INF("Failed to connect to %s (%d)", log_strdup(addr),
			conn_err);

		if (default_conn == conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;

			scan_start(false);
		}

		return;
	}

	LOG_INF("Connected: %s", log_strdup(addr));

	/*Allocate memory for this connection using the connection context library. For reference,
	this code was taken from hids.c
	*/
	struct bt_nus_client *nus_client = bt_conn_ctx_alloc(&conns_ctx_lib, conn);

	if (!nus_client) {
		LOG_WRN("There is no free memory to "
			"allocate the connection context");
	}

	memset(nus_client, 0, bt_conn_ctx_block_size_get(&conns_ctx_lib));

	err = bt_nus_client_init(nus_client, &init);

	bt_conn_ctx_release(&conns_ctx_lib, (void *)nus_client);
	
	if (err) {
		LOG_ERR("NUS Client initialization failed (err %d)", err);
	}else{
		LOG_INF("NUS Client module initialized");
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
	LOG_INF("disconnected");
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr),
		reason);

	err = bt_conn_ctx_free(&conns_ctx_lib, conn);

	if (err) {
		LOG_WRN("The memory was not allocated for the context of this "
			"connection.");
	}

	bt_conn_unref(conn);
	default_conn = NULL;

	for(int i = 0; i<CONFIG_BT_MAX_CONN; i++){
		if(address_array[i][0]=='\0'){
			LOG_INF("Array %i is empty", i);
			continue;
		}
		LOG_INF("Checkpoint %i, also %s and %s", i, log_strdup(addr), log_strdup(address_array[i]));

		bool equal = true;
		for(int t=0; t<17; t++){
			if(addr[t]!=address_array[i][t]){
				equal = false;
				break;
			}
		}
		if(equal){
			char empty[30] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
			strcpy(address_array[i], empty);
			LOG_INF("Connection %i removed", i);
			scan_start(true);
			return;
		}
	}
	LOG_INF("The address was not registered");
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	LOG_INF("security_changed");
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", log_strdup(addr),
			level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", log_strdup(addr),
			level, err);
	}

	gatt_discover(conn);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d",
		log_strdup(addr), connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	LOG_INF("scan_connecting");
	default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static int scan_init(void)
{
	LOG_INF("scan_init");
	int err;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	char *name = "Andreas53Test";
	// char *name = "TeppanTest";
	// err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_BEEHAVIOUR_MONITORING_SERVICE);
	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, name);
	if (err) {
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	LOG_INF("Scan module initialized");
	return err;
}


static void auth_cancel(struct bt_conn *conn)
{
	LOG_INF("auth_cancel");
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", log_strdup(addr));
}


static void pairing_confirm(struct bt_conn *conn)
{
	LOG_INF("pairing_confirm");
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_auth_pairing_confirm(conn);

	LOG_INF("Pairing confirmed: %s", log_strdup(addr));
}


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_INF("pairing_complete");
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", log_strdup(addr),
		bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	LOG_INF("pairing_failed");
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

static void module_thread_fn(void){
    
	int err;

	LOG_INF("Waiting to initialize BLE");

	k_sem_take(&cloud_connected, K_FOREVER);

	LOG_INF("Bluetooth initializing");

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

	bt_conn_cb_register(&conn_callbacks);

	int (*module_init[])(void) = {scan_init};//uart_init,  , nus_client_init};
	for (size_t i = 0; i < ARRAY_SIZE(module_init); i++) {
		err = (*module_init[i])();
		LOG_INF("Error: %i on operation %i", err, i);
		if (err) {
			return;
		}
	}
	// err = scan_init();
	// LOG_INF("Scan exited with code: %i",err);

	LOG_INF("Starting Bluetooth Central UART example\n");

	k_work_init_delayable(&ble_scan_stop, ble_scan_stop_fn);

	scan_start(true);

	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_INF("Scanning successfully started");

}
static bool event_handler(const struct event_header *eh)
{
	if (is_cloud_event_abbr(eh)) {

		struct cloud_event_abbr *event = cast_cloud_event_abbr(eh);
		if(event->type==CLOUD_CONNECTED){
			LOG_INF("Cloud connected");
			k_sem_give(&cloud_connected);
			return false;
		}
		if(event->type==CLOUD_SLEEP){
			scan_start(true);
			return false;
		}
		if(event->type==CLOUD_RECEIVED){
			cJSON *obj = NULL;
			obj = cJSON_Parse(event->dyndata.data);
			cJSON *message = cJSON_GetObjectItem(obj, "message");
			if(message!=NULL){
				if(message->valuestring!=NULL){
					LOG_INF("JSON message: %s, Length: %i", message->valuestring, strlen(message->valuestring));
					if(message->valuestring[0]=='*'){
						LOG_INF("Should be sent to BLE");
						struct ble_event *ble_event_routed = new_ble_event(strlen(message->valuestring));

						ble_event_routed->type = BLE_SEND;
						
						// char *address_ptr = "Placeholder"; //Finne ut hvordan man kan velge mellom to devicer.
						// char address_temp[17];
						// for(int i=0;i<17;i++){
						// 	address_temp[i]=address_ptr[i];
						// }
						char addr[2];
						addr[0]=message->valuestring[0];
						addr[1]=message->valuestring[1];
						// ble_event->address = address_temp;
						memcpy(ble_event_routed->address, addr, 2);

						memcpy(ble_event_routed->dyndata.data, message->valuestring, strlen(message->valuestring));

						EVENT_SUBMIT(ble_event_routed);
						return false;
					}
					if(!strcmp(message->valuestring, "StartScan")){
						scan_start(true);
						return false;
					}
					if(!strcmp(message->valuestring,"BLE_status")){
						LOG_INF("Checkpoint 1");
						char status[2];
						uint8_t connected = 0;
						LOG_INF("Checkpoint 2");
						for(int i=0; i < CONFIG_BT_MAX_CONN; i++){
							LOG_INF("Checkpoint 3");
							if(address_array[i][0]!='\0'){
								LOG_INF("Checkpoint 4");
								connected++;
							}
						}

						status[0] = '0' + (char)connected;

						status[1] = '0' + (char)(CONFIG_BT_MAX_CONN - connected);

						LOG_INF("Checkpoint 2, %i, %c, %c", connected, status[0], status[1]);

						// LOG_INF("%c, %c, %c", (char)status[0], (char)status[1], (char)connected);

						// char toretang = '1';

						// LOG_INF("%i", (uint8_t)toretang);

						struct ble_event *ble_event = new_ble_event(strlen(status));
						
						ble_event->type = BLE_STATUS;
						
						LOG_INF("Checkpoint 3");
						
						memcpy(ble_event->address, "Placeholder", strlen("Placeholder"));

						memcpy(ble_event->dyndata.data, status, strlen(status));

						LOG_INF("Checkpoint 4");
						EVENT_SUBMIT(ble_event);
						return false;
					}
				}
				return false;
			}
			else{
				LOG_INF("Message is null");
				return false;
			}
			//char addr[17] = event->address;

			// led_on=!led_on;
			
			// dk_set_led(DK_LED1, led_on);

			LOG_INF("Message: %.*s",event->dyndata.size, event->dyndata.data);

			return false;
		}
		return false;
	}
	if(is_ble_event(eh)){
		int err;
		struct ble_event *event = cast_ble_event(eh);
		if(event->type==BLE_SEND){
			if(event->address[0]=='*'){
				int id = (int)event->address[1]-(int)'0';
				if(id>=CONFIG_BT_MAX_CONN){
					LOG_INF("Id out of range");
					return false;
				}
				int test = (int)address_array[id][0];
				char ctest = (char)1;
				LOG_INF("Val: %i, char: %c", test, ctest);
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
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);


EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, ble_event);
EVENT_SUBSCRIBE(MODULE, cloud_event_abbr);