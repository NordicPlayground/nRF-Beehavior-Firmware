#include "app_event_manager.h"

struct woodpecker_event {
        struct app_event_header header;
        // enum bee_count_event_type type;

        /* Custom data fields. */
        uint8_t positive_triggers;
        uint16_t total_triggers;
        uint8_t highest_probability;
        uint8_t bat_percentage;
        
};

APP_EVENT_TYPE_DECLARE(woodpecker_event);