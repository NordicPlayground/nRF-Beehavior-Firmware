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

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>

#include <bluetooth/services/nus.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

static struct bt_conn *current_conn;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

#define BT_UUID_BEE_VAL \
	BT_UUID_128_ENCODE(0x6e400002, 0xb5a1, 0xf493, 0xe1a9, 0xe52e24dcca9e)

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_BEE_VAL),
};

#define LED0 6
#define LATCH 3 //A5 on the itsybitsy, used to pulse and load the shift registers

#define powerGate1 5 //10 on the itsybitsy
#define powerGate2 26 //11 on the itsybitsy

#define numberOfGates 24
#define startGate 0
#define endGate 24
#define debeebounce 30
#define outputDelay 10000

unsigned long lastOutput = 0;
unsigned long long current_time;

bool inSensorReading[numberOfGates];
bool outSensorReading[numberOfGates];

bool lastInSensorReading[numberOfGates];
bool lastOutSensorReading[numberOfGates];

bool checkStateIn[numberOfGates];
bool checkStateOut[numberOfGates];

static bool FLAG = 0;

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


/*
unsigned long lastInTime[numberOfGates];
unsigned long lastOutTime[numberOfGates];

unsigned long lastInReadingTimeHigh[numberOfGates];
unsigned long lastOutReadingTimeHigh[numberOfGates];

int totalTimeTravelGoingOut[numberOfGates];
int totalTimeTravelGoingIn[numberOfGates];

int firstTestInVariable[numberOfGates];

int firstTestOutVariable[numberOfGates];
*/

int inTotal;
int outTotal;

static uint8_t switchBank[6];
static uint8_t oldSwitchBank[6];

int n = 0;

static const struct spi_config spi_cfg = {
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		     SPI_MODE_CPOL, //Setting SPI mode 2 (hopefully)
	.frequency = 3000000,
	.slave = 0,
};

const struct device * spi_dev;
const struct device * dev;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed (err %u)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected %.*s\n", BT_ADDR_LE_STR_LEN, addr);

	current_conn = bt_conn_ref(conn);

	printk("Sending test data\n");

	// char msg[20];
	// uint8_t length = snprintf(msg, 20, "%i,%i\n", outTotal, inTotal);
	// bt_nus_send(current_conn, msg, length);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %.*s (reason %u)\n", BT_ADDR_LE_STR_LEN, addr, reason);

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected    = connected,
	.disconnected = disconnected,
};

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
			  uint16_t len)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN] = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

	printk("Received data from: %.*s\n", BT_ADDR_LE_STR_LEN, addr);

	printk("%.*s\n", len, data);

}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};

void sendData() {
    printk("Total out: ");
    printk("%i", outTotal);
    printk("\n");
    printk("Total in: %i\n", inTotal); 
    if(current_conn){
		uint16_t msg[2] = {outTotal, inTotal};
		bt_nus_send(current_conn, msg, sizeof(msg));
	}
}

void main(void){
	
	bt_conn_cb_register(&conn_callbacks);

	int err = bt_enable(NULL);
	if (err) {
		printk("BLE not enabled, %i\n", err);
	}

	printk("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_nus_init(&nus_cb);
	if (err) {
		printk("Failed to initialize UART service (err: %d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)", err);
	}

	//printk("Starting Nordic UART service example\n");

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
		current_time = k_uptime_get();
		static uint8_t rx_buffer[6]; //6 shift registers requires an array with length 6

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

				for (int i = 0; i < 6; i++){
					switchBank[i] = rx_buffer[i];
					if (switchBank[i] != oldSwitchBank[i]){
						FLAG = 1;
					}
				}
			}
		
		if (FLAG)
		{
			//Converting bytes to get values
			int gate = 0;
			for (int i = 0; i < 8; i++)
			{
				if ((switchBank[0] >> i) & 1)
					outSensorReading[gate] = true;
				else outSensorReading[gate] = false;
				i++;
				if ((switchBank[0] >> i) & 1)
					inSensorReading[gate] = true;
				else inSensorReading[gate] = false;
				gate++;
			}
			for(int i = 0; i < 8; i++)
		 	{
		  		if((switchBank[1] >> i) & 1)
		     		outSensorReading[gate] = true;
		   		else outSensorReading[gate] = false;
		  		i++;
		   		if((switchBank[1] >> i) & 1)
		       		inSensorReading[gate] = true;
		   		else inSensorReading[gate] = false;      
		   		gate++;  
		 	}
		 	for(int i = 0; i < 8; i++)
		 	{
		 		if((switchBank[2] >> i) & 1)
		     		outSensorReading[gate] = true;
		 		else outSensorReading[gate] = false;
		 		i++;
		   		if((switchBank[2] >> i) & 1)
		     		inSensorReading[gate] = true;
		   		else inSensorReading[gate] = false;       
		   		gate++;  
		 	}
		 	for(int i = 0; i < 8; i++)
		 	{
		 		if((switchBank[3] >> i) & 1)
		     		outSensorReading[gate] = true;
		   		else outSensorReading[gate] = false;
		   		i++;
		   		if((switchBank[3] >> i) & 1)
		       		inSensorReading[gate] = true;
		   		else inSensorReading[gate] = false;       
		   		gate++;  
		 	}
		 	for(int i = 0; i < 8; i++)
		 	{
		   		if((switchBank[4] >> i) & 1)
		       		outSensorReading[gate] = true;
		   		else outSensorReading[gate] = false;
		   		i++;
		   		if((switchBank[4] >> i) & 1)
		       		inSensorReading[gate] = true;
		   		else inSensorReading[gate] = false;       
		   		gate++;  
		 	}
		 	for(int i = 0; i < 8; i++)
		 	{
		   		if((switchBank[5] >> i) & 1)
		       		outSensorReading[gate] = true;
		   		else outSensorReading[gate] = false;
		   		i++;
		   		if((switchBank[5] >> i) & 1)
		       		inSensorReading[gate] = true;
		   		else inSensorReading[gate] = false;      
		   		gate++;  
		 	}  
		for (int i = 0; i < 6; i++){
			oldSwitchBank[i]=switchBank[i];
			FLAG = 0;
			}
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
		   		checkStateIn[i] = false;
		   		lastInSensorReading[i] = inSensorReading[i];
		   		inSensorTime[i] = current_time;
		 	} 
		 	if(outSensorReading[i] != lastOutSensorReading[i])  //change of state on OUT sensor
		 	{ 
		   		checkStateOut[i] = false;
		   		lastOutSensorReading[i] = outSensorReading[i];
		   		outSensorTime[i] = current_time;
			}    

		 	if(((current_time - inSensorTime[i]) > debeebounce) && (checkStateIn[i] == 0))
			{  //debounce IN sensor
		   		checkStateIn[i] = 1; //passed debounce      
		   	if(inSensorReading[i] == true) //a bee just entered the sensor
		   	{
				startInReadingTime[i] = current_time;
		   	}
		   	if(inSensorReading[i] == false)  //a bee just exits the sensor; that is, it was 1, now it is LOW (empty)
		   	{  
				lastInFinishedTime[i] = current_time;            
		     	inReadingTimeHigh[i] = current_time - startInReadingTime[i]; //this variable is how long the bee was present for
		     	printk("%i", i);
		     	printk(", IT ,");
		     	printk("%ld", inReadingTimeHigh[i]);
		     	printk(", ");    
		     	if(outReadingTimeHigh[i] < 650 && inReadingTimeHigh[i] < 650){ //should be less than 650ms
		       		if((current_time - lastOutFinishedTime[i]) < 200){ //the sensors are pretty cose together so the time it takes to trigger on and then the other should be small.. ~200ms
		        		inTotal++;
		        		printk("%lld", current_time);
		        		printk(",");
		        		printk("1\n");
		       	 	}else{
		         		printk("%lld\n", current_time); 
		        	}
		     	}else{
		      		printk("%lld\n", current_time); 
		     	}
		   	}           
		 }
		 if((current_time - outSensorTime[i]) > debeebounce && checkStateOut[i] == 0)  //debounce OUT sensor
		 {
		   checkStateOut[i] = 1; //passed debounce         
		   if(outSensorReading[i] == 1) //a bee just entered the sensor
		   {
		     startOutReadingTime[i] = current_time;
		   }
		   if(outSensorReading[i] == 0)  //a bee just exits the sensor; that is, it was HIGH, now it is LOW (empty)
		   {  
		     lastOutFinishedTime[i] = current_time;
		     outReadingTimeHigh[i] = (current_time- startOutReadingTime[i]); //this variable is how long the bee was present for
		     printk("%i", i);
		     printk(", OT ,");
		     printk("%ld", outReadingTimeHigh[i]);
		     printk(", ");        
		     if((outReadingTimeHigh[i] < 600) && (inReadingTimeHigh[i] < 600)){ //should be less than 600ms
		       if((current_time - lastInFinishedTime[i]) < 200){ //the sensors are pretty cose together so this time should be small
		         	outTotal++;
		     		printk("%lld\n", current_time);
		         	printk(",");
		        	printk("1\n"); 
		       }
			   else{
		         printk("%lld\n", current_time); 
		       		}
		     	}
				else{
		     	printk("%lld\n", current_time); 
		     	}
		    }          
		}        
	}

		k_busy_wait(15);   // debounce

		if ((current_time - lastOutput) > outputDelay) 
		{
			printk("sending data\n");
			sendData(); 
			lastOutput = current_time; 
			inTotal = 0;
			outTotal = 0; 
 	 	}
		
	}
}


