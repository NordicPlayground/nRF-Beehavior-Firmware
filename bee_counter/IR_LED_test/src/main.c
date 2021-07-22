/* 
Dank code for the dank people @ nordicsemi
*/

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <device.h>

#define powerGate1 5
#define powerGate2 26

struct device *dev;

// 
void main(void) {
	printk("Begin program");
	struct device * dev = device_get_binding("GPIO_0");
  	if (!dev) {
		printk("Device not found\n");
		return;
	}
	printk("Begin switch test.");
  	gpio_pin_configure(dev, powerGate1, GPIO_OUTPUT_INACTIVE);
  	gpio_pin_configure(dev, powerGate2, GPIO_OUTPUT_INACTIVE);
	while(true){
		printk("funker dette");
		gpio_pin_set(dev, powerGate1, 1);   // turn the LED on (HIGH is the voltage level)
  		gpio_pin_set(dev, powerGate2, 1);
  		k_msleep(1000);                       // wait for a second
  		gpio_pin_set(dev, powerGate1, 0);    // turn the LED off by making the voltage LOW
 		gpio_pin_set(dev, powerGate2, 0);
  		k_msleep(1000);                       // wait for a second
	}
}
