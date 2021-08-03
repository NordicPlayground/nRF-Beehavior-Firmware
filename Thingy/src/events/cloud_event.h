#include "event_manager.h"

enum cloud_event_type_abbr {
	CLOUD_CONNECTED,
	CLOUD_DISCONNECTED,
    CLOUD_SLEEP,
    CLOUD_RECEIVED,
	CLOUD_SEND
};

struct cloud_event_abbr {
        struct event_header header;
        enum cloud_event_type_abbr type;

        /* Custom data fields. */
        struct event_dyndata dyndata;

};

EVENT_TYPE_DYNDATA_DECLARE(cloud_event_abbr);