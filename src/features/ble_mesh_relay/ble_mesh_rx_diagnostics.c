#include "ble_mesh_rx_diagnostics.h"

void ble_mesh_rx_diagnostics_init(
    struct ble_mesh_rx_diagnostic_counters *counters)
{
    atomic_set(&counters->manufacturer_decode_failures, 0);
    atomic_set(&counters->rx_queue_drops, 0);
    atomic_set(&counters->decoded_frames, 0);
    atomic_set(&counters->decoded_text_frames, 0);
}

void ble_mesh_rx_diagnostics_record_decoded(
    struct ble_mesh_rx_diagnostic_counters *counters, bool is_text)
{
    atomic_inc(&counters->decoded_frames);
    if (is_text) {
        atomic_inc(&counters->decoded_text_frames);
    }
}

void ble_mesh_rx_diagnostics_record_decode_failure(
    struct ble_mesh_rx_diagnostic_counters *counters)
{
    atomic_inc(&counters->manufacturer_decode_failures);
}

void ble_mesh_rx_diagnostics_record_queue_drop(
    struct ble_mesh_rx_diagnostic_counters *counters)
{
    atomic_inc(&counters->rx_queue_drops);
}

void ble_mesh_rx_diagnostics_snapshot(
    const struct ble_mesh_rx_diagnostic_counters *counters,
    struct ble_mesh_rx_diagnostic_snapshot *snapshot)
{
    snapshot->manufacturer_decode_failures =
        (uint32_t)atomic_get(&counters->manufacturer_decode_failures);
    snapshot->rx_queue_drops =
        (uint32_t)atomic_get(&counters->rx_queue_drops);
    snapshot->decoded_frames =
        (uint32_t)atomic_get(&counters->decoded_frames);
    snapshot->decoded_text_frames =
        (uint32_t)atomic_get(&counters->decoded_text_frames);
}
