/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#define MODULE app_module

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <string.h>
#include <logging/log.h>
#include <dk_buttons_and_leds.h>

#include <settings/settings.h>

#include <event_manager.h>

#include "events/thingy_event.h"
#include "events/bm_w_event.h"

#if defined(CONFIG_WATCHDOG_APPLICATION)
#include "watchdog.h"
#endif

LOG_MODULE_REGISTER(MODULE, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

bool led_on;

static bool event_handler(const struct event_header *eh)
{	
	// if (is_cloud_event_abbr(eh)) {

	// 	struct cloud_event_abbr *event = cast_cloud_event_abbr(eh);
	// 	cJSON *obj = NULL;
	// 	obj = cJSON_Parse(event->dyndata.data);
	// 	cJSON *message = cJSON_GetObjectItem(obj, "message");
	// 	if(message!=NULL){
	// 		if(message->valuestring!=NULL){
	// 			if(message->valuestring[0]=='*'){
	// 				LOG_INF("Should be sent to BLE");
	// 				struct ble_event *ble_event_routed = new_ble_event(strlen(message->valuestring));

	// 				ble_event_routed->type = BLE_SEND;
					
	// 				// char *address_ptr = "Placeholder"; //Finne ut hvordan man kan velge mellom to devicer.
	// 				// char address_temp[17];
	// 				// for(int i=0;i<17;i++){
	// 				// 	address_temp[i]=address_ptr[i];
	// 				// }
	// 				char addr[2];
	// 				addr[0]=message->valuestring[0];
	// 				addr[1]=message->valuestring[1];
	// 				// ble_event->address = address_temp;
	// 				memcpy(ble_event_routed->address, addr, 2);

	// 				memcpy(ble_event_routed->dyndata.data, message->valuestring, strlen(message->valuestring));

	// 				EVENT_SUBMIT(ble_event_routed);
	// 				return false;
	// 			}
	// 			if(!strcmp(message->valuestring, "StartScan")){
	// 				scan_start(true);
	// 				return false;
	// 			}
	// 			//LOG_INF("Checkpoint 5");
	// 			LOG_INF("JSON message: %s, Length: %i", message->valuestring, strlen(message->valuestring));
	// 			if(!strcmp(message->valuestring,"LedOn")){
	// 				LOG_INF("Checkpoint 6");
	// 				dk_set_led(DK_LED1, 0);
	// 				return false;
	// 			}
	// 			else if(!strcmp(message->valuestring,"LedOff")){
	// 				LOG_INF("Checkpoint 7");
	// 				dk_set_led(DK_LED1, 1);
	// 				return false;
	// 			}
	// 			else if(!strcmp(message->valuestring,"BLE_LED_ON")){
	// 				struct ble_event *ble_event = new_ble_event(strlen(message->valuestring));

	// 				ble_event->type = BLE_SEND;
					
	// 				char *address_ptr = "Placeholder"; //Finne ut hvordan man kan velge mellom to devicer.
	// 				char address_temp[17];
	// 				for(int i=0;i<17;i++){
	// 					address_temp[i]=address_ptr[i];
	// 				}
	// 				// ble_event->address = address_temp;
	// 				memcpy(ble_event->address, address_temp, 17);

	// 				memcpy(ble_event->dyndata.data, message->valuestring, strlen(message->valuestring));

	// 				EVENT_SUBMIT(ble_event);
	// 				return false;
	// 			}
	// 		}
	// 		return false;
	// 	}
	// 	else{
	// 		LOG_INF("Message is null");
	// 		return false;
	// 	}

	return false;
}

void main(void)
{	
	// int err = dk_leds_init();
	// dk_set_leds_state(DK_ALL_LEDS_MSK, 0);
	// if (err) {
	// 	LOG_ERR("Could not initialize leds, err code: %d\n", err);
	// 	return err;
	// }
	int err;
	if(event_manager_init()){
		LOG_INF("Well this sucks");
	}
	else{
		LOG_INF("All good");
	}

	// #if defined(CONFIG_WATCHDOG_APPLICATION)
	err = watchdog_init_and_start();
	if (err) {
		LOG_INF("watchdog_init_and_start, error: %d", err);
		// SEND_ERROR(app, APP_EVT_ERROR, err);
	}
	LOG_INF("watchdog_init_and_start successful.");
	// #endif	
	
	// struct ble_event *test;
	// /*char test_string[17] ="Testing.........";
	// test->address=test_string;*/
	// EVENT_SUBMIT(test);
}

EVENT_LISTENER(app_module, event_handler);
EVENT_SUBSCRIBE(app_module, thingy_event);
EVENT_SUBSCRIBE(app_module, bm_w_event);
