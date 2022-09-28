#include "sms_event.h"

static void log_sms_event(const struct app_event_header *eh)
{
        struct sms_event *event = cast_sms_event(eh);

       APP_EVENT_MANAGER_LOG(eh, "Status: %.4s, Number: %.*s", event->subScribe, event->dyndata.size, event->dyndata.data);
}

APP_EVENT_TYPE_DEFINE(sms_event,                /* Unique event name. */
                  log_sms_event,                /* Function logging event data. */
                  NULL,                         /* No event info provided. */
                  APP_EVENT_FLAGS_CREATE());      /* Flags managing event type */