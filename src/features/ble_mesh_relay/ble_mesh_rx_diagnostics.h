#ifndef HYBRID_MESH_BLE_MESH_RX_DIAGNOSTICS_H
#define HYBRID_MESH_BLE_MESH_RX_DIAGNOSTICS_H

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/atomic.h>

struct ble_mesh_rx_diagnostic_counters {
    atomic_t manufacturer_decode_failures;
    atomic_t rx_queue_drops;
    atomic_t decoded_frames;
    atomic_t decoded_text_frames;
};

struct ble_mesh_rx_diagnostic_snapshot {
    uint32_t manufacturer_decode_failures;
    uint32_t rx_queue_drops;
    uint32_t decoded_frames;
    uint32_t decoded_text_frames;
};

/* Counts a frame decoded from a foreign node; is_text selects TEXT frames. */
void ble_mesh_rx_diagnostics_record_decoded(
    struct ble_mesh_rx_diagnostic_counters *counters, bool is_text);

void ble_mesh_rx_diagnostics_init(
    struct ble_mesh_rx_diagnostic_counters *counters);
void ble_mesh_rx_diagnostics_record_decode_failure(
    struct ble_mesh_rx_diagnostic_counters *counters);
void ble_mesh_rx_diagnostics_record_queue_drop(
    struct ble_mesh_rx_diagnostic_counters *counters);
void ble_mesh_rx_diagnostics_snapshot(
    const struct ble_mesh_rx_diagnostic_counters *counters,
    struct ble_mesh_rx_diagnostic_snapshot *snapshot);

#endif
