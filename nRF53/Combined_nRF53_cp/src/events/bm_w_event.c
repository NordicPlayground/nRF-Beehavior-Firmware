#include "bm_w_event.h"

static int log_bm_w_event(const struct event_header *eh, char *buf,
                            size_t buf_len)
{
        struct bm_w_event *event = cast_bm_w_event(eh);

        return snprintf(buf, buf_len, "WeightR: %.2f, WeightL: %.2f, RTW: %.2f, Temperature: %.2f", event->weightR, event->weightL, event->realTimeWeight, event->temperature);
}

EVENT_TYPE_DEFINE(bm_w_event,      /* Unique event name. */
                  true,              /* Event logged by default. */
                  log_bm_w_event,  /* Function logging event data. */
                  NULL);             /* No event info provided. */