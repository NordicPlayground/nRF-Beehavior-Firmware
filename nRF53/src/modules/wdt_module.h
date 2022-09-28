#include <zephyr.h>

static void wdt_callback_main_event(uint8_t channel_id, void *user_data);

static void wdt_callback_bee_counter_event(uint8_t channel_id, void *user_data);

static void wdt_callback_thingy_event(uint8_t channel_id, void *user_data);

void watchdog_setup(void);

void wdt_add_channels(int wdt_to_add);

enum wdt_channel {
        WDT_CHANNEL_MAIN,
        WDT_CHANNEL_BEE_COUNTER,
        WDT_CHANNEL_THINGY,
};