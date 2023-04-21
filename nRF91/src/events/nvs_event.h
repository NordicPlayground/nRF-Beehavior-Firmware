#include "app_event_manager.h"

enum nvs_event_type {
        NVS_SETUP,
        NVS_WRITE_BEFORE_REBOOT,
        NVS_SEND_TO_CLOUD,
        NVS_WIPE,
        NVS_CHECK_SPACE,
};

struct nvs_event {
        struct app_event_header header;
        enum nvs_event_type type;

        /* Custom NVS data fields. */
        uint8_t wdt_channel_id;    
};

APP_EVENT_TYPE_DECLARE(nvs_event);