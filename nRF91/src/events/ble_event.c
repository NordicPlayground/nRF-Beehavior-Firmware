#include "ble_event.h"

static void log_ble_event(const struct app_event_header *eh)
{
        struct ble_event *event = cast_ble_event(eh);

       APP_EVENT_MANAGER_LOG(eh, "Address: %.17s, Name:%.20s, Message: %s", event->address, event->name, event->dyndata.data);
}

APP_EVENT_TYPE_DEFINE(ble_event,                /* Unique event name. */
                  log_ble_event,                /* Function logging event data. */
                  NULL,                         /* No event info provided. */
                  APP_EVENT_FLAGS_CREATE());      /* Flags managing event type */