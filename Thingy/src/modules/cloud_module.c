/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <init.h>
#include <event_manager.h>
#include <dk_buttons_and_leds.h>

#include <modem/lte_lc.h>
#include <net/cloud.h>
#include <net/socket.h>

#include <modem/nrf_modem_lib.h>

#include <logging/log.h>

#include "modules_common.h"
#include "events/cloud_event.h"
#include "events/ble_event.h"


LOG_MODULE_REGISTER(cloud_module, CONFIG_LOG_DEFAULT_LEVEL);

static struct cloud_backend *cloud_backend;
static struct k_work_delayable cloud_update_work;
static struct k_work_delayable connect_work;

static K_SEM_DEFINE(lte_connected, 0, 1);

static K_SEM_DEFINE(ble_scanning, 0, 1);

static K_SEM_DEFINE(ble_init, 0, 1);

/* Flag to signify if the cloud client is connected or not connected to cloud,
 * used to abort/allow cloud publications.
 */
static bool cloud_connected;

static bool first = true;

static void connect_work_fn(struct k_work *work)
{
	int err;

	if (cloud_connected) {
		return;
	}

	err = cloud_connect(cloud_backend);
	if (err) {
		LOG_ERR("cloud_connect, error: %d", err);
	}

	LOG_INF("Next connection retry in %d seconds",30);

	k_work_schedule(&connect_work,
		K_SECONDS(30));
}

static void cloud_update_work_fn(struct k_work *work)
{
	int err;

	// if(!k_sem_count_get()){
	// 	LOG_INF()
	// 	return;
	// }
	LOG_INF("Does this ever get called?");

	if (!cloud_connected) {
		LOG_INF("Not connected to cloud, abort cloud publication");
		return;
	}

	/*LOG_INF("Publishing message: %s", log_strdup("Test"));

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.buf = "{\"message\":\"Test!\"}",
		.len = strlen("{\"message\":\"Test!\"}")
	};

	// When using the nRF Cloud backend data is sent to the message topic.
	// This is in order to visualize the data in the web UI terminal.
	// For Azure IoT Hub and AWS IoT, messages are addressed directly to the
	// device twin (Azure) or device shadow (AWS).
	
	msg.endpoint.type = CLOUD_EP_MSG; //For nRF Cloud
	
    //msg.endpoint.type = CLOUD_EP_STATE; //For the inferior Clouds

	err = cloud_send(cloud_backend, &msg);
	LOG_INF("Message sent with code %i", err);
	if (err) {
		LOG_ERR("cloud_send failed, error: %d", err);
	}

// #if defined(CONFIG_CLOUD_PUBLICATION_SEQUENTIAL)
	k_work_schedule(&cloud_update_work,
			K_SECONDS(10));
// #endif*/
}

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	ARG_UNUSED(user_data);
	ARG_UNUSED(backend);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTING:
		LOG_INF("CLOUD_EVT_CONNECTING");
		break;
	case CLOUD_EVT_CONNECTED:
		LOG_INF("CLOUD_EVT_CONNECTED");
		cloud_connected = true;
		/* This may fail if the work item is already being processed,
		 * but in such case, the next time the work handler is executed,
		 * it will exit after checking the above flag and the work will
		 * not be scheduled again.
		 */
		(void)k_work_cancel_delayable(&connect_work);
		break;
	case CLOUD_EVT_READY:
		LOG_INF("CLOUD_EVT_READY");
		struct cloud_event_abbr *cloud_event_ready = new_cloud_event_abbr(strlen("Cloud ready"));

        if(first){
			cloud_disconnect(cloud_backend);
			first = false;
		}

		cloud_event_ready->type = CLOUD_CONNECTED;

        // ble_event->address = address_temp;
        // memcpy(ble_event->address, address_temp, 17);

        memcpy(cloud_event_ready->dyndata.data, log_strdup("Cloud ready"), strlen("Cloud ready"));

        EVENT_SUBMIT(cloud_event_ready);
// #if defined(CONFIG_CLOUD_PUBLICATION_SEQUENTIAL)
		k_work_reschedule(&cloud_update_work, K_NO_WAIT);
// #endif
		break;
	case CLOUD_EVT_DISCONNECTED:
		LOG_INF("CLOUD_EVT_DISCONNECTED");
		cloud_connected = false;
		k_work_reschedule(&connect_work, K_SECONDS(30));
		break;
	case CLOUD_EVT_ERROR:
		LOG_INF("CLOUD_EVT_ERROR");
		break;
	case CLOUD_EVT_DATA_SENT:
		LOG_INF("CLOUD_EVT_DATA_SENT");
		break;
	case CLOUD_EVT_DATA_RECEIVED:
		LOG_INF("CLOUD_EVT_DATA_RECEIVED");
		LOG_INF("Data received from cloud: %.*s",
			evt->data.msg.len,
			log_strdup(evt->data.msg.buf));

        struct cloud_event_abbr *cloud_event_abbr = new_cloud_event_abbr(evt->data.msg.len);

        cloud_event_abbr->type = CLOUD_RECEIVED;

        // ble_event->address = address_temp;
        //memcpy(ble_event->address, address_temp, 17);

        memcpy(cloud_event_abbr->dyndata.data, log_strdup(evt->data.msg.buf), evt->data.msg.len);

        EVENT_SUBMIT(cloud_event_abbr);
		break;
	case CLOUD_EVT_PAIR_REQUEST:
		LOG_INF("CLOUD_EVT_PAIR_REQUEST");
		break;
	case CLOUD_EVT_PAIR_DONE:
		LOG_INF("CLOUD_EVT_PAIR_DONE");
		break;
	case CLOUD_EVT_FOTA_DONE:
		LOG_INF("CLOUD_EVT_FOTA_DONE");
		break;
	case CLOUD_EVT_FOTA_ERROR:
		LOG_INF("CLOUD_EVT_FOTA_ERROR");
		break;
	default:
		LOG_INF("Unknown cloud event type: %d", evt->type);
		break;
	}
}

static void work_init(void)
{
	k_work_init_delayable(&cloud_update_work, cloud_update_work_fn);
	k_work_init_delayable(&connect_work, connect_work_fn);
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_INF("Network registration status: %s", evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_DBG("%s", log_strdup(log_buf));
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_DBG("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		LOG_INF("Active LTE mode changed: %s",
			 evt->lte_mode == LTE_LC_LTE_MODE_NONE ? "None" :
			 evt->lte_mode == LTE_LC_LTE_MODE_LTEM ? "LTE-M" :
			 evt->lte_mode == LTE_LC_LTE_MODE_NBIOT ? "NB-IoT" :
			 "Unknown");
		break;
	default:
		break;
	}
}

static void modem_configure(void)
{
#if defined(CONFIG_NRF_MODEM_LIB)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		int err;

#if defined(CONFIG_POWER_SAVING_MODE_ENABLE)
		/* Requesting PSM before connecting allows the modem to inform
		 * the network about our wish for certain PSM configuration
		 * already in the connection procedure instead of in a separate
		 * request after the connection is in place, which may be
		 * rejected in some networks.
		 */
		err = lte_lc_psm_req(true);
		if (err) {
			LOG_ERR("Failed to set PSM parameters, error: %d",
				err);
		}

		LOG_INF("PSM mode requested");
#endif
		err = lte_lc_init_and_connect_async(lte_handler);
		if (err) {
			LOG_ERR("Modem could not be configured, error: %d",
				err);
			return;
		}

		/* Check LTE events of type LTE_LC_EVT_NW_REG_STATUS in
		 * lte_handler() to determine when the LTE link is up.
		 */
	}
#endif
}

//#if defined(CONFIG_CLOUD_PUBLICATION_BUTTON_PRESS)
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		// k_work_reschedule(&cloud_update_work, K_NO_WAIT);
		int err;
		
		nrf_cloud_process();

		LOG_INF("Publishing message: %s", log_strdup("Test"));

		struct cloud_msg msg = {
			.qos = CLOUD_QOS_AT_MOST_ONCE,
			.buf = "{\"message\":\"Test!\"}",
			.len = strlen("{\"message\":\"Test!\"}")
		};

		// When using the nRF Cloud backend data is sent to the message topic.
		// This is in order to visualize the data in the web UI terminal.
		// For Azure IoT Hub and AWS IoT, messages are addressed directly to the
		// device twin (Azure) or device shadow (AWS).
		
		msg.endpoint.type = CLOUD_EP_MSG; //For nRF Cloud
		
		//msg.endpoint.type = CLOUD_EP_STATE; //For the inferior Clouds

		err = cloud_send(cloud_backend, &msg);
		LOG_INF("Message sent with code %i", err);
		if (err) {
			LOG_ERR("cloud_send failed, error: %d", err);
		}
	}
	// if (has_changed & button_states & DK_BTN2_MSK) {
	// 	struct cloud_event_abbr *cloud_event_sleep = new_cloud_event_abbr(strlen("Cloud entering sleep mode"));

    //     cloud_event_sleep->type = CLOUD_SLEEP;

    //     // ble_event->address = address_temp;
    //     //memcpy(ble_event->address, address_temp, 17);

    //     memcpy(cloud_event_sleep->dyndata.data, log_strdup("Cloud entering sleep mode"), strlen("Cloud entering sleep mode"));

    //     EVENT_SUBMIT(cloud_event_sleep);
	// }
}
//#endif

void cloud_setup_fn(void)
{
	int err;

	// k_sem_take(&ble_init, K_FOREVER);

	LOG_INF("Cloud client has started");

	cloud_backend = cloud_get_binding("NRF_CLOUD");
	__ASSERT(cloud_backend != NULL, "%s backend not found",
		 "NRF_CLOUD");

	err = cloud_init(cloud_backend, cloud_event_handler);
	if (err) {
		LOG_ERR("Cloud backend could not be initialized, error: %d",
			err);
	}

	work_init();
	modem_configure();

//#if defined(CONFIG_CLOUD_PUBLICATION_BUTTON_PRESS)
	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}
//#endif
	LOG_INF("Connecting to LTE network, this may take several minutes...");

	k_sem_take(&lte_connected, K_FOREVER);

	LOG_INF("Connected to LTE network");
	LOG_INF("Connecting to cloud");

	k_work_schedule(&connect_work, K_NO_WAIT);
}
static bool event_handler(const struct event_header *eh)
{
    if(is_ble_event(eh)){
        int err;
        struct ble_event *event = cast_ble_event(eh);
        if(event->type==BLE_RECEIVED){
			if(strcmp(event->address, "Placeholder")){
				nrf_cloud_process();
				
				// int strip = (event->dyndata.size==20) ? 0 : 2; 
				LOG_INF("Publishing message: %.*s", event->dyndata.size, event->dyndata.data);// %.*s", event->dyndata.size, event->dyndata.size
				char message[100]; 
				// err = snprintf(message, 100, "{\"payload\":{\"message\":\"%.*s\",\"address\":\"%.17s\"}}",event->dyndata.size-strip, event->dyndata.data, log_strdup(event->address));
				err = snprintf(message, 100, "{\"message\":\"%.*s\"}",event->dyndata.size, event->dyndata.data);
				LOG_INF("Message formatted: %s", message);
				LOG_INF("Returned: %i", err);
				
				struct cloud_msg msg = {
					.qos = CLOUD_QOS_AT_MOST_ONCE,
					.buf = message,
					.len = strlen(message)
				};
				
				/* When using the nRF Cloud backend data is sent to the message topic.
				* This is in order to visualize the data in the web UI terminal.
				* For Azure IoT Hub and AWS IoT, messages are addressed directly to the
				* device twin (Azure) or device shadow (AWS).
				*/
				msg.endpoint.type = CLOUD_EP_MSG; //For nRF Cloud
				
				//msg.endpoint.type = CLOUD_EP_STATE; //For the inferior Clouds

				err = cloud_send(cloud_backend, &msg);
				LOG_INF("Published message with code: %i", err);

				if (err) {
					LOG_ERR("cloud_send failed, error: %d", err);
				}
			}
			return false;
		}
		if(event->type==BLE_STATUS){
			LOG_INF("BLE_STATUS");
			//nrf_cloud_process();
			LOG_INF("BLE_STATUS still");
			
			// int strip = (event->dyndata.size==20) ? 0 : 2; 
			// LOG_INF("{\"message\":\"Connected: %i, Missing: %i\"}", event->dyndata.data[0],event->dyndata.data[1]);// %.*s", event->dyndata.size, event->dyndata.size
			char message[100]; 
			// err = snprintf(message, 100, "{\"payload\":{\"message\":\"%.*s\",\"address\":\"%.17s\"}}",event->dyndata.size-strip, event->dyndata.data, log_strdup(event->address));
			err = snprintf(message, 100, "{\"message\":\"Connected: %c, Missing: %c\"}", (char)event->dyndata.data[0], (char)event->dyndata.data[1]);

			LOG_INF("%c, %c, %c", (char)event->dyndata.data[0], (char)event->dyndata.data[1], (char)event->dyndata.data[2]);

			char test = (char)event->dyndata.data[0];
			LOG_INF("%i",(uint8_t)test);
			// err = snprintf(message, 100, "{\"message\":\"Placeholder\"}");
			LOG_INF("Message formatted: %s", message);
			// LOG_INF("Returned: %i", err);
			
			struct cloud_msg msg = {
				.qos = CLOUD_QOS_AT_MOST_ONCE,
				.buf = message,
				.len = err
			};
			
			/* When using the nRF Cloud backend data is sent to the message topic.
			* This is in order to visualize the data in the web UI terminal.
			* For Azure IoT Hub and AWS IoT, messages are addressed directly to the
			* device twin (Azure) or device shadow (AWS).
			*/
			msg.endpoint.type = CLOUD_EP_MSG; //For nRF Cloud
			
			//msg.endpoint.type = CLOUD_EP_STATE; //For the inferior Clouds

			err = cloud_send(cloud_backend, &msg);
			LOG_INF("Published message with code: %i", err);

			if (err) {
				LOG_ERR("cloud_send failed, error: %d", err);
			}
			return false;
		}
		if(event->type==BLE_SCANNING){
			LOG_INF("Stopping Cloud sync");
			cloud_disconnect(cloud_backend);
			// k_sem_take(&ble_scanning, K_FOREVER)
			return false;;
		}
		if(event->type==BLE_DONE_SCANNING){
			LOG_INF("Starting Cloud sync");
			k_work_reschedule(&connect_work, K_NO_WAIT);
			// k_sem_take(&ble_scanning, K_FOREVER);
			return false;
		}
		if(event->type==BLE_INIT){
			k_sem_give(&ble_init);
		}
	}	
	return false;
}

// SYS_INIT(cloud_setup_fn, APPLICATION, 50);
K_THREAD_DEFINE(cloud_module_thread, 1024,
		cloud_setup_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);


EVENT_LISTENER(cloud_module, event_handler);
EVENT_SUBSCRIBE_EARLY(cloud_module, ble_event);