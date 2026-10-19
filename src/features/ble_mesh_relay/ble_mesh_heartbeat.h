#ifndef HYBRID_MESH_BLE_MESH_HEARTBEAT_H
#define HYBRID_MESH_BLE_MESH_HEARTBEAT_H

#include <stddef.h>
#include <stdint.h>

/* One line must fit a single 64-byte USB full-speed packet incl. NUL. */
#define BLE_MESH_HEARTBEAT_MAX_LEN 64U

struct ble_mesh_heartbeat {
    char role;
    uint32_t index;
    uint32_t decoded;
    uint32_t text;
    uint32_t decode_fail;
    uint32_t rx_drop;
    uint32_t uptime_ms;
};

/*
 * Formats "HB r=B n=1 d=2 t=0 f=0 q=0 u=5000 c=XX\n" where c is the XOR of
 * every byte before " c=" as two hex digits. Returns the length written
 * (excluding NUL) or -EMSGSIZE if it would not fit one USB packet.
 */
int ble_mesh_heartbeat_format(const struct ble_mesh_heartbeat *heartbeat,
                              char *buffer, size_t capacity);

#endif
