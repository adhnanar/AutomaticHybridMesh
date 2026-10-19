#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "ble_gatt_android_name.h"
#include "ble_gatt_value.h"
#include "ble_mesh_heartbeat.h"
#include "ble_mesh_evidence.h"
#include "ble_mesh_packet.h"
#include "ble_mesh_rssi_validation.h"
#include "ble_mesh_scan_policy.h"
#include "ble_mesh_relay_queue.h"
#include "ble_mesh_rx_diagnostics.h"
#include "ble_mesh_topology.h"

K_MSGQ_DEFINE(test_relay_queue, sizeof(struct ble_mesh_packet), 2U, 4U);
K_MSGQ_DEFINE(test_rx_queue, sizeof(struct ble_mesh_packet), 1U, 4U);

static const uint8_t topology_node_a[BLE_MESH_NODE_ID_LEN] = {
    0xebU, 0x34U, 0xedU, 0x25U, 0xa5U, 0xa4U,
};
static const uint8_t topology_node_b[BLE_MESH_NODE_ID_LEN] = {
    0x0cU, 0x13U, 0xdaU, 0xe1U, 0x25U, 0xc8U,
};
static const uint8_t topology_node_c[BLE_MESH_NODE_ID_LEN] = {
    0xeeU, 0x07U, 0x14U, 0x5aU, 0x5eU, 0x02U,
};

struct fake_advertiser {
    int results[2];
    size_t calls;
};

struct fake_notification {
    int result;
    size_t calls;
    uint8_t value[BLE_GATT_MAX_VALUE_LEN];
    uint16_t length;
};

static int fake_advertise(const struct ble_mesh_packet *packet,
                          void *context)
{
    struct fake_advertiser *advertiser = context;
    ARG_UNUSED(packet);
    return advertiser->results[advertiser->calls++];
}

static int fake_notify(const uint8_t *message, uint16_t length, void *context)
{
    struct fake_notification *notification = context;
    memcpy(notification->value, message, length);
    notification->length = length;
    ++notification->calls;
    return notification->result;
}

ZTEST(ble_mesh_packet, test_relay_queue_retains_head_on_eagain_and_retries)
{
    const struct ble_mesh_packet original = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {0xebU, 0x34U, 0xedU, 0x25U, 0xa5U, 0xa4U},
        .sequence = 47U,
        .ttl = BLE_MESH_MAX_HOPS - 1U,
        .payload = {'r', 'e', 'l', 'a', 'y'},
        .payload_length = 5U,
    };
    struct ble_mesh_packet packet;
    struct ble_mesh_packet queued;
    struct fake_advertiser advertiser = {
        .results = {-EAGAIN, 0},
    };
    int update_result = 0;
    k_msgq_purge(&test_relay_queue);
    zassert_ok(k_msgq_put(&test_relay_queue, &original, K_NO_WAIT));

    zassert_ok(ble_mesh_relay_queue_update_head(
        &test_relay_queue, fake_advertise, &advertiser, &packet,
        &update_result));
    zassert_equal(update_result, -EAGAIN);
    zassert_equal(advertiser.calls, 1U);
    zassert_equal(k_msgq_num_used_get(&test_relay_queue), 1U);
    zassert_ok(k_msgq_peek(&test_relay_queue, &queued));
    zassert_mem_equal(&queued, &original, sizeof(original));

    zassert_ok(ble_mesh_relay_queue_update_head(
        &test_relay_queue, fake_advertise, &advertiser, &packet,
        &update_result));
    zassert_equal(update_result, 0);
    zassert_equal(advertiser.calls, 2U);
    zassert_equal(k_msgq_num_used_get(&test_relay_queue), 0U);
    zassert_equal(ble_mesh_relay_queue_update_head(
                      &test_relay_queue, fake_advertise, &advertiser,
                      &packet, &update_result),
                  -ENOMSG);
    zassert_equal(advertiser.calls, 2U);
}

ZTEST(ble_mesh_packet, test_rx_failures_count_without_false_accept)
{
    struct ble_mesh_rx_diagnostic_counters counters;
    struct ble_mesh_rx_diagnostic_snapshot snapshot;
    struct ble_mesh_packet decoded;
    const uint8_t malformed_manufacturer[] = {0xffU, 0xffU, 0xffU};
    const struct ble_mesh_packet valid_packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {1U, 2U, 3U, 4U, 5U, 6U},
        .sequence = 9U,
        .ttl = BLE_MESH_MAX_HOPS,
        .payload = {'x'},
        .payload_length = 1U,
    };
    struct ble_mesh_packet second_packet = valid_packet;
    uint8_t encoded[BLE_MESH_PACKET_MAX_LEN];
    size_t encoded_length = 0U;

    ble_mesh_rx_diagnostics_init(&counters);
    zassert_equal(ble_mesh_packet_decode(&malformed_manufacturer[2],
                                         sizeof(malformed_manufacturer) - 2U,
                                         &decoded),
                  -EINVAL);
    ble_mesh_rx_diagnostics_record_decode_failure(&counters);

    zassert_ok(ble_mesh_packet_encode(&valid_packet, encoded, sizeof(encoded),
                                      &encoded_length));
    zassert_ok(ble_mesh_packet_decode(encoded, encoded_length, &decoded));
    zassert_ok(k_msgq_put(&test_rx_queue, &decoded, K_NO_WAIT));
    second_packet.sequence++;
    zassert_equal(k_msgq_put(&test_rx_queue, &second_packet, K_NO_WAIT),
                  -ENOMSG);
    ble_mesh_rx_diagnostics_record_queue_drop(&counters);
    ble_mesh_rx_diagnostics_record_decoded(&counters, true);
    ble_mesh_rx_diagnostics_record_decoded(&counters, false);

    ble_mesh_rx_diagnostics_snapshot(&counters, &snapshot);
    zassert_equal(snapshot.manufacturer_decode_failures, 1U);
    zassert_equal(snapshot.rx_queue_drops, 1U);
    zassert_equal(snapshot.decoded_frames, 2U);
    zassert_equal(snapshot.decoded_text_frames, 1U);
    zassert_equal(k_msgq_num_used_get(&test_rx_queue), 1U);
    k_msgq_purge(&test_rx_queue);
}

ZTEST(ble_mesh_packet, test_rssi_requires_valid_mesh_and_uses_received_value)
{
    const struct ble_mesh_packet expected_packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {1U, 2U, 3U, 4U, 5U, 6U},
        .sequence = 17U,
        .ttl = BLE_MESH_MAX_HOPS,
        .payload = {'r'},
        .payload_length = 1U,
    };
    const uint8_t non_mesh_data[] = {0x34U, 0x12U, 0U};
    const uint8_t short_manufacturer_data[] = {0xffU};
    const uint8_t malformed_mesh_data[] = {
        0xffU, 0xffU, (uint8_t)BLE_MESH_PACKET_TEXT,
    };
    uint8_t manufacturer_data[2U + BLE_MESH_PACKET_MAX_LEN] = {
        0xffU, 0xffU,
    };
    struct ble_mesh_packet decoded_packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {9U, 8U, 7U, 6U, 5U, 4U},
        .sequence = 99U,
        .ttl = 2U,
        .payload = {'s'},
        .payload_length = 1U,
    };
    const struct ble_mesh_packet unchanged_packet = decoded_packet;
    int8_t validated_rssi = 7;
    size_t frame_length = 0U;

    zassert_equal(ble_mesh_rssi_validation_decode(
                      NULL, 0U, -80, &decoded_packet, &validated_rssi),
                  BLE_MESH_RSSI_VALIDATION_MALFORMED);
    zassert_equal(validated_rssi, 7);
    zassert_mem_equal(&decoded_packet, &unchanged_packet,
                      sizeof(decoded_packet));

    zassert_equal(ble_mesh_rssi_validation_decode(
                      short_manufacturer_data,
                      sizeof(short_manufacturer_data), -80,
                      &decoded_packet, &validated_rssi),
                  BLE_MESH_RSSI_VALIDATION_MALFORMED);
    zassert_equal(validated_rssi, 7);
    zassert_mem_equal(&decoded_packet, &unchanged_packet,
                      sizeof(decoded_packet));

    zassert_equal(ble_mesh_rssi_validation_decode(
                      non_mesh_data, sizeof(non_mesh_data), -81,
                      &decoded_packet, &validated_rssi),
                  BLE_MESH_RSSI_VALIDATION_NOT_MESH);
    zassert_equal(validated_rssi, 7);
    zassert_mem_equal(&decoded_packet, &unchanged_packet,
                      sizeof(decoded_packet));

    zassert_equal(ble_mesh_rssi_validation_decode(
                      malformed_mesh_data, sizeof(malformed_mesh_data), -82,
                      &decoded_packet, &validated_rssi),
                  BLE_MESH_RSSI_VALIDATION_MALFORMED);
    zassert_equal(validated_rssi, 7);
    zassert_mem_equal(&decoded_packet, &unchanged_packet,
                      sizeof(decoded_packet));

    zassert_ok(ble_mesh_packet_encode(
        &expected_packet, &manufacturer_data[2],
        sizeof(manufacturer_data) - 2U, &frame_length));
    zassert_equal(ble_mesh_rssi_validation_decode(
                      manufacturer_data, frame_length + 2U, -84,
                      NULL, &validated_rssi),
                  BLE_MESH_RSSI_VALIDATION_MALFORMED);
    zassert_equal(validated_rssi, 7);
    zassert_equal(ble_mesh_rssi_validation_decode(
                      manufacturer_data, frame_length + 2U, -84,
                      &decoded_packet, NULL),
                  BLE_MESH_RSSI_VALIDATION_MALFORMED);
    zassert_mem_equal(&decoded_packet, &unchanged_packet,
                      sizeof(decoded_packet));
    zassert_equal(ble_mesh_rssi_validation_decode(
                      manufacturer_data, frame_length + 2U, -73,
                      &decoded_packet, &validated_rssi),
                  BLE_MESH_RSSI_VALIDATION_VALID);
    zassert_equal(validated_rssi, -73);
    zassert_equal(ble_mesh_rssi_validation_decode(
                      manufacturer_data, frame_length + 2U, -85,
                      &decoded_packet, &validated_rssi),
                  BLE_MESH_RSSI_VALIDATION_VALID);
    zassert_equal(validated_rssi, -85);
    zassert_equal(ble_mesh_rssi_validation_decode(
                      manufacturer_data, frame_length + 2U, -86,
                      &decoded_packet, &validated_rssi),
                  BLE_MESH_RSSI_VALIDATION_VALID);
    zassert_equal(validated_rssi, -86);
    zassert_equal(decoded_packet.type, expected_packet.type);
    zassert_mem_equal(decoded_packet.source_id, expected_packet.source_id,
                      BLE_MESH_NODE_ID_LEN);
    zassert_equal(decoded_packet.sequence, expected_packet.sequence);
    zassert_equal(decoded_packet.ttl, expected_packet.ttl);
    zassert_equal(decoded_packet.payload_length,
                  expected_packet.payload_length);
    zassert_mem_equal(decoded_packet.payload, expected_packet.payload,
                      expected_packet.payload_length);
}

ZTEST(ble_mesh_packet, test_gatt_text_storage_does_not_depend_on_notify)
{
    struct k_mutex mutex;
    struct ble_gatt_text_value value = {
        .value = "Ready",
        .length = sizeof("Ready") - 1U,
    };
    struct fake_notification notification = {
        .result = -ENOTCONN,
    };
    static const uint8_t accepted_text[] = "accepted";

    k_mutex_init(&mutex);
    zassert_equal(ble_gatt_text_value_publish(
                      &mutex, &value, accepted_text,
                      sizeof(accepted_text) - 1U, fake_notify, &notification),
                  -ENOTCONN);
    zassert_equal(notification.calls, 1U);
    zassert_equal(notification.length, sizeof(accepted_text) - 1U);
    zassert_mem_equal(notification.value, accepted_text,
                      sizeof(accepted_text) - 1U);
    zassert_equal(value.length, sizeof(accepted_text) - 1U);
    zassert_mem_equal(value.value, accepted_text,
                      sizeof(accepted_text) - 1U);
}

ZTEST(ble_mesh_packet, test_gatt_rx_control_result_policy)
{
    /* Ordinary text and errors behave identically with the flag on or off. */
    for (int enabled = 0; enabled <= 1; ++enabled) {
        zassert_equal(ble_gatt_rx_classify(0, enabled),
                      BLE_GATT_RX_ACTION_PUBLISH_TEXT);
        zassert_equal(ble_gatt_rx_classify(-EMSGSIZE, enabled),
                      BLE_GATT_RX_ACTION_ERR_LENGTH);
        zassert_equal(ble_gatt_rx_classify(-EACCES, enabled),
                      BLE_GATT_RX_ACTION_ERR_FAILED);
        zassert_equal(ble_gatt_rx_classify(-EAGAIN, enabled),
                      BLE_GATT_RX_ACTION_ERR_FAILED);
        zassert_equal(ble_gatt_rx_classify(2, enabled),
                      BLE_GATT_RX_ACTION_ERR_FAILED);
    }
    zassert_equal(ble_gatt_rx_classify(BLE_GATT_RX_CONTROL_HANDLED, true),
                  BLE_GATT_RX_ACTION_CONTROL_NO_TEXT);
    /* Guard: default-off keeps the legacy "non-zero is failure" rule. */
    zassert_equal(ble_gatt_rx_classify(BLE_GATT_RX_CONTROL_HANDLED, false),
                  BLE_GATT_RX_ACTION_ERR_FAILED);
}

ZTEST(ble_mesh_packet, test_gatt_android_discovery_name_uses_node_id_suffix)
{
    const uint8_t node_id[BLE_GATT_ANDROID_NODE_ID_LEN] = {
        0xebU, 0x34U, 0xedU, 0x25U, 0xa5U, 0xc8U,
    };
    char name[BLE_GATT_ANDROID_NAME_LEN];

    zassert_ok(ble_gatt_android_name_format(name, sizeof(name), node_id));
    zassert_equal(strcmp(name, "Mesh-25A5C8"), 0);
}

ZTEST(ble_mesh_packet, test_gatt_android_discovery_name_validates_capacity)
{
    const uint8_t node_id[BLE_GATT_ANDROID_NODE_ID_LEN] = {0U};
    char undersized_name[BLE_GATT_ANDROID_NAME_LEN - 1U];
    char name[BLE_GATT_ANDROID_NAME_LEN];

    zassert_equal(ble_gatt_android_name_format(
                      undersized_name, sizeof(undersized_name), node_id),
                  -ENOSPC);
    zassert_equal(ble_gatt_android_name_format(NULL, BLE_GATT_ANDROID_NAME_LEN,
                                               node_id),
                  -EINVAL);
    zassert_equal(ble_gatt_android_name_format(name, BLE_GATT_ANDROID_NAME_LEN,
                                               NULL),
                  -EINVAL);
}

ZTEST(ble_mesh_packet, test_text_packet_round_trip)
{
    const struct ble_mesh_packet original = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {1U, 2U, 3U, 4U, 5U, 6U},
        .sequence = 0x1234U,
        .ttl = BLE_MESH_MAX_HOPS,
        .payload = {'h', 'e', 'l', 'l', 'o'},
        .payload_length = 5U,
    };
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t frame_length = 0U;
    struct ble_mesh_packet decoded;

    zassert_ok(ble_mesh_packet_encode(&original, frame, sizeof(frame),
                                      &frame_length));
    zassert_equal(frame_length, BLE_MESH_PACKET_HEADER_LEN + 5U);
    zassert_ok(ble_mesh_packet_decode(frame, frame_length, &decoded));
    zassert_equal(decoded.type, original.type);
    zassert_mem_equal(decoded.source_id, original.source_id,
                      BLE_MESH_NODE_ID_LEN);
    zassert_equal(decoded.sequence, original.sequence);
    zassert_equal(decoded.ttl, original.ttl);
    zassert_equal(decoded.payload_length, original.payload_length);
    zassert_mem_equal(decoded.payload, original.payload,
                      original.payload_length);
}

ZTEST(ble_mesh_packet, test_invalid_packet_bounds_and_ttl)
{
    struct ble_mesh_packet packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {1U, 2U, 3U, 4U, 5U, 6U},
        .sequence = 1U,
        .ttl = 0U,
        .payload = {'x'},
        .payload_length = 1U,
    };
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t frame_length = 0U;

    zassert_equal(ble_mesh_packet_encode(&packet, frame, sizeof(frame),
                                         &frame_length), -EINVAL);

    packet.ttl = BLE_MESH_MAX_HOPS;
    packet.payload_length = BLE_MESH_MAX_TEXT_LEN + 1U;
    zassert_equal(ble_mesh_packet_encode(&packet, frame, sizeof(frame),
                                         &frame_length), -EINVAL);

    packet.payload_length = 1U;
    zassert_equal(ble_mesh_packet_encode(&packet, frame,
                                         BLE_MESH_PACKET_HEADER_LEN,
                                         &frame_length), -EMSGSIZE);
    zassert_equal(ble_mesh_packet_decode(frame, BLE_MESH_PACKET_MAX_LEN + 1U,
                                         &packet), -EINVAL);
}

ZTEST(ble_mesh_packet, test_beacon_requires_empty_payload)
{
    struct ble_mesh_packet packet = {
        .type = BLE_MESH_PACKET_BEACON,
        .source_id = {1U, 2U, 3U, 4U, 5U, 6U},
        .sequence = 1U,
        .ttl = 1U,
        .payload = {'x'},
        .payload_length = 1U,
    };
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t frame_length = 0U;

    zassert_equal(ble_mesh_packet_encode(&packet, frame, sizeof(frame),
                                         &frame_length), -EINVAL);
}

ZTEST(ble_mesh_packet, test_parser_rejects_unknown_type_and_ttl)
{
    struct ble_mesh_packet packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {1U, 2U, 3U, 4U, 5U, 6U},
        .sequence = 1U,
        .ttl = BLE_MESH_MAX_HOPS,
        .payload = {'x'},
        .payload_length = 1U,
    };
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t frame_length = 0U;

    zassert_ok(ble_mesh_packet_encode(&packet, frame, sizeof(frame),
                                      &frame_length));
    frame[0] = 0xffU;
    zassert_equal(ble_mesh_packet_decode(frame, frame_length, &packet),
                  -EINVAL);

    frame[0] = BLE_MESH_PACKET_TEXT;
    frame[9] = 0U;
    zassert_equal(ble_mesh_packet_decode(frame, frame_length, &packet),
                  -EINVAL);
    frame[9] = BLE_MESH_MAX_HOPS + 1U;
    zassert_equal(ble_mesh_packet_decode(frame, frame_length, &packet),
                  -EINVAL);
}

ZTEST(ble_mesh_packet, test_relay_ttl_stops_at_one)
{
    struct ble_mesh_packet packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .ttl = BLE_MESH_MAX_HOPS,
    };

    zassert_true(ble_mesh_packet_prepare_relay(&packet));
    zassert_equal(packet.ttl, BLE_MESH_MAX_HOPS - 1U);
    zassert_true(ble_mesh_packet_prepare_relay(&packet));
    zassert_equal(packet.ttl, 1U);
    zassert_false(ble_mesh_packet_prepare_relay(&packet));
    zassert_equal(packet.ttl, 1U);

    packet.type = BLE_MESH_PACKET_BEACON;
    zassert_false(ble_mesh_packet_prepare_relay(&packet));
}

ZTEST(ble_mesh_packet, test_deduplication_and_expiry)
{
    static const uint8_t node_a[BLE_MESH_NODE_ID_LEN] = {1U, 2U, 3U, 4U, 5U, 6U};
    static const uint8_t node_b[BLE_MESH_NODE_ID_LEN] = {6U, 5U, 4U, 3U, 2U, 1U};
    struct ble_mesh_dedup_cache cache;

    ble_mesh_dedup_init(&cache);
    zassert_true(ble_mesh_dedup_accept(&cache, node_a, 7U, 100U));
    zassert_false(ble_mesh_dedup_accept(&cache, node_a, 7U, 200U));
    zassert_true(ble_mesh_dedup_accept(&cache, node_a, 8U, 200U));
    zassert_true(ble_mesh_dedup_accept(&cache, node_b, 7U, 200U));
    zassert_true(ble_mesh_dedup_accept(&cache, node_a, 7U,
                                       100U + BLE_MESH_DEDUP_WINDOW_MS));
}

ZTEST(ble_mesh_packet, test_topology_uses_immutable_ids_and_explicit_relay)
{
    static const uint8_t unknown_id[BLE_MESH_NODE_ID_LEN] = {
        0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU,
    };

    zassert_equal(ble_mesh_topology_get_role(topology_node_a, topology_node_b),
                  BLE_MESH_TOPOLOGY_ROLE_A);
    zassert_equal(ble_mesh_topology_get_role(topology_node_b, topology_node_b),
                  BLE_MESH_TOPOLOGY_ROLE_B);
    zassert_equal(ble_mesh_topology_get_role(topology_node_c, topology_node_b),
                  BLE_MESH_TOPOLOGY_ROLE_C);
    zassert_equal(ble_mesh_topology_get_role(unknown_id, topology_node_b),
                  BLE_MESH_TOPOLOGY_ROLE_UNKNOWN);
    zassert_equal(ble_mesh_topology_get_role(topology_node_c, topology_node_c),
                  BLE_MESH_TOPOLOGY_ROLE_B);
    zassert_equal(ble_mesh_topology_get_role(topology_node_b, topology_node_c),
                  BLE_MESH_TOPOLOGY_ROLE_C);
    zassert_true(ble_mesh_topology_can_originate_text(
        BLE_MESH_TOPOLOGY_ROLE_A));
    zassert_false(ble_mesh_topology_can_originate_text(
        BLE_MESH_TOPOLOGY_ROLE_B));
    zassert_false(ble_mesh_topology_can_originate_text(
        BLE_MESH_TOPOLOGY_ROLE_C));
}

ZTEST(ble_mesh_packet, test_topology_forwards_a_ttl3_from_b_as_ttl2)
{
    struct ble_mesh_packet packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {0xebU, 0x34U, 0xedU, 0x25U, 0xa5U, 0xa4U},
        .sequence = 19U,
        .ttl = BLE_MESH_MAX_HOPS,
        .payload = {'t', 'w', 'o'},
        .payload_length = 3U,
    };
    struct ble_mesh_dedup_cache b_cache;
    struct ble_mesh_dedup_cache c_cache;
    bool should_relay = false;

    ble_mesh_dedup_init(&b_cache);
    zassert_equal(ble_mesh_topology_filter_text(
                      BLE_MESH_TOPOLOGY_ROLE_B, &packet, &b_cache, 100U,
                      &should_relay),
                  BLE_MESH_TOPOLOGY_ACCEPTED);
    zassert_true(should_relay);
    zassert_true(ble_mesh_packet_prepare_relay(&packet));
    zassert_equal(packet.ttl, BLE_MESH_MAX_HOPS - 1U);

    ble_mesh_dedup_init(&c_cache);
    should_relay = true;
    zassert_equal(ble_mesh_topology_filter_text(
                      BLE_MESH_TOPOLOGY_ROLE_C, &packet, &c_cache, 101U,
                      &should_relay),
                  BLE_MESH_TOPOLOGY_ACCEPTED);
    zassert_false(should_relay);
}

ZTEST(ble_mesh_packet, test_topology_rejects_direct_a_before_dedup)
{
    struct ble_mesh_packet packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {0xebU, 0x34U, 0xedU, 0x25U, 0xa5U, 0xa4U},
        .sequence = 23U,
        .ttl = BLE_MESH_MAX_HOPS,
        .payload = {'d', 'i', 'r'},
        .payload_length = 3U,
    };
    struct ble_mesh_dedup_cache cache;
    bool should_relay = true;

    ble_mesh_dedup_init(&cache);
    zassert_equal(ble_mesh_topology_filter_text(
                      BLE_MESH_TOPOLOGY_ROLE_C, &packet, &cache, 200U,
                      &should_relay),
                  BLE_MESH_TOPOLOGY_REJECTED);
    zassert_false(should_relay);
    for (size_t i = 0U; i < BLE_MESH_DEDUP_CACHE_SIZE; ++i) {
        zassert_false(cache.entries[i].valid);
    }

    packet.ttl = BLE_MESH_MAX_HOPS - 1U;
    zassert_equal(ble_mesh_topology_filter_text(
                      BLE_MESH_TOPOLOGY_ROLE_C, &packet, &cache, 201U,
                      &should_relay),
                  BLE_MESH_TOPOLOGY_ACCEPTED);
    zassert_false(should_relay);
}

ZTEST(ble_mesh_packet, test_topology_rejects_wrong_ttl_duplicate_and_source)
{
    struct ble_mesh_packet packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .source_id = {0xebU, 0x34U, 0xedU, 0x25U, 0xa5U, 0xa4U},
        .sequence = 29U,
        .ttl = BLE_MESH_MAX_HOPS - 1U,
        .payload = {'x'},
        .payload_length = 1U,
    };
    struct ble_mesh_dedup_cache cache;
    bool should_relay = false;

    ble_mesh_dedup_init(&cache);
    zassert_equal(ble_mesh_topology_filter_text(
                      BLE_MESH_TOPOLOGY_ROLE_B, &packet, &cache, 300U,
                      &should_relay),
                  BLE_MESH_TOPOLOGY_REJECTED);
    packet.ttl = BLE_MESH_MAX_HOPS;
    zassert_equal(ble_mesh_topology_filter_text(
                      BLE_MESH_TOPOLOGY_ROLE_B, &packet, &cache, 301U,
                      &should_relay),
                  BLE_MESH_TOPOLOGY_ACCEPTED);
    zassert_equal(ble_mesh_topology_filter_text(
                      BLE_MESH_TOPOLOGY_ROLE_B, &packet, &cache, 302U,
                      &should_relay),
                  BLE_MESH_TOPOLOGY_DUPLICATE);
    packet.sequence++;
    packet.source_id[0] ^= 0x01U;
    zassert_equal(ble_mesh_topology_filter_text(
                      BLE_MESH_TOPOLOGY_ROLE_B, &packet, &cache, 303U,
                      &should_relay),
                  BLE_MESH_TOPOLOGY_REJECTED);
}

ZTEST(ble_mesh_packet, test_topology_does_not_filter_beacons)
{
    struct ble_mesh_packet packet = {
        .type = BLE_MESH_PACKET_BEACON,
        .ttl = 1U,
    };
    struct ble_mesh_dedup_cache cache;
    bool should_relay = true;

    ble_mesh_dedup_init(&cache);
    zassert_equal(ble_mesh_topology_filter_text(
                      BLE_MESH_TOPOLOGY_ROLE_C, &packet, &cache, 400U,
                      &should_relay),
                  BLE_MESH_TOPOLOGY_NOT_APPLICABLE);
    zassert_false(should_relay);
    zassert_false(cache.entries[0].valid);
}

ZTEST(ble_mesh_packet, test_scan_does_not_filter_duplicates)
{
    struct bt_le_scan_param param;

    ble_mesh_scan_param_init(&param);
    zassert_equal(param.type, BT_LE_SCAN_TYPE_PASSIVE);
    zassert_equal(param.options & BT_LE_SCAN_OPT_FILTER_DUPLICATE, 0U);
    zassert_equal(param.interval, param.window);
}

ZTEST(ble_mesh_packet, test_text_hold_covers_many_advertising_events)
{
    zassert_true(BLE_MESH_TEXT_ADV_HOLD_MS >=
                 BLE_MESH_MIN_ADV_EVENTS_PER_HOLD *
                     BLE_MESH_ADV_INTERVAL_MAX_MS);
}

ZTEST(ble_mesh_packet, test_heartbeat_fits_one_usb_packet_with_checksum)
{
    const struct ble_mesh_heartbeat beat = {
        .role = 'B', .index = 4294967295U, .decoded = 4294967295U,
        .text = 4294967295U, .decode_fail = 4294967295U,
        .rx_drop = 4294967295U, .uptime_ms = 4294967295U,
    };
    char line[BLE_MESH_HEARTBEAT_MAX_LEN];
    int length = ble_mesh_heartbeat_format(&beat, line, sizeof(line));

    zassert_true(length > 0 && length < (int)BLE_MESH_HEARTBEAT_MAX_LEN);
    zassert_equal(line[length - 1], '\n');
    zassert_equal(strncmp(line, "HB r=B n=4294967295 ", 20), 0);

    char small[16];
    zassert_equal(ble_mesh_heartbeat_format(&beat, small, sizeof(small)),
                  -EMSGSIZE);
}

ZTEST(ble_mesh_packet, test_heartbeat_checksum_matches_body)
{
    const struct ble_mesh_heartbeat beat = {
        .role = 'C', .index = 1, .decoded = 2, .text = 3, .uptime_ms = 5000,
    };
    char line[BLE_MESH_HEARTBEAT_MAX_LEN];
    int length = ble_mesh_heartbeat_format(&beat, line, sizeof(line));
    const char *marker = strstr(line, " c=");
    uint8_t expected = 0U;

    zassert_true(length > 0);
    zassert_not_null(marker);
    for (const char *p = line; p < marker; ++p) {
        expected ^= (uint8_t)*p;
    }
    char text[8];
    snprintk(text, sizeof(text), " c=%02X\n", expected);
    zassert_str_equal(marker, text);
}

ZTEST(ble_mesh_packet, test_evidence_record_layout_and_checksum)
{
    struct ble_mesh_evidence ev;
    const uint8_t src[6] = {0xebU, 0x34U, 0xedU, 0x25U, 0xa5U, 0xa4U};
    uint8_t out[BLE_MESH_EVIDENCE_LEN];

    ble_mesh_evidence_init(&ev);
    ble_mesh_evidence_record_decoded(&ev, 1);
    ble_mesh_evidence_record_decoded(&ev, 0);
    ble_mesh_evidence_record_text(&ev, BLE_MESH_EVIDENCE_EVENT_REJECT, src,
                                  0x0138U, 3U);
    zassert_equal(ble_mesh_evidence_encode(&ev, 'C', out, sizeof(out)),
                  (int)BLE_MESH_EVIDENCE_LEN);
    zassert_equal(out[0], BLE_MESH_EVIDENCE_VERSION);
    zassert_equal(out[1], 'C');
    zassert_equal(out[2], 2);
    zassert_equal(out[4], 1);
    zassert_equal(out[6], 1);
    zassert_equal(out[8], 0);
    zassert_equal(out[12], 0x38);
    zassert_equal(out[13], 0x01);
    zassert_equal(out[14], 3);
    zassert_equal(out[15], BLE_MESH_EVIDENCE_EVENT_REJECT);
    zassert_equal(out[16], 0x25);
    zassert_equal(out[18], 0xa4);
    uint8_t x = 0U;
    for (size_t i = 0; i < 19U; ++i) {
        x ^= out[i];
    }
    zassert_equal(out[19], x);
}

ZTEST(ble_mesh_packet, test_evidence_counts_survive_unbounded_event_burst)
{
    struct ble_mesh_evidence ev;
    const uint8_t src[6] = {1, 2, 3, 4, 5, 6};
    uint8_t out[BLE_MESH_EVIDENCE_LEN];

    ble_mesh_evidence_init(&ev);
    for (int i = 0; i < 300; ++i) {
        ble_mesh_evidence_record_text(&ev, BLE_MESH_EVIDENCE_EVENT_REJECT,
                                      src, (uint16_t)i, 3U);
    }
    ble_mesh_evidence_record_text(&ev, BLE_MESH_EVIDENCE_EVENT_ACCEPT, src,
                                  7U, 2U);
    ble_mesh_evidence_record_text(&ev, BLE_MESH_EVIDENCE_EVENT_DUPLICATE,
                                  src, 7U, 2U);
    ble_mesh_evidence_record_text(&ev, BLE_MESH_EVIDENCE_EVENT_NONE, src,
                                  9U, 1U);
    zassert_equal(ble_mesh_evidence_encode(&ev, 'C', out, sizeof(out)),
                  (int)BLE_MESH_EVIDENCE_LEN);
    zassert_equal(out[6] | (out[7] << 8), 300);
    zassert_equal(out[8], 1);
    zassert_equal(out[10], 1);
    zassert_equal(out[12], 7);
    zassert_equal(out[15], BLE_MESH_EVIDENCE_EVENT_DUPLICATE);
}

ZTEST(ble_mesh_packet, test_evidence_encode_rejects_bad_arguments)
{
    struct ble_mesh_evidence ev;
    uint8_t small[BLE_MESH_EVIDENCE_LEN - 1U];
    uint8_t out[BLE_MESH_EVIDENCE_LEN];

    ble_mesh_evidence_init(&ev);
    zassert_equal(ble_mesh_evidence_encode(&ev, 'A', small, sizeof(small)),
                  -EMSGSIZE);
    zassert_equal(ble_mesh_evidence_encode(NULL, 'A', out, sizeof(out)),
                  -EINVAL);
    zassert_equal(ble_mesh_evidence_encode(&ev, 'A', NULL, sizeof(out)),
                  -EINVAL);
}

ZTEST_SUITE(ble_mesh_packet, NULL, NULL, NULL, NULL, NULL);
