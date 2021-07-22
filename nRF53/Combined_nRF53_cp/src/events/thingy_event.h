#include "event_manager.h"

// enum thingy_event_type {
// 	THINGY_READY,
// 	THINGY_CONNECTED,
// 	THINGY_DISCONNECTED,
//         THINGY_SCANNING,
//         THINGY_DONE_SCANNING,
// 	THINGY_RECEIVED,
// 	THINGY_SEND
// };

struct thingy_event {
        struct event_header header;
        // enum thingy_event_type type;

        /* Custom data fields. */
        uint8_t data_array[3];
};

EVENT_TYPE_DECLARE(thingy_event);