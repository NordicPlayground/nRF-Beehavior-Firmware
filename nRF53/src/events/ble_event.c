#include "ble_event.h"

static int log_ble_event(const struct app_event_header *eh, char *buf,
                            size_t buf_len)
{
        struct ble_event *event = cast_ble_event(eh);

        APP_EVENT_MANAGER_LOG(eh, "Event_type: %i", event->type);
}

APP_EVENT_TYPE_DEFINE(ble_event,      /* Unique event name. */
                log_ble_event,  /* Function logging event data. */
                NULL,              /* No event info provided. */
                APP_EVENT_FLAGS_CREATE()); /* Flags managing event type */     