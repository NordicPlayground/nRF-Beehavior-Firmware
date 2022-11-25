/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
** This module is based on the task_wdt sample in "...\zephyr\samples\subsys\task_wdt".
*/

#include <zephyr.h>
#include <device.h>
#include <drivers/watchdog.h>
#include <sys/reboot.h>
#include <task_wdt/task_wdt.h>
#include <sys/printk.h>
#include <stdbool.h>
#include "events/wdt_event.h"
#include "events/nvs_event.h"
#include <logging/log.h>
#include "wdt_module.h"

#define MODULE wdt_module
LOG_MODULE_REGISTER(MODULE, 4);

// Reset times for the different watchdog timer channels (1000 ms = 1 s):

#define RESET_TIME_MAIN 120000U
#define RESET_TIME_NRF53 30000U

/*
 * To use this sample, either the devicetree's /aliases must have a
 * 'watchdog0' property, or one of the following watchdog compatibles
 * must have an enabled node.
 *
 * If the devicetree has a watchdog node, we get the watchdog device
 * from there. Otherwise, the task watchdog will be used without a
 * hardware watchdog fallback.
 */

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_wdt)   // Check compatibility with Nordic nRF board.
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_wdt)
#else
__ASSERT(0, "Device not compatible with Nordic nRF board.\n");
#endif

int wdt_channel_main;
int wdt_channel_nrf53;

static void wdt_callback_main_event(uint8_t channel_id, void *user_data)
{
	// Create new main feed event.
	struct wdt_event *wdt_feed_main = new_wdt_event();
	wdt_feed_main->type = WDT_FEED_MAIN;
	wdt_channel_main = channel_id; // wdt_channel_id to be fed must be set to timed out channel id.
	// Submit main feed event.
	APP_EVENT_SUBMIT(wdt_feed_main);

	// Create new general write before reboot event.
	struct nvs_event *nvs_write_before_reboot = new_nvs_event();
	nvs_write_before_reboot->type = NVS_WRITE_BEFORE_REBOOT;
	nvs_write_before_reboot->wdt_channel_id = WDT_CHANNEL_NRF91_MAIN; // wdt_channel_id to be written to nvs must be set to timed out channel id.
	LOG_DBG("################## %d (actual %d) ################## WDT MAIN CB\n", 
			nvs_write_before_reboot->wdt_channel_id, channel_id); // Debugging
	// Submit main write before reboot event.
	APP_EVENT_SUBMIT(nvs_write_before_reboot);

	// Create new general wdt timeout event.
	struct wdt_event *wdt_timeout = new_wdt_event();
	wdt_timeout->type = WDT_TIMEOUT;
	wdt_timeout->wdt_channel_id = WDT_CHANNEL_NRF91_MAIN; // Setting wdt_channel_id to timed out channel id.
	// Submit general wdt timeout event.
	APP_EVENT_SUBMIT(wdt_timeout);
}

static void wdt_callback_nrf53_event(uint8_t channel_id, void *user_data)
{
	// Create new nRF53 feed event.
	struct wdt_event *wdt_feed_nrf53 = new_wdt_event();
	wdt_feed_nrf53->type = WDT_FEED_NRF53;
	wdt_channel_nrf53 = channel_id; // wdt_channel_id to be fed must be set to timed out channel id.
	// Submit nRF53 feed event.
	APP_EVENT_SUBMIT(wdt_feed_nrf53);

	// Create new general write before reboot event.
	struct nvs_event *nvs_write_before_reboot = new_nvs_event();
	nvs_write_before_reboot->type = NVS_WRITE_BEFORE_REBOOT;
	nvs_write_before_reboot->wdt_channel_id = WDT_CHANNEL_NRF91_NRF53_DEVICE; // wdt_channel_id to be written to nvs must be set to timed out channel id.
	LOG_DBG("################## %d (actual %d) ################## WDT nRF53 CB\n",
			nvs_write_before_reboot->wdt_channel_id, channel_id); // Debugging
	// Submit nRF53 write before reboot event.
	APP_EVENT_SUBMIT(nvs_write_before_reboot);

	// Create new general wdt timeout event.
	struct wdt_event *wdt_timeout = new_wdt_event();
	wdt_timeout->type = WDT_TIMEOUT;
	wdt_timeout->wdt_channel_id = WDT_CHANNEL_NRF91_NRF53_DEVICE; // Setting wdt_channel_id to timed out channel id.
	// Submit general wdt timeout event.
	APP_EVENT_SUBMIT(wdt_timeout);
}


void watchdog_setup(void)
{
	
	int ret;
	
	#ifdef WDT_NODE
		const struct device *hw_wdt_dev = DEVICE_DT_GET(WDT_NODE);
	#else
		const struct device *hw_wdt_dev = NULL;
	#endif

	LOG_DBG("wdt_setup(): Setting up task watchdog sample application.\n");

	if (!device_is_ready(hw_wdt_dev)) {
		LOG_WRN("wdt_setup(): Hardware watchdog %s is not ready; ignoring it.\n",
		       hw_wdt_dev->name);
		hw_wdt_dev = NULL;
	}

	ret = task_wdt_init(NULL); //task_wdt_init(hw_wdt_dev); When input variable set to NULL, hardware watchdog not used.
	if (ret != 0) {
		LOG_WRN("wdt_setup(): task wdt init failure: %d\n", ret);
		return;
	}

	// Setting up NVS
	// Create new nvs setup event.
	struct nvs_event *nvs_setup = new_nvs_event();
	nvs_setup->type = NVS_SETUP;
	// Submit nvs setup event.
	APP_EVENT_SUBMIT(nvs_setup);
/*
	// Wiping NVS
	// Create new nvs wipe event.
	struct nvs_event *nvs_wipe = new_nvs_event();
	nvs_wipe->type = NVS_WIPE;
	// Submit nvs wipe event.
	APP_EVENT_SUBMIT(nvs_wipe);
*/
}


// Adding wdt channels
void wdt_add_channels(int wdt_to_add) // wdt_to_add = WDT channel number to add
{
	int channel_id;
	
	// The order in which the wdt channels are added decides the channel numbers
	// (channel 0 for first added, channel 1 for second added etc).

	if (wdt_to_add == WDT_CHANNEL_NRF91_MAIN) {
		// Adding wdt channel for main event
		channel_id = task_wdt_add(RESET_TIME_MAIN, wdt_callback_main_event, (void *)k_current_get());
		LOG_DBG("wdt_add_channels(): Main watchdog added, ID: %d (actual %d)\n", WDT_CHANNEL_NRF91_MAIN, channel_id);

		// Create main feed event.
		struct wdt_event *wdt_feed_main = new_wdt_event();
		wdt_feed_main->type = WDT_FEED_MAIN;
		wdt_channel_main = channel_id;
		// Submit main feed event.
		APP_EVENT_SUBMIT(wdt_feed_main);
	}

	if (wdt_to_add == WDT_CHANNEL_NRF91_NRF53_DEVICE) {
		// Adding wdt channel for nRF53 event
		channel_id = task_wdt_add(RESET_TIME_NRF53, wdt_callback_nrf53_event, (void *)k_current_get());
		LOG_DBG("wdt_add_channels(): nRF53 watchdog added, ID: %d (actual %d)\n", WDT_CHANNEL_NRF91_NRF53_DEVICE, channel_id);

		// Create nRF53 feed event.
		struct wdt_event *wdt_feed_nrf53 = new_wdt_event();
		wdt_feed_nrf53->type = WDT_FEED_NRF53;
		wdt_channel_nrf53 = channel_id;
		// Submit nRF53 feed event.
		APP_EVENT_SUBMIT(wdt_feed_nrf53);
	}

}



// Event handler for WDT events.
static bool event_handler(const struct event_header *eh)
{

	if (is_wdt_event(eh)) {

		int err_feed;
		struct wdt_event *event = cast_wdt_event(eh);

		if (event->type==WDT_SETUP){
			watchdog_setup();
		}
		if (event->type==WDT_ADD_MAIN){
			wdt_add_channels(WDT_CHANNEL_NRF91_MAIN);
			LOG_DBG("event_handler(): Main watchdog added.\n");
		}
		if (event->type==WDT_ADD_NRF53){
			wdt_add_channels(WDT_CHANNEL_NRF91_NRF53_DEVICE);
			LOG_DBG("event_handler(): nRF53 watchdog added.\n");
		}
		if(event->type==WDT_FEED_MAIN){
			LOG_DBG("event_handler(): Main WDT being fed.\n");
			err_feed = task_wdt_feed(wdt_channel_main);
			LOG_DBG("event_handler(): channel %d (actual %d) FEEDING: %d\n", WDT_CHANNEL_NRF91_MAIN, wdt_channel_main, err_feed); // Debugging
			return false;
		}
		if(event->type==WDT_FEED_NRF53){
			LOG_DBG("event_handler(): nRF53 WDT being fed.\n");
			err_feed = task_wdt_feed(wdt_channel_nrf53);
			LOG_DBG("event_handler(): channel %d (actual %d) FEEDING: %d\n", WDT_CHANNEL_NRF91_NRF53_DEVICE, wdt_channel_nrf53, err_feed); // Debugging
			return false;
		}
		if(event->type==WDT_TIMEOUT){
			LOG_WRN("event_handler(): WDT channel %d timed out.\n", event->wdt_channel_id);
			LOG_DBG("event_handler(): Resetting device...\n");
			int count;
			int count_max = 1;
			for (int i = 0; i < count_max+1; i++) { // Need this loop to create delay before reset.
				count = i;
				// LOG_DBG("count %d\n", count);
				k_sleep(K_MSEC(1000));
			}
			if (count == count_max) {
				sys_reboot(SYS_REBOOT_COLD);
			}
			return false;
		}
		return false;
	}
	return false;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, wdt_event);