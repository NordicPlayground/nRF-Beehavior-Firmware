#include "app_event_manager.h"
#include <bluetooth/conn.h>

enum ble_event_type {
	BLE_READY,
	SCAN_START,
        SCAN_FAILED,
        SCAN_SUCCES,
	THINGY_FOUND,
	THINGY_DISCONNECTED,
	THINGY_READY,
	BM_W_READ,
        BEE_COUNTER_FOUND,
        BEE_COUNTER_DISCONNECTED,
        HUB_CONNECTED,
        BLE_NVS_SEND_TO_NRF91,
};

struct ble_event {
        struct app_event_header header;
        enum ble_event_type type;

        /* Custom data fields. */
        struct bt_addr_le_t *addr;
        char *scan_name;
        uint8_t len;
        uint8_t wdt_channel_id;
};

APP_EVENT_TYPE_DECLARE(ble_event);