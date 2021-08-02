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

/* SOME BLOAT/DUPLICATE CODE MAY BE RESIDING IN THIS MODULE WHICH ARE
 REDUNDANT DUE TO BEEING INCLUDED IN OTHER MODULES. NOT SURE WHAT.*/ 

#include <event_manager.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <bluetooth/scan.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/gatt.h>
#include <logging/log.h>
#include <zephyr.h>

#include <dk_buttons_and_leds.h>

#include "events/ble_event.h"
// #include "events/thingy_event.h"
#include "events/bm_w_event.h"
#include "led/led.h"

static K_SEM_DEFINE(thingy_ready, 0, 1);

static struct k_work_delayable weight_interval;
static struct k_work_delayable temperature_interval;

#define LOG_MODULE_NAME bm_w_module
LOG_MODULE_REGISTER(LOG_MODULE_NAME, 4);

#define REAL_TIME_WEIGHT 0x16
#define BROODMINDER_ADDR ((bt_addr_le_t[]) { { 0, \
			 { { 0xFD, 0x01, 0x57, 0x16, 0x09, 0x06 } } } })
#define BROODMINDER_ADDR_TEMPERATURE ((bt_addr_le_t[]) { { 0, \
			 { { 0x93, 0x05, 0x47, 0x16, 0x09, 0x06 } } } })

#define USE_BMW;
#define USE_TEMPERATURE;
static int scan_init(bool first);
static bool data_cb(struct bt_data *data, void *user_data);
static float lbs_to_kg(float weight);

static void ble_scan_start_fn(struct k_work *work)
{
	LOG_INF("Scanning for bm_w starting.");
	LOG_INF("LED 3 toggled while scanning. \n");
	dk_set_led_on(LED_3);

	int err = scan_init(false);
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
		for (int i = 0; i < len+8; i++){
			printk("%i: %02x\n", i, broodminder_data[i]);
		}
		return false;
	default:
		return true;
	}
}

void scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match,
		       bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];
	uint8_t adv_data_type = net_buf_simple_pull_u8(device_info->adv_data);
	
	/* Don't update weight value if the advertised data is a scan response */
	if (adv_data_type != BT_DATA_NAME_COMPLETE){
		LOG_INF("DO WE EVER USE THIS? \n");
		printk("Advertising data length: %i\n", device_info->adv_data->len);

		uint8_t broodminder_data[device_info->adv_data->len - 1];

		bt_data_parse((device_info->adv_data), data_cb, (void*)broodminder_data);
		printk("\n");

		uint8_t weight_lbs = broodminder_data[REAL_TIME_WEIGHT];
		uint8_t weight_kg = lbs_to_kg(weight_lbs);
		
		uint8_t decimal_weight_lbs = broodminder_data[REAL_TIME_WEIGHT - 1];

		float realTimeWeight = (float)(broodminder_data[22] * 256 + broodminder_data[21] - 32767 ) / 100 ;

		float realTimeWeightKg = lbs_to_kg(realTimeWeight);

		printk("Real Time Weight in Kg: %.2f\n", realTimeWeightKg);

		printf("Real Time Weight in lbs %.2f\n", realTimeWeight);

		float realTimeTemperature = ((float)(broodminder_data[11] * 256 + broodminder_data[13] - 5000) / 100); // * 9 / 5 + 32 (for Fahrenheit)

		int roundedRTT = (int)realTimeTemperature;

		printk("Real time Temperature rounded: %d\n", roundedRTT);

		printf("Real time Temperature float: %.2f\n", realTimeTemperature);

		bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

		printk("Device found: %s\n", addr);

		float weightR = broodminder_data[12 + 1] * 256 + broodminder_data[12 + 0] - 32767;
		float weightScaledR = weightR / 100;
		printf("weightScaledR in Kg%.2f\n", lbs_to_kg(weightScaledR));
		float weightL = broodminder_data[14 + 1] * 256 + broodminder_data[14 + 0] - 32767;
		float weightScaledL = weightL / 100;
		printf("weightScaledL in Kg %.2f\n", lbs_to_kg(weightScaledL));

	}
	k_sleep(K_MINUTES(1));

}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, NULL, NULL);


static int scan_init(bool first){
	int err = 0;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 0,
	};

    LOG_INF("Changing filters\n");
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



void module_thread_fn(void)
{
    LOG_INF("STOP!... (Wait for thingy_ready semaphore)\n");
    k_sem_take(&thingy_ready, K_FOREVER);
    LOG_INF("HAMMERTIME! (Thingy is ready) \n");

	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.options    = BT_LE_SCAN_OPT_CODED,
		.interval   = 0x0010,
		.window     = 0x0010,
	};
	int err;

	err = scan_init(true);
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
		return;
	}

	LOG_INF("Scanning for bm_w succesfully started\n");
	LOG_INF("LED 3 toggled while scanning for BM_Weight. \n");
	dk_set_led_on(LED_3);
	
	k_work_init_delayable(&weight_interval, ble_scan_start_fn);

	k_work_reschedule(&weight_interval ,K_MINUTES(1));

}

static bool event_handler(const struct event_header *eh)
{
	if (is_ble_event(eh)) {

		struct ble_event *event = cast_ble_event(eh);
		if(event->type==HUB_CONNECTED){
			LOG_INF("Thingy connected\n");
			k_sem_give(&thingy_ready);
			return false;
		}
		return false;
	}
    
    return false;
}

K_THREAD_DEFINE(bm_w_module_thread, 1024,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);


EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, ble_event);
// EVENT_SUBSCRIBE(MODULE, thingy_event);
// EVENT_SUBSCRIBE(MODULE, bm_w_event);