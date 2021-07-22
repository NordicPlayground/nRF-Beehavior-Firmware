/*
* Copyright (c) 2012-2014 Wind River Systems, Inc.
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#define LED_PIN 6
#define SS_PIN 11

#define TEST_STRING "Southic"

static const struct spi_config spi_cfg = {
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		     SPI_MODE_CPOL | SPI_MODE_CPHA,
	.frequency = 4000000,
	.slave = 0,
};

struct device * spi_dev;

static void spi_init(void)
{
	const char* const spiName = "SPI_1";
	spi_dev = device_get_binding(spiName);

	if (spi_dev == NULL) {
		printk("Could not get %s device\n", spiName);
		return;
	}
}

void spi_test_send(void)
{
	int err;
	static uint8_t tx_buffer[8] = TEST_STRING;
	static uint8_t rx_buffer[8];

	tx_buffer[0] = 0x3f;
	
	for (int i=0; i<sizeof(tx_buffer);i++)
	{
		//tx_buffer[i] = 0x61 + i;
		rx_buffer[i] = 0x00;
	}

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = sizeof(tx_buffer)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	struct spi_buf rx_buf = {
		.buf = rx_buffer,
		.len = sizeof(rx_buffer),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};
	
	err = spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	
	if (err) {
		printk("SPI error: %d\n", err);
	} else {
		/* Connect MISO to MOSI for loopback */
		printk("TX sent: ");
		for (uint8_t i=0; i<6; i++)
		{
			printk("%x", tx_buffer[i]);
		}
		printk("\r\nRX recv: ");
		for (uint8_t i=0; i<6; i++)
		{
			printk("%c", rx_buffer[i]);
		}
		printk("\r\n");
		//tx_buffer[0]++;
	}	
}

void main(void)
{
	int ret;
	bool my_bool = false;

	struct device * dev = device_get_binding("GPIO_0");
	if (!dev) {
		printk("Device not found\n");
		return;
	}
	ret = gpio_pin_configure(dev, LED_PIN, GPIO_OUTPUT_ACTIVE);
	if (ret < 0)
	{
		printk("failed %d", ret);
	}
	ret = gpio_pin_configure(dev, SS_PIN, GPIO_OUTPUT_ACTIVE);
	if (ret <0)
	{
		printk("failed %d", ret);
	}
	printk("SPIM Example\n");
	
	spi_init();
	

	while (1) {
		gpio_pin_set(dev, LED_PIN, my_bool);
		
		my_bool = !my_bool;
		printk("\nmy_bool %d\r\n", my_bool);
		gpio_pin_set(dev, SS_PIN, false);
		spi_test_send();
		gpio_pin_set(dev, SS_PIN, true);
		k_sleep(K_MSEC(1000));
	}
}