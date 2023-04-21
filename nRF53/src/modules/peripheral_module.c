/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <app_event_manager.h>

#include <errno.h>
#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys///printk.h>
#include <stdio.h>
#include <string.h>
#include <sys///printk.h>

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

// #include <dk_buttons_and_leds.h>

#include <drivers/uart.h>
#include <drivers/gpio.h>

#include <logging/log.h>

#include "events/ble_event.h"
#include "events/thingy_event.h"
#include "events/bm_w_event.h"
#include "events/bee_count_event.h"
#include "events/woodpecker_event.h"
#include "events/wdt_event.h"
#include "events/nvs_event.h"
// #include "led/led.h"
#include "peripheral_module.h"

#include <drivers/uart.h>

#include <device.h>
#include <soc.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

/*Temp bool for testing*/
bool is_swarming = false;

/* ID of this unit given to the nRF91. */
char id;
/* Variables used for the Thingy:52 data processing. */
static uint16_t sample_counter = 0;
static uint8_t thingy_buf_idx = 0;

static K_SEM_DEFINE(ble_init_ok, 0, 1);

static struct bt_conn *hub_conn;

/* Custom UUID that the nRF91 scans for. */
#define BT_UUID_BEEHAVIOUR_MONITORING_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5b3, 0xf393, 0xe1a9, 0xe50e14dcea9e)

/* Advertising data packet. */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_BEEHAVIOUR_MONITORING_VAL),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* Scan response data packet. */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
			  uint16_t len)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN] = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));
	
	char data_received[len];
	memcpy(&data_received, data, len);

	LOG_INF("Received data from %s: %s\n", log_strdup(addr), log_strdup(data_received));

	/* If the unit has gotten a new ID, it will be in the form of "*<new_id>". */
	if(data_received[0]=='*'){
		id=data_received[1];
		LOG_INF("New ID is: %c\n", id);
        hub_conn = conn;

		#if defined(CONFIG_THINGY53)
		/* Toggle the green light to signal that the Thingy:53 has connected to the nRF91. */
		const struct device *gpio0_dev;
		gpio0_dev = device_get_binding("GPIO_0");
		err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_LOW); //3V3 ENABLE (LOW means on)
		const struct device *gpio1_dev;
		gpio1_dev = device_get_binding("GPIO_1");
		err = gpio_pin_configure(gpio1_dev, 6, GPIO_OUTPUT_HIGH); //Green led	
		k_sleep(K_MSEC(500));
		err = gpio_pin_configure(gpio1_dev, 6, GPIO_OUTPUT_LOW);  //Green led	
		err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_HIGH); //3V3 ENABLE (LOW means on)
		#endif
		
		/* Notify the other modules that the central has connected. */ 
		struct ble_event *peripheral_ready = new_ble_event();

		peripheral_ready->type = HUB_CONNECTED;

		APP_EVENT_SUBMIT(peripheral_ready);
	}
}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};

// Connection parameters to decrease power consumption
static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(0x140, 0x140, 0, 100);
	// Interval = N * 1.25 ms = 0x140 * 1.25 ms = 400 ms , timeout = N * 10 ms = 1 s
	// FYI: Max connection interval allowed is 4 seconds.

/*------------------------- Connectivity, scanning and pairing functions -------------------------- */
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;

	char addr[BT_ADDR_LE_STR_LEN];
	
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_INF("connected(): Failed to connect to %s (%u). \n", addr, conn_err);
		return;
	}

	struct bt_conn_info conn_info;
	
	bt_conn_get_info(conn, &conn_info);

	LOG_DBG("connected(): Type: %i, Role: %i, Id: %i. \n", conn_info.type, conn_info.role, conn_info.id);
		
	LOG_DBG("connected(): Connected: %.17s", log_strdup(addr));

	/* Check if the new connection is a central (conn_info.role=1 means central). */
	if(conn_info.role){
		LOG_INF("connected(): Connected to central hub (91). \n");
		LOG_INF("Setting LED 2 for successful connection with 91. \n");
		//dk_set_led_on(LED_2);
		//Update connection parameters to reduce power consumption

		// // Send NVS to nRF91
		// // Create new nvs to nRF91 event.
		// struct nvs_event *nvs_send_to_nrf91 = new_nvs_event();
		// nvs_send_to_nrf91->type = NVS_SEND_TO_NRF91;
		// // Submit nvs to nRF91 event.
		// APP_EVENT_SUBMIT(nvs_send_to_nrf91);

		// k_sleep(K_MSEC(1));

		// // Wiping NVS
		// // Create new nvs wipe event.
		// struct nvs_event *nvs_wipe = new_nvs_event();
		// nvs_wipe->type = NVS_WIPE;
		// // Submit nvs wipe event.
		// APP_EVENT_SUBMIT(nvs_wipe);

		err = bt_conn_le_param_update(conn, conn_param);
		if (err) {
			LOG_ERR("Connection parameters update failed: %d",
					err);
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	struct bt_conn_info conn_info;
	
	bt_conn_get_info(conn, &conn_info);
	//Check if connection role is central (role = 1).
	if(conn_info.role){
		//dk_set_led_off(LED_2);
		LOG_WRN("disconnected(): Bluetooth disconnection occured: %s (reason %u)\n", log_strdup(addr),	reason);
		LOG_INF("Hub/nRF91 disconnected.");
		LOG_INF("disconnected(): Attempting to run bt_le_adv_start(). \n");
		err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
					ARRAY_SIZE(sd));
		if (err) {
			LOG_ERR("disconnected(): Advertising failed to start (err %d) \n", err);
			//TODO: reboot or schedule advertising later.
			return;
		}
	}
}
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u\n", log_strdup(addr),
			level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d\n", log_strdup(addr),
			level, err);
	}
}
static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param){
	LOG_DBG("%d, %d",param->interval_min,param->interval_max);
	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout){
	LOG_INF("Param updated %d, %d, %d",interval,latency, timeout);			
}

/* ------------------ Connected struct   ---------------------- */
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

uint8_t *thingy_event_add_to_data_message(struct thingy_event *event)
{
/* 
	Takes in latest measurement and appends the data to uint8_t thingy_data[11]

	thingy_data bits are represented as:

	[0] 	: (uint8_t)'*'
	[1] 	: id-(uint8_t)'0'
	[2] 	: event->data_array[0]
	[3] 	: event->data_array[1]
	[4] 	: event->data_array[2]
	[5-8] 	: integer part of relative air pressure in union-representation
	[9] 	: part of relative air pressure
	[10] 	: battery charge  

	returns 0 if successfull, 1 if error has occured

 */
	LOG_INF("thingy_event_add_to_data_message(thingy_event): Testing appending through function call instead of normal data_append. \n");
	LOG_INF("thingy_event_add_to_data_message(thingy_event): Temperature [C]: %i,%i, Humidity [%%]: %i, Air pressure [hPa]: %d,%i, Battery charge [%%]: %i, ID: %i,\n", event->data_array[0], \
				event->data_array[1], event->data_array[2], event->pressure_int, event->pressure_float, event->battery_charge, id-(uint8_t)'0');
	pressure_union.a = event->pressure_int;

	/* 
	
	Organizing the sensor data in a 11 byte data message which is sent to 91-module.
	A change here is that data_message is a static opposed to uint8_t data_message[11] = {(uint8_t)'*', id-(uint8_t)'0', event->data_array[0], event->data_array[1], event->data_array[2]};
	*/
	static uint8_t data_message[11];
	data_message[0] = (uint8_t)'*';
	data_message[1] = id-(uint8_t)'0';
	data_message[2] = event->data_array[0];
	data_message[3] = event->data_array[1];
	data_message[4] = event->data_array[2];

	/*Divide the 32bit integer for pressure into 4 separate 8bit integers. This is merged back to 32 bit integer when 91 module recieves the data.  */
	/*Data message elements number 5,6,7 and 8 */
	for(uint8_t i=0;  i<=3; i++){
		data_message[8-i] = (uint8_t)pressure_union.s[i];
	}

	data_message[9] = event->pressure_float;
	data_message[10] = event->battery_charge;
	
	return data_message;  
}

static int thingy_event_push_buffer(){
	/*
	Pseudocode for algorithm
	if (buffer full) do:
		* Push all rows one index down

		* Clear the row with highest index, i.e thingy_matrix[buf_size - 1]

		* Check if the latest row is all zeroes

		* If it is all zeroes, return err = 0
		
	*/
	int err;

	//Move all rows one index down
	for (uint8_t i = 1; i < THINGY_BUFFER_SIZE; i++){
		memcpy(thingy_matrix[i-1], thingy_matrix[i], sizeof(thingy_matrix[i]));
	} 

	//Clear the latest row
	memset(thingy_matrix[THINGY_BUFFER_SIZE-1], 0, sizeof(thingy_matrix[0]));

	// If the last row in the buffer is 0, then we've cleared the row
 	if (strlen(thingy_matrix[THINGY_BUFFER_SIZE - 1]) == 0) { 
		// LOG_INF("thingy_event_push_buffer(): Thingy_buffer[THINGY_BUFFER_SIZE] is zeroed \n");
        err = 0;
    } 
    else { 
		// LOG_INF("thingy_event_push_buffer(): Thingy_buffer[THINGY_BUFFER_SIZE] has non-zero elements \n");
	    err = 1;
    }
	return err;
}

// struct temp_sums_for_avg *temp_sums,
int *thingy_event_compute_temp_sums( struct thingy_event *event ){

		LOG_INF("Adding sum of sample number: %i (0 may equal every 12th) \n", sample_counter);
		temp_sums.pressure_int_sum += event->pressure_int;
		temp_sums.pressure_float_sum += event->pressure_float;
		temp_sums.temperature_int_sum += event->data_array[0];
		temp_sums.temperature_float_sum += event->data_array[1];
		temp_sums.humidity_sum += event->data_array[2];

	return 0;
}


void peripheral_module_thread_fn(void)
{
    /* Don't go any further until BLE is initialized */
	bt_conn_cb_register(&conn_callbacks);

    LOG_INF("peripheral_module_thread_fn(): Waiting for sem ble_init_ok, K_SECONDS(30). \n");
	k_sem_take(&ble_init_ok, K_SECONDS(60));
    LOG_INF("peripheral_module_thread_fn(): BLE is initialized. \n");

    int err;

	/* Stops scans, before it starts to advertise. */
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
		//reboot or schedule advertising later
		return;
	}

	LOG_INF("peripheral_module_thread_fn(): bt_nus_init and bt_le_adv_start completed. \n");

	
	// ***** WDT ***** //

	// // ** Send timed out wdt channel, registered in NVS, to nRF91 **
	// struct nvs_event *nvs_send_to_nrf91 = new_nvs_event();
	// nvs_send_to_nrf91->type = NVS_SEND_TO_NRF91;
	// // Submit send to nRF91 event.
	// APP_EVENT_SUBMIT(nvs_send_to_nrf91);

	// k_sleep(K_MSEC(1));

	// // ** Wipe NVS **
	// struct nvs_event *nvs_wipe = new_nvs_event();
	// nvs_wipe->type = NVS_WIPE;
	// // Submit NVS wipe event.
	// APP_EVENT_SUBMIT(nvs_wipe);

	// Set up wdt.
	struct wdt_event *wdt_setup = new_wdt_event();
	wdt_setup->type = WDT_SETUP;
	// Submit wdt setup event.
	APP_EVENT_SUBMIT(wdt_setup);

	// Add wdt channel.
	struct wdt_event *wdt_add_main = new_wdt_event();
	wdt_add_main->type = WDT_ADD_MAIN;
	// Submit wdt add main event.
	APP_EVENT_SUBMIT(wdt_add_main);
	// ***** *** ***** //

	k_sleep(K_MSEC(1));
	
	// // ** Send timed out wdt channel, registered in NVS, to nRF91 **
	// struct nvs_event *nvs_send_to_nrf91 = new_nvs_event();
	// nvs_send_to_nrf91->type = NVS_SEND_TO_NRF91;
	// // Submit send to nRF91 event.
	// APP_EVENT_SUBMIT(nvs_send_to_nrf91);

	// k_sleep(K_MSEC(1));

	// // ** Wipe NVS **
	// struct nvs_event *nvs_wipe = new_nvs_event();
	// nvs_wipe->type = NVS_WIPE;
	// // Submit NVS wipe event.
	// APP_EVENT_SUBMIT(nvs_wipe);

	// k_sleep(K_MSEC(1));

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

		if(event->type==BLE_NVS_SEND_TO_NRF91){
			
			// LOG_INF("event_handler(): Sending NVS to nRF91 over BLE, giving sem 'ble_init_ok'. \n");
			// k_sem_give(&ble_init_ok);
			
			LOG_INF("event_handler(): Data stored (channel %d timeout) in NVS being sent to nRF91.\n", event->wdt_channel_id);

			uint8_t nvs_wdt_data[3] = { (uint8_t)'*', id-(uint8_t)'0', event->wdt_channel_id};
			nvs_wdt_data[2] = event->wdt_channel_id;

			uint8_t nvs_wdt_data_to_send[sizeof(nvs_wdt_data)];
			// LOG_DBG("sizeof(nvs_wdt_data) = %d", sizeof(nvs_wdt_data));
			memcpy(nvs_wdt_data_to_send, nvs_wdt_data, sizeof(nvs_wdt_data));

        	int err = bt_nus_send(hub_conn, nvs_wdt_data_to_send, sizeof(nvs_wdt_data_to_send));
			LOG_DBG("Trying to send: err %d; %d; %d = %d", 
			nvs_wdt_data_to_send[0], nvs_wdt_data_to_send[1], nvs_wdt_data_to_send[2], event->wdt_channel_id);
			LOG_DBG("is_ble_event(eh): bt_nus_send() returns: err %d", err);

			// k_sleep(K_MSEC(1));

        	// if(hub_conn){
        	//     LOG_INF("event_handler(): Hub is connected, sending nvs_wdt_data over nus. ");

			// 	uint8_t nvs_wdt_data_to_send[sizeof(nvs_wdt_data)];
			// 	memcpy(nvs_wdt_data_to_send, nvs_wdt_data, sizeof(nvs_wdt_data));

        	//  	int err = bt_nus_send(hub_conn, nvs_wdt_data_to_send, sizeof(nvs_wdt_data_to_send));

			// 	k_sleep(K_MSEC(1));
        	// }
			// else{
			// 	//TODO: Save untill reconnected to nrf91 TO DO
			// 	LOG_WRN("event_handler(): Hub is NOT connected, cannot send nvs_wdt_data over nus. ");
			// 	return false;
			// }
			return false;
		}
		return false;
	}

	//Should move the logic to mean the Thingy samples to the thingy or a general purpose mean/data processing module
    if (is_thingy_event(eh)) {
   
		LOG_INF("event_handler(): Thingy event is being handled. \n");
		struct thingy_event *event = cast_thingy_event(eh);
		
		/* 
		Step 1:
		Add thingy measurement to thingy_data message
		
		*/
		if (FIRST_SAMPLE){
			THINGY_BUFFER_WRITABLE = true;
		}

		uint8_t thingy_data[11];
		memcpy(thingy_data, thingy_event_add_to_data_message(event),sizeof(thingy_data));

		/* 
		%%%%%%%%%	Print check to check contents of thingy_data at this time	%%%%%%%%%
		*/

		// //printk("Testing to print data_message OUTSIDE of function call. \n This should be same values as INSIDE print \n");
		// for ( uint8_t i = 0; i < 10; i++ ) {
        //     printf( "*(thingy_data + %d) : %d\n", i, *(thingy_data + i));
		// }
		
		/*	
		
		Step 2: Check if buffer matrix is writable i.e not full
		
		Step 2a): 
		If there are no empty rows in buffer, move all rows one index down, 
		and clear the row with highest index
		
		*/
		if (THINGY_BUFFER_WRITABLE == false){
			LOG_INF("Thingy_buffer is full, clearing room for new sample \n");

			int err = thingy_event_push_buffer();
			if (!err){
				thingy_buf_idx -= 1;
				LOG_INF("thingy_buf_idx decremented, thingy_buf_idx = %i \n", thingy_buf_idx);
				THINGY_BUFFER_WRITABLE = true;
				LOG_INF("THINGY_BUFFER_WRITABLE set to true, THINGY_BUFFER_WRITABLE = %i \n", THINGY_BUFFER_WRITABLE);
			}
			
		}
			

		/*	
		Step 2b): Append the latest sample to the buffer
		*/
		if (THINGY_BUFFER_WRITABLE == true){

			LOG_INF("Thingy_buffer not full. Adding latest sample to buffer\n");
			
			memcpy(thingy_matrix[thingy_buf_idx], thingy_data, sizeof(thingy_data));

			/*
			If everything is done correct, then this should work. NB there are no error catch. 
			This should probably be handled.


			The following lines should be in its own function, so feel free to fix it if desired.
			*/
			int err;
			if (memcmp(thingy_matrix[thingy_buf_idx],thingy_data,sizeof(thingy_data)) == 0){
				err = 0;
			}
			if (!err){
				thingy_buf_idx += 1;
				LOG_INF("thingy_buf_idx incremented, thingy_buf_idx = %i \n", thingy_buf_idx);
				if(thingy_buf_idx >= THINGY_BUFFER_SIZE ){
					LOG_INF("thingy_buf_idx >= THINGY_BUFFER_SIZE, setting write flag to false \n");
					THINGY_BUFFER_WRITABLE = false;
				}
			}
			// If it is not the first sample, add the values to temp-vals for average computation
			if (sample_counter > 0 && !FIRST_SAMPLE){
				thingy_event_compute_temp_sums(event);
				LOG_INF("Partial sums have been added");
			}
		}	

	    
	    // Print code to check contents of thingy_buffer at this given time

		//printk("Thingy data buffer: \n");
		for (uint8_t row = 0; row <= THINGY_BUFFER_SIZE-1; row++){
			//printk("\n Row # %i: \n", row);
			for (uint8_t col = 0; col <= 10; col++){
		 		//printk("%i, ", thingy_matrix[row][col]);
		 	}
		 	//printk("\n");	
		 }
		
		/* 
		Step 3: Sending data from 53 to 91

		Consists of 3 cases
		3a): 
		Edge case If this is the first sample received, we want to send it instantly

		3b): 
		Check if the current sample corresponds with when we should send, which 
		in turn depends on if we're in power save mode or high res mode. This is done by checking if
		!(sample_counter % THINGY_SAMPLE_TO_SEND) == 0

		3bI):
		Check swarm condition

		3c):
		If !(sample_counter % THINGY_SAMPLE_TO_SEND) != 0, then we skip sending, but append the
		partial sums so we can compute the pseudoaverage when we're set to send again.

		*/

		LOG_INF("Check if it is time to send: sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
		if(!(sample_counter % THINGY_SAMPLE_TO_SEND)){

			LOG_INF("Checking if first sample after start, or if 4th sample and swarmingstate. \n");
			LOG_INF("Hub is connected and sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
			
			/* 
			Step 3a): 
			First sample received after startup and/or restart: Do normal sending
				
			*/
			if (sample_counter == 0 && FIRST_SAMPLE){
				LOG_INF("event_handler(): Hub is connected, and first sample received. Sending sample over nus. \n");
				LOG_INF("Current sample number is now: %i \n", sample_counter);
				
				uint8_t first_sample[11];				
				memcpy(first_sample, thingy_data, sizeof(thingy_data));

				//printk("First sample: %i", sample_counter);
				for (uint8_t elem = 0; elem <= 10; elem++){
					//printk("\n First sample elem # %i: \n", elem);
					//printk("%i ", first_sample[elem]);
				//printk("\n");	
				}

				int err = bt_nus_send(hub_conn, first_sample, 11);
				LOG_DBG("1st is_thingy_event(eh): bt_nus_send() returns: err %d", err);
				if (!(err))	{
					LOG_INF("First sample: No fatal crash when sending");
				}
				else {
					LOG_INF("First sample: Could not be sent, err %d", err);
				}
				sample_counter += 1;
				LOG_INF("sample_counter incremented, now at sample num %i \n ", sample_counter);
				FIRST_SAMPLE = false;
			}
			
			/*
			Step 3b): THIS SHOULD BE MADE INTO ITS OWN FUNCTION
			Modulo condition has been met and it is not FIRST_SAMPLE
			*/
			else{

				/* 

				1) Computing the pseudoaverage of the states we're measuring
					
				*/
				LOG_INF("4th sample recieved. Computing average and sending \n");
				pressure_int_avg = temp_sums.pressure_int_sum / THINGY_SAMPLE_TO_SEND;
				pressure_float_avg = temp_sums.pressure_float_sum / THINGY_SAMPLE_TO_SEND;
				temperature_int_avg = temp_sums.temperature_int_sum / THINGY_SAMPLE_TO_SEND;
				temperature_float_avg = temp_sums.temperature_float_sum / THINGY_SAMPLE_TO_SEND;
				humidity_avg = temp_sums.humidity_sum  / THINGY_SAMPLE_TO_SEND;

				pressure_avg_union.a = pressure_int_avg;

				/* 
				
				Resetting the partial sums for next batch of sampling
					
				*/
				
				LOG_INF("Clearing partial sums for average computing \n");
				
				temp_sums.pressure_int_sum = 0;
				temp_sums.pressure_float_sum = 0;
				temp_sums.temperature_int_sum = 0;
				temp_sums.temperature_float_sum = 0;
				temp_sums.humidity_sum = 0;
				
				/* 
				
				Append the average data to the avg_thingy_data message we send to the 91
				
				*/
				

				uint8_t avg_thingy_data[11] = { (uint8_t)'*', id-(uint8_t)'0', temperature_int_avg, temperature_float_avg, humidity_avg};

				/*Divide the 32bit integer for pressure into 4 separate 8bit integers. This is merged back to 32 bit integer when 91 module recieves the data.  */
				for(uint8_t i=0;  i<=3; i++){
					avg_thingy_data[8-i] = pressure_avg_union.s[i];
				}

				avg_thingy_data[9] = pressure_float_avg;
				avg_thingy_data[10] = event->battery_charge;
				
				/* 
				
				This is used to verify that the data message we send is correct (i.e that the computations are correct)
				
				*/
				
				LOG_INF("This is the average data at this time: \n");
				for (uint8_t elem = 0; elem <= 10; elem++){
					//printk("\n Average elem # %i: \n", elem);
					//printk("%i ", avg_thingy_data[elem]);
				//printk("\n");	
				}

				LOG_INF("Checking swarming event before sending average of the last 4 measurement are beeing sent \n");

				
				/* 
				
				This is a dummy control variable. TODO by the fortunate student who creates the swarm detection
				algorithm
				
				*/
				IS_SWARMING = 0;
				
				// err = is_swarming(thingy_matrix)
				
				if (!IS_SWARMING){
					LOG_INF("No swarming detected. Sending data as normal. \n" );
					LOG_INF("event_handler(): Hub is connected, sending 4th sample over nus. \n");
					/*Send latest average to 91 */
					LOG_INF("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
					sample_counter += 1;
					LOG_INF("sample_counter incremented, now at sample num %i \n ", sample_counter);

					int err = bt_nus_send(hub_conn, thingy_data, 11);
					// int err = bt_nus_send(hub_conn, avg_thingy_data, 11);
					LOG_DBG("2nd is_thingy_event(eh): bt_nus_send() returns: err %d", err);
					LOG_INF("Nth sample: No fatal crash when sending");

					/* Try to send NVS data here, as 53 is likely connected to 91 */
					
					// Send NVS to nRF91
					// Create new nvs to nRF91 event.
					struct nvs_event *nvs_send_to_nrf91 = new_nvs_event();
					nvs_send_to_nrf91->type = NVS_SEND_TO_NRF91;
					// Submit nvs to nRF91 event.
					APP_EVENT_SUBMIT(nvs_send_to_nrf91);
			
					k_sleep(K_MSEC(1));
			
					// Wiping NVS
					// Create new nvs wipe event.
					struct nvs_event *nvs_wipe = new_nvs_event();
					nvs_wipe->type = NVS_WIPE;
					// Submit nvs wipe event.
					APP_EVENT_SUBMIT(nvs_wipe);

					/* **** */
				}
				
				if(IS_SWARMING){
					LOG_INF("Swarming detected. Sending data as with a swarming flag (TODO). \n" );
					LOG_INF("event_handler(): Hub is connected, sending 4th sample over nus. \n");
					/*Send latest average to 91 */
					LOG_INF("sample_counter %% THINGY_SAMPLE_TO_SEND = %i \n", sample_counter % THINGY_SAMPLE_TO_SEND);
					sample_counter += 1;
					LOG_INF("sample_counter incremented, now at sample num %i \n ", sample_counter);
					
					
					int err = bt_nus_send(hub_conn, thingy_data, 11);
					// int err = bt_nus_send(hub_conn, avg_thingy_data, 11);
					LOG_DBG("3rd is_thingy_event(eh): bt_nus_send() returns: err %d", err);
					LOG_INF("Nth sample: No fatal crash when sending");
				}
			}
		}
			/* 
			
			Step 3c): 
			If we're not ready to send (i.e haven't satisfied the modulo-condition), just increment the sample counter and don't initiate
			data transmission
			
			*/
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
			//TODO: Save untill reconnected to nrf91 TO DO
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
			//TODO: Save untill reconnected to nrf91 TO DO
		}
		return false;
	}
	if (is_woodpecker_event(eh)) {
        LOG_INF("event_handler(): woodpecker event is being handled. \n");
		struct woodpecker_event *event = cast_woodpecker_event(eh);
		LOG_INF("Total triggers: %i, Positive triggers: %i, Highest probability: %i, ID: %i .\n", event->total_triggers, event->positive_triggers, event->highest_probability, id-(uint8_t)'0');
		uint8_t woodpecker_data[7] = { (uint8_t)'*', id-(uint8_t)'0', event->positive_triggers};
        							
		object16.a = event->total_triggers;
		
		for(char i=0;  i<2; i++){
			woodpecker_data[4-i] = object16.s[i];
		}

		woodpecker_data[5] = event->highest_probability;
		woodpecker_data[6] = event->bat_percentage;

		if(hub_conn){
            LOG_INF("event_handler(): Hub is connected, sending woodpecker_data over nus. ");
            int err = bt_nus_send(hub_conn, woodpecker_data, 7);
        }
		else{
			//TODO: Save untill reconnected to nrf91 TO DO
		}
		return false;
	}
    
	return false;
}

K_THREAD_DEFINE(peripheral_module_thread, 1024,
		peripheral_module_thread_fn, NULL, NULL, NULL,
		K_HIGHEST_APPLICATION_THREAD_PRIO + 2, 0, 0);


APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_event);
APP_EVENT_SUBSCRIBE(MODULE, thingy_event);
APP_EVENT_SUBSCRIBE(MODULE, bm_w_event);
APP_EVENT_SUBSCRIBE(MODULE, bee_count_event);
APP_EVENT_SUBSCRIBE(MODULE, woodpecker_event);
