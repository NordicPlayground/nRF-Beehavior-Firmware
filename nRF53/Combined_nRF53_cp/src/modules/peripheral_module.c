/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <event_manager.h>

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
#include "events/bm_w_event.h"

#include <drivers/uart.h>

#include <device.h>
#include <soc.h>

#define MODULE peripheral_module
LOG_MODULE_REGISTER(MODULE);

// #define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
// #define PRIORITY 7

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

char id;

static K_SEM_DEFINE(ble_init_ok, 0, 1);

static struct bt_conn *hub_conn;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};
#define BT_UUID_BEEHAVIOUR_MONITORING_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5b3, 0xf393, 0xe1a9, 0xe50e14dcea9e)

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_BEEHAVIOUR_MONITORING_VAL),
};

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
			  uint16_t len)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN] = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));
	
	char data_received[len];
	for(int i = 0; i < len; i++)
		{
			data_received[i] = (char)data[i];
		}

	LOG_INF("Received data from %s: %s\n", log_strdup(addr), log_strdup(data_received));

	if(data_received[0]=='*'){
		id=data_received[1];
		LOG_INF("New ID is: %c\n", id);
        hub_conn = conn;
	}

}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};

void peripheral_module_thread_fn(void)
{
    /* Don't go any further until BLE is initialized */
    LOG_INF("What it do! (Wait for BLE to be initialized) \n");
	k_sem_take(&ble_init_ok, K_FOREVER);
    LOG_INF("What it is! (BLE is initialized) \n");

    int err;

	// if (IS_ENABLED(CONFIG_SETTINGS)) {
	// 	settings_load();
	// }

	err = bt_nus_init(&nus_cb);
	if (err) {
		LOG_ERR("Failed to initialize UART service (err: %d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
	}

}

static bool event_handler(const struct event_header *eh)
{
	if (is_ble_event(eh)) {
        LOG_INF("BLE event is being handled\n");
		struct ble_event *event = cast_ble_event(eh);
		if(event->type==BM_W_READ){
			LOG_INF("BM-W ready\n");
			k_sem_give(&ble_init_ok);
			return false;
		}
		return false;
	}

    if (is_thingy_event(eh)) {
        LOG_INF("Thingy event is being handled\n");
		struct thingy_event *event = cast_thingy_event(eh);
		LOG_INF("Temperature [C]: %i,%i, Humidity [%%]: %i, Air pressure [hPa]: %d,%i ID: %i\n", event->data_array[0], \
				event->data_array[1], event->data_array[2], event->pressure_int, event->pressure_float, id-(uint8_t)'0');


        uint8_t thingy_data[11] = { (uint8_t)'*', id-(uint8_t)'0', event->data_array[0], event->data_array[1], event->data_array[2],\
									event->pressure_int, event->pressure_float};
        if(hub_conn){
            LOG_INF("Hub is connected\n");
            int err = bt_nus_send(hub_conn, thingy_data, 11);
        }
		return false;
	}
	
	if (is_bm_w_event(eh)) {
        LOG_INF("BM_W event is being handled\n");
		struct bm_w_event *event = cast_bm_w_event(eh);
		LOG_INF("WeightR: %.2f, WeightL: %.2f, RTWeight: %.2f, Temperature: %.2f, ID: %i\n", event->weightR, event->weightL, event->realTimeWeight, event->temperature, id-(uint8_t)'0');
        uint8_t bm_w_data[10] = { (uint8_t)'*', id-(uint8_t)'0', (uint8_t)event->weightR, (uint8_t)((event->weightR - (uint8_t)event->weightR) * 100)
			, (uint8_t)event->weightL, (uint8_t)((event->weightL - (uint8_t)event->weightL) * 100)
			, (uint8_t)event->realTimeWeight, (uint8_t)((event->realTimeWeight - (uint8_t)event->realTimeWeight) * 100) 
			, (uint8_t)event->temperature, (uint8_t)((event->temperature - (uint8_t)event->temperature) * 100) };
        if(hub_conn){
            LOG_INF("Hub is connected\n");
            int err = bt_nus_send(hub_conn, bm_w_data, 10);
        }
		else{
			//Save untill reconnected TO DO
		}
		return false;
	}
    
	return false;
}

K_THREAD_DEFINE(peripheral_module_thread, 1024,
		peripheral_module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);


EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, ble_event);
EVENT_SUBSCRIBE(MODULE, thingy_event);
EVENT_SUBSCRIBE(MODULE, bm_w_event);