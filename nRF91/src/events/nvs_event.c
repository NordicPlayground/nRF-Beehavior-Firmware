#include "nvs_event.h"

static int log_nvs_event(const struct app_event_header *eh, char *buf,
                            size_t buf_len)
{
        struct nvs_event *event = cast_nvs_event(eh);

        APP_EVENT_MANAGER_LOG(eh, "Event_type: %i", event->type);
}

APP_EVENT_TYPE_DEFINE(nvs_event,      /* Unique event name. */
                log_nvs_event,  /* Function logging event data. */
                NULL,                         /* No event info provided. */
                APP_EVENT_FLAGS_CREATE());      /* Flags managing event type */