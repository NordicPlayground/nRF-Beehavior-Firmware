#include <zephyr.h>

static void wdt_callback_main_event(uint8_t channel_id, void *user_data);

static void wdt_callback_nrf53_event(uint8_t channel_id, void *user_data);

void watchdog_setup(void);

void wdt_add_channels(int wdt_to_add);

enum wdt_channel {
        WDT_CHANNEL_NRF91_MAIN,
        WDT_CHANNEL_NRF91_NRF53_DEVICE,
        WDT_CHANNEL_NRF53_MAIN,
        WDT_CHANNEL_NRF53_BEE_COUNTER,
        WDT_CHANNEL_NRF53_THINGY,
};