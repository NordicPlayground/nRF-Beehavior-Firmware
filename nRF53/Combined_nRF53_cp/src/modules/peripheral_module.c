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

#include <dk_buttons_and_leds.h>

#include <drivers/uart.h>

#include <logging/log.h>

#include "events/ble_event.h"
#include "events/thingy_event.h"
#include "events/bm_w_event.h"
#include "led/led.h"

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
uint16_t T52_Counter = 0;
uint16_t BM_Counter = 0;

// Used for thingy_event
union tagname{
	int a;
	unsigned char s[4];
};

union tagname object;
union tagname object16;

static K_SEM_DEFINE(ble_init_ok, 0, 1);

static struct bt_conn *hub_conn;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};
#define BT_UUID_BEEHAVIOUR_MONITORING_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5b3, 0xf393, 0xe1a9, 0xe50e14dcea9e)

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL) //BT_UUID_BEEHAVIOUR_MONITORING_VAL),
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

		struct ble_event *peripheral_ready = new_ble_event();

		peripheral_ready->type = HUB_CONNECTED;

		EVENT_SUBMIT(peripheral_ready);
	}

}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};

void peripheral_module_thread_fn(void)
{
    /* Don't go any further until BLE is initialized */
    LOG_INF("peripheral_module_thread_fn(): Waiting for sem ble_init_ok, K_SECONDS(30). \n");
	k_sem_take(&ble_init_ok, K_SECONDS(60));
    LOG_INF("peripheral_module_thread_fn(): BLE is initialized. \n");

    int err;

	bt_le_scan_stop();

	LOG_INF("peripheral_module_thread_fn(): Attempting to initialise bt_nus. \n");
	err = bt_nus_init(&nus_cb);
	if (err) {
		LOG_ERR("peripheral_module_thread_fn(): Failed to initialize UART service (err: %d) .\n", err);
		return;
	}

	LOG_INF("peripheral_module_thread_fn(): Attempting to run bt_le_adv_start(). \n");
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("peripheral_module_thread_fn(): Advertising failed to start (err %d) \n", err);
		return;
	}
	LOG_INF("peripheral_module_thread_fn(): bt_nus_init and bt_le_adv_start completed. \n");
	// struct ble_event *peripheral_ready = new_ble_event();

	// peripheral_ready->type = HUB_CONNECTED;

	// EVENT_SUBMIT(peripheral_ready);
	// LOG_INF("peripheral_module_thread_fn(): Peripheral_ready event submitted. \n");
}


static bool event_handler(const struct event_header *eh)
{
	if (is_ble_event(eh)) {
        LOG_INF("event_handler(): BLE event is being handled. \n");
		struct ble_event *event = cast_ble_event(eh);
		if(event->type==THINGY_READY){
			LOG_INF("event_handler(): Thingy ready, giving sem 'ble_init_ok'. \n");
			k_sem_give(&ble_init_ok);
			return false;
		}
		return false;
	}

	/* The comments in this part of the code is only for debugging and implementation. Functional code is in ble_module.c in nrf91*/
    if (is_thingy_event(eh)) {
        LOG_INF("event_handler(): Thingy event is being handled. \n");
		// LOG_INF("Toggling LED 4 while Thingy event is handled.\n");
		// dk_set_led_on(LED_4);
		struct thingy_event *event = cast_thingy_event(eh);
		LOG_INF("Temperature [C]: %i,%i, Humidity [%%]: %i, Air pressure [hPa]: %d,%i ID: %i.\n", event->data_array[0], \
				event->data_array[1], event->data_array[2], event->pressure_int, event->pressure_float, id-(uint8_t)'0');

		// LOG_INF("Hex-version: Temperature [C]: %x,%x, Humidity [%%]: %x, Air pressure [hPa]: %x,%x ID: %x\n", event->data_array[0], \
		// 		event->data_array[1], event->data_array[2], event->pressure_int, event->pressure_float, id-(uint8_t)'0');
		
		object.a = event->pressure_int;
		/*LOG_INF("Object.a = %d, %x \n", object.a, object.a);
		printf("%d\n",sizeof(object));
		printf("%X\n",object.a);
		printf("%X\n", object.s);*/

        uint8_t thingy_data[11] = { (uint8_t)'*', id-(uint8_t)'0', event->data_array[0], event->data_array[1], event->data_array[2]};//,\
									event->pressure_int, event->pressure_float};

		// printf("Individual bytes: \n");
		for(char i=0;  i<=3; i++){
			// printf("index of data_array: %i\n", 8-i);
			// printf("%02X \n",object.s[i]);
			// printf("Test %X \n",object.s[i]);
			thingy_data[8-i] = object.s[i];
		}
		// printf("\n");
		// printf("%d\n",sizeof(thingy_data));
		thingy_data[9] = event->pressure_float;

		// LOG_INF("Checking Thingy_data element 4,5,6,7, 8: %d,%d,%d,%d,%d\n", thingy_data[5], thingy_data[6], thingy_data[7], thingy_data[8],thingy_data[9]);
		// LOG_INF("HEX VERSION Checking Thingy_data element 4,5,6,7, 8: %x,%x,%x,%x,%x \n", thingy_data[5], thingy_data[6], thingy_data[7], thingy_data[8],thingy_data[9]);

		// char temp[4];
		// for (uint8_t i = 5; i <= 8; i++){
		// 	temp[i-5] = thingy_data[i];
		// 	/*printf("index of temp: %i\n", i-5);
		// 	printf("Address of this element: %pn \n",&temp[i-5]);
		// 	printf("Value of element: %X\n", temp[i-5]);*/

		// }
		// printf("\n"); 
		// int32_t tempvar;//= (int32_t)temp;
		// int32_t tempvar2;

		// memcpy(&tempvar, temp, sizeof(tempvar));
		// printf("The number is %X,%i \n",tempvar,tempvar);

		// char reverse_temp[4];
		// for (uint8_t i = 0; i <=3; i++){
		// 	reverse_temp[i] = temp[3-i];
		// 	printf("Index of reverse temp %i \n", i);
		// 	printf("temp[i] after reversing: %X\n", reverse_temp[i]);
		// }

		// memcpy(&tempvar2, reverse_temp, sizeof(tempvar2));
		
    	// printf("The number after reversing is %X,%i \n",tempvar2,tempvar2);

		// printf("From int to array to int %i, %X \n", tempvar, tempvar);
		// printf("From int to array to int %i, %X \n", tempvar2, tempvar2);

        if(hub_conn){
            LOG_INF("event_handler(): Hub is connected, sending thingy_data over nus. \n");
            int err = bt_nus_send(hub_conn, thingy_data, 11);
        }
		// LOG_INF("Toggling LED 4 off after finishing Thingy Event.\n");
		// dk_set_led_off(LED_4);
		return false;
		
	}
	
	if(is_bee_count_event(eh)){
		LOG_INF("Bee Counter event is being handled\n");
		struct bee_count_event *event = cast_bee_count_event(eh);
		uint8_t bee_count_data[6] = { (uint8_t)'*', id-(uint8_t)'0'};

		object16.a = event->out;
		
		for(char i=0;  i<2; i++){
			bee_count_data[3-i] = object16.s[i];
		}

		object16.a = event->in;
		
		for(char i=0;  i<2; i++){
			bee_count_data[5-i] = object16.s[i];
		}
		
        if(hub_conn){
            LOG_INF("Hub is connected\n");
            int err = bt_nus_send(hub_conn, bee_count_data, 6);
        }
		else{
			//Save untill reconnected to nrf91 TO DO
		}
		return false;
	}

	if (is_bm_w_event(eh)) {
        LOG_INF("event_handler(): BM_W event is being handled. \n");
		struct bm_w_event *event = cast_bm_w_event(eh);
		LOG_INF("WeightR: %.2f, WeightL: %.2f, RTWeight: %.2f, Temperature: %.2f, ID: %i .\n", event->weightR, event->weightL, event->realTimeWeight, event->temperature, id-(uint8_t)'0');
        uint8_t bm_w_data[10] = { (uint8_t)'*', id-(uint8_t)'0', (uint8_t)event->weightR, (uint8_t)((event->weightR - (uint8_t)event->weightR) * 100)
			, (uint8_t)event->weightL, (uint8_t)((event->weightL - (uint8_t)event->weightL) * 100)
			, (uint8_t)event->realTimeWeight, (uint8_t)((event->realTimeWeight - (uint8_t)event->realTimeWeight) * 100) 
			, (uint8_t)event->temperature, (uint8_t)((event->temperature - (uint8_t)event->temperature) * 100) };
        if(hub_conn){
            LOG_INF("event_handler(): Hub is connected, sending bm_w_data over nus. ");
            int err = bt_nus_send(hub_conn, bm_w_data, 10);
        }
		else{
			//Save untill reconnected to nrf91 TO DO
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