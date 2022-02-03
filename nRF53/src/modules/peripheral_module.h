#include <zephyr.h>

#define MODULE peripheral_module
LOG_MODULE_REGISTER(MODULE, 4);

/* If Thingy:52 is enabled, create these variables which are defined in the config */
#if defined(CONFIG_THINGY_ENABLE)
#define THINGY_BUFFER_SIZE CONFIG_THINGY_DATA_BUFFER_SIZE
#define THINGY_SAMPLE_RATE CONFIG_THINGY_SAMPLE_RATE
#define THINGY_SAMPLE_TO_SEND CONFIG_THINGY_SAMPLE_TO_SEND
#endif
/* Union for pressure measurement

The union is used for splitting a 32bit variable into 4 separate 8 bit variables. It takes the first 8 bits 
of the 32 bit variable and writes that in to the first 8bit variable and repeats this for the rest. This was the simplest way to 
allocate the 32+8 bit air pressure measurement reading to the Bluetooth data message which we send from the nRF5340 to the nRF9160

Example: int32_t  (in binary) = [11110000 11110000 11110000] becomes [11110000],[11110000],[11110000],[11110000] = 4x8bit elements
after using the union. 

*/
union tagname{
	int a;
	unsigned char s[4];
};
/*Union for average pressure */
union tagname_avg{
	int a;
	unsigned char s[4];
};

union tagname pressure_union;
union tagname_avg pressure_avg_union;

/*
Some declarations of variables used in thingy_event
*/
uint8_t thingy_matrix[THINGY_BUFFER_SIZE][11] = {0};
uint8_t sample_counter = 0;

//The temprorary sum variables are larger to acommodate for the accumulated sums. 20x32bit becomes larger than what 32bit can represent
int64_t pressure_int_sum = 0;
uint16_t pressure_float_sum = 0;
int32_t pressure_int_avg = 0;
uint8_t pressure_float_avg = 0;

uint16_t temperature_int_sum = 0;
uint16_t temperature_float_sum = 0;
uint8_t temperature_int_avg = 0;
uint8_t temperature_float_avg = 0;

uint16_t humidity_sum = 0;
uint8_t humidity_avg = 0;

/*Control parameters */
//Control parameter that states if there are any available slots in the data buffer
bool THINGY_BUFFER_WRITABLE = true;

//Not sure if this is necessary
bool INSERT_AT_LAST_ROW = false;

/*Control parameter that states that this is the first sample we receive after start up. This is used to avoid having to wait until a
Average reading has been received (i.e not having to wait 20 samples before we send something to the 91.*/
bool FIRST_SAMPLE = true;



/*Temp for debugging*/
bool IS_SWARMING;




/* Something used for the beecounter. Not Andreas' creation*/
union tagname16{
	uint16_t a;
	unsigned char s[2];
};

union tagname16 object16; //For beecounter

