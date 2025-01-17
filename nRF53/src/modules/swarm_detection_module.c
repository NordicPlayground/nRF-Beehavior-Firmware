#include <zephyr.h>

#include "events/thingy_event.h"
#include "events/bm_w_event.h"
#include "events/bee_count_event.h"
#include <stdlib.h>
#include "peripheral_module.h"


#define TEMPTHRESH 30 //this is not correct, but will be adjusted for
#define MOISTTHRESH 30 //Moisture levels constantly dropping may be indicator if swarming
#define WEIGHTTHRESH 3.5 //Drop in weight 
#define ARRAY_MAX 20 //The number of samples saved for thingy data
/*
Notes about swarming:
- One indicator is sudden weightloss, from data we can seee about 3.5 ish kg og sudden drop in the span of an hour maybe?
- Normally, temperature is tightly controlled at around 34 degrees Celsius, during swarming however the temperature has a higher standard deviation
- Just before swarming, temperature has to increase to above the mean, ish 35 degrees right before, this is a sharp increase in maybe 10 mins. 
- Also drop in humidity drop of like 10%?

*/

struct k_fifo data_fifo;

k_fifo_init(&my_fifo);



int beecount_func(uint8_t out){ //Algorithms for when swarming happens has not been developed yet...
	//Do something
	//This part of the code should include what flux has been like when an actual swarming has happend
}

int weight_func(float bm_w_data[2]){
	//float weight_data = atof(thingy_data[0]) + (atof(thingy_data[1])/100)
}



static bool event_handler(const struct event_header *eh)
{
    if (is_thingy_matrix_event(eh)) {
		for (int i = 0; i < THINGY_BUFFER_SIZE; i++){
				float temp_data = atof(thingy_data[0]) + (atof(thingy_data[1])/100);
				float humidity_data = atof(thingy_data[2]);
				
		}
		/*
	    struct thingy_event *event = cast_thingy_event(eh);
        uint8_t thingy_data[3] = {event->data_array[0], event->data_array[1], event->data_array[2]}; //Index 0 before comma, index 1 after comma (temperature). Index 2 is moisture.
		*/
	    return false;

	}

	if(is_bee_count_event(eh)){

		struct bee_count_event *event = cast_bee_count_event(eh);
		uint16_t out = event->out;

		return false;
	}

	if (is_bm_w_event(eh)){

		struct bm_w_event *event = cast_bm_w_event(eh);
    	float bm_w_data[2] = {event->realTimeWeight,
			, event->temperature};
        //Sudden drop in weight and hight beeactivity in to the hive can indicate robbing, sus activity
        return false;
	}

        return false;
}

int thingyFunc(uint8_t thingy_data[3]){
	float temp_data = atof(thingy_data[0]) + (atof(thingy_data[1])/100);
	float humidity_data = atof(thingy_data[2]);
}

void swarm_indication_alert(){
	//Send data to the 9160 if level of alert is high enough?
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, thingy_event);
EVENT_SUBSCRIBE(MODULE, bm_w_event);
EVENT_SUBSCRIBE(MODULE, bee_count_event);