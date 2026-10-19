#include <errno.h>

#include "ble_mesh_relay_queue.h"

int ble_mesh_relay_queue_update_head(
    struct k_msgq *queue, ble_mesh_relay_update_fn update, void *context,
    struct ble_mesh_packet *packet, int *update_result)
{
    if (queue == NULL || update == NULL || packet == NULL ||
        update_result == NULL) {
        return -EINVAL;
    }

    *update_result = -ECANCELED;
    int err = k_msgq_peek(queue, packet);
    if (err != 0) {
        return err;
    }

    *update_result = update(packet, context);
    if (*update_result != 0) {
        return 0;
    }

    struct ble_mesh_packet removed_packet;
    return k_msgq_get(queue, &removed_packet, K_NO_WAIT);
}
