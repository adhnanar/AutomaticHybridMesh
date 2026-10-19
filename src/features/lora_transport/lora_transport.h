#ifndef HYBRID_MESH_LORA_TRANSPORT_H
#define HYBRID_MESH_LORA_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "ble_mesh_packet.h"

struct lora_transport_ops {
    int (*send_frame)(void *context, const uint8_t *frame,
                      size_t frame_length);
};

struct lora_transport {
    const struct lora_transport_ops *ops;
    void *context;
};

int lora_transport_send_packet(const struct lora_transport *transport,
                               const struct ble_mesh_packet *packet);
int lora_transport_decode_packet(const uint8_t *frame, size_t frame_length,
                                 struct ble_mesh_packet *packet);

#endif
