/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

#include <zephyr/types.h>
#include <zephyr.h>
#include <drivers/uart.h>

#include <device.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>

#include <dk_buttons_and_leds.h>

#include <settings/settings.h>

#include <stdio.h>

#include <logging/log.h>

#include <drivers/sensor.h>
#include "battery.h"

// #include "C:/ncs/zephyr/drivers/sensor/hts221/hts221.h"


#define LOG_MODULE_NAME GATT
LOG_MODULE_REGISTER(LOG_MODULE_NAME, 4);

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY 7

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

// #define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000

static K_SEM_DEFINE(ble_init_ok, 0, 1);
// #define CON_STATUS_LED DK_LED2

// #define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
// #define KEY_PASSKEY_REJECT DK_BTN2_MSK

static const struct battery_level_point levels[] = {
#if DT_NODE_HAS_PROP(DT_INST(0, voltage_divider), io_channels)
	/* "Curve" here eyeballed from captured data for the [Adafruit
	 * 3.7v 2000 mAh](https://www.adafruit.com/product/2011) LIPO
	 * under full load that started with a charge of 3.96 V and
	 * dropped about linearly to 3.58 V over 15 hours.  It then
	 * dropped rapidly to 3.10 V over one hour, at which point it
	 * stopped transmitting.
	 *
	 * Based on eyeball comparisons we'll say that 15/16 of life
	 * goes between 3.95 and 3.55 V, and 1/16 goes between 3.55 V
	 * and 3.1 V.
	 */

	{ 10000, 4100 },
	{ 625, 3550 },
	{ 0, 3100 },
#else
	/* Linear from maximum voltage to minimum voltage. */
	{ 10000, 3600 },
	{ 0, 1700 },
#endif
};

static struct bt_conn *current_conn;

#define BT_UUID_BEE_VAL 0x0BEE

#define BT_UUID_BEE \
	BT_UUID_DECLARE_16(BT_UUID_BEE_VAL)
/** @def BT_UUID_HIDS_VAL
 *  @brief HID Service UUID value
*/

#define BT_UUID_TEMP_SENSOR_VAL 0x1BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_TEMP_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_TEMP_SENSOR_VAL)

#define BT_UUID_HUMID_SENSOR_VAL 0x2BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_HUMID_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_HUMID_SENSOR_VAL)
	
#define BT_UUID_PRESSURE_SENSOR_VAL 0x3BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_PRESSURE_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_PRESSURE_SENSOR_VAL)

#define BT_UUID_BATTERY_VAL 0x4BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_BATTERY \
	BT_UUID_DECLARE_16(BT_UUID_BATTERY_VAL)

#define BT_UUID_WRITE_VAL 0x5BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_WRITE \
	BT_UUID_DECLARE_16(BT_UUID_WRITE_VAL)


uint8_t sens_val[2] = {0xFF,0xFF};
uint8_t humid_val[2] = {0xFF,0xFF};
int32_t pres_val[2] = {0xFF,0xFF};
uint8_t bat_val[1] = {0xFF};
uint8_t delay = 20;

static bool active;

uint8_t indicating;

static struct bt_gatt_indicate_params ind_params;

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	LOG_INF("Indication %s\n", err != 0U ? "fail" : "success");
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	LOG_INF("Indication complete\n");
	indicating = 0U;
}


static void sens_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_INF("SENS EY");
	active = (value == BT_GATT_CCC_NOTIFY);
}
static void humid_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_INF("SENS EY");
	active = (value == BT_GATT_CCC_NOTIFY);
}
static void pres_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_INF("SENS EY");
	active = (value == BT_GATT_CCC_NOTIFY);
}
static void bat_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_INF("SENS EY");
	active = (value == BT_GATT_CCC_NOTIFY);
}
static void write_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_INF("SENS EY");
	active = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t write_conf(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset,
			uint8_t flags)
{
	LOG_INF("WRITE_CT, %i", len);
	delay = ((uint8_t *)buf)[0];

	LOG_INF("%x", delay);

	// if (offset + len > sizeof(ct)) {
	// 	LOG_INF("AYO");
	// 	return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	// }

	// memcpy(value + offset, buf, len);
	// ct_update = 1U;
	LOG_INF("WRITE LEN YO");

	return len;
}

BT_GATT_SERVICE_DEFINE(sensor_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BEE),
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMP_SENSOR,
					BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
					BT_GATT_PERM_READ_ENCRYPT,
					NULL, NULL, &sens_val),
	BT_GATT_CCC(sens_ccc,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMID_SENSOR,
					BT_GATT_CHRC_NOTIFY,
			       	BT_GATT_PERM_READ_ENCRYPT,
			       	NULL, NULL, &humid_val),
	BT_GATT_CCC(humid_ccc,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_PRESSURE_SENSOR,
					BT_GATT_CHRC_NOTIFY,
			       	BT_GATT_PERM_READ_ENCRYPT,
			       	NULL, NULL, &pres_val),
	BT_GATT_CCC(pres_ccc,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_BATTERY,
					BT_GATT_CHRC_NOTIFY,
			       	BT_GATT_PERM_READ_ENCRYPT,
			       	NULL, NULL, &bat_val),
	BT_GATT_CCC(bat_ccc,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_WRITE,
					BT_GATT_CHRC_READ,
			       	BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
			       	NULL, write_conf, NULL),
	BT_GATT_CCC(write_ccc,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_TEMP_SENSOR_VAL),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected %s", log_strdup(addr));

	current_conn = bt_conn_ref(conn);

	// dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr), reason);

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
		// dk_set_led_off(CON_STATUS_LED);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected    = connected,
	.disconnected = disconnected,
};

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

void main(void)
{
	int blink_status = 0;
	int err = 0;

	// err = dk_leds_init();

	// // dk_set_led_on(DK_LED2);
	// dk_set_leds_state(DK_ALL_LEDS_MSK, 0);

	// dk_set_leds_state(DK_ALL_LEDS_MSK, 0);

	LOG_INF("main()");
	// dk_set_led_on(DK_LED1);

	// // dk_set_leds_state(DK_ALL_LEDS_MSK, 0);
	bt_gatt_cb_register(&gatt_callbacks);
	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);
	
	// dk_set_leds_state(DK_ALL_LEDS_MSK, 0);

	err = bt_enable(NULL);
	if (err) {
		LOG_INF("Darn");
	}
	
	// dk_set_leds_state(DK_ALL_LEDS_MSK, 0);

	// err = bt_gatt_service_register(&sensor_svc);
	// if(err){
	// 	LOG_INF("Shoot");
	// }

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	int rc = battery_measure_enable(true);

	if (rc != 0) {
		LOG_INF("Failed initialize battery measurement: %d\n", rc);
	}
	
	// dk_set_leds_state(DK_ALL_LEDS_MSK, 0);
	
	k_sem_give(&ble_init_ok);

	for (;;) {
		// dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}

void ble_write_thread(void)
{
	/* Don't go any further until BLE is initialized */
	k_sem_take(&ble_init_ok, K_FOREVER);

	int err;

	for (;;) {
		const struct device *hts = device_get_binding("HTS221");
		err = sensor_sample_fetch(hts);
		LOG_INF("Sensor sample fetch %i", err);
		struct sensor_value humid;
		err = sensor_channel_get(hts, SENSOR_CHAN_HUMIDITY, &humid);
		LOG_INF("Sensor sample get %i", err);
		LOG_INF("HTS221: Humidity: %f %\n",
		       sensor_value_to_double(&humid));
		// err = hts221_sample_fetch(dev, SENSOR_CHAN_AMBIENT_TEMP);

		struct sensor_value temp;
		err = sensor_channel_get(hts, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		LOG_INF("Sensor sample get %i", err);
		LOG_INF("%i,%i", temp.val1, temp.val2);
		LOG_INF("HTS221: Temperature: %f C\n",
		       sensor_value_to_double(&temp));
		// err = hts221_sample_fetch(dev, SENSOR_CHAN_AMBIENT_TEMP);

		const struct device *lps = device_get_binding("LPS22HB");
		err = sensor_sample_fetch(lps);
		LOG_INF("Sensor sample fetch %i", err);
		struct sensor_value pres;
		// Pressure is read out in kPa
		err = sensor_channel_get(lps, SENSOR_CHAN_PRESS, &pres);
		LOG_INF("Sensor sample get %i", err);
		// LOG_INF("Air pressure: %x,%x", pres.val1, pres.val2);
		LOG_INF("LPS22HB: Air Pressure: %f kPa\n",
		       sensor_value_to_double(&pres));

		int batt_mV = battery_sample();

		if (batt_mV < 0) {
			LOG_INF("Failed to read battery voltage: %d\n",
			       batt_mV);
		}
		unsigned int batt_pptt = battery_level_pptt(batt_mV, levels);

		// const struct device *adc = device_get_binding("ADC_0");
		// err = sensor_sample_fetch(adc);
		// LOG_INF("Sensor sample fetch %i", err);
		// struct sensor_value volt;
		// err = sensor_channel_get(adc, SENSOR_CHAN_PRESS, &volt);
		// LOG_INF("Sensor sample get %i", err);
		// LOG_INF("ADC: %i", volt)
		// int len = strlen(to_string(temp.val2));
		// LOG_INF("Length: %i, %i", len, temp.val2);
		// char buf[20];
		// uint8_t data[6];
		// data[0] = 
		// // int len = snprintf(buf, 50, "%i,%i and %i,%i, batt: %d",temp.val1, (uint8_t)((float)(temp.val2)/10000), pres.val1, (uint8_t)((float)(pres.val2)/10000), batt_mV);
		// int len = snprintf(buf, 20, "Batt: %i, %d", (uint8_t)((float)batt_pptt/100), batt_mV);
		// sprintf(buf, "%i,%i and %i,%i",temp.val1, (uint8_t)((float)(temp.val2)/10000), pres.val1, (uint8_t)((float)(pres.val2)/10000));
		// sprintf(buf, "%i,%i and %i,%i and %i",temp.val1, (uint8_t)((float)(temp.val2)/10000), pres.val1, (uint8_t)((float)(pres.val2)/10000), batt_mV);
		// sprintf(buf, "%i",len);

		if(current_conn){
			batt_mV = battery_sample();

			if (batt_mV < 0) {
				LOG_INF("Failed to read battery voltage: %d\n",
					batt_mV);
			}
			batt_pptt = battery_level_pptt(batt_mV, levels);
			LOG_INF("Battery: %i", batt_pptt);
			sens_val[0] = (uint8_t)temp.val1;
			sens_val[1] = (uint8_t)((float)(temp.val2)/10000);
			err = bt_gatt_notify(current_conn, &sensor_svc.attrs[1],
						 sens_val, sizeof(sens_val));
			// LOG_INF("Err: %i", err);
			// LOG_INF("Handle: %i", sensor_svc.attrs[1].handle);
			// LOG_INF("UUID: %i", sensor_svc.attrs[1].uuid->type);

			// ind_params.attr = &sensor_svc.attrs[1];
			// ind_params.func = indicate_cb;
			// ind_params.destroy = indicate_destroy;
			// ind_params.data = &indicating;
			// ind_params.len = sizeof(indicating);
			// err = bt_gatt_indicate(current_conn, &ind_params);
			// LOG_INF("Err: %i", err);
			k_sleep(K_SECONDS(1));

			// Why no work?
			humid_val[0] = (uint8_t)humid.val1;
			humid_val[1] = (uint8_t)((float)(humid.val2)/10000);
			err = bt_gatt_notify(current_conn, &sensor_svc.attrs[4],
						humid_val, sizeof(humid_val));
			LOG_INF("Err: %i", err);

			k_sleep(K_SECONDS(1));

			// Why no work?
			pres_val[0] = pres.val1;
			pres_val[1] = pres.val2;
			err = bt_gatt_notify(current_conn, &sensor_svc.attrs[7],
						pres_val, sizeof(pres_val));
			LOG_INF("Err: %i", err);

			bat_val[0] = (uint8_t *)(batt_pptt/100);
			LOG_INF("Battery: %i", bat_val[0]);
			err = bt_gatt_notify(current_conn, &sensor_svc.attrs[10],
						bat_val, sizeof(bat_val));
			LOG_INF("Err: %i", err);

			// ind_params.attr = sensor_svc.attrs;
			// ind_params.func = indicate_cb;
			// ind_params.destroy = indicate_destroy;
			// ind_params.data = &indicating;
			// ind_params.len = sizeof(indicating);
			// err = bt_gatt_indicate(current_conn, &ind_params);
			// LOG_INF("Err: %i", err);
		}
		k_sleep(K_SECONDS(delay));
	}
}

K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, NULL, NULL,
		NULL, PRIORITY, 0, 0);
