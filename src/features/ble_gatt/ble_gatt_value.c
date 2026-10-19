#include <errno.h>
#include <string.h>

#include "ble_gatt_value.h"

enum ble_gatt_rx_action ble_gatt_rx_classify(int handler_result,
                                             bool control_enabled)
{
    if (handler_result == 0) {
        return BLE_GATT_RX_ACTION_PUBLISH_TEXT;
    }
    if (handler_result == -EMSGSIZE) {
        return BLE_GATT_RX_ACTION_ERR_LENGTH;
    }
    if (control_enabled && handler_result == BLE_GATT_RX_CONTROL_HANDLED) {
        return BLE_GATT_RX_ACTION_CONTROL_NO_TEXT;
    }
    return BLE_GATT_RX_ACTION_ERR_FAILED;
}

int ble_gatt_text_value_publish(
    struct k_mutex *mutex, struct ble_gatt_text_value *stored,
    const uint8_t *message, size_t length,
    ble_gatt_text_notify_fn notify, void *context)
{
    if (mutex == NULL || stored == NULL || message == NULL || notify == NULL ||
        length == 0U) {
        return -EINVAL;
    }
    if (length > BLE_GATT_MAX_VALUE_LEN) {
        return -EMSGSIZE;
    }

    uint8_t notification[BLE_GATT_MAX_VALUE_LEN];
    memcpy(notification, message, length);

    k_mutex_lock(mutex, K_FOREVER);
    memcpy(stored->value, notification, length);
    stored->length = (uint16_t)length;
    k_mutex_unlock(mutex);

    return notify(notification, (uint16_t)length, context);
}
