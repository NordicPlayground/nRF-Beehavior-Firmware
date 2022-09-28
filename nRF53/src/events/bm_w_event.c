#include "bm_w_event.h"

static int log_bm_w_event(const struct app_event_header *eh, char *buf,
                            size_t buf_len)
{
        struct bm_w_event *event = cast_bm_w_event(eh);

        APP_EVENT_MANAGER_LOG(eh, "WeightR: %.2f, WeightL: %.2f, RTW: %.2f, Temperature: %.2f", event->weightR, event->weightL, event->realTimeWeight, event->temperature);
}

APP_EVENT_TYPE_DEFINE(bm_w_event,      /* Unique event name. */
                log_bm_w_event,  /* Function logging event data. */
                NULL,                         /* No event info provided. */
                APP_EVENT_FLAGS_CREATE());      /* Flags managing event type */