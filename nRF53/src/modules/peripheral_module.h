#include <zephyr.h>

#define MODULE peripheral_module
LOG_MODULE_REGISTER(MODULE, 4);


#if defined(CONFIG_THINGY_ENABLE)
#define THINGY_BUFFER_SIZE CONFIG_THINGY_DATA_BUFFER_SIZE
#define THINGY_SAMPLE_RATE CONFIG_THINGY_SAMPLE_RATE
#define THINGY_SAMPLE_TO_SEND CONFIG_THINGY_SAMPLE_TO_SEND
#endif

// Used for thingy_event
union tagname{
	int a;
	unsigned char s[4];
};

union tagname_avg{
	int a;
	unsigned char s[4];
};

union tagname pressure_union;
union tagname_avg pressure_avg_union;


union tagname16{
	uint16_t a;
	unsigned char s[2];
};

union tagname16 object16; //For beecounter

uint8_t thingy_matrix[THINGY_BUFFER_SIZE][11] = {0};
uint8_t sample_counter = 0;

/* Dette er jallamekk og i praksis kun temp-variabler 
	Do sums need to be 2x size? Maybe.
*/

//The sums are larger to acommodate for the accumulated sums. 20x32bit becomes larger than what 32bit can represent
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

bool THINGY_BUFFER_WRITABLE = true;



/*Temp for debugging*/
bool IS_SWARMING;