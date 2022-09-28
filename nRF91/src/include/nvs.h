#include <zephyr.h>
#include <device.h>
#include <fs/nvs.h>
#include <storage/flash_map.h>
#include <string.h>

#define DEVICE_LABEL storage
#define TELEPHONE_ID_START 6

static struct nvs_fs nvs = {
    .sector_size = DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size), //Got this from samples/nfc/writable_ndef_msg
    .sector_count = 2, //Decided based on how much the application needs
    .offset = FLASH_AREA_OFFSET(DEVICE_LABEL),
};                         

