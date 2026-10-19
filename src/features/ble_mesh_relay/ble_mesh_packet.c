#include <errno.h>
#include <string.h>

#include "ble_mesh_packet.h"

static bool packet_is_valid(const struct ble_mesh_packet *packet)
{
    if (packet == NULL || packet->ttl == 0U ||
        packet->ttl > BLE_MESH_MAX_HOPS ||
        packet->payload_length > BLE_MESH_MAX_TEXT_LEN) {
        return false;
    }

    if (packet->type == BLE_MESH_PACKET_BEACON) {
        return packet->payload_length == 0U;
    }

    return packet->type == BLE_MESH_PACKET_TEXT &&
           packet->payload_length != 0U;
}

int ble_mesh_packet_encode(const struct ble_mesh_packet *packet,
                           uint8_t *frame, size_t frame_capacity,
                           size_t *frame_length)
{
    if (!packet_is_valid(packet) || frame == NULL || frame_length == NULL) {
        return -EINVAL;
    }

    size_t required_length = BLE_MESH_PACKET_HEADER_LEN +
                             packet->payload_length;
    if (frame_capacity < required_length) {
        return -EMSGSIZE;
    }

    frame[0] = (uint8_t)packet->type;
    memcpy(&frame[1], packet->source_id, BLE_MESH_NODE_ID_LEN);
    frame[7] = (uint8_t)(packet->sequence & 0xffU);
    frame[8] = (uint8_t)(packet->sequence >> 8);
    frame[9] = packet->ttl;
    if (packet->payload_length != 0U) {
        memcpy(&frame[BLE_MESH_PACKET_HEADER_LEN], packet->payload,
               packet->payload_length);
    }

    *frame_length = required_length;
    return 0;
}

int ble_mesh_packet_decode(const uint8_t *frame, size_t frame_length,
                           struct ble_mesh_packet *packet)
{
    if (frame == NULL || packet == NULL ||
        frame_length < BLE_MESH_PACKET_HEADER_LEN ||
        frame_length > BLE_MESH_PACKET_MAX_LEN) {
        return -EINVAL;
    }

    struct ble_mesh_packet parsed = {
        .type = (enum ble_mesh_packet_type)frame[0],
        .sequence = (uint16_t)frame[7] | ((uint16_t)frame[8] << 8),
        .ttl = frame[9],
        .payload_length =
            (uint8_t)(frame_length - BLE_MESH_PACKET_HEADER_LEN),
    };
    memcpy(parsed.source_id, &frame[1], BLE_MESH_NODE_ID_LEN);
    if (parsed.payload_length != 0U) {
        memcpy(parsed.payload, &frame[BLE_MESH_PACKET_HEADER_LEN],
               parsed.payload_length);
    }
    if (!packet_is_valid(&parsed)) {
        return -EINVAL;
    }

    *packet = parsed;
    return 0;
}

bool ble_mesh_packet_prepare_relay(struct ble_mesh_packet *packet)
{
    if (packet == NULL || packet->type != BLE_MESH_PACKET_TEXT ||
        packet->ttl <= 1U || packet->ttl > BLE_MESH_MAX_HOPS) {
        return false;
    }

    --packet->ttl;
    return true;
}

void ble_mesh_dedup_init(struct ble_mesh_dedup_cache *cache)
{
    if (cache != NULL) {
        memset(cache, 0, sizeof(*cache));
    }
}

bool ble_mesh_dedup_accept(struct ble_mesh_dedup_cache *cache,
                           const uint8_t source_id[BLE_MESH_NODE_ID_LEN],
                           uint16_t sequence, uint32_t now_ms)
{
    if (cache == NULL || source_id == NULL) {
        return false;
    }

    for (size_t i = 0U; i < BLE_MESH_DEDUP_CACHE_SIZE; ++i) {
        struct ble_mesh_dedup_entry *entry = &cache->entries[i];
        if (!entry->valid || entry->sequence != sequence ||
            memcmp(entry->source_id, source_id, BLE_MESH_NODE_ID_LEN) != 0) {
            continue;
        }

        if ((uint32_t)(now_ms - entry->seen_at_ms) <
            BLE_MESH_DEDUP_WINDOW_MS) {
            return false;
        }

        entry->seen_at_ms = now_ms;
        return true;
    }

    struct ble_mesh_dedup_entry *entry =
        &cache->entries[cache->next_entry];
    memcpy(entry->source_id, source_id, BLE_MESH_NODE_ID_LEN);
    entry->sequence = sequence;
    entry->seen_at_ms = now_ms;
    entry->valid = true;
    cache->next_entry = (cache->next_entry + 1U) % BLE_MESH_DEDUP_CACHE_SIZE;
    return true;
}
