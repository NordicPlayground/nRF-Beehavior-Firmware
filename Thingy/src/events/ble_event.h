#include "event_manager.h"

enum ble_event_type {
	BLE_READY,
	BLE_CONNECTED,
	BLE_DISCONNECTED,
        BLE_SCANNING,
        BLE_DONE_SCANNING,
	BLE_STATUS,
	BLE_RECEIVED,
	BLE_SEND
};

struct ble_event {
        struct event_header header;
        enum ble_event_type type;

        /* Custom data fields. */
        char address[17];
        char name[20];
        struct event_dyndata dyndata;

};

EVENT_TYPE_DYNDATA_DECLARE(ble_event);