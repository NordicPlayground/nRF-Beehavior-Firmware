#include "thingy_event.h"

static int log_thingy_event(const struct event_header *eh, char *buf,
                            size_t buf_len)
{
        struct thingy_event *event = cast_thingy_event(eh);

        return snprintf(buf, buf_len, "Temperature: %i,%i, Humidity: %i", event->data_array[0], event->data_array[1], event->data_array[2]);
}

EVENT_TYPE_DEFINE(thingy_event,      /* Unique event name. */
                  true,              /* Event logged by default. */
                  log_thingy_event,  /* Function logging event data. */
                  NULL);             /* No event info provided. */