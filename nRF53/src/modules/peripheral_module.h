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

union tagname pressure_union;

union tagname16{
	uint16_t a;
	unsigned char s[2];
};
uint8_t thingy_matrix[THINGY_BUFFER_SIZE][11] = {0};
uint8_t sample_counter = 0;

union tagname16 object16;

/*Temp for debugging*/
bool IS_SWARMING;