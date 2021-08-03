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
#include <stdbool.h>
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

#include <dk_buttons_and_leds.h>

#include <settings/settings.h>

#include <drivers/uart.h>

#include <logging/log.h>

#include "events/ble_event.h"
#include "events/thingy_event.h"
#include "events/bm_w_event.h"
#include "led/led.h"
#include "watchdog/watchdog.h"

// #include "ble.h"

primary_feed

#define MODULE ble_module
LOG_MODULE_REGISTER(MODULE, 4);

bool console_prints = true; /*Toggle this to display console prints*/

static float lbs_to_kg(float lbs){
	return lbs*0.4536;
}


static bool data_cb(struct bt_data *data, void *user_data)
{
	uint8_t *broodminder_data = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_MANUFACTURER_DATA:

		len = data->data_len;
		memcpy(broodminder_data, data->data, len);

		return false;
	default:
		return true;
	}
}

// ----------------------------- Scanning and pairing -------------------------
static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d\n", log_strdup(addr), connectable);

	// if(!strncmp(addr, "06:09:16:57:01:FD", 17)){
	if(!strncmp(addr, "06:09:16:47:05:93", 17)){
		uint8_t adv_data_type = net_buf_simple_pull_u8(device_info->adv_data);

		/* Don't update weight value if the advertised data is a scan response */
		if (adv_data_type != BT_DATA_NAME_COMPLETE){
			float data_array[4];

			bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

			uint8_t broodminder_data[device_info->adv_data->len - 1];

			bt_data_parse((device_info->adv_data), data_cb, (void*)broodminder_data);

			data_array[2] = lbs_to_kg((float)(broodminder_data[22] * 256 + broodminder_data[21] - 32767 ) / 100);

			data_array[3] = ((float)(broodminder_data[11] * 256 + broodminder_data[13] - 5000) / 100); // * 9 / 5 + 32 (for Fahrenheit)

			data_array[0]  = lbs_to_kg((float)(broodminder_data[12 + 1] * 256 + broodminder_data[12 + 0] - 32767) / 100);
			
			data_array[1] = lbs_to_kg((float)(broodminder_data[14 + 1] * 256 + broodminder_data[14 + 0] - 32767) / 100);


			if (console_prints){ /*Print outputs for debugging purposes */
				printk("\n");

				printk("Device found: %s\n", addr);

				printk("Bm_w advertising data length: %i\n", device_info->adv_data->len);

				printk("Real Time Weight [Kg]: %.2f\n", data_array[2]);

				printf("Temperature [Celsius]: %.2f\n", data_array[3]);

				printf("WeightR [Kg]: %.2f\n", data_array[0]);

				printf("WeightL [Kg]: %.2f\n", data_array[1]);

			}

			struct bm_w_event *bm_w_send = new_bm_w_event();

			bm_w_send->weightR = data_array[0];
			bm_w_send->weightL = data_array[1];
			bm_w_send->realTimeWeight = data_array[2];
			bm_w_send->temperature = data_array[3];

			EVENT_SUBMIT(bm_w_send);
			LOG_INF("LED 3  toggled off. Data from BM has been sent from 53 to 91.\n");
			dk_set_led_off(LED_3);
		}
		
		//k_sleep(K_SECONDS(30));
	}
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed\n");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	LOG_INF("Scan connecting\n");
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	// if(!strncmp(addr, "06:09:16:57:01:FD", 17)){
	if(!strncmp(addr, "06:09:16:47:05:93", 17)){
		LOG_INF("Weight connecting\n");
		// LOG_INF("Temperature connecting.\n");
		return;
	}
	// default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		NULL, NULL);

void ble_module_thread_fn(void)
{
	/*Initializing Buttons and LEDS */ 
	int err;

	// err = dk_buttons_init(button_changed);
	// if (err) {
	// 	LOG_ERR("Cannot init buttons (err: %d)", err);
	// }

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
	else{
		LOG_INF("Leds initialized and set to LED1 = 53 Connected to T52, LED2 = 53 Connected to T91, LED3 = 53 Scanning for BM_Weight.\n");
	}


	LOG_INF("Starting BLE\n");
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
	LOG_INF("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		LOG_INF("Loading settings\n");
		settings_load();
	}

	struct bt_scan_init_param scan_init = {
			.connect_if_match = 1,
		};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	struct ble_event *ble_ready = new_ble_event();

	ble_ready->type = BLE_READY;

	EVENT_SUBMIT(ble_ready);
}

static bool event_handler(const struct event_header *eh)
{
	return false;
}

K_THREAD_DEFINE(ble_module_thread, 1024,
		ble_module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, ble_event);
EVENT_SUBSCRIBE(MODULE, thingy_event);
EVENT_SUBSCRIBE(MODULE, bm_w_event);