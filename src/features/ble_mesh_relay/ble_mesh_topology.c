#include <string.h>

#include "ble_mesh_topology.h"

static const uint8_t node_a_id[BLE_MESH_NODE_ID_LEN] = {
    0xebU, 0x34U, 0xedU, 0x25U, 0xa5U, 0xa4U,
};
static const uint8_t possible_b_or_c_ids[][BLE_MESH_NODE_ID_LEN] = {
    {0x0cU, 0x13U, 0xdaU, 0xe1U, 0x25U, 0xc8U},
    {0xeeU, 0x07U, 0x14U, 0x5aU, 0x5eU, 0x02U},
};

enum ble_mesh_topology_role ble_mesh_topology_get_role(
    const uint8_t local_node_id[BLE_MESH_NODE_ID_LEN],
    const uint8_t relay_b_node_id[BLE_MESH_NODE_ID_LEN])
{
    if (local_node_id == NULL || relay_b_node_id == NULL) {
        return BLE_MESH_TOPOLOGY_ROLE_UNKNOWN;
    }

    bool valid_b_id = false;
    for (size_t i = 0U; i < sizeof(possible_b_or_c_ids) /
                                sizeof(possible_b_or_c_ids[0]); ++i) {
        valid_b_id = valid_b_id ||
                     memcmp(relay_b_node_id, possible_b_or_c_ids[i],
                            BLE_MESH_NODE_ID_LEN) == 0;
    }
    if (!valid_b_id) {
        return BLE_MESH_TOPOLOGY_ROLE_UNKNOWN;
    }
    if (memcmp(local_node_id, node_a_id, BLE_MESH_NODE_ID_LEN) == 0) {
        return BLE_MESH_TOPOLOGY_ROLE_A;
    }
    if (memcmp(local_node_id, relay_b_node_id, BLE_MESH_NODE_ID_LEN) == 0) {
        return BLE_MESH_TOPOLOGY_ROLE_B;
    }
    for (size_t i = 0U; i < sizeof(possible_b_or_c_ids) /
                                sizeof(possible_b_or_c_ids[0]); ++i) {
        if (memcmp(local_node_id, possible_b_or_c_ids[i],
                   BLE_MESH_NODE_ID_LEN) == 0) {
            return BLE_MESH_TOPOLOGY_ROLE_C;
        }
    }
    return BLE_MESH_TOPOLOGY_ROLE_UNKNOWN;
}

bool ble_mesh_topology_can_originate_text(enum ble_mesh_topology_role role)
{
    return role == BLE_MESH_TOPOLOGY_ROLE_A;
}

enum ble_mesh_topology_filter_result ble_mesh_topology_filter_text(
    enum ble_mesh_topology_role role, const struct ble_mesh_packet *packet,
    struct ble_mesh_dedup_cache *dedup_cache, uint32_t now_ms,
    bool *should_relay)
{
    if (should_relay != NULL) {
        *should_relay = false;
    }
    if (packet == NULL || packet->type != BLE_MESH_PACKET_TEXT) {
        return BLE_MESH_TOPOLOGY_NOT_APPLICABLE;
    }
    if (dedup_cache == NULL || should_relay == NULL ||
        memcmp(packet->source_id, node_a_id, BLE_MESH_NODE_ID_LEN) != 0) {
        return BLE_MESH_TOPOLOGY_REJECTED;
    }

    bool relay = false;
    switch (role) {
    case BLE_MESH_TOPOLOGY_ROLE_B:
        if (packet->ttl != BLE_MESH_MAX_HOPS) {
            return BLE_MESH_TOPOLOGY_REJECTED;
        }
        relay = true;
        break;
    case BLE_MESH_TOPOLOGY_ROLE_C:
        if (packet->ttl != (BLE_MESH_MAX_HOPS - 1U)) {
            return BLE_MESH_TOPOLOGY_REJECTED;
        }
        break;
    default:
        return BLE_MESH_TOPOLOGY_REJECTED;
    }

    if (!ble_mesh_dedup_accept(dedup_cache, packet->source_id,
                               packet->sequence, now_ms)) {
        return BLE_MESH_TOPOLOGY_DUPLICATE;
    }
    *should_relay = relay;
    return BLE_MESH_TOPOLOGY_ACCEPTED;
}
