#include <errno.h>

#include "lora_transport.h"

int lora_transport_send_packet(const struct lora_transport *transport,
                               const struct ble_mesh_packet *packet)
{
    if (transport == NULL || transport->ops == NULL ||
        transport->ops->send_frame == NULL) {
        return -EINVAL;
    }

    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t frame_length = 0U;
    int err = ble_mesh_packet_encode(packet, frame, sizeof(frame),
                                     &frame_length);
    if (err != 0) {
        return err;
    }

    return transport->ops->send_frame(transport->context, frame,
                                      frame_length);
}

int lora_transport_decode_packet(const uint8_t *frame, size_t frame_length,
                                 struct ble_mesh_packet *packet)
{
    return ble_mesh_packet_decode(frame, frame_length, packet);
}
