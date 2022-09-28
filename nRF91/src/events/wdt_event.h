#include "app_event_manager.h"

enum wdt_event_type {
        WDT_SETUP,
        WDT_ADD_MAIN,
        WDT_ADD_NRF53,
        WDT_FEED_MAIN,
        WDT_FEED_NRF53,
	WDT_TIMEOUT,
};

struct wdt_event {
        struct app_event_header header;
        enum wdt_event_type type;

        /* Custom WDT data fields. */
       uint8_t wdt_channel_id; // Used for timeout events.
};

APP_EVENT_TYPE_DECLARE(wdt_event);