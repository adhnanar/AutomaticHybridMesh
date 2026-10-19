#ifndef HYBRID_MESH_BLE_GATT_ANDROID_NAME_H
#define HYBRID_MESH_BLE_GATT_ANDROID_NAME_H

#include <stddef.h>
#include <stdint.h>

#define BLE_GATT_ANDROID_NODE_ID_LEN 6U
#define BLE_GATT_ANDROID_NAME_LEN 12U

int ble_gatt_android_name_format(
    char *name, size_t capacity,
    const uint8_t node_id[BLE_GATT_ANDROID_NODE_ID_LEN]);

#endif
