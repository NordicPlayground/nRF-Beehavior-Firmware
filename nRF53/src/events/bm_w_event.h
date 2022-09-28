#include "app_event_manager.h"

struct bm_w_event {
        struct app_event_header header;
        // enum bm_w_event_type type;

        /* Custom data fields. */
        float weightR;
        float weightL;
        float realTimeWeight;
        float temperature;

};

APP_EVENT_TYPE_DECLARE(bm_w_event);