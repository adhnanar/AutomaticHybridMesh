#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "lora_mesh_relay_policy.h"
#include "lora_transport.h"
#include "lora_transport_mock.h"

static const struct ble_mesh_packet test_packet = {
    .type = BLE_MESH_PACKET_TEXT,
    .source_id = {1U, 2U, 3U, 4U, 5U, 6U},
    .sequence = 0x1234U,
    .ttl = BLE_MESH_MAX_HOPS,
    .payload = {'m', 'o', 'c', 'k'},
    .payload_length = 4U,
};

ZTEST(lora_transport, test_packet_serialization_uses_shared_wire_format)
{
    struct lora_transport_mock mock;
    struct lora_transport transport = {0};
    struct ble_mesh_packet decoded;
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t frame_length = 0U;

    lora_transport_mock_init(&mock);
    lora_transport_mock_bind(&mock, &transport);
    zassert_ok(lora_transport_send_packet(&transport, &test_packet));
    zassert_equal(mock.send_count, 1U);
    zassert_equal(mock.frame_count, 1U);
    zassert_ok(lora_transport_mock_read(&mock, 0U, frame, sizeof(frame),
                                        &frame_length));
    zassert_ok(lora_transport_decode_packet(frame, frame_length, &decoded));
    zassert_equal(decoded.type, test_packet.type);
    zassert_mem_equal(decoded.source_id, test_packet.source_id,
                      BLE_MESH_NODE_ID_LEN);
    zassert_equal(decoded.sequence, test_packet.sequence);
    zassert_equal(decoded.ttl, test_packet.ttl);
    zassert_equal(decoded.payload_length, test_packet.payload_length);
    zassert_mem_equal(decoded.payload, test_packet.payload,
                      test_packet.payload_length);
}

ZTEST(lora_transport, test_mock_propagates_send_failure_without_queuing)
{
    struct lora_transport_mock mock;
    struct lora_transport transport = {0};

    lora_transport_mock_init(&mock);
    lora_transport_mock_set_send_result(&mock, -EIO);
    lora_transport_mock_bind(&mock, &transport);
    zassert_equal(lora_transport_send_packet(&transport, &test_packet),
                  -EIO);
    zassert_equal(mock.send_count, 1U);
    zassert_equal(mock.frame_count, 0U);
}

ZTEST(lora_transport, test_shared_cache_blocks_duplicate_cross_radio_relay)
{
    struct ble_mesh_dedup_cache shared_cache;
    struct ble_mesh_packet ble_received = test_packet;
    struct ble_mesh_packet lora_received = test_packet;

    ble_mesh_dedup_init(&shared_cache);
    zassert_true(ble_mesh_dedup_accept(&shared_cache, ble_received.source_id,
                                       ble_received.sequence, 100U));
    zassert_equal(lora_mesh_prepare_relay(&lora_received, &shared_cache,
                                          101U),
                  LORA_MESH_RELAY_DUPLICATE);
    zassert_equal(lora_received.ttl, BLE_MESH_MAX_HOPS);
}

ZTEST(lora_transport, test_unique_lora_relay_decrements_ttl_once)
{
    struct ble_mesh_dedup_cache shared_cache;
    struct ble_mesh_packet packet = test_packet;
    struct ble_mesh_packet duplicate;

    ble_mesh_dedup_init(&shared_cache);
    zassert_equal(lora_mesh_prepare_relay(&packet, &shared_cache, 200U),
                  LORA_MESH_RELAY_READY);
    zassert_equal(packet.ttl, BLE_MESH_MAX_HOPS - 1U);

    duplicate = test_packet;
    zassert_equal(lora_mesh_prepare_relay(&duplicate, &shared_cache, 201U),
                  LORA_MESH_RELAY_DUPLICATE);
    zassert_equal(duplicate.ttl, BLE_MESH_MAX_HOPS);
}

ZTEST(lora_transport, test_nonforwardable_packet_is_unchanged)
{
    struct ble_mesh_dedup_cache shared_cache;
    struct ble_mesh_packet packet = test_packet;

    ble_mesh_dedup_init(&shared_cache);
    packet.ttl = 1U;
    zassert_equal(lora_mesh_prepare_relay(&packet, &shared_cache, 300U),
                  LORA_MESH_RELAY_NOT_FORWARDABLE);
    zassert_equal(packet.ttl, 1U);
    zassert_false(shared_cache.entries[0].valid);
}

ZTEST_SUITE(lora_transport, NULL, NULL, NULL, NULL, NULL);
