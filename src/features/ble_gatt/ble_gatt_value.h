#ifndef HYBRID_MESH_BLE_GATT_VALUE_H
#define HYBRID_MESH_BLE_GATT_VALUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include "ble_gatt.h"

struct ble_gatt_text_value {
    uint8_t value[BLE_GATT_MAX_VALUE_LEN];
    uint16_t length;
};

typedef int (*ble_gatt_text_notify_fn)(const uint8_t *message,
                                       uint16_t length, void *context);

enum ble_gatt_rx_action {
    BLE_GATT_RX_ACTION_PUBLISH_TEXT,
    BLE_GATT_RX_ACTION_CONTROL_NO_TEXT,
    BLE_GATT_RX_ACTION_ERR_LENGTH,
    BLE_GATT_RX_ACTION_ERR_FAILED,
};

/* Maps an RX handler result to a GATT action; see BLE_GATT_RX_CONTROL_HANDLED. */
enum ble_gatt_rx_action ble_gatt_rx_classify(int handler_result,
                                             bool control_enabled);

int ble_gatt_text_value_publish(
    struct k_mutex *mutex, struct ble_gatt_text_value *stored,
    const uint8_t *message, size_t length,
    ble_gatt_text_notify_fn notify, void *context);

#endif
