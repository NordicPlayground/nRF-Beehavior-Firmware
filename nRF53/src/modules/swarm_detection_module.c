#include <zephyr.h>

#include "events/thingy_event.h"
#include "events/bm_w_event.h"
#include "events/bee_count_event.h"

#define TEMPTHRESH 30 //this is not correct, but will be adjusted for
#define FLUX 200000 //Amount of bees going out at at span of 20 mins?
#define MOISTTHRESH 30 //Moisture levels constantly dropping may be indicator if swarming
#define WEIGHTTHRESH 3.5 //Drop in weight 



static bool event_handler(const struct event_header *eh)
{
     if (is_thingy_event(eh)) {

	struct thingy_event *event = cast_thingy_event(eh);
        
        uint8_t thingy_data[3] = {event->data_array[0], event->data_array[1], event->data_array[2]};//,\

	return false;
		
	}
	
	if(is_bee_count_event(eh)){
		LOG_INF("Bee Counter event is being handled\n");
		struct bee_count_event *event = cast_bee_count_event(eh);

		uint16_t out = event->out

		return false;
	}

	if (is_bm_w_event(eh)) {
	struct bm_w_event *event = cast_bm_w_event(eh);
        float bm_w_data[2] = {event->realTimeWeight,
			, event->temperature};
        
	}
    
	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, thingy_event);
EVENT_SUBSCRIBE(MODULE, bm_w_event);
EVENT_SUBSCRIBE(MODULE, bee_count_event);