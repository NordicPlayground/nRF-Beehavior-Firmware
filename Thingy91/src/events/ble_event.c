#include "ble_event.h"

static int log_ble_event(const struct event_header *eh, char *buf,
                            size_t buf_len)
{
        struct ble_event *event = cast_ble_event(eh);

        return snprintf(buf, buf_len, "Address: %.17s, Name:%.20s, Message: %s", event->address, event->name, event->dyndata.data);
}

EVENT_TYPE_DEFINE(ble_event,      /* Unique event name. */
                  true,              /* Event logged by default. */
                  log_ble_event,  /* Function logging event data. */
                  NULL);             /* No event info provided. */