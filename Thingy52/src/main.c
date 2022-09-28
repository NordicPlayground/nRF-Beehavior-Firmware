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
#include <sys/reboot.h>

#include <device.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>

#include <settings/settings.h>

#include <stdio.h>

#include <logging/log.h>

#include <drivers/sensor.h>
#include "battery.h"

#include <drivers/gpio.h>

// GPIO-0 Pins to control power supply to different sensors.
#define VDD_PWR_CTRL_GPIO_PIN 30
#define VDD_SPK_CTRL_GPIO_PIN 29
// GPIO-P0 (sx1509b) Pins to control power supply to different sensors.
#define BAT_MEAS_ENABLE 	  4
#define VDD_MPU_CTRL_GPIO_PIN 8
#define VDD_MIC_CTRL_GPIO_PIN 9
#define VDD_CSS_CTRL_GPIO_PIN 10

#define MODULE main
LOG_MODULE_REGISTER(MODULE, CONFIG_LOG_DEFAULT_LEVEL);

#define WRITE_THREAD_STACKSIZE 1024
#define WRITE_THREAD_PRIORITY 7

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

static K_SEM_DEFINE(sem_connected, 0, 1);

//Used for conversion from voltage to percent
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

// Struct to hold information about current connection.
// Used when sending data, and for checking if the Thingy:52 
// is connected to anyone.
static struct bt_conn *current_conn;

// UUID's the nRF53-module uses to subscribe to the characteristics. 
// These must match on both sides.

// UUID for the primary service. 
#define BT_UUID_BEE_VAL 0x0BEE
#define BT_UUID_BEE \
	BT_UUID_DECLARE_16(BT_UUID_BEE_VAL)

// UUID for notifying new temperature data.
#define BT_UUID_TEMP_SENSOR_VAL 0x1BEE
#define BT_UUID_TEMP_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_TEMP_SENSOR_VAL)

// UUID for notifying new humidity data.
#define BT_UUID_HUMID_SENSOR_VAL 0x2BEE
#define BT_UUID_HUMID_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_HUMID_SENSOR_VAL)

// UUID for notifying new pressure data.
#define BT_UUID_PRESSURE_SENSOR_VAL 0x3BEE
#define BT_UUID_PRESSURE_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_PRESSURE_SENSOR_VAL)

// UUID for notifying new battery level.
#define BT_UUID_BATTERY_VAL 0x4BEE
#define BT_UUID_BATTERY \
	BT_UUID_DECLARE_16(BT_UUID_BATTERY_VAL)

// UUID for the nRF53-module to send new sampling delay.
#define BT_UUID_WRITE_VAL 0x5BEE
#define BT_UUID_WRITE \
	BT_UUID_DECLARE_16(BT_UUID_WRITE_VAL)

//Temperature value, first byte is an integer, second byte is an integer representing the decimal value.
// [23, 25] = 23.25 degrees Celsius
uint8_t temp_val[2] = {0xFF,0xFF};

//Humidity value, first byte is an integer, second byte is an integer representing the decimal value.
// [85, 25] = 85.25 percent air humidity
uint8_t humid_val[2] = {0xFF,0xFF};

//Note: does not match with nRF5340 side. (But it does work)
int32_t pres_val[2] = {0xFF,0xFF};

//Battery value, integer value represnting battery percentage.
uint8_t bat_val[1] = {0xFF};

//Delay in seconds, should be increased to uint16_t if we want to increase delay. Current max is 255 seconds.
uint8_t delay = 60;

//This isn't used for anything, it was in the example I was heavily inspired by, and I have not dared to remove it.
static bool active;

// CCC attribute changed callbacks
static void temp_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_DBG("Temperature CCC");
	active = (value == BT_GATT_CCC_NOTIFY);
}
static void humid_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_DBG("Humidity CCC");
	active = (value == BT_GATT_CCC_NOTIFY);
}
static void pres_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_DBG("Pressure CCC");
	active = (value == BT_GATT_CCC_NOTIFY);
}
static void bat_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_DBG("Battery CCC");
	active = (value == BT_GATT_CCC_NOTIFY);
}
static void write_ccc(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_DBG("Write CCC");
	active = (value == BT_GATT_CCC_NOTIFY);
}

// Callback function for when the nRF53/central tries to read from the battery characteristic.
static ssize_t read_battery(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	LOG_DBG("The central has requested the battery level, sending value of: %i %%", bat_val[0]);
	// Send the battery value to the central.
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &bat_val, 1);
}

//Write callback of the BT_UUID_WRITE service. Triggered when the central tries to write to the Thingy:52.
static ssize_t write_conf(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset,
			uint8_t flags)
{
	LOG_DBG("Write message with length, %i", len);
	// Set new delay received from the central
	delay = ((uint8_t *)buf)[0];

	LOG_INF("New delay: %i", delay);
	
	return len;
}

// Macro to create a service. The primary service UUID is BT_UUID_BEE, this is used in bt_gatt_dm_start() on the nRF53. 
// It then uses the unique UUID of each characteristic to subscribe to the characteristics one by one.
// The UUID for any CCCD is always the standard 16-bit UUIDCCCD (0x2902).
// The UUID is the standard 16-bit UUID for a characteristic declaration, UUIDcharacteristic (0x2803).
// When notifying a characteristic you want to use the GATT_CHARACTERISTIC ID (2,5,8,11)
BT_GATT_SERVICE_DEFINE(sensor_service, //0
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BEE), //1
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMP_SENSOR, //2 
					BT_GATT_CHRC_NOTIFY,
					BT_GATT_PERM_NONE,
					NULL, NULL, &temp_val),
	BT_GATT_CCC(temp_ccc, //3 (2902 CCCD), 4 (2803)
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMID_SENSOR, //5
					BT_GATT_CHRC_NOTIFY,
			       	BT_GATT_PERM_NONE,
			       	NULL, NULL, &humid_val),
	BT_GATT_CCC(humid_ccc, //6 (2902 CCCD), 7 (2803)
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_PRESSURE_SENSOR, //8
					BT_GATT_CHRC_NOTIFY,
			       	BT_GATT_PERM_NONE,
			       	NULL, NULL, &pres_val),
	BT_GATT_CCC(pres_ccc, //9 (2902 CCCD), 10 (2803)
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_BATTERY, //11
					BT_GATT_CHRC_NOTIFY,
			       	BT_GATT_PERM_READ,
			       	read_battery, NULL, &bat_val),
	BT_GATT_CCC(bat_ccc, //12, 13
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_WRITE, //14
					BT_GATT_CHRC_READ,
			       	BT_GATT_PERM_WRITE,
			       	NULL, write_conf, NULL),
	BT_GATT_CCC(write_ccc, //15, 16 (550c)
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

//Data that is advertised, can potentially be minimized to reduce data sent over the radio.
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_BEE_VAL),
};

//Data that is used in the scan response, can possibly be empty to reduce data sent over the radio.
static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_BEE_VAL),
};

// Connection parameters to decrease power consumption, the first two parameters are min and max interval,
// which is how often they ping eachother. The last parameter is timeout, which is how long it can go without 
// being pinged before it disconnects.
static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(320, 320, 0, 100);
	// Interval = N * 1.25 ms = 320 * 1.25 ms = 400 ms , timeout = N * 10 ms = 1 s


static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected %s", log_strdup(addr));

	// Save the connection so you can use it to notify when new data is available.
	current_conn = bt_conn_ref(conn);

	// Stop advertising, since you only want to connect to one nRF53.
	err = bt_le_adv_stop();
	if (err) {
		LOG_ERR("Advertising failed to stop (err %d)", err);
		return;
	}

	// Ask the connection to update the connection parameters to use less power.
	// Because of the Bluetooth standard, the Thingy will wait 5 seconds to send 
	// the new parameters since it is a peripheral.
	err = bt_conn_le_param_update(conn, conn_param);
	if (err) {
		LOG_ERR("Connection parameters update failed: %d",
				err);
	}
	
	// Tell the bluetooth write thread to start sampling the sensors.
	k_sem_give(&sem_connected);
}
 
static void adv_start(void)
{
	// Advertising params are used to tell the radio how often it should broadcast advertising packets, 
	// higher advertising intervals means less power consumption but makes the device "harder" to find.
	struct bt_le_adv_param *adv_param =
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE |
			BT_LE_ADV_OPT_ONE_TIME,
			BT_GAP_ADV_SLOW_INT_MIN, //1s
			BT_GAP_ADV_SLOW_INT_MAX, //1.2s
			NULL);
	int err;

	LOG_INF("Starting advertising");

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Failed to start advertiser (%d), rebooting system", err);
		// If the Thingy:52 fails to start advertise, it is pretty much useless.
		// Therefore we reboot it.
    	sys_reboot(SYS_REBOOT_COLD);
		return;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr), reason);

	// Clear the current_conn
	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
	// Start advertising for a new connection.
	adv_start();
}

static struct bt_conn_cb conn_callbacks = {
	.connected    = connected,
	.disconnected = disconnected,
};

void main(void)
{
	int err = 0;

	// Turn off power to unused peripherals to keep the power consumption low
	const struct device *gpio_dev;
	const struct device *gpiop0_dev;
	gpio_dev = device_get_binding("GPIO_0");	
	gpiop0_dev = device_get_binding("GPIO_P0");
	err = gpio_pin_configure(gpio_dev, VDD_SPK_CTRL_GPIO_PIN, GPIO_OUTPUT_LOW); 	
	err = gpio_pin_configure(gpiop0_dev, VDD_MPU_CTRL_GPIO_PIN, GPIO_OUTPUT_LOW); 	
	err = gpio_pin_configure(gpiop0_dev, VDD_MIC_CTRL_GPIO_PIN, GPIO_OUTPUT_LOW); 	
	err = gpio_pin_configure(gpiop0_dev, VDD_CSS_CTRL_GPIO_PIN, GPIO_OUTPUT_LOW); 

	LOG_DBG("main()");

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth unable to start");
    	sys_reboot(SYS_REBOOT_COLD);
	}
	
	LOG_DBG("Bluetooth enabled");

	bt_conn_cb_register(&conn_callbacks);
	
	LOG_DBG("Connection callbacks registered");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		LOG_DBG("Loading settings");
		settings_load();
	}

	//Start bluetooth advertising
	adv_start();

	// Setup and enable the ADC
	err = battery_measure_enable(true);

	if(err) {
		LOG_ERR("Failed initialize battery measurement: %d\n", err);
	}else{
		LOG_DBG("Battery measuring enabled");
	}

	//Sample the ADC
	int batt_mV = battery_sample();

	if (batt_mV < 0) {
		LOG_ERR("Failed to read battery voltage: %d\n",
			batt_mV);
	}
	// Convert the ADC data into part per thousand based on a approximation of the battery life
	unsigned int batt_pptt = battery_level_pptt(batt_mV, levels);

	//Converting from parts per ten thousand to percent
	bat_val[0] = (uint8_t)(batt_pptt/100);
}

/* Thread for sampling sensors and sending over bluetooth */
void ble_write_thread(void)
{
	/* Don't go any further until connection have been made */
	k_sem_take(&sem_connected, K_FOREVER);

	// Wait until the nRF53 have discovered all characteristics and new connection interval has been sent. 
	// Takes around 5 seconds. 
	k_sleep(K_SECONDS(6));

	int err;
	int batt_mV;
	unsigned int batt_pptt;

	const struct device *gpioP0_dev;
	gpioP0_dev = device_get_binding("GPIO_P0");

	for (;;) {
		//Only sample sensors if connected to nRF53
		if(current_conn){
			//Turn on Battery measurment
			err = gpio_pin_configure(gpioP0_dev, BAT_MEAS_ENABLE, GPIO_OUTPUT_HIGH);
			k_sleep(K_MSEC(100));

			batt_mV = battery_sample();

			if (batt_mV < 0) {
				LOG_ERR("Failed to read battery voltage: %d\n",
					batt_mV);
			}
			batt_pptt = battery_level_pptt(batt_mV, levels);
			LOG_INF("Percentage: %i", batt_pptt/100);

			//Only send battery percentage if different from last
			if(bat_val[0] != (uint8_t)(batt_pptt/100)){
				bat_val[0] = (uint8_t)(batt_pptt/100);
				LOG_INF("Battery: %i", bat_val[0]);
				err = bt_gatt_notify(current_conn, &sensor_service.attrs[11],
							bat_val, sizeof(bat_val));
				if(err){
					LOG_ERR("Gatt Notify error: %d on sending battery data", err);
				}
			}

			//Temperature and humidity sensor
			const struct device *hts = device_get_binding("HTS221");
			err = sensor_sample_fetch(hts);
			if(err){
				LOG_ERR("Failed to fetch hts sample: %i", err);
			}

			struct sensor_value humid;
			err = sensor_channel_get(hts, SENSOR_CHAN_HUMIDITY, &humid);
			if(err){
				LOG_ERR("Failed to get humidity sample: %i", err);
			}
			LOG_INF("HTS221: Humidity: %f %\n",
				sensor_value_to_double(&humid));

			struct sensor_value temp;
			err = sensor_channel_get(hts, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			if(err){
				LOG_ERR("Failed to get temperature sample: %i", err);
			}
			LOG_INF("HTS221: Temperature: %f C\n",
				sensor_value_to_double(&temp));
			
			
			//Pressure sensor
			const struct device *lps = device_get_binding("LPS22HB");
			err = sensor_sample_fetch(lps);
			if(err){
				LOG_ERR("Failed to fetch lps sample: %i", err);
			}

			struct sensor_value pres;
			// Pressure is read out in kPa
			err = sensor_channel_get(lps, SENSOR_CHAN_PRESS, &pres);
			if(err){
				LOG_ERR("Failed to get pressure sample: %i", err);
			}
			LOG_INF("LPS22HB: Air Pressure: %f kPa\n",
				sensor_value_to_double(&pres));

			
			temp_val[0] = (uint8_t)temp.val1;
			//Convert from one-millionth parts to one-hundred parts.
			temp_val[1] = (uint8_t)((float)(temp.val2)/10000);
			err = bt_gatt_notify(current_conn, &sensor_service.attrs[2],
						 &temp_val, sizeof(temp_val));
			if(err){
				LOG_ERR("Gatt Notify error: %d on sending temparture data", err);
			}
			
			humid_val[0] = (uint8_t)humid.val1;
			//Convert from one-millionth parts to one-hundred parts.
			humid_val[1] = (uint8_t)((float)(humid.val2)/10000);
			err = bt_gatt_notify(current_conn, &sensor_service.attrs[5],
						&humid_val, sizeof(humid_val));
			if(err){
				LOG_ERR("Gatt Notify error: %d on sending humidity data", err);
			}

			pres_val[0] = pres.val1;
			pres_val[1] = pres.val2;
			err = bt_gatt_notify(current_conn, &sensor_service.attrs[8],
						&pres_val, sizeof(pres_val));
			if(err){
				LOG_ERR("Gatt Notify error: %d on sending pressure data", err);
			}
			//Turn off the battery measure enable transistor between samples
			err = gpio_pin_configure(gpioP0_dev, BAT_MEAS_ENABLE, GPIO_OUTPUT_LOW);
			k_sleep(K_SECONDS(delay));
		}else{
			//Should set a deadline to prevent deadlock
			k_sem_take(&sem_connected, K_FOREVER);
		}
	}
}

// Thread for sampling sensors and sending the data over Bluetooth.
K_THREAD_DEFINE(ble_write_thread_id, WRITE_THREAD_STACKSIZE, ble_write_thread, NULL, NULL,
		NULL, WRITE_THREAD_PRIORITY, 0, 0);
