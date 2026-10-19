#ifndef HYBRID_MESH_TEST_HYBRID_POLICY_VECTORS_H
#define HYBRID_MESH_TEST_HYBRID_POLICY_VECTORS_H

#define HYBRID_POLICY_VECTORS(X) \
    X("fresh_ble_preferred", 100U, -60, true, true, 0, 0, \
      HYBRID_ROUTER_SENT, HYBRID_ROUTER_LINK_BLE) \
    X("ble_rssi_threshold_is_inclusive", 100U, -85, true, true, 0, 0, \
      HYBRID_ROUTER_SENT, HYBRID_ROUTER_LINK_BLE) \
    X("ble_freshness_boundary_is_inclusive", 10000U, -60, true, true, 0, 0, \
      HYBRID_ROUTER_SENT, HYBRID_ROUTER_LINK_BLE) \
    X("stale_ble_uses_authorized_lora", 10001U, -60, true, true, 0, 0, \
      HYBRID_ROUTER_SENT, HYBRID_ROUTER_LINK_LORA) \
    X("rssi_below_threshold_is_not_reachable", 100U, -86, true, true, 0, 0, \
      HYBRID_ROUTER_SENT, HYBRID_ROUTER_LINK_LORA) \
    X("weak_ble_uses_authorized_lora", 100U, -95, true, true, 0, 0, \
      HYBRID_ROUTER_SENT, HYBRID_ROUTER_LINK_LORA) \
    X("ble_failure_falls_back_to_lora", 100U, -60, true, true, -5, 0, \
      HYBRID_ROUTER_SENT_FALLBACK, HYBRID_ROUTER_LINK_LORA) \
    X("unauthorized_lora_is_blocked", 10001U, -60, true, false, 0, 0, \
      HYBRID_ROUTER_LORA_BLOCKED_QUEUED, HYBRID_ROUTER_LINK_NONE) \
    X("missing_lora_hardware_is_queued", 10001U, -60, false, false, 0, 0, \
      HYBRID_ROUTER_NO_LINK_QUEUED, HYBRID_ROUTER_LINK_NONE)

#endif
