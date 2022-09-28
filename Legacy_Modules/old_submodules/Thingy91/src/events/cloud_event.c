#include "cloud_event.h"

static int log_cloud_event_abbr(const struct event_header *eh, char *buf,
                            size_t buf_len)
{
        struct cloud_event_abbr *event = cast_cloud_event_abbr(eh);

        return snprintf(buf, buf_len, "Message: %.*s", event->dyndata.size, event->dyndata.data);
}

EVENT_TYPE_DEFINE(cloud_event_abbr,      /* Unique event name. */
                  true,              /* Event logged by default. */
                  log_cloud_event_abbr,  /* Function logging event data. */
                  NULL);             /* No event info provided. */