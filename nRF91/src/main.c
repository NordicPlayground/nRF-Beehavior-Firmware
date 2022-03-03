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

#include "events/ble_event.h"
#include "events/cloud_event.h"

#include "cJSON.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

void main(void)
{	

	if(event_manager_init()){
		LOG_ERR("Event manager failed to initialize");
		//Restart manually somehow
		// k_sleep(K_SECONDS(5));
		// sys_reboot();
	}
	else{
		LOG_DBG("Event manager initalized");
	}
}