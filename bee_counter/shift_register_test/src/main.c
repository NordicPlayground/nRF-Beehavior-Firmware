/* 
Dank code for the dank people @ nordicsemi
*/

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <device.h>
#include <drivers/spi.h>

#define powerGate1 5
#define powerGate2 26
#define LATCH 3

const struct device * spi_dev;
const struct device * dev;

static uint8_t switchBank1;
static uint8_t switchBank2;
static uint8_t switchBank3;
static uint8_t switchBank4;
static uint8_t switchBank5;
static uint8_t switchBank6;
static uint8_t oldSwitchBank1; 
static uint8_t oldSwitchBank2; 
static uint8_t oldSwitchBank3; 
static uint8_t oldSwitchBank4; 
static uint8_t oldSwitchBank5; 
static uint8_t oldSwitchBank6; 

static const struct spi_config spi_cfg = {
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		     SPI_MODE_CPOL, //Setting SPI mode 2 (hopefully)
	.frequency = 4000000,
	.slave = 0,
};


void main(void){
	const struct device *spi_dev = device_get_binding("SPI_1");
	const struct device * dev = device_get_binding("GPIO_0");
	if (spi_dev == NULL) {
		printk("SPI device not found\n");
		return;
	}
	else if (!dev) {
		printk("Device not found\n");
		return;
	}
	while(1){
		int err;
    	static uint8_t rx_buffer[6];
    
    	struct spi_buf rx_buf = {
    		.buf = rx_buffer,
    		.len = sizeof(rx_buffer),
    	};
    	const struct spi_buf_set rx = {
    		.buffers = &rx_buf,
    		.count = 1
    	};
    
    	gpio_pin_configure(dev, LATCH, GPIO_OUTPUT_ACTIVE);
      	gpio_pin_configure(dev, powerGate1, GPIO_OUTPUT_INACTIVE);
      	gpio_pin_configure(dev, powerGate2, GPIO_OUTPUT_INACTIVE);
		gpio_pin_configure(dev, 6, GPIO_OUTPUT_ACTIVE); //LED
    
    	gpio_pin_set(dev, powerGate1, 1);
    	gpio_pin_set(dev, powerGate2, 1);
    
    	k_usleep(75);
    
    	gpio_pin_set(dev, LATCH, 0);
    	k_usleep(3);
    	gpio_pin_set(dev, LATCH, 1);
    
    	k_usleep(3);
    
    	gpio_pin_set(dev, powerGate1, 0);
    	gpio_pin_set(dev, powerGate2, 0);
    
    	err = spi_read(spi_dev, &spi_cfg, &rx);
    	if (err)
		{
			printk("SPI error: %d\n", err);
		}
		else{
			switchBank1 = rx_buffer[0];
			switchBank2 = rx_buffer[1];
			switchBank3 = rx_buffer[2];
			switchBank4 = rx_buffer[3];
			switchBank5 = rx_buffer[4];
			switchBank6 = rx_buffer[5];
		}

    	unsigned char mask = 1;
    	int sensor = 1;
    
    	for (int i = 1; i <= 8; i++)
    	{
    		if ((switchBank1 & mask) != (oldSwitchBank1 & mask))
    		{
    			printk("Switch %i ", sensor);
    			printk((switchBank1 & mask) ? "ir triggered \n" : "none \n");
    		}
    	mask <<= 1;
    	sensor++;
    	} //End of each bit
    	mask = 1;
    	sensor = 9;
    	for (int i = 1; i <= 8; i++)
    	{
    		if ((switchBank2 & mask) != (oldSwitchBank2 & mask))
    		{
    			printk("Switch %i ", sensor);
    			printk((switchBank2 & mask) ? "ir triggered \n" : "none \n");
    		} //End of bit has changed
    	mask <<= 1;
    	sensor++;
    	}
    	mask = 1;
    	sensor = 17;
    	for (int i = 1; i <= 8; i++)
    	{
    		if ((switchBank3 & mask) != (oldSwitchBank3 & mask))
    		{
    			printk("Switch %i ", sensor);
    			printk((switchBank3 & mask) ? "ir triggered \n" : "none \n");
    		} //End of bit has changed
    	mask <<= 1;
    	sensor++;
    	}
    	mask = 1;
    	sensor = 25;
    	for (int i = 1; i <= 8; i++)
    	{
    		if ((switchBank4 & mask) != (oldSwitchBank4 & mask))
    		{
    			printk("Switch %i ", sensor);
    			printk((switchBank4 & mask) ? "ir triggered \n" : "none \n");
    		} //End of bit has changed
    	mask <<= 1;
    	sensor++;
    	}
    	mask = 1;
    	sensor = 33;
    	for (int i = 1; i <= 8; i++)
    	{
    		if ((switchBank5 & mask) != (oldSwitchBank5 & mask))
    		{
    			printk("Switch %i ", sensor);
    			printk((switchBank5 & mask) ? "ir triggered \n" : "none \n");
    		} //End of bit has changed
    	mask <<= 1;
    	sensor++;
    	}
    	mask = 1;
    	sensor = 41;
    	for (int i = 1; i <= 8; i++)
    	{
    		if ((switchBank6 & mask) != (oldSwitchBank6 & mask))
    		{
    			printk("Switch %i ", sensor);
    			printk((switchBank6 & mask) ? "ir triggered \n" : "none \n");
    		} //End of bit has changed
    	mask <<= 1;
    	sensor++;
    	}
    	oldSwitchBank1 = switchBank1;
    	oldSwitchBank2 = switchBank2;
    	oldSwitchBank3 = switchBank3;
    	oldSwitchBank4 = switchBank4;
    	oldSwitchBank5 = switchBank5;
    	oldSwitchBank6 = switchBank6;
    	k_msleep(10);
	}
}
