#include "cloud_event.h"

// Called cloud_event_abbr, because cloud_event was allready taken.
static void log_cloud_event_abbr(const struct app_event_header *eh)
{
        struct cloud_event_abbr *event = cast_cloud_event_abbr(eh);

        APP_EVENT_MANAGER_LOG(eh, "Message: %.*s", event->dyndata.size, event->dyndata.data);
}

APP_EVENT_TYPE_DEFINE(cloud_event_abbr,         /* Unique event name. */
                  log_cloud_event_abbr,         /* Function logging event data. */
                  NULL,                         /* No event info provided. */
                  APP_EVENT_FLAGS_CREATE());      /* Flags managing event type */