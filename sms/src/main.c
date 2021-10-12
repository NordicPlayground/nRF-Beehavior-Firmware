/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <kernel.h>
#include <string.h>
#include <modem/sms.h>
#include <modem/lte_lc.h>
#include <zephyr.h>
#include <init.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

static K_SEM_DEFINE(lte_connected, 0, 1);

static void sms_callback(struct sms_data *const data, void *context)
{
	if (data == NULL) {
		printk("%s with NULL data\n", __func__);
		return;
	}

	if (data->type == SMS_TYPE_DELIVER) {
		/* When SMS message is received, print information */
		struct sms_deliver_header *header = &data->header.deliver;

		printk("\nSMS received:\n");
		printk("\tTime:   %02d-%02d-%02d %02d:%02d:%02d\n",
			header->time.year,
			header->time.month,
			header->time.day,
			header->time.hour,
			header->time.minute,
			header->time.second);

		printk("\tText:   '%s'\n", data->payload);
		printk("\tLength: %d\n", data->payload_len);

		if (header->app_port.present) {
			printk("\tApplication port addressing scheme: dest_port=%d, src_port=%d\n",
				header->app_port.dest_port,
				header->app_port.src_port);
		}
		if (header->concatenated.present) {
			printk("\tConcatenated short message: ref_number=%d, msg %d/%d\n",
				header->concatenated.ref_number,
				header->concatenated.seq_number,
				header->concatenated.total_msgs);
		}
	} else if (data->type == SMS_TYPE_STATUS_REPORT) {
		printk("SMS status report received\n");
	} else {
		printk("SMS protocol message with unknown type received\n");
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


void main(void)
{
	int err = lte_lc_init_and_connect_async(lte_handler);
		if (err) {
			LOG_ERR("Modem could not be configured, error: %d",
				err);
			return;
		}

	int handle = 0;
	int ret = 0;

	printk("\nSMS sample starting\n");

	k_sem_take(&lte_connected, K_FOREVER);

	handle = sms_register_listener(sms_callback, NULL);
	if (handle) {
		printk("sms_register_listener returned err: %d\n", handle);
		return;
	}

	printk("SMS sample is ready for receiving messages\n");

	/* Sending is done to the phone number specified in the configuration,
	 * or if it's left empty, an information text is printed.
	 */
	if (strcmp(CONFIG_SMS_SEND_PHONE_NUMBER, "")) {
		printk("Sending SMS: number=%s, text=\"Hva enn som står her er ikke saa viktig egentlig\"\n",
			CONFIG_SMS_SEND_PHONE_NUMBER);
		ret = sms_send_text(CONFIG_SMS_SEND_PHONE_NUMBER, "God bedring mvh terrorbruh");
		if (ret) {
			printk("sms_send returned err: %d\n", ret);
		}
	} else {
		printk("\nSMS sending is skipped but receiving will still work.\n"
			"If you wish to send SMS, please configure CONFIG_SMS_SEND_PHONE_NUMBER\n");
	}

	/* In our application, we should unregister SMS in some conditions with:
	 *   sms_unregister_listener(handle);
	 * However, this sample will continue to be registered for
	 * received SMS messages and they can be seen in serial port log.
	 */
}
