#include "ble_mesh_heartbeat.h"

#include <errno.h>
#include <stdio.h>

int ble_mesh_heartbeat_format(const struct ble_mesh_heartbeat *heartbeat,
                              char *buffer, size_t capacity)
{
    if (heartbeat == NULL || buffer == NULL || capacity == 0U) {
        return -EINVAL;
    }

    int body = snprintf(buffer, capacity,
                        "HB r=%c n=%u d=%u t=%u f=%u q=%u u=%u",
                        heartbeat->role, (unsigned int)heartbeat->index,
                        (unsigned int)heartbeat->decoded,
                        (unsigned int)heartbeat->text,
                        (unsigned int)heartbeat->decode_fail,
                        (unsigned int)heartbeat->rx_drop,
                        (unsigned int)heartbeat->uptime_ms);
    if (body < 0 || (size_t)body >= capacity) {
        return -EMSGSIZE;
    }

    uint8_t checksum = 0U;
    for (int i = 0; i < body; ++i) {
        checksum ^= (uint8_t)buffer[i];
    }

    int total = snprintf(buffer + body, capacity - (size_t)body,
                         " c=%02X\n", checksum);
    if (total < 0 || (size_t)(body + total) >= BLE_MESH_HEARTBEAT_MAX_LEN ||
        (size_t)(body + total) >= capacity) {
        return -EMSGSIZE;
    }
    return body + total;
}
