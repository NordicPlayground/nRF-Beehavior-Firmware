#include "wdt_event.h"

static int log_wdt_event(const struct app_event_header *eh, char *buf,
                            size_t buf_len)
{
        struct wdt_event *event = cast_wdt_event(eh);

        APP_EVENT_MANAGER_LOG(eh, "Event_type: %i", event->type);
}

APP_EVENT_TYPE_DEFINE(wdt_event,      /* Unique event name. */
                log_wdt_event,  /* Function logging event data. */
                NULL,                         /* No event info provided. */
                APP_EVENT_FLAGS_CREATE());      /* Flags managing event type */