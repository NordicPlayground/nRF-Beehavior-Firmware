/* 
Dank code for the dank people @ nordicsemi
*/

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <timing/timing.h>
#include <device.h>
#include <stdio.h>

#define LED0 6
#define LATCH 3 //A5 on the itsybitsy, used to pulse and load the shift registers

#define powerGate1 5 //10 on the itsybitsy
#define powerGate2 26 //11 on the itsybitsy

#define numberOfGates 24
#define startGate 0
#define endGate 24
#define debeebounce 30
#define outputDelay 15000

unsigned long lastOutput = 0;
timing_t start_time, end_time;

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
	.frequency = 3000000,
	.slave = 0,
};

const struct device * spi_dev;
const struct device * dev;

unsigned long current_time(timing_t *start_time, timing_t *end_time){ 
    *end_time = timing_counter_get(); 
	unsigned long long current_time_ns = timing_cycles_to_ns(timing_cycles_get(start_time, end_time)) * 0.000001; //Converts to ms
	return current_time_ns; 
}


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
		start_time = timing_counter_get();
		static uint8_t rx_buffer[6]; //6 shift regisers requires an array with length 6

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
		gpio_pin_configure(dev, LED0, GPIO_OUTPUT); //LED pin is 6, one could use aliases here
		
		int err;	

		gpio_pin_set(dev, powerGate1, 1);
		gpio_pin_set(dev, powerGate2, 1);

		k_busy_wait(75); //Setting first 24 gates takes ~ 15us, while the gates closer to the end takes ~40-75us. This needs to be verified.

		gpio_pin_set(dev, LATCH, 0);
		k_busy_wait(3);
		gpio_pin_set(dev, LATCH, 1);

		k_busy_wait(3);

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
		  	gpio_pin_set(dev, LED0, 1);
		   	break;
			}
		 	gpio_pin_set(dev, LED0, 0);
			}

		
			for (int i = startGate; i < endGate; i++) 
			{ 
		 	if(inSensorReading[i] != lastInSensorReading[i])  //change of state on IN sensor
		 	{ 
		   		checkStateIn[i] = 0;
		   		lastInSensorReading[i] = inSensorReading[i];
		   		inSensorTime[i] = current_time(&start_time, &end_time);
		 	} 
		 	if(outSensorReading[i] != lastOutSensorReading[i])  //change of state on OUT sensor
		 	{ 
		   		checkStateOut[i] = 0;
		   		lastOutSensorReading[i] = outSensorReading[i];
		   		outSensorTime[i] = current_time(&start_time, &end_time);
				}    

		 	if(((current_time(&start_time, &end_time) - inSensorTime[i]) > debeebounce) && (checkStateIn[i] == 0)){  //debounce IN sensor
		   		checkStateIn[i] = 1; //passed debounce      
		   		if(inSensorReading[i] == 1) //a bee just entered the sensor
		   	{
				startInReadingTime[i] = current_time(&start_time, &end_time);
		   	}
		   		if(inSensorReading[i] == 0)  //a bee just exits the sensor; that is, it was 1, now it is LOW (empty)
		   	{  
		     	lastInFinishedTime[i] = current_time(&start_time, &end_time);            
		     	inReadingTimeHigh[i] = current_time(&start_time, &end_time) - startInReadingTime[i]; //this variable is how long the bee was present for
		     	printk("%i", i);
		     	printk(", IT ,");
		     	printk("%ld", inReadingTimeHigh[i]);
		     	printk(", ");    
		     	if(outReadingTimeHigh[i] < 650 && inReadingTimeHigh[i] < 650000000){ //should be less than 650ms
		       if((current_time(&start_time, &end_time) - lastOutFinishedTime[i]) < 200){ //the sensors are pretty cose together so the time it takes to trigger on and then the other should be small.. ~200ms
		         inTotal++;
				unsigned long print_time = current_time(&start_time, &end_time);
		         printk("%ld", print_time);
		         printk(",");
		         printk("1\n");
		       }else{
				 	unsigned long print_time = current_time(&start_time, &end_time);
		         	printk("%ld\n", print_time); 
		       }
		     }else{
				unsigned long print_time = current_time(&start_time, &end_time);
		      	printk("%ld\n", print_time); 
		     }
		   }           
		 }
		 if((current_time(&start_time, &end_time) - outSensorTime[i]) > debeebounce && checkStateOut[i] == 0)  //debounce OUT sensor
		 {
		   checkStateOut[i] = 1; //passed debounce         
		   if(outSensorReading[i] == 1) //a bee just entered the sensor
		   {
		     startOutReadingTime[i] = current_time(&start_time, &end_time);
		   }
		   if(outSensorReading[i] == 0)  //a bee just exits the sensor; that is, it was HIGH, now it is LOW (empty)
		   {  
		     lastOutFinishedTime[i] = current_time(&start_time, &end_time);
		     outReadingTimeHigh[i] = (current_time(&start_time, &end_time) - startOutReadingTime[i]); //this variable is how long the bee was present for
		     printk("%i", i);
		     printk(", OT ,");
		     printk("%ld", outReadingTimeHigh[i]);
		     printk(", ");        
		     if(outReadingTimeHigh[i] < 600 && inReadingTimeHigh[i] < 600){ //should be less than 600ms
		       if((current_time(&start_time, &end_time) - lastInFinishedTime[i]) < 200){ //the sensors are pretty cose together so this time should be small
		         	outTotal++;
		         	unsigned long printable_time = current_time(&start_time, &end_time);
		     		printk("%ld\n", printable_time);
		         	printk(",");
		        	printk("1\n"); 
		       }
			   else{
				unsigned long printable_time = current_time(&start_time, &end_time);
		         printk("%ld\n", printable_time); 
		       		}
		     	}
				else{
			  	unsigned long printable_time = current_time(&start_time, &end_time);
		     	printk("%ld\n", printable_time); 
		     }
		   }          
		 }        


		k_busy_wait(15);   // debounce
		if ((current_time(&start_time, &end_time) - lastOutput) > outputDelay) 
		{
		printk("sending data\n");
		sendData(); 
		lastOutput = current_time(&start_time, &end_time); 
		inTotal = 0;
		outTotal = 0; 
 	 	}
	}
	}
}
      

void sendData() {
  printk("Total out: ");
  printk("%i", outTotal);
  printk("\n");
  printk("Total in: %i\n", inTotal); 
//Innsett bluetooth til nrf53 her
}

