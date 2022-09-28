#include <app_event_manager.h>

enum sms_event_type {
	NEW_NUMBER,
        NUMBER_STATUS
};

struct sms_event {
        struct app_event_header header;
        enum sms_event_type type;

        /* Custom data fields. */
        uint8_t subScribe;
        struct event_dyndata dyndata;
        

};
APP_EVENT_TYPE_DYNDATA_DECLARE(sms_event);