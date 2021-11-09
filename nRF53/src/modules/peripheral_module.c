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

K_FIFO_DEFINE(thingy_buffer_fifo);

// static K_FIFO_DEFINE(fifo_uart_tx_data);
// static K_FIFO_DEFINE(fifo_uart_rx_data);

/* Do we need this?
*/
// Payload buffer element size. 
#define THINGY_DATA_SIZE 11

//Create a data item
struct thingy_data_t {
	void *fifo_reserved;
	uint8_t thingy_data[THINGY_DATA_SIZE];
	// uint16_t len; //?
};



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
		struct thingy_data_t *thingy_data; 

		if (sample_counter == THINGY_BUFFER_SIZE){
			THINGY_BUFFER_WRITABLE = false;
			LOG_INF("THINGY_BUFFER_full = false\n");
		}
        LOG_INF("event_handler(): Thingy event is being handled. \n");

		struct thingy_event *event = cast_thingy_event(eh);
		LOG_INF("event_handler(thingy_event): Temperature [C]: %i,%i, Humidity [%%]: %i, Air pressure [hPa]: %d,%i, Battery charge [%%]: %i, ID: %i,\n", event->data_array[0], \
				event->data_array[1], event->data_array[2], event->pressure_int, event->pressure_float, event->battery_charge, id-(uint8_t)'0');

		pressure_union.a = event->pressure_int;

		/* Organizing the sensor data in a 11 byte data message which is sent to 91-module. */
		// uint8_t thingy_data[11];
		thingy_data->thingy_data[0] = (uint8_t)'*';
		thingy_data->thingy_data[1] = id-(uint8_t)'0';
		thingy_data->thingy_data[2] = event->data_array[0];
		thingy_data->thingy_data[3] = event->data_array[1];
		thingy_data->thingy_data[4] = event->data_array[2];
        // uint8_t thingy_data[11] = {(uint8_t)'*', id-(uint8_t)'0', event->data_array[0], event->data_array[1], event->data_array[2]};
	
		/*Divide the 32bit integer for pressure into 4 separate 8bit integers. This is merged back to 32 bit integer when 91 module recieves the data.  */
		for(uint8_t i=0;  i<=3; i++){
			thingy_data->thingy_data[8-i] = (uint8_t)pressure_union.s[i];
		}

		thingy_data->thingy_data[9] = event->pressure_float;
		thingy_data->thingy_data[10] = event->battery_charge;

		/*Append to buffer */
		LOG_INF("Size of THINGY_DATA_BUFFER_SIZE: %i \n", THINGY_BUFFER_SIZE);
		LOG_INF("Current sample number is now %i: \n", sample_counter);
		printk("Current sample: %i", sample_counter);
		for (uint8_t elem = 0; elem <= 10; elem++){
			// printk("test \n");
			printk("\n Current sample elem # %i: \n", elem);
			printk("%i ", thingy_data->thingy_data[elem]);
		printk("\n");	
		}

		if (!THINGY_BUFFER_WRITABLE){
			// LOG_INF("Thingy matrix is full, ejecting oldest entry and moving all entries one index down. \n");
			LOG_INF("Thingy matrix is full, clearing buffer completely (This will change). \n");
			/*	Push all entries one index down. This should leave thingy_matrix[-1][] open to write
				Maybe this can handle the former "delete element 0".
			*/
			// thingy_matrix[THINGY_BUFFER_SIZE][11] = {0};
			// for (uint8_t row = 1; row <= THINGY_BUFFER_SIZE -1 ; row++){
			// 	memset(thingy_matrix, 0, sizeof(thingy_matrix));
			// }

			// This part contains code for testing out FIFO-struct:
			LOG_INF("FIFO: Buffer is full. Clearing oldest sample to make space for newest. \n");
			// k_fifo_get(&thingy_buffer_fifo, K_FOREVER);
			/* 
			Reset the counter to avoid overflow. 
			If it is desired to send this data to also include number of samples. Remember to increase the bitsize in the header to avoid overflow
			and add it to the data message. */
			sample_counter = 0;

			THINGY_BUFFER_WRITABLE = true;
		}


		/*	If there are one or more empty rows in the buffer, append the new data this row */
		if (THINGY_BUFFER_WRITABLE){
			LOG_INF("THINGY_BUFFER_WRITABLE = true\n");

			// LOG_INF("Row(s) in buffer still not filled. Appending thingy data to empty row");
			// for (uint8_t i = 0; i<=10; i++){
			// 	thingy_matrix[sample_counter][i] = thingy_data->thingy_data[i];
			// }
			LOG_INF("Buffer still not full. Adding samples to queue:\n");
			// k_fifo_put(&thingy_buffer_fifo, thingy_data->thingy_data);	
			LOG_INF("Sample has been appended to buffer \n");
			LOG_INF("Thingy data at current row index: \n");
			// k_fifo_peek_head(&thingy_buffer_fifo);
		}
		// LOG_INF("Thingy data at current row index: \n");

		//Only for debugging
		// LOG_INF("Thingy data buffer: \n");
		// for (uint8_t row = 0; row <= THINGY_BUFFER_SIZE-1; row++){
		// 	// printk("test \n");
		// 	printk("\n Row # %i: \n", row);
		// 	for (uint8_t col = 0; col <= 10; col++){
		// 		// printk("col, ");
		// 		printk("%i, ", thingy_matrix[row][col]);
		// 		// printk("%i, \n", thingy_data[col]);
		// 	}
		// 	printk("\n");	
		// }
		/* 
		Sum up values and average here
		This will become some kind of pseudoaverage where int part and decimal part are computed separately.
		Probably we won't need higher precision than int part average.

		Discussion: Could save some bytes by rounding temperature int + float (16byte) to only int (8byte)

		# Jallaverage oppsummering
		*/

		/*If it is not the first sample, include this as the nth sample to ensure
		that the average is divided on correct amount of elements*/
		if (sample_counter >= 0 && !FIRST_SAMPLE){
			LOG_INF("Adding sum of sample number: %i (0 may equal every 12th) \n", sample_counter);
			pressure_int_sum += event->pressure_int;
			pressure_float_sum += event->pressure_float;
			temperature_int_sum += event->data_array[0];
			temperature_float_sum += event->data_array[1];
			humidity_sum += event->data_array[2];
		}

		/*
		We send "normally" to 91 from 53 whenever we've met the following conditions:
		1) Connected to hub
		2) The sample corresponds to every 20th minute

		!sample_counter % THINGY_SAMPLE_TO_SEND should be equal to (4,8,12,16,20 or 24) % 4 for default settings
		3) It is not swarming 
		*/

		LOG_INF("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
        // if(hub_conn && !(sample_counter % THINGY_SAMPLE_TO_SEND)){
		if(!(sample_counter % THINGY_SAMPLE_TO_SEND)){

			LOG_INF("Checking if first sample after start, or if 4th sample and swarmingstate. \n");

			LOG_INF("Hub is connected and sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
			
			/* 
				First sample received after startup and/or restart
			*/
			if (sample_counter == 0 && FIRST_SAMPLE){
				LOG_INF("event_handler(): Hub is connected, and first sample received. Sending sample over nus. \n");
				LOG_INF("Current sample number is now %i: \n", sample_counter);
				printk("Current sample: %i", sample_counter);
				for (uint8_t elem = 0; elem <= 10; elem++){
					// printk("test \n");
					printk("\n Current sample elem # %i: \n", elem);
					printk("%i ", thingy_data->thingy_data[elem]);
				printk("\n");	
				}
				uint8_t first_sample[11];
				memcpy(first_sample, thingy_data -> thingy_data,11);

				printk("First sample: %i", sample_counter);
				for (uint8_t elem = 0; elem <= 10; elem++){
					// printk("test \n");
					printk("\n First sample elem # %i: \n", elem);
					printk("%i ", first_sample[elem]);
				printk("\n");	
				}

				int err = bt_nus_send(hub_conn, &thingy_data, 11);
				// int err = bt_nus_send(hub_conn, &first_sample, 11);
				LOG_INF("First sample: No fatal crash when sending");
				sample_counter += 1;
				LOG_INF("SAMPLE_COUNTER INCREMENTED BY 1");
				FIRST_SAMPLE = false;
			}
			else{

				/* 
					Computing the pseudoaverage of the states we're measuring
				*/
				LOG_INF("4th sample recieved. Computing average and sending \n");
				pressure_int_avg = pressure_int_sum / THINGY_SAMPLE_TO_SEND;
				pressure_float_avg = pressure_float_sum / THINGY_SAMPLE_TO_SEND;
				temperature_int_avg = temperature_int_sum / THINGY_SAMPLE_TO_SEND;
				temperature_float_avg = temperature_float_sum / THINGY_SAMPLE_TO_SEND;
				humidity_avg = humidity_sum  / THINGY_SAMPLE_TO_SEND;

				pressure_avg_union.a = pressure_int_avg;
				// printk("pressure_int_avg = %i \n ", pressure_int_avg);

				/* 
					Resetting the partial sums for next batch of sampling
				*/
				LOG_INF("Clearing partial sums for average computing \n");
				pressure_int_sum = 0;
				pressure_float_sum = 0;
				temperature_int_sum = 0;
				temperature_float_sum = 0;
				humidity_sum = 0;

				uint8_t avg_thingy_data[11] = { (uint8_t)'*', id-(uint8_t)'0', temperature_int_avg, temperature_float_avg, humidity_avg};

				/*Divide the 32bit integer for pressure into 4 separate 8bit integers. This is merged back to 32 bit integer when 91 module recieves the data.  */
				for(uint8_t i=0;  i<=3; i++){
					avg_thingy_data[8-i] = pressure_avg_union.s[i];
				}

				avg_thingy_data[9] = pressure_float_avg;
				avg_thingy_data[10] = event->battery_charge;

				LOG_INF("This is the average data at this time: \n");
				for (uint8_t elem = 0; elem <= 10; elem++){
					// printk("test \n");
					printk("\n Average elem # %i: \n", elem);
					printk("%i ", avg_thingy_data[elem]);
				printk("\n");	
				}

				LOG_INF("Checking swarming event before sending average of the last 4 measurement are beeing sent \n");

				IS_SWARMING = 0;
				// err = is_swarming(data_array)
				if (!IS_SWARMING){
					LOG_INF("No swarming detected. Sending data as normal. \n" );
					LOG_INF("event_handler(): Hub is connected, sending 4th sample over nus. \n");
					/*Send latest average to 91 */
					LOG_INF("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
					sample_counter += 1;
					LOG_INF("SAMPLE_COUNTER INCREMENTED BY 1\n");
					
					// int err = bt_nus_send(hub_conn, avg_thingy_data, 11);
					// int err = bt_nus_send(hub_conn, thingy_data, 11);
					int err = bt_nus_send(hub_conn, avg_thingy_data, 11);
					LOG_INF("Nth sample: No fatal crash when sending");
				}
				if(IS_SWARMING){
					LOG_INF("Swarming detected. Sending data as with a swarming flag (TODO). \n" );
					LOG_INF("event_handler(): Hub is connected, sending 4th sample over nus. \n");
					/*Send latest average to 91 */
					LOG_INF("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
					sample_counter += 1;
					LOG_INF("SAMPLE_COUNTER INCREMENTED BY 1\n");
					
					
					// int err = bt_nus_send(hub_conn, thingy_data, 11);
					int err = bt_nus_send(hub_conn, avg_thingy_data, 11);
					LOG_INF("Nth sample: No fatal crash when sending");
				}
			}
		}
		else{
			LOG_INF("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
			sample_counter += 1;
			LOG_INF("SAMPLE_COUNTER INCREMENTED BY 1\n");
		}
		// return false;
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