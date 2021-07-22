#include "event_manager.h"

enum ble_event_type {
	BLE_READY,
	THINGY_SCANNING,
	THINGY_READY,
	THINGY_DISCONNECTED,
	BM_W_SCANNING,
        BM_W_READ,
        HUB_SCANNING,
        HUB_CONNECTED,
        HUB_DISCONNECTED,
};

struct ble_event {
        struct event_header header;
        enum ble_event_type type;

        /* Custom data fields. */

};

EVENT_TYPE_DECLARE(ble_event);