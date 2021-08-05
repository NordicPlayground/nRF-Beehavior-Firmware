#include "event_manager.h"

struct bee_count_event {
        struct event_header header;
        // enum bee_count_event_type type;

        /* Custom data fields. */
        uint16_t out;
        uint16_t in;
        
};

EVENT_TYPE_DECLARE(bee_count_event);