#include "app_event_manager.h"

struct thingy_event {
        struct app_event_header header;
        // enum thingy_event_type type;

        /* Custom data fields. */
        uint8_t data_array[3]; /* 2 first bytes are temperature ([integer],[decimal]) and last byte is relative humidity */

        int32_t pressure_int; /*Integer part of the air pressure */
        uint8_t pressure_float; /*Decimal part of the air pressure */
        uint8_t battery_charge;
};



APP_EVENT_TYPE_DECLARE(thingy_event);
