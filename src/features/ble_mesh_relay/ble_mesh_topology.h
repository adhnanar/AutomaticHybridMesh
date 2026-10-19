#ifndef HYBRID_MESH_BLE_MESH_TOPOLOGY_H
#define HYBRID_MESH_BLE_MESH_TOPOLOGY_H

#include "ble_mesh_packet.h"

enum ble_mesh_topology_role {
    BLE_MESH_TOPOLOGY_ROLE_UNKNOWN,
    BLE_MESH_TOPOLOGY_ROLE_A,
    BLE_MESH_TOPOLOGY_ROLE_B,
    BLE_MESH_TOPOLOGY_ROLE_C,
};

enum ble_mesh_topology_filter_result {
    BLE_MESH_TOPOLOGY_NOT_APPLICABLE,
    BLE_MESH_TOPOLOGY_REJECTED,
    BLE_MESH_TOPOLOGY_DUPLICATE,
    BLE_MESH_TOPOLOGY_ACCEPTED,
};

enum ble_mesh_topology_role ble_mesh_topology_get_role(
    const uint8_t local_node_id[BLE_MESH_NODE_ID_LEN],
    const uint8_t relay_b_node_id[BLE_MESH_NODE_ID_LEN]);
bool ble_mesh_topology_can_originate_text(enum ble_mesh_topology_role role);
enum ble_mesh_topology_filter_result ble_mesh_topology_filter_text(
    enum ble_mesh_topology_role role, const struct ble_mesh_packet *packet,
    struct ble_mesh_dedup_cache *dedup_cache, uint32_t now_ms,
    bool *should_relay);

#endif
