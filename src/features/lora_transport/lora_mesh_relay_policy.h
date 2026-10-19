#ifndef HYBRID_MESH_LORA_RELAY_POLICY_H
#define HYBRID_MESH_LORA_RELAY_POLICY_H

#include "ble_mesh_packet.h"

enum lora_mesh_relay_result {
    LORA_MESH_RELAY_READY,
    LORA_MESH_RELAY_DUPLICATE,
    LORA_MESH_RELAY_NOT_FORWARDABLE,
    LORA_MESH_RELAY_INVALID,
};

enum lora_mesh_relay_result lora_mesh_prepare_relay(
    struct ble_mesh_packet *packet, struct ble_mesh_dedup_cache *shared_cache,
    uint32_t now_ms);

#endif
