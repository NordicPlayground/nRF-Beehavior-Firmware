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
#include <net/nrf_cloud.h>
#include <net/socket.h>

#include <modem/nrf_modem_lib.h>

#include <logging/log.h>

#include "modules_common.h"
#include "events/cloud_event.h"
#include "events/ble_event.h"

#include <date_time.h>

#include <bluetooth/bluetooth.h>

// #include <device.h>
// #include <pm/pm.h>

// const struct device *cons;

// #define CONSOLE_LABEL DT_LABEL(DT_CHOSEN(zephyr_console))

LOG_MODULE_REGISTER(cloud_module, CONFIG_LOG_DEFAULT_LEVEL);

#if defined(CONFIG_BOARD_THINGY91)
#include <modem/at_cmd.h>
#define AT_CMD_VBAT		"AT%XVBAT"
static struct k_work_delayable cloud_update_work;
#endif

static struct cloud_backend *cloud_backend;
static struct k_work_delayable connect_work;

static K_SEM_DEFINE(lte_connected, 0, 1);
static K_SEM_DEFINE(cloud_connecting, 0, 1);

/* Flag to signify if the cloud client is connected or not connected to cloud,
 * used to abort/allow cloud publications.
 */
static bool cloud_connected;

static bool lte_sleep;

static bool first = true;

static void connect_work_fn(struct k_work *work)
{
	int err;
	LOG_DBG("Connect_work_fn");
	if (cloud_connected) {
		return;
	}
	enum lte_lc_func_mode mode;
	LOG_INF("Check 2");
	err = lte_lc_func_mode_get(&mode);
	LOG_INF("Check 3, %d, %d", err, mode);
	if(mode==LTE_LC_FUNC_MODE_OFFLINE){
		cloud_connected = false;
		return;
	}
	LOG_DBG("LTE in normal mode");

	err = cloud_connect(cloud_backend);
	if (err) {
		LOG_ERR("cloud_connect, error: %d", err);
	}

	LOG_INF("Next connection retry in %d seconds",30);

	k_work_schedule(&connect_work,
		K_SECONDS(30));
}

#if defined(CONFIG_BOARD_THINGY91)
static void cloud_update_work_fn(struct k_work *work)
{
	if(lte_sleep){
		//Save percentage to send at next opportunity
		return;
	}
	if (!cloud_connected) {
		LOG_INF("Not connected to cloud, abort cloud publication");
		k_work_reschedule(&cloud_update_work, K_SECONDS(30));
		return;
	}
	int err;

	char buf[20];
	enum at_cmd_state *response;
	err = at_cmd_write(AT_CMD_VBAT, &buf, 20, response);

	LOG_DBG("%.20s, with code %i", buf, err);
	char voltage[4];
	for(uint8_t i=8; i<12; i++){
		voltage[i-8] = buf[i];
	}
	LOG_DBG("Voltage: %.4s [mV]", voltage);

	int64_t unix_time_ms = k_uptime_get();
	err = date_time_now(&unix_time_ms);
	int64_t divide = 1000;
	int64_t ts = unix_time_ms / divide;

	LOG_DBG("Time: %d", ts);

	char message[50];
	int len = snprintk(message, 50, "{\"BAT\":\"%.4s\"\"TIME\":\"%lld\"}", voltage, ts);

	LOG_INF("%.50s", message);

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.buf = message,
		.len = len
	};

	// When using the nRF Cloud backend data is sent to the message topic.
	// This is in order to visualize the data in the web UI terminal.
	// For Azure IoT Hub and AWS IoT, messages are addressed directly to the
	// device twin (Azure) or device shadow (AWS).
	
	msg.endpoint.type = CLOUD_EP_MSG; //For nRF Cloud
	
	// //msg.endpoint.type = CLOUD_EP_STATE; //For the inferior Clouds

	err = cloud_send(cloud_backend, &msg);
	LOG_DBG("Message sent with code %i", err);
	if (err) {
		LOG_ERR("cloud_send failed, error: %d", err);
	}
	k_work_reschedule(&cloud_update_work, K_MINUTES(15));
}
#endif

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	int err;
	int64_t unix_time_ms;
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
		k_sem_give(&cloud_connecting);
		(void)k_work_cancel_delayable(&connect_work);
		break;
	case CLOUD_EVT_READY:
		err = dk_set_led_off(DK_LED1);
		err = dk_set_led_on(DK_LED3);
		LOG_INF("CLOUD_EVT_READY, led: %i", err);
		struct cloud_event_abbr *cloud_event_ready = new_cloud_event_abbr(strlen("Cloud ready"));

		/* If it's the first time connected to cloud, disconnect to scan for peripheral units */
        if(first){
			dk_set_led_on(DK_LED1);
			cloud_disconnect(cloud_backend);
			first = false;
		}

		cloud_event_ready->type = CLOUD_CONNECTED;

        memcpy(cloud_event_ready->dyndata.data, log_strdup("Cloud ready"), strlen("Cloud ready"));

        EVENT_SUBMIT(cloud_event_ready);

		break;
	case CLOUD_EVT_DISCONNECTED:
		LOG_INF("CLOUD_EVT_DISCONNECTED");
		cloud_connected = false;
		k_sem_take(&cloud_connecting, K_NO_WAIT);
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
		lte_sleep = false;
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

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	/* Press Button 1 to turn of the lights to save power */
	if (has_changed & button_states & DK_BTN1_MSK) {
		int err;
		dk_set_leds_state(DK_ALL_LEDS_MSK, 0);
		LOG_INF("Check");
		enum lte_lc_func_mode mode;
		LOG_INF("Check 2");
		err = lte_lc_func_mode_get(&mode);
		LOG_INF("Check 3, %d, %d", err, mode);
		if(mode==LTE_LC_FUNC_MODE_OFFLINE){
			err = lte_lc_normal();
			LOG_INF("LTE normal");
			if(err){
				dk_set_led_on(DK_LED2_MSK);
			}
		}
		else{
			err = lte_lc_offline();
			LOG_INF("LTE offline");
			if(err){
				dk_set_led_on(DK_LED1_MSK);
			}
		}
	}
	#if defined(CONFIG_BOARD_THINGY91)
	#else
	/* Press Button 2 to search for peripherals */
	if (has_changed & button_states & DK_BTN2_MSK) {
		cloud_disconnect(cloud_backend);
		struct cloud_event_abbr *cloud_event_sleep = new_cloud_event_abbr(strlen("Cloud entering sleep mode"));

        cloud_event_sleep->type = CLOUD_SLEEP;

        memcpy(cloud_event_sleep->dyndata.data, log_strdup("Cloud entering sleep mode"), strlen("Cloud entering sleep mode"));

        EVENT_SUBMIT(cloud_event_sleep);
	}
	#endif
}

void cloud_setup_fn(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	
    // cons=device_get_binding(CONSOLE_LABEL);
	
	/* Prevent deep sleep (system off) from being entered */
    // pm_constraint_set(PM_STATE_SOFT_OFF);

	err = dk_leds_init();
	dk_set_leds_state(DK_ALL_LEDS_MSK, 0);

	LOG_INF("Cloud client has started");

	cloud_backend = cloud_get_binding("NRF_CLOUD");
	__ASSERT(cloud_backend != NULL, "NRF_CLOUD backend not found");

	err = cloud_init(cloud_backend, cloud_event_handler);
	if (err) {
		LOG_ERR("Cloud backend could not be initialized, error: %d",
			err);
	}

	k_work_init_delayable(&connect_work, connect_work_fn);
	#if defined(CONFIG_BOARD_THINGY91)
	k_work_init_delayable(&cloud_update_work, cloud_update_work_fn);
	#endif
	modem_configure();

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}

	LOG_INF("Connecting to LTE network, this may take several minutes...");

	k_sem_take(&lte_connected, K_FOREVER);

	date_time_update_async(NULL);

	LOG_INF("Connecting to cloud");

	k_work_schedule(&connect_work, K_NO_WAIT);
	#if defined(CONFIG_BOARD_THINGY91)
	k_work_reschedule(&cloud_update_work, K_NO_WAIT);
	#endif
	while(true){
			k_sleep(K_SECONDS(30));
			enum lte_lc_func_mode mode;
			err = lte_lc_func_mode_get(&mode);
			LOG_INF("Check 3, %d, %d", err, mode);
			if(mode!=LTE_LC_FUNC_MODE_OFFLINE){
				// cloud_connected = false;
		
				LOG_DBG("Bee Counter data is being JSON-formatted");

				/* Logic to turn two bytes into one uint16_t*/
				// char out_arr[2] = {0x15, 0x25};
				// char in_arr[2]= {0x35, 0x45};
				// for (uint8_t i = 0; i < 2; i++){
				// 	out_arr[i] = event->dyndata.data[1-i];
				// 	in_arr[i] = event->dyndata.data[3-i];	
				// }
				uint16_t totalOut = 0x1623;
				uint16_t totalIn = 0x1234;

				// memcpy(&totalOut, out_arr, sizeof(totalOut));
				// memcpy(&totalIn, in_arr, sizeof(totalIn));

				char message[100]; 

				/* Get timestamp */
				int64_t unix_time_ms = k_uptime_get();
				err = date_time_now(&unix_time_ms);
				int64_t divide = 1000;
				int64_t ts = unix_time_ms / divide;

				LOG_DBG("Time: %d", ts);

				/* Format to JSON-string */
				uint16_t len = snprintk(message, 100, "{\"appID\":\"BEE-CNT\",\"OUT\":\"%i\",\"IN\":\"%i\",\"TIME\":\"%lld\",\"NAME\":Test}" \
					, totalOut, totalIn, ts);
				LOG_INF("Message formatted: %s, length: %i", message, len);
			
				struct cloud_msg msg = {
					.qos = CLOUD_QOS_AT_MOST_ONCE,
					.buf = message,
					.len = len
				};
				
				msg.endpoint.type = CLOUD_EP_MSG; 
				
				/* Send data to cloud */
				err = cloud_send(cloud_backend, &msg);
				
				if (err) {
					LOG_ERR("cloud_send failed, error: %d", err);
				}
				else{
					LOG_INF("Message published succesfully.");
				}		
			}
	}
}
static bool event_handler(const struct event_header *eh)
{
    if(is_ble_event(eh)){
        int err;
        struct ble_event *event = cast_ble_event(eh);
        if(event->type==BLE_RECEIVED){
			enum lte_lc_func_mode mode;
			LOG_INF("Check 2");
			err = lte_lc_func_mode_get(&mode);
			LOG_INF("Check 3, %d, %d", err, mode);
			if(mode==LTE_LC_FUNC_MODE_OFFLINE){
				err = lte_lc_normal();
				LOG_INF("LTE normal");
				if(err){
					dk_set_led_on(DK_LED2_MSK);
				}
				LOG_INF("Waiting for lte");
				k_sem_take(&lte_connected, K_FOREVER);
				LOG_INF("LTE ready");
				err = cloud_connect(cloud_backend);
				if (err) {
					LOG_ERR("cloud_connect, error: %d", err);
				}
				k_sem_take(&cloud_connecting, K_SECONDS(20));
				LOG_INF("Cloud connected: %i", cloud_connected);
				if(!cloud_connected){
					LOG_INF("Cloud not connected");
					k_work_reschedule(&connect_work, K_NO_WAIT);
					if (err) {
						LOG_ERR("cloud_connect, error: %d", err);
					}
					k_sem_take(&cloud_connecting, K_SECONDS(10));
				}
			}
			else{
				LOG_INF("LTE allready on");
			}

			// nrf_cloud_process();
			LOG_DBG("Size: %i", event->dyndata.size);
			if(event->dyndata.size == 4){
				LOG_DBG("Bee Counter data is being JSON-formatted");

				/* Logic to turn two bytes into one uint16_t*/
				char out_arr[2];
				char in_arr[2];
				for (uint8_t i = 0; i < 2; i++){
					out_arr[i] = event->dyndata.data[1-i];
					in_arr[i] = event->dyndata.data[3-i];	
				}
				uint16_t totalOut;
				uint16_t totalIn;

				memcpy(&totalOut, out_arr, sizeof(totalOut));
				memcpy(&totalIn, in_arr, sizeof(totalIn));

				char message[100]; 

				/* Get timestamp */
				int64_t unix_time_ms = k_uptime_get();
				err = date_time_now(&unix_time_ms);
				int64_t divide = 1000;
				int64_t ts = unix_time_ms / divide;

				LOG_DBG("Time: %d", ts);

				/* Format to JSON-string */
				uint16_t len = snprintk(message, 100, "{\"appID\":\"BEE-CNT\"\"OUT\":\"%i\"\"IN\":\"%i\"\"TIME\":\"%lld\"\"NAME\":\"%s\"}" \
					, totalOut, totalIn, ts, event->name);
				LOG_INF("Message formatted: %s, length: %i", message, len);
			
				struct cloud_msg msg = {
					.qos = CLOUD_QOS_AT_MOST_ONCE,
					.buf = message,
					.len = len
				};
				
				msg.endpoint.type = CLOUD_EP_MSG; 
				
				/* Send data to cloud */
				err = cloud_send(cloud_backend, &msg);
				
				if (err) {
					LOG_ERR("cloud_send failed, error: %d", err);
				}
				else{
					LOG_INF("Message published succesfully.");
				}
			}
			if(event->dyndata.size == 8){
				LOG_DBG("Broodminder weight data is being JSON-formatted");

				char message[100]; 

				/* Get timestamp */
				int64_t unix_time_ms = k_uptime_get();
				err = date_time_now(&unix_time_ms);
				int64_t divide = 1000;
				int64_t ts = (int64_t)(unix_time_ms / divide);
				
				LOG_DBG("Time: %d", ts);

				/* Format to JSON-string */
				uint16_t len = snprintk(message, 100, "{\"appID\":\"BM-W\"\"RTW\":\"%i.%i\"\"TEMP\":\"%i.%i\"\"TIME\":\"%lld\"\"NAME\":\"%s\"}" \
					, event->dyndata.data[4], event->dyndata.data[5] \
					, event->dyndata.data[6], event->dyndata.data[7], ts, event->name);
				LOG_INF("Message formatted: %s, length: %i", message, len);
			
				struct cloud_msg msg = {
					.qos = CLOUD_QOS_AT_MOST_ONCE,
					.buf = message,
					.len = len
				};
				
				msg.endpoint.type = CLOUD_EP_MSG; 

				/* Send data to the cloud */
				err = cloud_send(cloud_backend, &msg);
				if (err) {
					LOG_ERR("cloud_send failed, error: %d", err);
				}
				else{
					LOG_INF("Message published succesfully.");
				}
			}
			if(event->dyndata.size == 9){
				LOG_DBG("Thingy:52 data is being JSON-formatted");

				/* Logic to convert 4 bytes into int32_t */
				char pressure_arr[4];
				for (uint8_t i = 3; i <= 6; i++){
					pressure_arr[i-3] = event->dyndata.data[i];

				}
				int32_t pressure_little_endian;

				char reverse_press_arr[4];
				for (uint8_t i = 0; i <=3; i++){
					reverse_press_arr[i] = pressure_arr[3-i];
				}

				memcpy(&pressure_little_endian, reverse_press_arr, sizeof(pressure_little_endian));
			
				char message[110]; 

				/* Get timestamp */
				int64_t unix_time_ms = k_uptime_get();
				err = date_time_now(&unix_time_ms);
				int64_t divide = 1000;
				int64_t ts = unix_time_ms / divide;

				LOG_DBG("Time: %d", ts);

				/* Format to JSON-string */
				uint16_t len = snprintk(message, 110, "{\"appID\":\"Thingy\"\"TEMP\":\"%i.%i\"\"HUMID\":\"%i\"\"AIR\":\"%d.%i\"\"BTRY\":\"%i\"\"TIME\":\"%lld\"\"NAME\":\"%s\"}" \
					, event->dyndata.data[0], event->dyndata.data[1], event->dyndata.data[2], pressure_little_endian, event->dyndata.data[7],event->dyndata.data[8], ts, event->name);
				LOG_INF("Message formatted: %s, length: %i", message, len);
			
				struct cloud_msg msg = {
					.qos = CLOUD_QOS_AT_MOST_ONCE,
					.buf = message,
					.len = len
				};
				
				msg.endpoint.type = CLOUD_EP_MSG; 

				/* Send data to cloud */
				err = cloud_send(cloud_backend, &msg);

				k_sleep(K_SECONDS(1));

				/* Send data to cloud */
				err = cloud_send(cloud_backend, &msg);
				
				k_sleep(K_SECONDS(1));

				/* Send data to cloud */
				err = cloud_send(cloud_backend, &msg);
				
				if (err) {
					LOG_ERR("cloud_send failed, error: %d", err);
				}
				else{
					LOG_INF("Message published succesfully");
				}
			}
			k_sleep(K_SECONDS(10));
			err = lte_lc_offline();
			lte_sleep = true;
			LOG_INF("LTE offline");
			if(err){
				dk_set_led_on(DK_LED1_MSK);
			}
			LOG_INF("1");
			//err = pm_device_state_set(cons, PM_DEVICE_STATE_LOW_POWER,NULL,NULL); 
			//LOG_INF("2");
			//pm_power_state_force((struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
			
			return false;
		}
		/* Function to check number of connected peripherals */
		if(event->type==BLE_STATUS){
			LOG_DBG("BLE_STATUS");
			
			char message[100]; 
			/* Format data to JSON-format */
			uint16_t len = snprintf(message, 100, "{\"message\":\"Connected: %c, Missing: %c\"}", (char)event->dyndata.data[0], (char)event->dyndata.data[1]);

			LOG_DBG("%c, %c, %c", (char)event->dyndata.data[0], (char)event->dyndata.data[1], (char)event->dyndata.data[2]);

			LOG_INF("Message formatted: %s", message);
			
			struct cloud_msg msg = {
				.qos = CLOUD_QOS_AT_MOST_ONCE,
				.buf = message,
				.len = len
			};
			
			msg.endpoint.type = CLOUD_EP_MSG; 

			/* Send data to cloud */
			err = cloud_send(cloud_backend, &msg);
			
			if (err) {
				LOG_ERR("cloud_send failed, error: %d", err);
			}
			else{
				LOG_INF("Message published succesfully.");
			}
			return false;
		}
		/* Being connected to Cloud while Bluetooth is scanning can make
		* the program crash */
		if(event->type==BLE_SCANNING){
			LOG_INF("Stopping Cloud sync");
			cloud_disconnect(cloud_backend);
			return false;;
		}
		if(event->type==BLE_DONE_SCANNING){
			LOG_INF("Starting Cloud sync");
			k_work_reschedule(&connect_work, K_NO_WAIT);
			return false;
		}
	}	
	return false;
}

SYS_INIT(cloud_setup_fn, APPLICATION, 50);

EVENT_LISTENER(cloud_module, event_handler);
EVENT_SUBSCRIBE_EARLY(cloud_module, ble_event);
