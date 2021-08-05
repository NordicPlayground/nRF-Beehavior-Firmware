/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

#define LOG_MODULE_NAME central_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME, 4);

#define REAL_TIME_WEIGHT 0x16
#define BROODMINDER_ADDR ((bt_addr_le_t[]) { { 0, \
			 { { 0xFD, 0x01, 0x57, 0x16, 0x09, 0x06 } } } })


static int scan_init(void);
static bool data_cb(struct bt_data *data, void *user_data);
static uint8_t lbs_to_kg(uint8_t weight);


static uint8_t lbs_to_kg(uint8_t lbs){
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
		for (int i = 0; i < len; i++){
			printk("%x\n", broodminder_data[i]);
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
		uint8_t broodminder_data[device_info->adv_data->len];

		bt_data_parse((device_info->adv_data), data_cb, (void*)broodminder_data);
		printk("\n");

		uint8_t weight_lbs = broodminder_data[REAL_TIME_WEIGHT];
		uint8_t weight_kg = lbs_to_kg(weight_lbs);

		bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

		printk("Weight (lbs): %d\n", weight_lbs);
		printk("Weight (kg): %d\n", weight_kg);
		printk("Device found: %s\n", addr);

	}

}


BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, NULL, NULL);


static int scan_init(void){
	int err = 0;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, BROODMINDER_ADDR);
	if (err){
		LOG_INF("Filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_ADDR_FILTER, false);
    if (err) {
        LOG_INF("Filters cannot be turned on (err %d)", err);
        return err;
	}

	printk("Scan module initialized\n");
    return err;
}



void main(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.options    = BT_LE_SCAN_OPT_CODED,
		.interval   = 0x0010,
		.window     = 0x0010,
	};
	int err;

	printk("Starting Scanner Demo\n");

	

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		LOG_INF("Bluetooth init failed (err %d)\n", err);
		return;
	}

	LOG_INF("Bluetooth initialized\n");
	
	err = scan_init();
	if(err){
		LOG_INF("Scanning failed to initialize");
		return;
	}

	err = bt_scan_params_set(&scan_param);
	if (err){
		LOG_INF("Scanning parameters failed to set");
	}
	
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_INF("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_INF("Scanning succesfully started");
	
}
