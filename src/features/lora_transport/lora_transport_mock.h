#ifndef HYBRID_MESH_LORA_TRANSPORT_MOCK_H
#define HYBRID_MESH_LORA_TRANSPORT_MOCK_H

#include <stddef.h>
#include <stdint.h>

#include "lora_transport.h"

#define LORA_TRANSPORT_MOCK_FRAME_COUNT 4U

struct lora_transport_mock {
    uint8_t frames[LORA_TRANSPORT_MOCK_FRAME_COUNT]
                  [BLE_MESH_PACKET_MAX_LEN];
    size_t frame_lengths[LORA_TRANSPORT_MOCK_FRAME_COUNT];
    size_t frame_count;
    size_t send_count;
    int send_result;
};

void lora_transport_mock_init(struct lora_transport_mock *mock);
void lora_transport_mock_set_send_result(struct lora_transport_mock *mock,
                                         int result);
void lora_transport_mock_bind(struct lora_transport_mock *mock,
                              struct lora_transport *transport);
int lora_transport_mock_read(const struct lora_transport_mock *mock,
                             size_t index, uint8_t *frame,
                             size_t frame_capacity, size_t *frame_length);

#endif
