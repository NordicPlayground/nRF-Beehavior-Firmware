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
#include <sys/reboot.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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

#if defined(CONFIG_DK_LIBRARY)
#include <dk_buttons_and_leds.h>
#endif

#include <settings/settings.h>

#include <drivers/uart.h>
#include <drivers/gpio.h>

#include <logging/log.h>

#include "events/ble_event.h"
#include "events/thingy_event.h"
#include "events/bm_w_event.h"

#define MODULE scan_module
LOG_MODULE_REGISTER(MODULE, CONFIG_LOG_DEFAULT_LEVEL);

#define LED_1 DK_LED1
#define LED_2 DK_LED2
#define LED_3 DK_LED3
#define LED_4 DK_LED4

/* Used to save the name that is currently being scanned after. */
char peripheral_name[CONFIG_BT_SCAN_NAME_MAX_LEN];

static struct k_work_delayable scan_timeout;
static struct k_work_delayable weight_interval;

/* Flag to show if there are currently any scans active. */
bool scanning = false;
/* Flag to show if the current scan is for the Broodminder. */
bool broodminder_scan = false;

/* Helper function to fill a string with '\0'. */
void clear_string(char *buf, int len){
	for(int i = 0; i<len; i++){
		buf[i] = '\000';
	}
}

static float lbs_to_kg(float lbs){
	return lbs*0.4536;
}

/* Function for retrieving data from advertising packets. */
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

/* Function for retrieving name from advertising packet. */
static bool name_cb(struct bt_data *data, void *user_data)
{
	switch (data->type) {
	case BT_DATA_NAME_COMPLETE:

		memcpy(user_data, data->data, data->data_len);

		return false;
	default:
		return true;
	}
}

// ----------------------------- Scanning -------------------------
static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("scan_filter_match(): Filters matched. Address: %s connectable: %d\n", log_strdup(addr), connectable);

	if(broodminder_scan){
		#if defined(CONFIG_THINGY53)
		/* Toggle red led to signal succesfull scan. */
		const struct device *gpio0_dev;
		gpio0_dev = device_get_binding("GPIO_0");
		err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_LOW); //3V3 ENABLE (LOW means on)
		const struct device *gpio1_dev;
		gpio1_dev = device_get_binding("GPIO_1");
		err = gpio_pin_configure(gpio1_dev, 8, GPIO_OUTPUT_HIGH); //Red led	
		
		k_sleep(K_MSEC(500));
		
		err = gpio_pin_configure(gpio1_dev, 8, GPIO_OUTPUT_LOW);  //Red led	
		err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_HIGH); //3V3 ENABLE (HIGH means off)
		#endif
			
		LOG_INF("Connected to the weight");
		uint8_t adv_data_type = net_buf_simple_pull_u8(device_info->adv_data);

		/* Don't update weight value if the advertised data is a scan response */
		// if (adv_data_type != BT_DATA_NAME_COMPLETE){
			float data_array[4];

			bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

			uint8_t broodminder_data[device_info->adv_data->len - 1];

			for(int i = 0; i < device_info->adv_data->len; i++){
				LOG_INF("%i: %x", i, broodminder_data[i]);
			}

			bt_data_parse((device_info->adv_data), data_cb, (void*)broodminder_data);

			/* Retrieval of data from the broodminder weight.
			 * See 13.2 Appendix B in the Broodminder User Guide for more information */

			data_array[2] = lbs_to_kg((float)(broodminder_data[22] * 256 + broodminder_data[21] - 32767 ) / 100);

			data_array[3] = ((float)(broodminder_data[11] * 256 + broodminder_data[13] - 5000) / 100); // * 9 / 5 + 32 (for Fahrenheit)

			data_array[0]  = lbs_to_kg((float)(broodminder_data[12 + 1] * 256 + broodminder_data[12 + 0] - 32767) / 100);
			
			data_array[1] = lbs_to_kg((float)(broodminder_data[14 + 1] * 256 + broodminder_data[14 + 0] - 32767) / 100);


			LOG_INF("\n");

			LOG_INF("Device found: %s\n", addr);

			LOG_INF("Bm_w advertising data length: %i\n", device_info->adv_data->len);

			LOG_INF("Real Time Weight [Kg]: %.2f\n", data_array[2]);

			LOG_INF("Temperature [Celsius]: %.2f\n", data_array[3]);

			LOG_INF("WeightR [Kg]: %.2f\n", data_array[0]);

			LOG_INF("WeightL [Kg]: %.2f\n", data_array[1]);

			/* Send the data to the peripheral module. */
			struct bm_w_event *bm_w_send = new_bm_w_event();

			bm_w_send->weightR = data_array[0];
			bm_w_send->weightL = data_array[1];
			bm_w_send->realTimeWeight = data_array[2];
			bm_w_send->temperature = data_array[3];

			APP_EVENT_SUBMIT(bm_w_send);
			#if defined(CONFIG_DK_LIBRARY)
			LOG_INF("LED 3  toggled off. Data from BM has been sent from 53 to 91.\n");
			dk_set_led_off(LED_3);
			#endif
		// }
		scanning = false;
		broodminder_scan = false;
	}
	else{
		/* Notify the other modules that a new peripheral has been found. 
		 * The connection is made in the respective modules. */
		struct ble_event *peripheral_found = new_ble_event();

		peripheral_found->type = SCAN_SUCCES;
		peripheral_found->scan_name = peripheral_name;
		peripheral_found->len = strlen(peripheral_name);
		peripheral_found->addr = device_info->recv_info->addr;

		APP_EVENT_SUBMIT(peripheral_found);

		scanning = false;
	}
	
	err = bt_scan_stop();
	if (err) {
		LOG_ERR("Scan stop (err %d)\n", err);
	}
	else{
		k_work_cancel_delayable(&scan_timeout);
		LOG_DBG("Scan stopped");
	}
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		NULL, NULL);

/* Delayable function that cancels the scan.
 * Prevents the unit from scanning indefinitely. */ 
static void scan_timeout_fn(struct k_work *work){
	int err = bt_scan_stop();
	LOG_DBG("Stopping scan");
	if(err){
		LOG_ERR("Failed to stop scan: %i", err);
	}
	scanning = false;
	broodminder_scan = false;
}

void start_scan(char *scan_name, uint8_t len){
	int err = 0;

	//Only start scan if one isn't already active
	if(!scanning){
		clear_string(peripheral_name, CONFIG_BT_SCAN_NAME_MAX_LEN);
		memcpy(peripheral_name, scan_name, len);
		
		//Remove all filters before starting scan to make sure you match with intended device
		bt_scan_filter_remove_all();

		LOG_DBG("Changing filters to %s. Length %i", peripheral_name, strlen(peripheral_name));
		LOG_INF("Changing filters to %s. Length %i", scan_name, strlen(scan_name));

		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, scan_name);
		if (err){
			LOG_ERR("Filters cannot be set (err %d)\n", err);
		}

		err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
		if (err) {
			LOG_ERR("Filters cannot be turned on (err %d)\n", err);
		}

		err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
		if (err) {
			LOG_ERR("Scanning for %s failed to start (err %d)\n", scan_name, err);
		}
		
		scanning = true;

		k_work_schedule(&scan_timeout, K_SECONDS(30));
	}
	else{
		LOG_WRN("Already scanning, try again later");
	}
}

#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)

//Can't read scan response packet if you don't scan for address. Should find a better way to do this.
#define BROODMINDER_ADDR ((bt_addr_le_t[]) { { 0, \
			 { { 0xFD, 0x01, 0x57, 0x16, 0x09, 0x06 } } } })
static void weight_scan_fn(struct k_work *work)
{
	LOG_INF("Scanning for bm_w starting.");
	#if defined(CONFIG_DK_LIBRARY)
	LOG_INF("LED 3 toggled while scanning. \n");
	dk_set_led_on(LED_3);
	#endif
	broodminder_scan = true;
	bt_scan_filter_remove_all();
	int err;
	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, BROODMINDER_ADDR);
	if (err){
		LOG_ERR("Filters cannot be set (err %d)\n", err);
	}

	err = bt_scan_filter_enable(BT_SCAN_ADDR_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)\n", err);
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning for %s failed to start (err %d)\n", "BroodMinder weight", err);
	}
	k_work_reschedule(&weight_interval, K_MINUTES(15));
}
#endif

void scan_module_thread_fn(void)
{
	/*Initializing Buttons and LEDS */ 
	int err;

	LOG_DBG("scan_module_thread have started");
	k_sleep(K_SECONDS(5));
	#if defined(CONFIG_DK_LIBRARY)
	err = dk_leds_init();
	if (err) {
		LOG_ERR("ble_module_thread_fn(): Cannot init LEDs (err: %d)", err);
	}
	else{
		LOG_INF("ble_module_thread_fn(): Leds initialized and set to LED1 = 53 Connected to T52, LED2 = 53 Connected to T91, LED3 = 53 Scanning for BM_Weight.\n");
	}
	#endif

	LOG_DBG("scan_module_thread_fn(): Enabling BLE.");
	err = bt_enable(NULL);
	if (err) {	
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
	LOG_DBG("ble_module_thread_fn(): Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		LOG_DBG("ble_module_thread_fn(): Loading settings\n");
		settings_load();
	}
	
	bt_scan_init(NULL);
	bt_scan_cb_register(&scan_cb);

	k_work_init_delayable(&scan_timeout, scan_timeout_fn);
	#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
	k_work_init_delayable(&weight_interval, weight_scan_fn);
	#endif

	LOG_DBG("scan_module_thread finished, sending a ready message");

	struct ble_event *ble_ready = new_ble_event();

	ble_ready->type = BLE_READY;

	APP_EVENT_SUBMIT(ble_ready);
	LOG_DBG("ble_module_thread_fn(): BLE_READY event submitted.");
}

static bool event_handler(const struct event_header *eh)
{
	if (is_ble_event(eh)) {
		struct ble_event *event = cast_ble_event(eh);
		if(event->type==SCAN_START){
			start_scan(event->scan_name, event->len);
		}
		if(event->type==HUB_CONNECTED){
			// LOG_INF("HUB connected, searching for weight");
			#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
			LOG_INF("HUB connected, searching for weight");
			k_work_reschedule(&weight_interval, K_NO_WAIT);
			#endif
		}
	}
	return false;
}

K_THREAD_DEFINE(scan_module_thread, 1024,
		scan_module_thread_fn, NULL, NULL, NULL,
		K_HIGHEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_event);