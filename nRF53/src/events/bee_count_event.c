#include "bee_count_event.h"

static int log_bee_count_event(const struct app_event_header *eh, char *buf,
                            size_t buf_len)
{
        struct bee_count_event *event = cast_bee_count_event(eh);

        APP_EVENT_MANAGER_LOG(eh, "In: %i, Out: %i", event->in, event->out);
}

APP_EVENT_TYPE_DEFINE(bee_count_event,      /* Unique event name. */
                log_bee_count_event,  /* Function logging event data. */
                NULL,              /* No event info provided. */
                APP_EVENT_FLAGS_CREATE()); /* Flags managing event type */      