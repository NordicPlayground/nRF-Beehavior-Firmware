/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#define MODULE app_module

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <logging/log.h>

#include <app_event_manager.h>

LOG_MODULE_REGISTER(MODULE, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

void main(void)
{	

	if(app_event_manager_init()){
		LOG_ERR("Event manager failed to initialize");
		//Restart manually somehow
	}
	else{
		LOG_DBG("Event manager initalized");
	}
}
