#include "app_event_manager.h"

struct bee_count_event {
        struct app_event_header header;
        // enum bee_count_event_type type;

        /* Custom data fields. */
        uint16_t out;
        uint16_t in;
        
};

APP_EVENT_TYPE_DECLARE(bee_count_event);