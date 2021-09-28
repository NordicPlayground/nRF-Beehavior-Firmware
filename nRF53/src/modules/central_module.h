#include <zephyr.h>

#define MODULE central_module
LOG_MODULE_REGISTER(MODULE, 4);

/* ----------------------- Thingy declaration and initialization -------------------------*/

static K_SEM_DEFINE(ble_ready, 0, 1);
static K_SEM_DEFINE(peripheral_done, 0, 1);
#if defined(CONFIG_BEE_COUNTER_ENABLE)
static K_SEM_DEFINE(bee_count_done, 0, 1);
#endif
#if defined(CONFIG_THINGY_ENABLE)
static K_SEM_DEFINE(temperature_received, 0, 1);
static K_SEM_DEFINE(humidity_received, 0, 1);
static K_SEM_DEFINE(air_pressure_received, 0, 1);

bool configured = false;

uint8_t data_array[3];

int32_t pressure_int;
uint8_t pressure_float;
uint8_t battery_charge;

static struct bt_conn *thingy_conn;
#endif

#if defined(CONFIG_BEE_COUNTER_ENABLE)
#define BEE_COUNTER CONFIG_BEE_COUNTER_NAME

static struct bt_conn *bee_conn;

static struct bt_nus_client nus_client; //Handles communication for the bee_conn
#endif

K_SEM_DEFINE(service_ready, 0, 1)

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

				/*Thingy Battery service */
#define BT_UUID_TBS                                                           \
	BT_UUID_DECLARE_16(0x180F)
				/*Thingy Battery characteristic */
#define BT_UUID_TBC                                                            \
	BT_UUID_DECLARE_16(0x2A19)
	 

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

/* ------------------ Battery headers and cb --------------------
	This can be left out or used for alarm purposes, i.e "Notify when sensor is moving and trigger "hive has been
	toppled"- alarm.

*/

// static struct bt_gatt_read_params read_params){

// };
static void discovery_battery_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_battery_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_battery_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_battery_cb = {
	.completed = discovery_battery_completed,
	.service_not_found = discovery_battery_service_not_found,
	.error_found = discovery_battery_error_found,
};


 /* -------------------------- write to led and write discovery callbscks --------------------*/
static void discovery_write_to_led_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_write_to_led_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_write_to_led_error_found(struct bt_conn *conn, int err, void *ctx);


static struct bt_gatt_dm_cb discovery_write_to_led_cb = {
	.completed = discovery_write_to_led_completed,
	.service_not_found = discovery_write_to_led_service_not_found,
	.error_found = discovery_write_to_led_error_found,
};

static void discovery_write_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_write_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_write_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_write_cb = {
	.completed = discovery_write_completed,
	.service_not_found = discovery_write_service_not_found,
	.error_found = discovery_write_error_found,
};

/* ------------------------ Declaration of connection and gattp functions ----------------------*/
static void discover_temperature_gattp(struct bt_conn *conn);
static void discover_humidity_gattp(struct bt_conn *conn);
static void discover_air_pressure_gattp(struct bt_conn *conn);
static void discover_orientation_gattp(struct bt_conn *conn);
static void discover_battery_gattp(struct bt_conn *conn);
static void write_to_led_gattp(struct bt_conn *conn);
static void write_to_characteristic_gattp(struct bt_conn *conn);

static void connected(struct bt_conn *conn, uint8_t conn_err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);


/* ------------------ Connected struct   ---------------------- */
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

#if defined(CONFIG_THINGY_ENABLE)
static int scan_init(bool first);
#endif
#if defined(CONFIG_BROODMINDER_WEIGHT_ENABLE)
static int scan_init_bm(bool first);
#endif
#if defined(CONFIG_BEE_COUNTER_ENABLE)
static int bee_scan_init(bool first);
#endif

/* ----------------------- Bee discovery_cb  -------------------------*/

static void bee_discovery_complete(struct bt_gatt_dm *dm, void *context);
static void bee_discovery_service_not_found(struct bt_conn *conn, void *context);
static void bee_discovery_error(struct bt_conn *conn, int err, void *context);
				   
struct bt_gatt_dm_cb bee_discovery_cb = {
	.completed         = bee_discovery_complete,
	.service_not_found = bee_discovery_service_not_found,
	.error_found       = bee_discovery_error,
};

/* ----------------------- Authorization callback struct  -------------------------*/
static void auth_cancel(struct bt_conn *conn);
static void pairing_confirm(struct bt_conn *conn);
static void pairing_complete(struct bt_conn *conn, bool bonded);
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason);

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};	