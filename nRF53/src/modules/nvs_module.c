/*
** This module is based on the nvs sample in "...\zephyr\samples\subsys\nvs".
*/


/*
 * NVS Sample for Zephyr using high level API, the sample illustrates the usage
 * of NVS for storing data of different kind (strings, binary blobs, unsigned
 * 32 bit integer) and also how to read them back from flash. The reading of
 * data is illustrated for both a basic read (latest added value) as well as
 * reading back the history of data (previously added values). Next to reading
 * and writing data it also shows how data can be deleted from flash.
 *
 * The sample stores the following items:
 * 1. A string representing an IP-address: stored at id=1, data="192.168.1.1"
 * 2. A binary blob representing a key: stored at id=2, data=FF FE FD FC FB FA
 *    F9 F8
 * 3. A reboot counter (32bit): stored at id=3, data=reboot_counter
 * 4. A string: stored at id=4, data="DATA" (used to illustrate deletion of
 * items)
 *
 * At first boot the sample checks if the data is available in flash and adds
 * the items if they are not in flash.
 *
 * Every reboot increases the values of the reboot_counter and updates it in
 * flash.
 *
 * At the 10th reboot the string item with id=4 is deleted (or marked for
 * deletion).
 *
 * At the 11th reboot the string item with id=4 can no longer be read with the
 * basic nvs_read() function as it has been deleted. It is possible to read the
 * value with nvs_read_hist()
 *
 * At the 78th reboot the first sector is full and a new sector is taken into
 * use. The data with id=1, id=2 and id=3 is copied to the new sector. As a
 * result of this the history of the reboot_counter will be removed but the
 * latest values of address, key and reboot_counter is kept.
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <sys/reboot.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <zephyr/fs/nvs.h>

#include <sys/printk.h>
#include <stdbool.h>
#include <logging/log.h>

#include "nvs_module.h"
#include "events/nvs_event.h"
#include "events/wdt_event.h"
#include "events/ble_event.h"
#include "sys/types.h"

#include "wdt_module.h"

#include <bluetooth/services/nus.h>

#define MODULE nvs_module
LOG_MODULE_REGISTER(MODULE, 4);

static struct nvs_fs fs;

#define STORAGE_NODE_LABEL storage

/* 1000 msec = 1 sec */
#define SLEEP_TIME      100
/* maximum reboot counts, make high enough to trigger sector change (buffer */
/* rotation). */
#define MAX_REBOOT 400

#define WDT_CHANNEL_ID 1

#define NUM_OF_WDT_CHANNELS 3


void nvs_setup_function(void) {
    int rc = 0;
	struct flash_pages_info info;

	/* define the nvs file system by settings with:
	**	sector_size equal to the pagesize,
	**	2 sectors
	**	starting at FLASH_AREA_OFFSET(storage)
	*/

	fs.flash_device = FLASH_AREA_DEVICE(STORAGE_NODE_LABEL);
	if (!device_is_ready(fs.flash_device)) {
		LOG_WRN("nvs_setup_function(): Flash device %s is not ready\n", fs.flash_device->name);
		return;
	}
	fs.offset = FLASH_AREA_OFFSET(storage);
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		LOG_WRN("nvs_setup_function(): Unable to get page info\n");
		return;
	}
	fs.sector_size = info.size;
	fs.sector_count = 2U; // 2U is the lowest allowed sector count.

	rc = nvs_mount(&fs);
	if (rc) {
		LOG_WRN("nvs_setup_function(): Flash Init failed\n");
		return;
	}

	/* The channel calling its wdt callback function is saved to
	** WDT_CHANNEL_ID in the NVS, lets see
	** if we can read it from flash.
	*/

	struct nvs_event *event = new_nvs_event();

	rc = nvs_read(&fs, WDT_CHANNEL_ID, &event->wdt_channel_id, sizeof(event->wdt_channel_id));
	
	LOG_DBG("**************** %d ****************** NVS SETUP", event->wdt_channel_id);
	
	if (rc > 0) { /* item was found, show it */
		LOG_INF("nvs_setup_function(): Id: %d, wdt_channel_id: %d\n",
			WDT_CHANNEL_ID, event->wdt_channel_id);
	}
	else { /* item was not found, add it */
		LOG_INF("nvs_setup_function(): No wdt_channel_id found, adding it at id %d\n",
		       WDT_CHANNEL_ID);
		(void)nvs_write(&fs, WDT_CHANNEL_ID, &event->wdt_channel_id,
			  sizeof(event->wdt_channel_id));
	}
}

/*
void nvs_write_before_reboot(void) {

}
*/

void nvs_wipe_function(void) {
	int rc = 0;
	int err = 0;
	
	struct nvs_event *event = new_nvs_event();

	//LOG_DBG("*************** %d ***************** before reading\n", event->wdt_channel_id); // Debugging

	rc = nvs_read(&fs, WDT_CHANNEL_ID, &event->wdt_channel_id, sizeof(event->wdt_channel_id));
	
	LOG_DBG("*************** %d ***************** NVS WIPE\n", event->wdt_channel_id); // Debugging

	LOG_DBG("nvs_wipe_function(): Id: %d, wdt_channel_id: %d\n",
			WDT_CHANNEL_ID, event->wdt_channel_id); // FOR TESTING
	err = nvs_delete(&fs, WDT_CHANNEL_ID);

	if (err == 0) {
		LOG_INF("nvs_wipe_function(): nvs_delete returned %d, delete SUCCESSFUL.\n\r", err);
	}
	else {
		LOG_ERR("nvs_wipe_function(): nvs_delete returned %d, delete FAILED.\n\r", err);
	}
	LOG_DBG("nvs_wipe_function(): rc = %d\n", rc);
	rc = nvs_read(&fs, WDT_CHANNEL_ID, &event->wdt_channel_id, sizeof(event->wdt_channel_id));
	LOG_DBG("nvs_wipe_function(): rc = %d\n", rc);
	LOG_DBG("nvs_wipe_function(): Id: %d, wdt_channel_id: %d\n",
			WDT_CHANNEL_ID, event->wdt_channel_id); // FOR TESTING
}

void nvs_send_to_nrf91(void) {
	int rc;

	struct nvs_event *event = new_nvs_event();
	
	rc = nvs_read(&fs, WDT_CHANNEL_ID, &event->wdt_channel_id, sizeof(event->wdt_channel_id));
	
	if (rc > 0) { // Item was found, send it
		LOG_INF("nvs_send_to_nrf91(): NVS read, id: %d, wdt_channel_id: %d;\t%d\n",
				WDT_CHANNEL_ID, event->wdt_channel_id, rc);
		if (event->wdt_channel_id >= 0 && event->wdt_channel_id < NUM_OF_WDT_CHANNELS) {
			struct ble_event *ble_nvs_send_to_nrf91 = new_ble_event();
			ble_nvs_send_to_nrf91->type = BLE_NVS_SEND_TO_NRF91;
			ble_nvs_send_to_nrf91->wdt_channel_id = event->wdt_channel_id;
			// Submit nvs send to nRF91 event.
			APP_EVENT_SUBMIT(ble_nvs_send_to_nrf91);
			LOG_INF("nvs_send_to_nrf91(): wdt channel number being sent from NVS to nRF91.\n");
			
			return false;
		}
		else {
			LOG_INF("nvs_send_to_nrf91(): invalid wdt channel number. Nothing sent to nRF91.\n");
			
			return false;
		}
		return false;		
	}
	else { /* item was not found */
		LOG_INF("nvs_send_to_nrf91(): nothing to read from NVS;\t%d\n", rc);
		
		return false;
	}
	return false;
}

// Event handler for NVS events.
static bool event_handler(const struct event_header *eh) {

	if (is_nvs_event(eh)) {

		int rc = 0;

		struct nvs_event *event = cast_nvs_event(eh);

		if(event->type==NVS_SETUP){
			LOG_INF("event_handler(): NVS setup initialized.\n");
			nvs_setup_function();

			return false;
		}
		if(event->type==NVS_WRITE_BEFORE_REBOOT){
			
			ssize_t valueReturned = nvs_write(&fs, WDT_CHANNEL_ID, &event->wdt_channel_id,
			sizeof(event->wdt_channel_id));
			if (valueReturned == 0)	{
				LOG_INF("event_handler(): Nothing new written to NVS. Channel %d timeout. WDT_CHANNEL_ID: %d.\n",
				event->wdt_channel_id, WDT_CHANNEL_ID);
				return false;
			}
			else if (valueReturned < 0) {
				LOG_ERR("event_handler(): nvs_write returned %d. Failed to write to NVS!\n", valueReturned);
				return false;
			}
			else {
				LOG_INF("event_handler(): Channel %d timeout. WDT_CHANNEL_ID: %d. nvs_write: %d.\n",
						event->wdt_channel_id, WDT_CHANNEL_ID, valueReturned);
				return false;
			}
			return false;
		}
		if(event->type==NVS_SEND_TO_NRF91){
			LOG_INF("event_handler(): NVS send to nRF91 initialized.\n");
			nvs_send_to_nrf91();
			
			return false;
		}
		if(event->type==NVS_WIPE){
			LOG_INF("event_handler(): NVS wipe initialized.\n");
			nvs_wipe_function();
			
			return false;
		}	
		return false;
	}
	return false;
}


//K_THREAD_DEFINE(nvs_thread, 1024, nvs_setup_function, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, nvs_event);
//APP_EVENT_SUBSCRIBE(MODULE, wdt_event);