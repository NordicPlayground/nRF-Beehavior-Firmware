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
#include "events/bee_count_event.h"
#include "led/led.h"
#include "peripheral_module.h"

#include <drivers/uart.h>

#include <device.h>
#include <soc.h>

// #define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
// #define PRIORITY 7

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)
/*Temp bool for testing*/
bool is_swarming = false;

char id;
uint16_t T52_Counter = 0;
uint16_t BM_Counter = 0;



static K_SEM_DEFINE(ble_init_ok, 0, 1);

static struct bt_conn *hub_conn;

#define BT_UUID_BEEHAVIOUR_MONITORING_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5b3, 0xf393, 0xe1a9, 0xe50e14dcea9e)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_BEEHAVIOUR_MONITORING_VAL),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	// BT_DATA(BT_DATA_UUID128_ALL, BT_UUID_BEEHAVIOUR_MONITORING_VAL, sizeof(BT_UUID_BEEHAVIOUR_MONITORING_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	// BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_BEEHAVIOUR_MONITORING_VAL) //BT_UUID_NUS_VAL) 
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

    if (is_thingy_event(eh)) {
        LOG_INF("event_handler(): Thingy event is being handled. \n");

		struct thingy_event *event = cast_thingy_event(eh);
		LOG_INF("event_handler(thingy_event): Temperature [C]: %i,%i, Humidity [%%]: %i, Air pressure [hPa]: %d,%i, Battery charge [%%]: %i, ID: %i,\n", event->data_array[0], \
				event->data_array[1], event->data_array[2], event->pressure_int, event->pressure_float, event->battery_charge, id-(uint8_t)'0');

		pressure_union.a = event->pressure_int;

		/* Organizing the sensor data in a 11 byte data message which is sent to 91-module. */

		// This is to be replaced with matrix push and pop. FILO?
		// if (sample_counter >= THINGY_BUFFER_SIZE){
		// 	sample_counter = 0;
		// }

        uint8_t thingy_data[11] = { (uint8_t)'*', id-(uint8_t)'0', event->data_array[0], event->data_array[1], event->data_array[2]};

		/*Divide the 32bit integer for pressure into 4 separate 8bit integers. This is merged back to 32 bit integer when 91 module recieves the data.  */
		for(uint8_t i=0;  i<=3; i++){
			thingy_data[8-i] = pressure_union.s[i];
		}

		thingy_data[9] = event->pressure_float;
		thingy_data[10] = event->battery_charge;

		/*First sample after startup is to be sent*/
		// if(hub_conn && sample_counter == 0){
		// 	LOG_INF("event_handler(): Hub is connected, sending bm_w_data over nus. ");
		// 	LOG_INF("event_handler(thingy): Appending first sample after startup to buffer.");
		// 	for (uint8_t i = 0; i<=10; i++){
		// 		thingy_matrix[sample_counter][i] = thingy_data[i];
		// 	}
		// 	sample_counter += 1;		
        //     int err = bt_nus_send(hub_conn, thingy_data, 11);
        // }
		// else{
		// 	//Save untill reconnected to nrf91 TO DO
		// }

		/*Append to buffer */
		LOG_INF("Size of THINGY_DATA_BUFFER_SIZE: %i \n", THINGY_BUFFER_SIZE);
		LOG_INF("Current sample number is now %i: \n", sample_counter);

		for (uint8_t i = 0; i<=10; i++){
			thingy_matrix[sample_counter][i] = thingy_data[i];
		}
		LOG_INF("Sample has been appended to buffer \n");
		LOG_INF("Thingy data at current row index: \n");
		for (uint8_t i = 0; i<=10; i++){
			LOG_INF(" %i, ", thingy_matrix[sample_counter][i]);
		}

		LOG_INF("event_handler(): Printing thingy_matrix for debugging:");
		for (uint8_t row = 0; row <= sample_counter; row++){
			// printk("test \n");
			printk("\n Sample # %i: \n", row);
			for (uint8_t col = 0; col <= 10; col++){
				// printk("col, ");
				printk("%i, ", thingy_matrix[row][col]);
				// printk("%i, \n", thingy_data[col]);
			}
			LOG_INF("\n");	
		}

		/*Otherwise we only send samples corresponding to every 20th minute. For default with T = 5min,
		 this means every 4th sample */

		/*
		We send "normally" to 91 from 53 whenever we've met the following conditions:
		1) Connected to hub
		2) The sample corresponds to every 20th minute

		!sample_counter % THINGY_SAMPLE_TO_SEND should be equal to (4,8,12,16,20 or 24) % 4 for default settings
		3) It is not swarming 
		*/
		
		printk("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
        if(hub_conn && !(sample_counter % THINGY_SAMPLE_TO_SEND)){

			LOG_INF("Hub is connected and sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
			LOG_INF("Checking if first sample after start, or if 4th sample and swarmingstate. \n");
			/*Clear an array with data to be sent*/
			uint8_t latest_thingy_data[11] = {0};

			if (sample_counter == 0){
				LOG_INF("event_handler(): Hub is connected, and first sample received. Sending sample over nus. \n");
				sample_counter += 1;
				LOG_INF("SAMPLE_COUNTER INCREMENTED BY 1");
				int err = bt_nus_send(hub_conn, thingy_data, 11);
			}
			else{
				LOG_INF("NÅ ER VI HER \n");
				/* 
				SEND RELEVANT INFO TO SWARM DETECTION AND RETURN IS_SWARMING
				*/
				/*Average 4 or 20 last measurements recieved*/

				LOG_INF("Computing average of last n samples and preparing to send over NUS");
				for (uint8_t col = 0; col <=1; col++){
					uint8_t temp_val = 0;
					uint8_t avg = 0;
					for (uint8_t row = 3; row >= 0; row--){
						temp_val += thingy_matrix[row][col];
					}
					avg = temp_val / THINGY_SAMPLE_TO_SEND;
				
					/*Append the average of the 4 or 20 last measurements to the empty vector*/ 
					latest_thingy_data[col] = avg;
				}

				LOG_INF("\n Average of last 4 or 20 samples:\n");
				for (uint8_t col = 0; col <= 10; col++){
					// printk("col, ");
					LOG_INF("%i, ", latest_thingy_data[col]);
				}
				LOG_INF("\n");	
			
				// printk("Average of the last 4 measurement are beeing sent \n");
				// for (uint8_t col = 0; col <= 10; col++){
				// 	latest_thingy_data[col] = thingy_matrix[row_index-1][col];
				// 	printk("%i, ", latest_thingy_data[col]);
				// }

				IS_SWARMING = 0;
				if (!IS_SWARMING){
					LOG_INF("No swarming detected. Sending data as normal. \n" );
					LOG_INF("event_handler(): Hub is connected, sending 4th sample over nus. \n");
					/*Send latest average to 91 */
					LOG_INF("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
					sample_counter += 1;
					LOG_INF("SAMPLE_COUNTER INCREMENTED BY 1\n");
					
					int err = bt_nus_send(hub_conn, latest_thingy_data, 11);
					// int err = bt_nus_send(hub_conn, thingy_data, 11);
				}
				if(IS_SWARMING){
					LOG_INF("Swarming detected. Sending data as with a swarming flag (TODO). \n" );
					LOG_INF("event_handler(): Hub is connected, sending 4th sample over nus. \n");
					/*Send latest average to 91 */
					LOG_INF("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
					sample_counter += 1;
					LOG_INF("SAMPLE_COUNTER INCREMENTED BY 1\n");
					
					int err = bt_nus_send(hub_conn, latest_thingy_data, 11);
				}
			}
		}
		else{
			LOG_INF("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
			sample_counter += 1;
			LOG_INF("SAMPLE_COUNTER INCREMENTED BY 1\n");
		}
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
EVENT_SUBSCRIBE(MODULE, bee_count_event);