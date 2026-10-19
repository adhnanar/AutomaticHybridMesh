#ifndef HYBRID_MESH_BLE_MESH_RSSI_VALIDATION_H
#define HYBRID_MESH_BLE_MESH_RSSI_VALIDATION_H

#include <stddef.h>
#include <stdint.h>

#include "ble_mesh_packet.h"

enum ble_mesh_rssi_validation_result {
    BLE_MESH_RSSI_VALIDATION_VALID,
    BLE_MESH_RSSI_VALIDATION_NOT_MESH,
    BLE_MESH_RSSI_VALIDATION_MALFORMED,
};

enum ble_mesh_rssi_validation_result ble_mesh_rssi_validation_decode(
    const uint8_t *manufacturer_data, size_t manufacturer_data_length,
    int8_t received_rssi, struct ble_mesh_packet *packet,
    int8_t *validated_rssi);

#endif
