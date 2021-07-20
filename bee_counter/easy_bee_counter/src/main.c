/* 
Dank code for the dank people @ nordicsemi
*/

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <timing/timing.h>
#include <device.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios) //LED for testing

#define LATCH 3 //A5 on the DK, used to pulse and load the shift registers
#define SS_PIN //Slave-select pin

#define powerGate1 5 //10 on the DK
#define powerGate2 26 //11 on the DK

#define numberOfGates 24
#define startGate 0
#define endGate 24
#define debeebounce 30
#define outputDelay 15000

unsigned long lastOutput = 0;

//To do: Clean up this code so it looks more aesthetic...

bool inSensorReading[numberOfGates];
bool outSensorReading[numberOfGates];

bool lastInSensorReading[numberOfGates];
bool lastOutSensorReading[numberOfGates];

bool checkStateIn[numberOfGates];
bool checkStateOut[numberOfGates];

int inCount[numberOfGates];
int outCount[numberOfGates];

unsigned long startInReadingTime[numberOfGates];
unsigned long startOutReadingTime[numberOfGates];

unsigned long inSensorTime[numberOfGates];
unsigned long outSensorTime[numberOfGates];
 
unsigned long lastInFinishedTime[numberOfGates];
unsigned long lastOutFinishedTime[numberOfGates];
  
unsigned long inReadingTimeHigh[numberOfGates];
unsigned long outReadingTimeHigh[numberOfGates];

unsigned long lastInTime[numberOfGates];
unsigned long lastOutTime[numberOfGates];

unsigned long lastInReadingTimeHigh[numberOfGates];
unsigned long lastOutReadingTimeHigh[numberOfGates];

int totalTimeTravelGoingOut[numberOfGates];
int totalTimeTravelGoingIn[numberOfGates];

int firstTestInVariable[numberOfGates];

int firstTestOutVariable[numberOfGates];

int inTotal;
int outTotal;

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

int n = 0;

static const struct spi_config spi_cfg = {
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		     SPI_MODE_CPOL, //Setting SPI mode 2 (hopefully)
	.frequency = 4000000,
	.slave = 0,
};

struct device * spi_dev;
struct device * led_dev;
struct device * dev;

static void peri_init(void)
{
	const char* const spiName = "SPI_1";
	spi_dev = device_get_binding(spiName);
	struct device * led_dev = device_get_binding("GPIO_1");
	struct device * dev = device_get_binding("GPIO_0");
	if (spi_dev == NULL) {
		printk("Could not get %s device\n", spiName);
		return;
	}
	else if (!led_dev){
		printk("Device not found\n");
		return;
	}
	else if (!dev) {
		printk("Device not found\n");
		return;
	}
}

void setup(){
	gpio_pin_configure(dev, LATCH, GPIO_OUTPUT_ACTIVE); 
	gpio_pin_configure(spi_dev, powerGate1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(spi_dev, powerGate2, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(led_dev, LED0, GPIO_OUTPUT);
}





void main(void){
	while(true){
	timing_init();
    timing_start();
	peri_init();

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

	gpio_pin_set(spi_dev, powerGate1, 1);
	gpio_pin_set(spi_dev, powerGate2, 1);
	k_usleep(75); //Setting first 24 gates takes ~ 15us, while the gates closer to the end takes ~40-75us. This needs to be verified.
	 
	gpio_pin_set(dev, LATCH, 0);
	k_usleep(3);
	gpio_pin_set(dev, LATCH, 1);

	k_usleep(3);

	gpio_pin_set(spi_dev, powerGate1, 0);
	gpio_pin_set(spi_dev, powerGate2, 0);

	err = spi_transceive(spi_dev, &spi_cfg, NULL, &rx);
	if (err) {
		printk("SPI error: %d\n", err);
	} else {
		//Code to save the data from reading??
		switchBank1 = err;
		switchBank2 = err;
		switchBank3 = err;
		switchBank4 = err;
		switchBank5 = err;
		switchBank6 = err;
	}

	if (switchBank1 != oldSwitchBank1 || switchBank2 != oldSwitchBank2 || switchBank3 != oldSwitchBank3 || switchBank4 != oldSwitchBank4 || switchBank5 != oldSwitchBank5 || switchBank6 != oldSwitchBank6)
	{
		//Converting bytes to get values
		int gate = 0;
		for (int i = 0; i < 8; i++)
		{
			if ((switchBank1 >> i) & 1)
				outSensorReading[gate] = true;
			else outSensorReading[gate] = false;
			i++;
			if ((switchBank1 >> i) & 1)
				inSensorReading[gate] = true;
			else inSensorReading[gate] = false;
			gate++;
		}
		for(int i = 0; i < 8; i++)
    	{
     		if((switchBank2 >> i) & 1)
        		outSensorReading[gate] = true;
      		else outSensorReading[gate] = false;
     		i++;
      		if((switchBank2 >> i) & 1)
          		inSensorReading[gate] = true;
      		else inSensorReading[gate] = false;      
      		gate++;  
    	}
    	for(int i = 0; i < 8; i++)
    	{
    		if((switchBank3 >> i) & 1)
        		outSensorReading[gate] = true;
    		else outSensorReading[gate] = false;
    		i++;
      		if((switchBank3 >> i) & 1)
        		inSensorReading[gate] = true;
      		else inSensorReading[gate] = false;       
      		gate++;  
    	}
    	for(int i = 0; i < 8; i++)
    	{
    		if((switchBank4 >> i) & 1)
        		outSensorReading[gate] = true;
      		else outSensorReading[gate] = false;
      		i++;
      		if((switchBank4 >> i) & 1)
          		inSensorReading[gate] = true;
      		else inSensorReading[gate] = false;       
      		gate++;  
    	}
    	for(int i = 0; i < 8; i++)
    	{
      		if((switchBank5 >> i) & 1)
          		outSensorReading[gate] = true;
      		else outSensorReading[gate] = false;
      		i++;
      		if((switchBank5 >> i) & 1)
          		inSensorReading[gate] = true;
      		else inSensorReading[gate] = false;       
      		gate++;  
    	}
    	for(int i = 0; i < 8; i++)
    	{
      		if((switchBank6 >> i) & 1)
          		outSensorReading[gate] = true;
      		else outSensorReading[gate] = false;
      		i++;
      		if((switchBank6 >> i) & 1)
          		inSensorReading[gate] = true;
      		else inSensorReading[gate] = false;      
      		gate++;  
    	}  
    oldSwitchBank1 = switchBank1;
    oldSwitchBank2 = switchBank2;
    oldSwitchBank3 = switchBank3;
    oldSwitchBank4 = switchBank4;
    oldSwitchBank5 = switchBank5;
    oldSwitchBank6 = switchBank6;
  	}

	for (int i = startGate; i < endGate; i++) 
	{ 
    if(inSensorReading[i] == true || outSensorReading[i] == true) 
    {
     	gpio_pin_set(led_dev, LED0, 1);
      	break;
		}
    	gpio_pin_set(led_dev, LED0, 0);
  	}

  
  	for (int i = startGate; i < endGate; i++) 
  	{ 
    	if(inSensorReading[i] != lastInSensorReading[i])  //change of state on IN sensor
    	{ 
      		checkStateIn[i] = 0;
      		lastInSensorReading[i] = inSensorReading[i];
      		inSensorTime[i] = timing_counter_get();
      		//printk(i);
      		//printk(", ");
      		//printkln(inSensorReading[i]);
    	} 
    	if(outSensorReading[i] != lastOutSensorReading[i])  //change of state on OUT sensor
    	{ 
      		checkStateOut[i] = 0;
      		lastOutSensorReading[i] = outSensorReading[i];
      		outSensorTime[i] = current_time;
     		 //printk(i);
     		 //printk(", ");
     		 //printkln(outSensorReading[i]);
   		}       
    	if(current_time - inSensorTime[i] > debeebounce && checkStateIn[i] == 0)  //debounce IN sensor
    	{
      		checkStateIn[i] = 1; //passed debounce         
      		//printk(i);
      		//printk(", IN sensor - high_or_low: ");
      		//printkln(inSensorReading[i]);
      		if(inSensorReading[i] == 1) //a bee just entered the sensor
      	{
       		startInReadingTime[i] = current_time;
        	//printk(i);
        	//printk(", I ,");
        	//printkln(current_time);
      	}
      		if(inSensorReading[i] == 0)  //a bee just exits the sensor; that is, it was 1, now it is LOW (empty)
      	{  
        	lastInFinishedTime[i] = current_time;            
        	inReadingTimeHigh[i] = current_time - startInReadingTime[i]; //this variable is how long the bee was present for
        	printk(i);
        	printk(", IT ,");
        	printk(inReadingTimeHigh[i]);
        	printk(", ");    
        	if(outReadingTimeHigh[i] < 650 && inReadingTimeHigh[i] < 650){ //should be less than 650ms
          if(current_time - lastOutFinishedTime[i] < 200){ //the sensors are pretty cose together so the time it takes to trigger on and then the other should be small.. ~200ms
            inTotal++;
            printk(current_time);
            printk(",");
            printk(1); //Serial.printkln igjen
          }else{
            printk(current_time); //Serial.println må kanskje endres?
          }
        }else{
          printk(current_time); //Serial.println må kanskje endres?
        }
      }           
    }
    if(current_time - outSensorTime[i] > debeebounce && checkStateOut[i] == 0)  //debounce OUT sensor
    {
      checkStateOut[i] = 1; //passed debounce         
      //printk(i);
      //printk(", IN sensor - high_or_low: ");
      //printkln(outSensorReading[i]);
      if(outSensorReading[i] == 1) //a bee just entered the sensor
      {
        startOutReadingTime[i] = current_time;
        //printk(i);
        //printk(", O ,");
        //printkln(current_time);
      }
      if(outSensorReading[i] == 0)  //a bee just exits the sensor; that is, it was HIGH, now it is LOW (empty)
      {  
        lastOutFinishedTime[i] = current_time;            
        outReadingTimeHigh[i] = current_time - startOutReadingTime[i]; //this variable is how long the bee was present for
        printk(i);
        printk(", OT ,");
        printk(outReadingTimeHigh[i]);
        printk(", ");        
        if(outReadingTimeHigh[i] < 600 && inReadingTimeHigh[i] < 600){ //should be less than 600ms
          if(current_time - lastInFinishedTime[i] < 200){ //the sensors are pretty cose together so this time should be small
            outTotal++;
            printk(current_time);
            printk(",");
            printk(1); //Serial.println må kanskje endres?
          }else{
            printk(current_time); //Serial.println må kanskje endres?
          }
        }else{
          printk(current_time); //Serial.println må kanskje endres?
        }
      }          
    }        
  }    

  k_usleep(15);   // debounce

  if (current_time - lastOutput > outputDelay) 
    {
    //printkln("sending data");
    sendData(); 
    lastOutput = current_time; 
    inTotal = 0;
    outTotal = 0; 
  }
}
}
      

void sendData() {
  printk("T, ");
  printk(outTotal);
  printk(", ");
  printk(inTotal); //Serial.println må kanskje endres?
// over wifi or ethernet or serial
//Innsett bluetooth til nrf91 her
}

		

