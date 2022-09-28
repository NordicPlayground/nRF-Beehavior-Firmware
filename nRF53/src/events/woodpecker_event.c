#include "woodpecker_event.h"

static int log_woodpecker_event(const struct app_event_header *eh, char *buf,
                            size_t buf_len)
{
        struct woodpecker_event *event = cast_woodpecker_event(eh);

        APP_EVENT_MANAGER_LOG(eh, "Positive: %i, Total: %i, Highest percentage: %i", event->positive_triggers, event->total_triggers, event->highest_probability);
}

APP_EVENT_TYPE_DEFINE(woodpecker_event,      /* Unique event name. */
                log_woodpecker_event,  /* Function logging event data. */
                NULL,              /* No event info provided. */
                APP_EVENT_FLAGS_CREATE()); /* Flags managing event type */      