#ifndef HYBRID_MESH_BLE_MESH_RELAY_H
#define HYBRID_MESH_BLE_MESH_RELAY_H

#include <stddef.h>
#include <stdint.h>

int ble_mesh_relay_init(void);
int ble_mesh_relay_handle_phone_write(const uint8_t *value, size_t length);

#endif
