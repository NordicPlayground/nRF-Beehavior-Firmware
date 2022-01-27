#include "event_manager.h"

struct thingy_matrix_event {
        struct event_header header;
        // enum thingy_event_type type;

        /* Custom data fields. */
        //add matrix here
};



EVENT_TYPE_DECLARE(thingy_matrix_event);