#ifndef HYBRID_MESH_BLE_MESH_RELAY_QUEUE_H
#define HYBRID_MESH_BLE_MESH_RELAY_QUEUE_H

#include <zephyr/kernel.h>

#include "ble_mesh_packet.h"

typedef int (*ble_mesh_relay_update_fn)(
    const struct ble_mesh_packet *packet, void *context);

int ble_mesh_relay_queue_update_head(
    struct k_msgq *queue, ble_mesh_relay_update_fn update, void *context,
    struct ble_mesh_packet *packet, int *update_result);

#endif
