#include <errno.h>

#include "ble_gatt_android_name.h"

static const char hex_digits[] = "0123456789ABCDEF";

int ble_gatt_android_name_format(
    char *name, size_t capacity,
    const uint8_t node_id[BLE_GATT_ANDROID_NODE_ID_LEN])
{
    if (name == NULL || node_id == NULL) {
        return -EINVAL;
    }
    if (capacity < BLE_GATT_ANDROID_NAME_LEN) {
        return -ENOSPC;
    }

    name[0] = 'M';
    name[1] = 'e';
    name[2] = 's';
    name[3] = 'h';
    name[4] = '-';

    for (size_t i = 0U; i < 3U; ++i) {
        uint8_t byte = node_id[i + 3U];
        name[5U + (i * 2U)] = hex_digits[byte >> 4U];
        name[6U + (i * 2U)] = hex_digits[byte & 0x0fU];
    }

    name[BLE_GATT_ANDROID_NAME_LEN - 1U] = '\0';
    return 0;
}
