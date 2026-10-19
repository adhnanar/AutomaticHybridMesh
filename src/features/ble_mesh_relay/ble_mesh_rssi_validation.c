#include <zephyr/sys/byteorder.h>

#include "ble_mesh_rssi_validation.h"

#define BLE_MESH_MANUFACTURER_ID 0xffffU
#define BLE_MESH_MANUFACTURER_ID_LEN 2U

enum ble_mesh_rssi_validation_result ble_mesh_rssi_validation_decode(
    const uint8_t *manufacturer_data, size_t manufacturer_data_length,
    int8_t received_rssi, struct ble_mesh_packet *packet,
    int8_t *validated_rssi)
{
    if (manufacturer_data == NULL || packet == NULL ||
        validated_rssi == NULL ||
        manufacturer_data_length < BLE_MESH_MANUFACTURER_ID_LEN) {
        return BLE_MESH_RSSI_VALIDATION_MALFORMED;
    }
    if (sys_get_le16(manufacturer_data) != BLE_MESH_MANUFACTURER_ID) {
        return BLE_MESH_RSSI_VALIDATION_NOT_MESH;
    }

    struct ble_mesh_packet decoded_packet;
    if (ble_mesh_packet_decode(
            &manufacturer_data[BLE_MESH_MANUFACTURER_ID_LEN],
            manufacturer_data_length - BLE_MESH_MANUFACTURER_ID_LEN,
            &decoded_packet) != 0) {
        return BLE_MESH_RSSI_VALIDATION_MALFORMED;
    }

    *packet = decoded_packet;
    *validated_rssi = received_rssi;
    return BLE_MESH_RSSI_VALIDATION_VALID;
}
