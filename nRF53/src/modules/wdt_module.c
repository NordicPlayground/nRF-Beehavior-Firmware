/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
** This module is based on the task_wdt sample in "...\zephyr\samples\subsys\task_wdt".

The code is an implementation of a watchdog timer on an nRF53 chip, using the Zephyr real-time operating system. 
The watchdog timer is used to monitor the system's behavior and reset the device in case of a malfunction. The code is structured as follows:

- The necessary header files are included, including the watchdog driver and Zephyr system libraries.
- The reset times for the different watchdog timer channels are defined.
- The hardware watchdog device is obtained from the device tree, if available. Otherwise, the task watchdog will be used without a hardware watchdog fallback.
- Three callback functions are defined, one for each watchdog timer channel. These functions create events to be handled by the event handler,
  indicating that the corresponding watchdog timer channel has timed out.
- The watchdog_setup function initializes the task watchdog and sets up the NVS (Non-Volatile Storage) module.
- The wdt_add_channels function adds the defined watchdog timer channels and creates the corresponding feed events.
- The event_handler function handles the events created by the callback functions and the feed events,
  and takes appropriate actions depending on the event type.
- Finally, the code defines an event listener and subscribes to the wdt_event and nvs_event events.

To summarize the code's functionality, it sets up the watchdog timer with three channels, each with a specific reset time.
Whenever a channel times out, an event is created and handled by the event handler, which resets the device after a delay.
The code also uses the NVS module to store the channel of the timed-out watchdog timer channel before the reset.

When it comes to usage of the code in relation to the rest of the system, it will be wise to initiate the different wdts either at start-up
or when different components are being connected. Then, it is important to set the different RESET_TIME_xxxxx to values that are high enough to avoid
unnecessary restarts and also find suitable places in the proper codes (e.g., peripheral_module) to feed the different wdt channels.

*/

#include <zephyr.h>
#include <device.h>
#include <drivers/watchdog.h>
#include <sys/reboot.h>
#include <task_wdt/task_wdt.h>
#include <sys/printk.h>
#include <stdbool.h>
#include "events/wdt_event.h"
#include <logging/log.h>

#include "wdt_module.h"

#include "nvs_module.h"
#include "events/nvs_event.h"

#define MODULE wdt_module
LOG_MODULE_REGISTER(MODULE, 4);

// Reset times for the different watchdog timer channels (1000 ms = 1 s):

#define RESET_TIME_MAIN 120000U
#define RESET_TIME_BEE_COUNTER 90000U
#define RESET_TIME_THINGY 120000U

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
int wdt_channel_bee_counter;
int wdt_channel_thingy;

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
	nvs_write_before_reboot->wdt_channel_id = WDT_CHANNEL_MAIN; // wdt_channel_id to be written to nvs must be set to timed out channel id.
	// Submit main write before reboot event.
	APP_EVENT_SUBMIT(nvs_write_before_reboot);

	// Create new general wdt timeout event.
	struct wdt_event *wdt_timeout = new_wdt_event();
	wdt_timeout->type = WDT_TIMEOUT;
	wdt_timeout->wdt_channel_id = WDT_CHANNEL_MAIN; // Setting wdt_channel_id to timed out channel id.
	// Submit general wdt timeout event.
	APP_EVENT_SUBMIT(wdt_timeout);
}

static void wdt_callback_bee_counter_event(uint8_t channel_id, void *user_data)
{
	LOG_INF("wdt_callback_bee_counter_event(): Voff, voff!");

	// Create new bee counter feed event.
	struct wdt_event *wdt_feed_bee_counter = new_wdt_event();
	wdt_feed_bee_counter->type = WDT_FEED_BEE_COUNTER;
	wdt_channel_bee_counter = channel_id; // wdt_channel_id to be fed must be set to timed out channel id.
	// Submit bee counter feed event.
	APP_EVENT_SUBMIT(wdt_feed_bee_counter);
	
	// Create new bee counter write before reboot event.
	struct nvs_event *nvs_write_before_reboot = new_nvs_event();
	nvs_write_before_reboot->type = NVS_WRITE_BEFORE_REBOOT;
	nvs_write_before_reboot->wdt_channel_id = WDT_CHANNEL_BEE_COUNTER; // wdt_channel_id to be written to nvs must be set to timed out channel id.
	// Submit bee counter write before reboot event.
	APP_EVENT_SUBMIT(nvs_write_before_reboot);

	// Create new general wdt timeout event.
	struct wdt_event *wdt_timeout = new_wdt_event();
	wdt_timeout->type = WDT_TIMEOUT;
	wdt_timeout->wdt_channel_id = WDT_CHANNEL_BEE_COUNTER; // Setting wdt_channel_id to timed out channel id.
	// Submit general wdt timeout event.
	APP_EVENT_SUBMIT(wdt_timeout);
}

static void wdt_callback_thingy_event(uint8_t channel_id, void *user_data)
{
	LOG_INF("wdt_callback_thingy_event(): Voff, voff!");

	// Create new thingy feed event.
	struct wdt_event *wdt_feed_thingy = new_wdt_event();
	wdt_feed_thingy->type = WDT_FEED_THINGY;
	wdt_channel_thingy = channel_id; // wdt_channel_id to be fed must be set to timed out channel id.
	// Submit thingy feed event.
	APP_EVENT_SUBMIT(wdt_feed_thingy);

	// Create new nvs write before reboot event.
	struct nvs_event *nvs_write_before_reboot = new_nvs_event();
	nvs_write_before_reboot->type = NVS_WRITE_BEFORE_REBOOT;
	nvs_write_before_reboot->wdt_channel_id = WDT_CHANNEL_THINGY; // wdt_channel_id to be written to nvs must be set to timed out channel id.
	// Submit nvs write before reboot event.
	APP_EVENT_SUBMIT(nvs_write_before_reboot);

	// Create new general wdt timeout event.
	struct wdt_event *wdt_timeout = new_wdt_event();
	wdt_timeout->type = WDT_TIMEOUT;
	wdt_timeout->wdt_channel_id = WDT_CHANNEL_THINGY; // Setting wdt_channel_id to timed out channel id.
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

	LOG_INF("wdt_setup(): Setting up task watchdog sample application.\n");

	if (!device_is_ready(hw_wdt_dev)) {
		LOG_WRN("wdt_setup(): Hardware watchdog %s is not ready; ignoring it.\n",
		       hw_wdt_dev->name);
		hw_wdt_dev = NULL;
	}

	ret = task_wdt_init(NULL); //task_wdt_init(hw_wdt_dev); //When input variable set to NULL, hardware watchdog not used.
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

}


// Adding wdt channels
void wdt_add_channels(int wdt_to_add)
{
	int channel_id;
	
	// The order in which the wdt channels are added decides the actual channel numbers
	// (channel 0 for first added, channel 1 for second added etc).

	if (wdt_to_add == WDT_CHANNEL_MAIN) {
		// Adding wdt channel for main event
		channel_id = task_wdt_add(RESET_TIME_MAIN, wdt_callback_main_event, (void *)k_current_get());
		LOG_INF("wdt_add_channels(): Main watchdog added, ID: %d (actual: %d)\n", WDT_CHANNEL_MAIN, channel_id);

		// Create main feed event.
		struct wdt_event *wdt_feed_main = new_wdt_event();
		wdt_feed_main->type = WDT_FEED_MAIN;
		wdt_channel_main = channel_id;
		// Submit main feed event.
		APP_EVENT_SUBMIT(wdt_feed_main);
	}

	if (wdt_to_add == WDT_CHANNEL_BEE_COUNTER) {
		// Adding wdt channel for bee counter event
		channel_id = task_wdt_add(RESET_TIME_BEE_COUNTER, wdt_callback_bee_counter_event, (void *)k_current_get());
		LOG_INF("wdt_add_channels(): Bee counter watchdog added, ID: %d (actual %d)\n", WDT_CHANNEL_BEE_COUNTER, channel_id);

		// Create new bee counter feed event.
		struct wdt_event *wdt_feed_bee_counter = new_wdt_event();
		wdt_feed_bee_counter->type = WDT_FEED_BEE_COUNTER;
		wdt_channel_bee_counter = channel_id;
		// Submit bee counter feed event.
		APP_EVENT_SUBMIT(wdt_feed_bee_counter);
	}

	if (wdt_to_add == WDT_CHANNEL_THINGY) {
		// Adding wdt channel for thingy event
		channel_id = task_wdt_add(RESET_TIME_THINGY, wdt_callback_thingy_event, (void *)k_current_get());
		LOG_INF("wdt_add_channels(): Thingy watchdog added, ID: %d (actual %d)\n", WDT_CHANNEL_THINGY, channel_id);
		
		// Create new thingy feed event.
		struct wdt_event *wdt_feed_thingy = new_wdt_event();
		wdt_feed_thingy->type = WDT_FEED_THINGY;
		wdt_channel_thingy = channel_id;
		// Submit thingy feed event.
		APP_EVENT_SUBMIT(wdt_feed_thingy);
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
			wdt_add_channels(WDT_CHANNEL_MAIN);
			LOG_INF("event_handler(): Main watchdog added.\n");
		}
		if (event->type==WDT_ADD_BEE_COUNTER){
			wdt_add_channels(WDT_CHANNEL_BEE_COUNTER);
			LOG_INF("event_handler(): Bee counter watchdog added.\n");
		}
		if (event->type==WDT_ADD_THINGY){
			wdt_add_channels(WDT_CHANNEL_THINGY);
			LOG_INF("event_handler(): Thingy watchdog added.\n");
		}
		if(event->type==WDT_FEED_MAIN){
			LOG_INF("event_handler(): Main WDT being fed.\n");
			err_feed = task_wdt_feed(wdt_channel_main);
			return false;
		}
		if(event->type==WDT_FEED_BEE_COUNTER){
			LOG_INF("event_handler(): Bee counter WDT being fed.\n");
			err_feed = task_wdt_feed(wdt_channel_bee_counter);
			return false;
		}
		if(event->type==WDT_FEED_THINGY){
			LOG_INF("event_handler(): Thingy WDT being fed.\n");
			err_feed = task_wdt_feed(wdt_channel_thingy);
			return false;
		}
		if(event->type==WDT_TIMEOUT){
			LOG_WRN("event_handler(): WDT channel %d timed out.\n", event->wdt_channel_id);
			LOG_INF("event_handler(): Resetting device...\n");
			int count;
			int count_max = 1;
			for (int i = 0; i < count_max+1; i++) { // Need this loop to create delay before reset
				count = i;
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
APP_EVENT_SUBSCRIBE(MODULE, nvs_event);