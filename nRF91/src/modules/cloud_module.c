/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <init.h>
#include <app_event_manager.h>
#include <dk_buttons_and_leds.h>

#include <modem/lte_lc.h>
#include <zephyr/net/socket.h>
#include <net/nrf_cloud.h>
#include <net/socket.h>

#include <modem/nrf_modem_lib.h>

#include <logging/log.h>

#include "modules_common.h"
#include "events/cloud_event.h"
#include "events/ble_event.h"
#include "events/wdt_event.h"
#include "events/nvs_event.h"

#include "fota_support.h"
#include "wdt_module.h"

#include <date_time.h>

#include <bluetooth/bluetooth.h>

// Define cloud_module as current module.
#define MODULE cloud_module

LOG_MODULE_REGISTER(MODULE, CONFIG_LOG_DEFAULT_LEVEL);

// Battery measurment stuff, only applicable to the Thingy:91.
#if defined(CONFIG_BOARD_THINGY91)
#include <nrf_modem_at.h>
#define AT_CMD_VBAT		"AT%XVBAT"
static struct k_work_delayable get_thingy_voltage;
#endif

// Not currently used, but can be convenient for dummy data for testing.
static struct k_work_delayable connect_work;

static K_SEM_DEFINE(lte_connected, 0, 1);
static K_SEM_DEFINE(cloud_connecting, 0, 1);
static K_SEM_DEFINE(cloud_disconnecting, 0, 1);

/* Flag to signify if the cloud client is connected or not connected to cloud,
 * used to abort/allow cloud publications. */
static bool cloud_connected;

/* Flag to signify if lte is sleeping or connected.
 * Used to check the status of the LTE before sending messages or connecting to cloud.
 * Could not use lte_connected, because the name was taken. */
static bool lte_sleep = true;

#define MAX_MESSAGES CONFIG_MAX_OUTGOING_MESSAGES
#define MESSAGE_SIZE CONFIG_MAX_MESSAGE_SIZE

/* Queue of JSON-formatted messages to be sent to nRF Cloud. 
 * All messages are sent once the queue is full. */
static char message_queue[MAX_MESSAGES][MESSAGE_SIZE];

/* Array of message lengths, corresponding to the message_queue. */
static int message_lengths[MAX_MESSAGES];

/* Amount of messages currently waiting to be sent. */
static uint8_t queued = 0;
/* Hot fix, the whole queue/sendMessage should be implemented better. */
static uint8_t last_queue = 0;

#define SEND_QUEUE_STACK_SIZE 1024
#define SEND_QUEUE_PRIORITY 5

static void cloud_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt);
/* Initialize nrf_cloud library. */
struct nrf_cloud_init_param params = {
	.event_handler = cloud_event_handler
};

/**
 * @brief Reset connection to nRF Cloud, and reset connection status event state.
 * For internal use only. Externally, disconnect_cloud() may be used instead.
 */
static void reset_cloud(void)
{
	int err;
	LOG_INF("Resetting nRF Cloud transport");

	/* Disconnect from nRF Cloud and uninit the cloud library. */
	err = nrf_cloud_uninit();

	/* nrf_cloud_uninit returns -EBUSY if reset is blocked by a FOTA job. */
	if (err == -EBUSY) {
		LOG_ERR("Could not reset nRF Cloud transport due to ongoing FOTA job. "
			"Continuing without resetting");
	} else if (err) {
		LOG_ERR("Could not reset nRF Cloud transport, error %d. "
			"Continuing without resetting", err);
	} else {
		LOG_INF("nRF Cloud transport has been reset");
	}

}


/**
 * @brief Establish a connection to nRF Cloud (presuming we are connected to LTE).
 *
 * @return int - 0 on success, otherwise negative error code.
 */

//Finish implementation of this

static int connect_cloud()
{
	int err;

	LOG_INF("Connecting to nRF Cloud");

	/* Initialize nrf_cloud library. */
	err = nrf_cloud_init(&params);
	if (err) {
		LOG_ERR("Cloud lib could not be initialized, error: %d", err);
		return err;
	}

	/* Begin attempting to connect persistently. */
	while (true) {
		LOG_INF("Trying to connect to NRF CLOUD");

		err = nrf_cloud_connect(NULL);
		if (err) {
			LOG_ERR("cloud_connect, error: %d", err);
		}

		/* Wait for cloud connection success. If succeessful, break out of the loop. */
		if(!k_sem_take(&cloud_connecting, K_SECONDS(20))) {
			break;
		}
		else{
			LOG_DBG("Cloud connecting timed out, trying again.");
		}
	}

	LOG_INF("Connected to nRF Cloud");

	// Set up WDT and add a channel for the main device.
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
	
	return 0;
}


//Needs to be a thread to be able to wait for LTE connecting
extern void send_message(){
	int err;
	LOG_DBG("Send message thread started");
	
	if(lte_sleep){
		LOG_DBG("LTE normal");
		err = lte_lc_normal();
		if(err){
			#if defined(CONFIG_DK_LIBRARY)
			dk_set_led_on(DK_LED2_MSK);
			#endif
			LOG_ERR("LTE refuses to be normal");
		}
		
		LOG_INF("Waiting for lte");
		k_sem_take(&lte_connected, K_FOREVER);
		LOG_INF("LTE ready");
		
		err = connect_cloud(true);
		if (err) {
			LOG_ERR("cloud_connect, error: %d", err);
			return;
		}
		
		if(!cloud_connected){
			LOG_INF("Cloud not connected");
			err = connect_cloud(true);
			if (err) {
				LOG_ERR("cloud_connect, error: %d", err);
				LOG_ERR("Unable to connect to cloud, cannot send messages!");
				//Should save the message_queue to nvs and restart.
			return;
			}
		}
	}
	else{
		LOG_DBG("LTE allready on");
	}

	for(uint8_t i=0; i<last_queue;i++){
		LOG_INF("Sending message %i", i);
		struct nrf_cloud_tx_data mqtt_msg = {
					.data.ptr = message_queue[i],
					.data.len = message_lengths[i],
					.qos = MQTT_QOS_1_AT_LEAST_ONCE,
					.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
				};

		err = nrf_cloud_send(&mqtt_msg);
		if (err) {
			LOG_ERR("cloud_send failed, error: %d", err);
		}
		else{
			LOG_INF("Message published succesfully.");
		}
		//Sleep to ensure the messages are sent in the correct order
		k_sleep(K_MSEC(750));
	}
	//Disconnect from cloud
	reset_cloud();
	k_sem_take(&cloud_disconnecting, K_SECONDS(10));
	//Turn off LTE
	err = lte_lc_offline();
	lte_sleep = true;
	LOG_INF("LTE offline");
	if(err){
		#if defined(CONFIG_DK_LIBRARY)
		dk_set_led_on(DK_LED1_MSK);
		#endif
	}
}

K_THREAD_STACK_DEFINE(send_queue_stack_area, SEND_QUEUE_STACK_SIZE);
struct k_thread send_queue_thread_data;

static void enqueue_message(void* message, uint8_t length){
	/* Copy the message into the message queue */
	snprintk(message_queue[queued], MESSAGE_SIZE, "%.*s", length, message);
	
	message_lengths[queued] = length;

	LOG_INF("Message %d: %.*s", queued, message_lengths[queued], message_queue[queued]);
	queued++;
	if(queued==MAX_MESSAGES){
		last_queue = queued;
		/* Create a thread for sending the messages.
		 * It's a thread because we don't want to halt the app event manager. 
		 * This way we can receive new while we send. 
		 * There is a race condition here, where we can get new data before the first message is sent,
		 * which would overwrite the first message in the old queue. */
		k_tid_t my_tid = k_thread_create(&send_queue_thread_data, send_queue_stack_area,
										K_THREAD_STACK_SIZEOF(send_queue_stack_area),
										send_message,
										NULL, NULL, NULL,
										SEND_QUEUE_PRIORITY, 0, K_NO_WAIT);

		queued = 0;
	}
}

//Unused function, can be filled with dummy data for testing
static void connect_work_fn(struct k_work *work)
{	
	// struct ble_event *ble_event = new_ble_event(8);

	// ble_event->type = BLE_RECEIVED;

	// /* Send the data without the * and ID to cloud_module */
	// uint8_t data_array[8] = {0, 0, 0, 0, 0, 0, 36, 0};

	// /* Format to JSON-string */
	// uint16_t len = snprintk(message, 110, "{\"appID\":\"TEST\",\"TEMP\":\"24.50\",\"HUMID\":\"50\",\"AIR\":\"101.0\",\"BTRY\":\"61\",\"TIME\":\"%lld\",\"NAME\":\"Hive2\"}", ts);
	// LOG_INF("Message formatted: %s, length: %i", message, len);
	// enqueue_message(message, len);
	struct ble_event *ble_event = new_ble_event(8);

	ble_event->type = BLE_RECEIVED;

	/* Send the data without the * and ID to cloud_module */
	uint8_t data_array[8] = {0, 0, 0, 0, 0, 0, 42, 0};

	memcpy(ble_event->dyndata.data, data_array, 8);
	
	// /* Get name from received ID */
	// memcpy(ble_event->name, "Test", 4);

	// APP_EVENT_SUBMIT(ble_event);

	// k_work_schedule(&connect_work,
	// 	K_SECONDS(30));
}

/* Function to get the voltage from Thingy:91's battery */
#if defined(CONFIG_BOARD_THINGY91)
static void get_thingy_voltage_fn(struct k_work *work)
{
	int err;

	char buf[20];
	enum at_cmd_state *response;
	err = nrf_modem_at_cmd(response, sizeof(response), AT_CMD_VBAT);

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
	int len = snprintk(message, 50, "{\"appID\":\"HUB-VOLT\",\"BAT\":\"%.4s\",\"TIME\":\"%lld\"}", voltage, ts);

	LOG_DBG("%.50s", message);

	enqueue_message(message, len);

	k_work_reschedule(&get_thingy_voltage, K_MINUTES(15));
}
#endif


/**
 * @brief Handler for events from nRF Cloud Lib.
 *
 * @param nrf_cloud_evt Passed in event.
 */
/* Taken from nrf/samples/nrf9160/nrf_cloud_mqtt_multi_service and modified */
static void cloud_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt)
{
	switch (nrf_cloud_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		/* Notify that we have connected to the nRF Cloud. */
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		cloud_connected = true;

		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		LOG_DBG("Cloud event status: %i", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		/* This event indicates that the user must associate the device with their
		 * nRF Cloud account in the nRF Cloud portal.
		 */
		LOG_INF("Please add this device to your cloud account in the nRF Cloud portal.");

		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATED");
		/* Indicates successful association with an nRF Cloud account.
		 * Will be fired every time the device connects.
		 * If an association request has been previously received from nRF Cloud,
		 * this means this is the first association of the device, and we must disconnect
		 * and reconnect to ensure proper function of the nRF Cloud connection.
		 */

		// if (cloud_has_requested_association()) {
		// 	/* We rely on the connection loop to reconnect automatically afterwards. */
		// 	LOG_INF("Device successfully associated with cloud!");
		// }
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_DBG("NRF_CLOUD_EVT_READY");
		/* Notify that nRF Cloud is ready for communications from us. */
		
		k_sem_give(&cloud_connecting);
			
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
		LOG_DBG("Cloud event status: %i", nrf_cloud_evt->status);
		/* Notify that we have lost contact with nRF Cloud. */
		cloud_connected = false;
		k_sem_give(&cloud_disconnecting);
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_ERROR: %d", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_RX_DATA:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA");
		LOG_DBG("%d bytes received from cloud", nrf_cloud_evt->data.len);

		struct cloud_event_abbr *cloud_event_abbr = new_cloud_event_abbr(nrf_cloud_evt->data.len);

        cloud_event_abbr->type = CLOUD_RECEIVED;

        memcpy(cloud_event_abbr->dyndata.data, log_strdup(nrf_cloud_evt->data.ptr), nrf_cloud_evt->data.len);

        APP_EVENT_SUBMIT(cloud_event_abbr);
		break;
	case NRF_CLOUD_EVT_FOTA_START:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_START");
		break;
	case NRF_CLOUD_EVT_FOTA_DONE: {
		enum nrf_cloud_fota_type fota_type = NRF_CLOUD_FOTA_TYPE__INVALID;

		if (nrf_cloud_evt->data.ptr) {
			fota_type = *((enum nrf_cloud_fota_type *) nrf_cloud_evt->data.ptr);
		}

		LOG_DBG("NRF_CLOUD_EVT_FOTA_DONE, FOTA type: %s",
			fota_type == NRF_CLOUD_FOTA_APPLICATION	  ?		"Application"	:
			fota_type == NRF_CLOUD_FOTA_MODEM	  ?		"Modem"		:
			fota_type == NRF_CLOUD_FOTA_BOOTLOADER	  ?		"Bootloader"	:
										"Invalid");

		/* Notify fota_support of the completed download. */
		on_fota_downloaded();
		break;
	}
	case NRF_CLOUD_EVT_FOTA_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_ERROR");
		break;
	default:
		LOG_DBG("Unknown event type: %d", nrf_cloud_evt->type);
		break;
	}
}

/* Taken from nrf/samples/nrf9160/nrf_cloud_mqtt_multi_service and modified */
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
		/* Notify that LTE has connected. */
		k_sem_give(&lte_connected);
		lte_sleep = false;

		/* Send a LTE_CONNECTED event, not currently used, but will be necessary for the SMS and https modules. 
		 * Should also have a event for disconnecting aswell as requesting LTE to connect to communicate in between modules that use LTE. */
		struct cloud_event_abbr *cloud_event_sleep = new_cloud_event_abbr(strlen("LTE is connected."));

        cloud_event_sleep->type = LTE_CONNECTED;

        memcpy(cloud_event_sleep->dyndata.data, log_strdup("LTE is connected."), strlen("LTE is connected."));
	
		APP_EVENT_SUBMIT(cloud_event_sleep);

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

/* Taken from nrf/samples/nrf9160/lte_ble_gateway. */
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

#if defined(CONFIG_DK_LIBRARY)
/* Button handler, can be used for testing. There are no practical use for them as of now. */
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	/* Press Button 1 to turn of the lights to save power */
	if (has_changed & button_states & DK_BTN1_MSK) {
		int err;
		dk_set_leds_state(DK_ALL_LEDS_MSK, 0);
		enum lte_lc_func_mode mode;
		err = lte_lc_func_mode_get(&mode);
		if(mode==LTE_LC_FUNC_MODE_OFFLINE){
			err = lte_lc_normal();
			LOG_INF("LTE normal");
			if(err){
				LOG_ERR("LTE refuses to be normal");
				dk_set_led_on(DK_LED2_MSK);
			}
		}
		else{
			err = lte_lc_offline();
			lte_sleep = true;
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
		struct cloud_event_abbr *cloud_event_sleep = new_cloud_event_abbr(strlen("Cloud entering sleep mode"));

        cloud_event_sleep->type = CLOUD_SLEEP;

        memcpy(cloud_event_sleep->dyndata.data, log_strdup("Cloud entering sleep mode"), strlen("Cloud entering sleep mode"));

	 	APP_EVENT_SUBMIT(cloud_event_sleep);
	}
	#endif
}
#endif

void cloud_setup_fn(void)
{
	int err;

	/* We need to enable bluetooth before we connect to LTE */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	#if defined(CONFIG_DK_LIBRARY)
	err = dk_leds_init();
	dk_set_leds_state(DK_ALL_LEDS_MSK, 0);

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}
	#endif

	LOG_INF("Cloud client has started");

	k_work_init_delayable(&connect_work, connect_work_fn);
	#if defined(CONFIG_BOARD_THINGY91)
	k_work_init_delayable(&get_thingy_voltage, get_thingy_voltage_fn);
	#endif
	/* Configures the modem and starts connection to LTE */
	modem_configure();

	LOG_INF("Connecting to LTE network, this may take several minutes...");

	/* Wait untill LTE is connected to continue. */
	k_sem_take(&lte_connected, K_FOREVER);

	/* cloud_setup_fn has higher priority than the date time modem thread,
	so we need to sleep this thread until date time modem is done running */
	while(!date_time_is_valid()){
		LOG_DBG("Date time is not yet gotten from the modem, going back to sleep.");

		k_sleep(K_SECONDS(5));
	}

	LOG_INF("Time extracted from LTE");

	#if defined(CONFIG_BOARD_THINGY91)
	k_work_reschedule(&get_thingy_voltage, K_NO_WAIT);
	#endif

	/* Turn off the LTE while it is not used. */
	lte_lc_offline();
	lte_sleep = true;

	/* Notify the other modules that cloud_setup_fn is done. */
	struct cloud_event_abbr *cloud_setup = new_cloud_event_abbr(strlen("Cloud ready"));
	
	cloud_setup->type = CLOUD_SETUP_COMPLETE;

	memcpy(cloud_setup->dyndata.data, log_strdup("Cloud ready"), strlen("Cloud ready"));

	APP_EVENT_SUBMIT(cloud_setup);
}

static bool event_handler(const struct app_event_header *eh)
{
    if(is_ble_event(eh)){
        int err;
        struct ble_event *event = cast_ble_event(eh);
		/* Logic to convert Bluetooth data to JSON-formatted strings. */
        if(event->type==BLE_RECEIVED){

			if(event->dyndata.size == 1){
				
				// Create new CLOUD_SEND_NVS event.
				struct cloud_event_abbr *cloud_send_wdt = new_cloud_event_abbr(10);
				cloud_send_wdt->type = CLOUD_SEND_WDT;
				cloud_send_wdt->dyndata.data[0] = event->dyndata.data[0]; // Timed out channel id.
				for (uint8_t i=0; i < sizeof(event->name); i++){
					cloud_send_wdt->name[i] = event->name[i];
				}
				// Submit main write before reboot event.
				APP_EVENT_SUBMIT(cloud_send_wdt);
				
				// LOG_DBG("WDT data is being JSON-formatted");

				// char message[100];

				// /* Get timestamp */
				// int64_t unix_time_ms = k_uptime_get();
				// err = date_time_now(&unix_time_ms);
				// int64_t divide = 1000;
				// int64_t ts = unix_time_ms / divide;

				// LOG_DBG("Time: %d", ts);

				// /* Format to JSON-string */
				// uint16_t len = snprintk(message, 100, "{\"appID\":\"WDT\",\"Channel\":%d,\"TIME\":%lld,\"NAME\":\"%s\"}" \
				// 	, event->dyndata.data, ts, event->name);
				// LOG_INF("Message formatted: %s, length: %i", message, len);

				// enqueue_message(message, len);
			}
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
				uint16_t len = snprintk(message, 100, "{\"appID\":\"BEE-CNT\",\"OUT\":%i,\"IN\":%i,\"TIME\":%lld,\"NAME\":\"%s\"}" \
					, totalOut, totalIn, ts, event->name);
				LOG_DBG("Message formatted: %s, length: %i", message, len);

				enqueue_message(message, len);
			}
			if(event->dyndata.size == 5){
				LOG_DBG("Woodpecker data is being JSON-formatted");

				/* Logic to turn two bytes into one uint16_t*/
				char total_arr[2];
				char highest_arr[2];
				for (uint8_t i = 0; i < 2; i++){
					total_arr[i] = event->dyndata.data[2-i];
					highest_arr[i] = event->dyndata.data[4-i];
				}

				uint16_t total_triggers;
				uint8_t positive_triggers = event->dyndata.data[0];
				uint8_t highest_probability = event->dyndata.data[3];
				uint8_t bat_percentage = event->dyndata.data[4];

				memcpy(&total_triggers, total_arr, sizeof(uint16_t));
				
				char message[120];

				/* Get timestamp */
				int64_t unix_time_ms = k_uptime_get();
				err = date_time_now(&unix_time_ms);
				int64_t divide = 1000;
				int64_t ts = unix_time_ms / divide;

				LOG_DBG("Time: %d", ts);

				/* Format to JSON-string */
				uint16_t len = snprintk(message, 120, "{\"appID\":\"W-PECK\",\"TOTAL\":%i,\"POSITIVE\":%i,\"PROBABILITY\":%i,\"BTRY\":%i,\"TIME\":%lld,\"NAME\":\"%s\"}" \
					, total_triggers, positive_triggers, highest_probability, bat_percentage, ts, event->name);
				LOG_DBG("Message formatted: %s, length: %i", message, len);

				enqueue_message(message, len);
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
				uint16_t len = snprintk(message, 100, "{\"appID\":\"BM-W\",\"RTW\":%i.%i,\"TEMP\":%i.%i,\"TIME\":%lld,\"NAME\":\"%s\"}" \
					, event->dyndata.data[4], event->dyndata.data[5] \
					, event->dyndata.data[6], event->dyndata.data[7], ts, event->name);
				LOG_DBG("Message formatted: %s, length: %i", message, len);

				enqueue_message(message, len);
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
				uint16_t len = snprintk(message, 110, "{\"appID\":\"Thingy\",\"TEMP\":%i.%i,\"HUMID\":%i,\"AIR\":%d.%i,\"BTRY\":%i,\"TIME\":%lld,\"NAME\":\"%s\"}" \
					, event->dyndata.data[0], event->dyndata.data[1], event->dyndata.data[2], pressure_little_endian, event->dyndata.data[7],event->dyndata.data[8], ts, event->name);
				LOG_DBG("Message formatted: %s, length: %i", message, len);

				enqueue_message(message, len);
			}

			return false;
		}
		/* Function to send number of connected peripherals */
		if(event->type==BLE_STATUS){
			LOG_DBG("BLE_STATUS");

			char message[100];
			/* Format data to JSON-format */
			uint16_t len = snprintf(message, 100, "{\"message\":\"Connected: %c, Missing: %c\"}", (char)event->dyndata.data[0], (char)event->dyndata.data[1]);

			LOG_INF("Message formatted: %s", message);

			enqueue_message(message, len);

			return false;
		}
		/* Being connected to Cloud while Bluetooth is scanning can make
		* the program crash. Might not be a problem in revision 2.0.0 of NCS. */
		// Legacy, no longer used due to LTE is off when not used.
		if(event->type==BLE_SCANNING){
			LOG_INF("Stopping Cloud sync");
			return false;
		}
		//Legacy
		if(event->type==BLE_DONE_SCANNING){
			LOG_INF("Starting Cloud sync");
			return false;
		}
	}
	if(is_cloud_event_abbr(eh)) {
        int err;
        struct cloud_event_abbr *event = cast_cloud_event_abbr(eh);
        
		if(event->type==CLOUD_SEND_WDT){
			
			char message[100];
			
			while(!date_time_is_valid()){
				// LOG_DBG("cloud_module: event_handler(): Time! %i", date_time_is_valid());
				k_sleep(K_SECONDS(1));
			}
			// LOG_DBG("cloud_module: event_handler(): Time! %i", date_time_is_valid());

			// Get timestamp
			int64_t unix_time_ms = k_uptime_get();
			int err = date_time_now(&unix_time_ms);
			int64_t divide = 1000;
			int64_t ts = unix_time_ms / divide;
			
			if (event->dyndata.data[0] > WDT_CHANNEL_NRF91_NRF53_DEVICE) { // If wdt on nRF53
				LOG_DBG("nRF53 WDT data is being JSON-formatted");
				// Format to JSON-string
				uint16_t len = snprintk(message, 100, "{\"appID\":\"WDT\",\"Channel\":\"%d\",\"TIME\":\"%lld\",\"NAME\":\"%s\"}", 
										event->dyndata.data, ts, event->name);
				LOG_INF("cloud_module: event_handler(): Message formatted: %s, length: %i", message, len);
				enqueue_message(message, len);

				return false;
			}
			else { // Else wdt on nRF91
				LOG_DBG("nRF91 WDT data is being JSON-formatted");
				// Format to JSON-string
				uint16_t len = snprintk(message, 100, "{\"appID\":\"WDT\",\"Channel\":\"%d\",\"TIME\":\"%lld\",\"NAME\":\"%s\"}", 
										*event->dyndata.data, ts, "nRF91");
				LOG_INF("cloud_module: event_handler(): Message formatted: %s, length: %i", message, len);
				enqueue_message(message, len);

				return false;
			}
			return false;
		}
		return false;
	}
	return false;
}

#define STACKSIZE 2048

K_THREAD_DEFINE(cloud_setup_thread, STACKSIZE,
		cloud_setup_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, ble_event);
APP_EVENT_SUBSCRIBE(MODULE, cloud_event_abbr);