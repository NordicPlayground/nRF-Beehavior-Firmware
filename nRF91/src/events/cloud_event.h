#include <app_event_manager.h>

enum cloud_event_type_abbr {
    LTE_CONNECTED,
    LTE_SLEEP,
	CLOUD_SETUP_COMPLETE,
	CLOUD_CONNECTED,
	CLOUD_DISCONNECTED,
    CLOUD_SLEEP,
    CLOUD_RECEIVED,
	CLOUD_SEND,
    CLOUD_SEND_WDT
};

struct cloud_event_abbr {
        struct app_event_header header;
        enum cloud_event_type_abbr type;

        /* Custom data fields. */
        char name[20];
        struct event_dyndata dyndata;

};

APP_EVENT_TYPE_DYNDATA_DECLARE(cloud_event_abbr);