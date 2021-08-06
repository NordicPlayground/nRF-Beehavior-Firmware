#include <zephyr.h>

#if defined(CONFIG_THINGY_ENABLE)
#define THINGY CONFIG_THINGY_NAME
/* Thinghy advertisement UUID */
#define BT_UUID_THINGY                                                         \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x01, 0x68, 0xEF)

/* Thingy Motion service UUID */
#define BT_UUID_TMS                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x04, 0x68, 0xEF)

/* Thingy Orientation characteristic UUID */
#define BT_UUID_TOC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x03, 0x04, 0x68, 0xEF)

/* Thingy environment service - EF68xxxx-9B35-4933-9B10-52FFA9740042 */
					/* Thingy Environment service UUID */
#define BT_UUID_TES                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x02, 0x68, 0xEF)

/*				Thingy Temperature characteristic UUID		*/
#define BT_UUID_TTC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x01, 0x02, 0x68, 0xEF)


/*				Thingy Pressure characteristic UUID - 12 bytes			*/
#define BT_UUID_TPC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x02, 0x02, 0x68, 0xEF)

		/*		Thingy Humidity characteristic UUID		*/
#define BT_UUID_THC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x03, 0x02, 0x68, 0xEF)


		/*		Thingy Environment Configuration characteristic UUID - 12 bytes */
#define BT_UUID_TECC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x06, 0x02, 0x68, 0xEF)

				/* Thingy Environment User Interface UUID */
#define BT_UUID_UIS                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x03, 0x68, 0xEF)

				/* Thingy Environment LED UUID */
#define BT_UUID_LED                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x01, 0x03, 0x68, 0xEF)

#define BT_UUID_BTRY                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x0F, 0x18, 0x68, 0xEF)


struct ble_tes_color_config_t
{
    uint8_t  led_red;
    uint8_t  led_green;
    uint8_t  led_blue;
};

struct ble_tes_config_t
{
    uint16_t                temperature_interval_ms;
    uint16_t                pressure_interval_ms;
    uint16_t                humidity_interval_ms;
    uint16_t                color_interval_ms;
    uint8_t                 gas_interval_mode;
    struct ble_tes_color_config_t  color_config;
};

bool thingy_scan = true;
#endif

#if defined(CONFIG_THINGY_ENABLE)
/* -------------- Temperature headers and cb -------------------- */
static void discovery_temperature_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_temperature_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_temperature_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_temperature_cb = {
	.completed = discovery_temperature_completed,
	.service_not_found = discovery_temperature_service_not_found,
	.error_found = discovery_temperature_error_found,
};

/* -------------- Humidity headers and cb -------------------- */
static void discovery_humidity_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_humidity_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_humidity_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_humidity_cb = {
	.completed = discovery_humidity_completed,
	.service_not_found = discovery_humidity_service_not_found,
	.error_found = discovery_humidity_error_found,
};

/* -------------- Air pressure discovery headers and cb --------------------*/
static void discovery_air_pressure_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_air_pressure_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_air_pressure_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_air_pressure_cb = {
	.completed = discovery_air_pressure_completed,
	.service_not_found = discovery_air_pressure_service_not_found,
	.error_found = discovery_air_pressure_error_found,
};

/* ------------------ Orientation headers and cb --------------------
	This can be left out or used for alarm purposes, i.e "Notify when sensor is moving and trigger "hive has been
	toppled"- alarm.

*/
static void discovery_orientation_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_orientation_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_orientation_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_orientation_cb = {
	.completed = discovery_orientation_completed,
	.service_not_found = discovery_orientation_service_not_found,
	.error_found = discovery_orientation_error_found,
};

/* ------------------------ Declaration of connection and gattp functions ----------------------*/
static void discover_temperature_gattp(struct bt_conn *conn);
static void discover_humidity_gattp(struct bt_conn *conn);
static void discover_air_pressure_gattp(struct bt_conn *conn);
static void discover_orientation_gattp(struct bt_conn *conn);
// static void discover_battery_gattp(struct bt_conn *conn);

static void connected(struct bt_conn *conn, uint8_t conn_err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);


// ------------------ Connected struct
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};
#endif


/* ----------------------- BM_W Initialization and declarations  -------------------------*/
#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)

static struct k_work_delayable weight_interval;
static struct k_work_delayable temperature_interval;

// #define LOG_MODULE_NAME bm_w_module
// LOG_MODULE_REGISTER(LOG_MODULE_NAME, 4);

#define REAL_TIME_WEIGHT 0x16
#define BROODMINDER_ADDR ((bt_addr_le_t[]) { { 0, \
			 { { 0xFD, 0x01, 0x57, 0x16, 0x09, 0x06 } } } })
#define BROODMINDER_ADDR_TEMPERATURE ((bt_addr_le_t[]) { { 0, \
			 { { 0x93, 0x05, 0x47, 0x16, 0x09, 0x06 } } } })

#endif
// // #define USE_BMW;
// // #define USE_TEMPERATURE;

#if defined(CONFIG_THINGY_ENABLE)
static int scan_init(bool first);
#endif
#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
static int scan_init_bm(bool first);
#endif
#if defined(CONFIG_BEE_COUNTER_ENABLE)
static int bee_scan_init(bool first);
#endif