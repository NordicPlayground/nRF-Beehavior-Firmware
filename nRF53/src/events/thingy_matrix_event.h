#include "event_manager.h"
#include "peripheral_module.h"

struct thingy_matrix_event {
        struct event_header header;
        // enum thingy_event_type type;

        /* Custom data fields. */
        uint8_t thingy_matrix[THINGY_BUFFER_SIZE][11];
};

EVENT_TYPE_DECLARE(thingy_matrix_event);