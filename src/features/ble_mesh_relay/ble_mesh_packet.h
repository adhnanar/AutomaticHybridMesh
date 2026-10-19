#ifndef HYBRID_MESH_BLE_MESH_PACKET_H
#define HYBRID_MESH_BLE_MESH_PACKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BLE_MESH_NODE_ID_LEN 6U
#define BLE_MESH_PACKET_HEADER_LEN 10U
#define BLE_MESH_PACKET_MAX_LEN 24U
#define BLE_MESH_MAX_TEXT_LEN \
    (BLE_MESH_PACKET_MAX_LEN - BLE_MESH_PACKET_HEADER_LEN)
#define BLE_MESH_MAX_HOPS 3U
#define BLE_MESH_DEDUP_CACHE_SIZE 16U
#define BLE_MESH_DEDUP_WINDOW_MS 30000U

enum ble_mesh_packet_type {
    BLE_MESH_PACKET_BEACON = 0x11U,
    BLE_MESH_PACKET_TEXT = 0x12U,
};

struct ble_mesh_packet {
    enum ble_mesh_packet_type type;
    uint8_t source_id[BLE_MESH_NODE_ID_LEN];
    uint16_t sequence;
    uint8_t ttl;
    uint8_t payload[BLE_MESH_MAX_TEXT_LEN];
    uint8_t payload_length;
};

struct ble_mesh_dedup_entry {
    uint8_t source_id[BLE_MESH_NODE_ID_LEN];
    uint16_t sequence;
    uint32_t seen_at_ms;
    bool valid;
};

struct ble_mesh_dedup_cache {
    struct ble_mesh_dedup_entry entries[BLE_MESH_DEDUP_CACHE_SIZE];
    size_t next_entry;
};

int ble_mesh_packet_encode(const struct ble_mesh_packet *packet,
                           uint8_t *frame, size_t frame_capacity,
                           size_t *frame_length);
int ble_mesh_packet_decode(const uint8_t *frame, size_t frame_length,
                           struct ble_mesh_packet *packet);
bool ble_mesh_packet_prepare_relay(struct ble_mesh_packet *packet);
void ble_mesh_dedup_init(struct ble_mesh_dedup_cache *cache);
bool ble_mesh_dedup_accept(struct ble_mesh_dedup_cache *cache,
                           const uint8_t source_id[BLE_MESH_NODE_ID_LEN],
                           uint16_t sequence, uint32_t now_ms);

#endif
