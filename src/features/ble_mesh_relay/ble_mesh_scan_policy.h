#ifndef HYBRID_MESH_BLE_MESH_SCAN_POLICY_H
#define HYBRID_MESH_BLE_MESH_SCAN_POLICY_H

#include <zephyr/bluetooth/bluetooth.h>

/*
 * Zephyr's controller duplicate filter (hci.c dup_found/is_dup_or_update)
 * keys legacy ADV_IND on address and PDU type only, never on payload. With
 * BT_LE_SCAN_OPT_FILTER_DUPLICATE a node whose beacon was seen once never
 * reports its later TEXT frame, so the filter must stay off: the mesh
 * payload changes under a fixed advertiser address.
 */
#define BLE_MESH_TEXT_ADV_HOLD_MS 1000U
/* Longest legacy connectable interval used by BT_LE_ADV_CONN_FAST_1 (60 ms). */
#define BLE_MESH_ADV_INTERVAL_MAX_MS 60U
#define BLE_MESH_MIN_ADV_EVENTS_PER_HOLD 10U

static inline void ble_mesh_scan_param_init(struct bt_le_scan_param *param)
{
    *param = (struct bt_le_scan_param)BT_LE_SCAN_PARAM_INIT(
        BT_LE_SCAN_TYPE_PASSIVE, BT_LE_SCAN_OPT_NONE,
        BT_GAP_SCAN_FAST_INTERVAL_MIN, BT_GAP_SCAN_FAST_WINDOW);
}

#endif
