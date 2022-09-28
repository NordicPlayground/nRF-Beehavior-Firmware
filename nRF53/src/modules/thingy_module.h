#define THINGY_SAMPLE_RATE CONFIG_THINGY_SAMPLE_RATE

/* UUID's used to subscribe to the Thingy:52 service */

/* Primary service UUID */
#define BT_UUID_BEE_VAL 0x0BEE
#define BT_UUID_BEE \
	BT_UUID_DECLARE_16(BT_UUID_BEE_VAL)
/** @def BT_UUID_HIDS_VAL
 *  @brief HID Service UUID value
*/

/* Temperature characteristic UUID */
#define BT_UUID_TEMP_SENSOR_VAL 0x1BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_TEMP_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_TEMP_SENSOR_VAL)

/* Humidity characteristic UUID */
#define BT_UUID_HUMID_SENSOR_VAL 0x2BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_HUMID_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_HUMID_SENSOR_VAL)
	
/* Pressure characteristic UUID */
#define BT_UUID_PRESSURE_SENSOR_VAL 0x3BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_PRESSURE_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_PRESSURE_SENSOR_VAL)

/* Battery characteristic UUID */
#define BT_UUID_BATTERY_VAL 0x4BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_BATTERY \
	BT_UUID_DECLARE_16(BT_UUID_BATTERY_VAL)

/* Write/configure characteristic UUID */
#define BT_UUID_WRITE_VAL 0x5BEE
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_WRITE \
	BT_UUID_DECLARE_16(BT_UUID_WRITE_VAL)

static void discovery_write_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_write_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_write_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_write_cb = {
	.completed = discovery_write_completed,
	.service_not_found = discovery_write_service_not_found,
	.error_found = discovery_write_error_found,
};

static void discovery_bee_completed(struct bt_gatt_dm *disc, void *ctx);
static void discovery_bee_service_not_found(struct bt_conn *conn, void *ctx);
static void discovery_bee_error_found(struct bt_conn *conn, int err, void *ctx);

static struct bt_gatt_dm_cb discovery_bee_cb = {
	.completed = discovery_bee_completed,
	.service_not_found = discovery_bee_service_not_found,
	.error_found = discovery_bee_error_found,
};