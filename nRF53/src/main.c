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

#include <app_event_manager.h>
#include <drivers/gpio.h>
#include <sys/reboot.h>

LOG_MODULE_REGISTER(MODULE, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

bool led_on;

void main(void)
{	

	LOG_INF("In main() main.c");

#if defined(CONFIG_THINGY53)
	/* Disable unnecessary peripherals to save power. */ 
	const struct device *gpio0_dev;
	gpio0_dev = device_get_binding("GPIO_0");
	int err = gpio_pin_configure(gpio0_dev, 31, GPIO_OUTPUT_LOW); //SENS POWER CONTROL
	err = gpio_pin_configure(gpio0_dev, 8, GPIO_OUTPUT_LOW); //PMIC MODE
	err = gpio_pin_configure(gpio0_dev, 18, GPIO_OUTPUT_HIGH); //Set External flash CS' High to turn it off
	err = gpio_pin_configure(gpio0_dev, 22, GPIO_OUTPUT_HIGH); //Set ADXL365 CS' High to turn it off

	/* Toggle the red led to signal that the program has started */
	err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_LOW); //3V3 ENABLE (LOW means on, need for LED's) 
	const struct device *gpio1_dev;
	gpio1_dev = device_get_binding("GPIO_1");
	err = gpio_pin_configure(gpio1_dev, 8, GPIO_OUTPUT_HIGH); //Red led	
	
	k_sleep(K_MSEC(500));
	
	err = gpio_pin_configure(gpio1_dev, 8, GPIO_OUTPUT_LOW);  //Red led	
	err = gpio_pin_configure(gpio0_dev, 15, GPIO_OUTPUT_HIGH); //3V3 ENABLE (HIGH means off)
	#endif
	if(app_event_manager_init()){
		LOG_INF("Well this sucks");
	}
	else{
		LOG_INF("All good");
		uint32_t resetreas = NRF_RESET->RESETREAS;
		LOG_INF("%08x", resetreas);
	}
}
