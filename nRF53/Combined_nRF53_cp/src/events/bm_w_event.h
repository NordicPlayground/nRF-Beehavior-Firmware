#include "event_manager.h"

// enum bm_w_event_type {
// 	BM_W_READY,
// 	BM_W_CONNECTED,
// 	BM_W_DISCONNECTED,
//         BM_W_SCANNING,
//         BM_W_DONE_SCANNING,
// 	BM_W_RECEIVED,
// 	BM_W_SEND
// };

struct bm_w_event {
        struct event_header header;
        // enum bm_w_event_type type;

        /* Custom data fields. */
        float weightR;
        float weightL;
        float realTimeWeight;
        float temperature;

};

EVENT_TYPE_DECLARE(bm_w_event);