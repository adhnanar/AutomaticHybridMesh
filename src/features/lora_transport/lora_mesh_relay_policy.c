#include <stdbool.h>

#include "lora_mesh_relay_policy.h"

enum lora_mesh_relay_result lora_mesh_prepare_relay(
    struct ble_mesh_packet *packet, struct ble_mesh_dedup_cache *shared_cache,
    uint32_t now_ms)
{
    if (packet == NULL || shared_cache == NULL) {
        return LORA_MESH_RELAY_INVALID;
    }

    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t frame_length = 0U;
    if (ble_mesh_packet_encode(packet, frame, sizeof(frame),
                               &frame_length) != 0) {
        return LORA_MESH_RELAY_INVALID;
    }

    if (packet->type != BLE_MESH_PACKET_TEXT || packet->ttl <= 1U) {
        return LORA_MESH_RELAY_NOT_FORWARDABLE;
    }

    if (!ble_mesh_dedup_accept(shared_cache, packet->source_id,
                               packet->sequence, now_ms)) {
        return LORA_MESH_RELAY_DUPLICATE;
    }

    if (!ble_mesh_packet_prepare_relay(packet)) {
        return LORA_MESH_RELAY_NOT_FORWARDABLE;
    }

    return LORA_MESH_RELAY_READY;
}
