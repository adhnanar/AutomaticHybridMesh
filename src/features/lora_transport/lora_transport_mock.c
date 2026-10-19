#include <errno.h>
#include <string.h>

#include "lora_transport_mock.h"

static int mock_send_frame(void *context, const uint8_t *frame,
                           size_t frame_length)
{
    struct lora_transport_mock *mock = context;
    if (mock == NULL || frame == NULL || frame_length == 0U ||
        frame_length > BLE_MESH_PACKET_MAX_LEN) {
        return -EINVAL;
    }

    ++mock->send_count;
    if (mock->send_result != 0) {
        return mock->send_result;
    }
    if (mock->frame_count >= LORA_TRANSPORT_MOCK_FRAME_COUNT) {
        return -ENOSPC;
    }

    size_t index = mock->frame_count++;
    memcpy(mock->frames[index], frame, frame_length);
    mock->frame_lengths[index] = frame_length;
    return 0;
}

static const struct lora_transport_ops mock_ops = {
    .send_frame = mock_send_frame,
};

void lora_transport_mock_init(struct lora_transport_mock *mock)
{
    if (mock != NULL) {
        memset(mock, 0, sizeof(*mock));
    }
}

void lora_transport_mock_set_send_result(struct lora_transport_mock *mock,
                                         int result)
{
    if (mock != NULL) {
        mock->send_result = result;
    }
}

void lora_transport_mock_bind(struct lora_transport_mock *mock,
                              struct lora_transport *transport)
{
    if (mock != NULL && transport != NULL) {
        transport->ops = &mock_ops;
        transport->context = mock;
    }
}

int lora_transport_mock_read(const struct lora_transport_mock *mock,
                             size_t index, uint8_t *frame,
                             size_t frame_capacity, size_t *frame_length)
{
    if (mock == NULL || frame == NULL || frame_length == NULL ||
        index >= mock->frame_count) {
        return -EINVAL;
    }
    if (frame_capacity < mock->frame_lengths[index]) {
        return -EMSGSIZE;
    }

    memcpy(frame, mock->frames[index], mock->frame_lengths[index]);
    *frame_length = mock->frame_lengths[index];
    return 0;
}
