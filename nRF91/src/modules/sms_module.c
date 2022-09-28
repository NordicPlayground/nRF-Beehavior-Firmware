#include <app_event_manager.h>
#include <init.h>

#include <stdio.h>
#include <kernel.h>
#include <string.h>
#include <modem/lte_lc.h>
#include <zephyr.h>

#include "events/ble_event.h"
#include "events/cloud_event.h"
#include <modem/sms.h>
#include <logging/log.h>

#include "include/nvs.h"

#include <nrf_modem_at.h>
#include "events/sms_event.h"
#define AT_CMD_PSM_OFF		"AT+CPSMS=0"

LOG_MODULE_REGISTER(sms_module, CONFIG_LOG_DEFAULT_LEVEL);

static struct k_work_delayable warn_delay;

#define WARNING_DELAY_HOURS CONFIG_WARNING_DELAY_HOURS

char str[80];

static K_SEM_DEFINE(lte_connected, 0, 1);

int warn_sent = 0;

static void warn_delay_fn(struct k_work *work)
{
    int warn_sent = 0;

	LOG_INF("Warning has been reset.");
}

static void sms_callback(struct sms_data *const data, void *context)
{
    LOG_INF("DATA : %s", data);
	if (data == NULL) {
		LOG_INF("%s with NULL data\n", __func__);
		return;
	}

	if (data->type == SMS_TYPE_DELIVER) {
		/* When SMS message is received, print information */
		struct sms_deliver_header *header = &data->header.deliver;

		LOG_INF("\nSMS received:\n");
		LOG_INF("\tTime:   %02d-%02d-%02d %02d:%02d:%02d\n",
			header->time.year,
			header->time.month,
			header->time.day,
			header->time.hour,
			header->time.minute,
			header->time.second);

		LOG_INF("\tText:   '%s'\n", data->payload);
        if(strstr(data->payload, "STOPP")){
            LOG_INF(" Received \"STOPP\" from %s", header->originating_address.address_str);
            sms_send_text(header->originating_address.address_str, "You sent \"STOPP\" and will no longer receive warning messages. Your number is now banned, to resubscribe please send \"START\".");

            //LOG_INF("\nSMS received from %s:\n", header->originating_address.address_str);

        }
        if(strstr(data->payload, "FORBRUK")){
            LOG_INF(" Received \"FORBRUK\" from %s", header->originating_address.address_str);
            sms_send_text("1999", "FORBRUK");
 
            //LOG_INF("\nSMS received from %s:\n", header->originating_address.address_str);
        }
        
        if(strstr(data->payload, "START")){
            uint8_t sub = 1;
            LOG_INF(" Received \"START\" from %s", header->originating_address.address_str);
            sms_send_text(header->originating_address.address_str, "You sent \"START\". You number is registered and you can now recieve warning messages again.");
            struct sms_event *sms_event = new_sms_event(strlen(header->originating_address.address_str));

            sms_event->type = NUMBER_STATUS;

            sms_event->subScribe = sub;
            LOG_INF("Mulig kresj, %i", strlen(header->originating_address.address_str));
            memcpy(sms_event->dyndata.data, &header->originating_address.address_str, strlen(header->originating_address.address_str));
            memcpy(sms_event->subScribe, sub, sizeof(sub));

            
            LOG_INF("NVVVVVVVVM: %i", sub);
            
            APP_EVENT_SUBMIT(sms_event);
            //LOG_INF("\nSMS received from %s:\n", header->originating_address.address_str);
        }
		
		LOG_INF("\tLength: %d\n", data->payload_len);

		if (header->app_port.present) {
			LOG_INF("\tApplication port addressing scheme: dest_port=%d, src_port=%d\n",
				header->app_port.dest_port,
				header->app_port.src_port);
		}
		if (header->concatenated.present) {
			LOG_INF("\tConcatenated short message: ref_number=%d, msg %d/%d\n",
				header->concatenated.ref_number,
				header->concatenated.seq_number,
				header->concatenated.total_msgs);
		}
	} else if (data->type == SMS_TYPE_STATUS_REPORT) {
		LOG_INF("SMS status report received\n");
	} else {
		LOG_INF("SMS protocol message with unknown type received\n");
	}
}



void sms_https_number(const char *text){

    char buf[20] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

    uint8_t start = 0;
    int i = 0;
    for(i; i<strlen(str); i++){
        // LOG_INF("%c", str[i]);
        if(str[i]==','){
            memcpy(&buf, &str[start], i-start);
            // char useless = '\0';
            // memcpy(&buf[i-start], &useless, 1);
            LOG_INF("Buffer: %.*s, length: %i", i-start, buf, strlen(buf));
            start = i+1;
            // sms_send_text(buf, text);
        }
    }
    memcpy(&buf, &str[start], strlen(str)-start);
    LOG_INF("Buffer: %.*s, length: %i", strlen(str)-start, buf, strlen(buf));
    // sms_send_text(buf, text);
    // LOG_INF("Buffer: %s", buf);
    start = i;
}

void sms_setup_fn(void)
{
    LOG_INF("SMS waiting to set up");
    k_sem_take(&lte_connected, K_FOREVER);

    k_sleep(K_SECONDS(15));

    char * test_number = "4747448146";

    struct sms_event *sms_event = new_sms_event(strlen(test_number));

    sms_event->type = NUMBER_STATUS;

    LOG_INF("Mulig kræsj, %i", strlen(test_number));
    memcpy(sms_event->dyndata.data, test_number, strlen(test_number));
    LOG_INF("NVM");
    
    APP_EVENT_SUBMIT(sms_event);

    char buf[50];
	int err = nrf_modem_at_cmd(buf, 50, AT_CMD_PSM_OFF);
    if(err){
        LOG_ERR("Fak ass: %i", err);
    }
    else{
        LOG_INF("%s", buf);
    }
    LOG_INF("SMS setting up");
    k_work_init_delayable(&warn_delay, warn_delay_fn);

    int handle = 0;
	int ret = 0;

	LOG_INF("\nSMS sample starting\n");

    handle = sms_register_listener(sms_callback, NULL);

    if (handle) {
		LOG_INF("sms_register_listener returned err: %d\n", handle);
		return;
	}
	LOG_INF("SMS sample is ready for receiving messages\n");

	/* Sending is done to the phone number specified in the configuration,
	 * or if it's left empty, an information text is printed.
	 */
    int rc = 0;
    err = 0;

    const struct device *nvs_dev;

    nvs_dev = FLASH_AREA_DEVICE(DEVICE_LABEL);

    nvs_init(&nvs, nvs_dev->name);

    for(int i=0;i<10;i++){
        char buf[20];   

        rc = nvs_read(&nvs, TELEPHONE_ID_START+i, &buf, 20);
        // rc = nvs_read(&nvs, TELEPHONE_ID_START+i, &buf, 11);
        //memcpy(&buf, &buf, rc);
        if (rc > 0) {
            LOG_INF("Length of data available: %i", rc);
            for(int i=0; i<rc; i++){
                LOG_INF("%c", buf[i]);
            }
            /* item was found, show it */
            LOG_INF("WIIIIHOOOOO NEW DATA Id: %d, Data: %.*s, Length: %i\n",
            TELEPHONE_ID_START+i, rc, buf, strlen(buf));
            // strcat(str, buf);

            memcpy(&str[0+strlen(str)], &buf[0], rc);
            strcat(str, ",");
        }
        // else if (rc != 0)
        // {
        //     LOG_ERR("The following number is either too short or too long: %.*s. It will now be deleted", rc, buf);
        //     err = nvs_delete(&nvs, TELEPHONE_ID_START+i);
        //     if(err){
        //         LOG_ERR("Deleting https number failed, error: %i", err);
        //     }
        // }
    }
    //sms_send_text("+4747707504", "Suck a dick...");
    str[strlen(str)-1] = '\0';
    LOG_INF("%s", str);

	// if (strcmp(CONFIG_SMS_SEND_PHONE_NUMBER, "")) {
	// 	LOG_INF("Sending SMS: number=%s, text=\"SMS sample: testing\"\n",
	// 		CONFIG_SMS_SEND_PHONE_NUMBER);
	// 	ret = sms_send_text(CONFIG_SMS_SEND_PHONE_NUMBER, "SMS sample: testing");
	// 	if (ret) {
	// 		LOG_INF("sms_send returned err: %d\n", ret);
	// 	}
	// } else {
	// 	LOG_INF("\nSMS sending is skipped but receiving will still work.\n"
	// 		"If you wish to send SMS, please configure CONFIG_SMS_SEND_PHONE_NUMBER\n");
	// }


}

/*event_handler taken from C:\ncs\nRF-Beehavior-Firmware\nRF91\src\modules\cloud_module.c
made som adjustments */

static bool event_handler(const struct app_event_header *eh)
{
    if(is_sms_event(eh)){
    
        int err;
        struct sms_event *event = cast_sms_event(eh);

        if(event->type==NEW_NUMBER){
            LOG_INF("would send to number: %.*s", event->dyndata.size, event->dyndata.data);
            //sms_send_text(event->dyndata.data, "HELLO U HAVE SUBSCRIBED TO THE FOLLOWING.");
        }
        return false;
    }
    if(is_cloud_event_abbr(eh)){
        struct cloud_event_abbr *event = cast_cloud_event_abbr(eh);
        if(event->type==LTE_CONNECTED){
            LOG_DBG("LTE is connected? That's coolio");
            //sms_setup_fn();
            k_sem_give(&lte_connected);
            return false;
        }
        return false;
    }
    if(is_ble_event(eh)){
        
        int err;
        struct ble_event *event = cast_ble_event(eh);

        if(event->type==BLE_RECEIVED){

            char sms_buf[140];

			if(event->dyndata.size == 4){
			/*Maybe include a warning for beecounter, incase there are extreme amounts of activity*/
            
			}

			if(event->dyndata.size == 8){

				LOG_DBG("Broodminder weight data is being JSON-formatted");

				/* Get timestamp */
				int64_t unix_time_ms = k_uptime_get();
				err = date_time_now(&unix_time_ms);
				int64_t divide = 1000;
				int64_t ts = (int64_t)(unix_time_ms / divide);

				LOG_DBG("Time: %d", ts);

                /* setting the warning limits for the temperature outside and the weights*/
                if (!warn_sent && event->dyndata.data[6] >= 40){

                    k_work_schedule(&warn_delay, K_HOURS(WARNING_DELAY_HOURS));
                    snprintk(sms_buf, sizeof(sms_buf), "YEEEEET!Temperature warning, at time: %lld! The OUTSIDE temperture of hive \"%s\" is reaching an alarmingly high value of %i.%i.",
                    ts, event->name, event->dyndata.data[6], event->dyndata.data[7]);
                    LOG_WRN("Temperature warning, at time: %lld! The OUTSIDE temperture of hive \"%s\" is reaching an alarmingly high value of %i.%i.",
                    ts, event->name, event->dyndata.data[6], event->dyndata.data[7]);
                    sms_https_number(sms_buf);
                }

                if(!warn_sent && event->dyndata.data[6] <= 0){

                    k_work_schedule(&warn_delay, K_HOURS(WARNING_DELAY_HOURS));
                    snprintk(sms_buf, sizeof(sms_buf), "Temperature warning, at time: %lld! The OUTSIDE temperture of hive \"%s\" is reaching an alarmingly low value of %i.%i.",
                    ts, event->name, event->dyndata.data[6], event->dyndata.data[7]);
                    LOG_WRN("Temperature warning, at time: %lld! The OUTSIDE temperture of hive \"%s\" is reaching an alarmingly low value of %i.%i.",
                    ts, event->name, event->dyndata.data[6], event->dyndata.data[7]);
                    sms_https_number(sms_buf);
                }

                if(!warn_sent && event->dyndata.data[4] >= 20){

                    k_work_schedule(&warn_delay, K_HOURS(WARNING_DELAY_HOURS));
                    snprintk(sms_buf, sizeof(sms_buf), "Weight warning, at time: %lld! The weight of hive \"%s\" is reaching an alarmingly high value of %i.%i.",
                    ts, event->name, event->dyndata.data[4], event->dyndata.data[5]);
                    sms_https_number(sms_buf);
                }

                if(!warn_sent && event->dyndata.data[4] <= 2){

                    k_work_schedule(&warn_delay, K_HOURS(WARNING_DELAY_HOURS));
                    snprintk(sms_buf, sizeof(sms_buf), "cucumber! Weight warning, at time: %lld! The weight of hive \"%s\" is reaching an alarmingly low value of %i.%i.",
                    ts, event->name, event->dyndata.data[4], event->dyndata.data[5]);
                    sms_https_number(sms_buf);
                }
			}


			if(event->dyndata.size == 9){

				LOG_DBG("Thingy:52 data is being JSON-formatted");

				/* Get timestamp */
				int64_t unix_time_ms = k_uptime_get();
				err = date_time_now(&unix_time_ms);
				int64_t divide = 1000;
				int64_t ts = unix_time_ms / divide;

				LOG_DBG("Time: %d", ts);

                if(!warn_sent && event->dyndata.data[0] >= 40){

                    k_work_schedule(&warn_delay, K_HOURS(WARNING_DELAY_HOURS));
                    snprintk(sms_buf, sizeof(sms_buf), "Temperature warning, at time: %lld! The INSIDE temperture of hive \"%s\" is reaching an alarmingly high value of %i.%i.",
                    ts, event->name, event->dyndata.data[0], event->dyndata.data[1]);
                    sms_https_number(sms_buf);
                }

                if(!warn_sent && event->dyndata.data[0] <= 30){

                    k_work_schedule(&warn_delay, K_HOURS(WARNING_DELAY_HOURS));
                    snprintk(sms_buf, sizeof(sms_buf), "Temperature warning, at time: %lld! The INSIDE temperture of hive \"%s\" is reaching an alarmingly low value of %i.%i.",
                    ts, event->name, event->dyndata.data[0], event->dyndata.data[1]);
                    sms_https_number(sms_buf);
                }

                if(!warn_sent && event->dyndata.data[2] >= 65){

                    k_work_schedule(&warn_delay, K_HOURS(WARNING_DELAY_HOURS));
                    snprintk(sms_buf, sizeof(sms_buf), "Humidity warning, at time: %lld! The humidity of hive \"%s\" is reaching an alarmingly high value of %i.",
                    ts, event->name, event->dyndata.data[2]);
                    sms_https_number(sms_buf);
                }

                if(!warn_sent && event->dyndata.data[2] <= 35){

                    k_work_schedule(&warn_delay, K_HOURS(WARNING_DELAY_HOURS));
                    snprintk(sms_buf, sizeof(sms_buf), "Humidity warning, at time: %lld! The humidity of hive \"%s\" is reaching an alarmingly low value of %i.",
                    ts, event->name, event->dyndata.data[2]);
                    sms_https_number(sms_buf);
                }

                if(!warn_sent && event->dyndata.data[8] == 10){

                    k_work_schedule(&warn_delay, K_HOURS(WARNING_DELAY_HOURS));
                    snprintk(sms_buf, sizeof(sms_buf), "Battery warning, at time: %lld! The battery of hive \"%s\" is reaching an alarmingly low value of %i.",
                    ts, event->name, event->dyndata.data[8]);
                    sms_https_number(sms_buf);
                }

			}
            warn_sent = 1;

			return false;
		}
    }

    return false;
}

#define STACKSIZE 2048

K_THREAD_DEFINE(sms_setup_thread, STACKSIZE,
		sms_setup_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(sms_module, event_handler);
APP_EVENT_SUBSCRIBE_EARLY(sms_module, ble_event);
APP_EVENT_SUBSCRIBE(sms_module, cloud_event_abbr);
APP_EVENT_SUBSCRIBE(sms_module, sms_event);
