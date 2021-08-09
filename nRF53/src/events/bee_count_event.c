#include "bee_count_event.h"

static int log_bee_count_event(const struct event_header *eh, char *buf,
                            size_t buf_len)
{
        struct bee_count_event *event = cast_bee_count_event(eh);

        return snprintf(buf, buf_len, "In: %i, Out: %i", event->in, event->out);
}

EVENT_TYPE_DEFINE(bee_count_event,      /* Unique event name. */
                  true,              /* Event logged by default. */
                  log_bee_count_event,  /* Function logging event data. */
                  NULL);             /* No event info provided. */