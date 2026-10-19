#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <hal/nrf_ficr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include "ble_mesh_packet.h"
#include "ble_mesh_rssi_validation.h"
#include "ble_mesh_topology.h"
#include "ble_mesh_relay.h"
#include "ble_mesh_relay_queue.h"
#include "ble_mesh_rx_diagnostics.h"
#include "ble_mesh_heartbeat.h"
#include "ble_mesh_evidence.h"
#include "ble_mesh_scan_policy.h"
#include "../ble_gatt/ble_gatt.h"
#if defined(CONFIG_HYBRID_ROUTER_POLICY) && CONFIG_HYBRID_ROUTER_POLICY
#include "../hybrid_router/hybrid_router.h"
#endif
#if defined(CONFIG_BLE_INBOX) && CONFIG_BLE_INBOX
#include "../ble_inbox/ble_inbox_service.h"
#endif
#define BLE_MESH_MAX_PEERS 8U
#define BLE_MESH_RX_QUEUE_DEPTH 8U
#define BLE_MESH_RELAY_QUEUE_DEPTH 4U
#define BLE_MESH_PEER_TIMEOUT_MS 60000U
#define BLE_MESH_ADVERTISEMENT_HOLD K_MSEC(BLE_MESH_TEXT_ADV_HOLD_MS)
#define BLE_MESH_ADVERTISEMENT_RETRY K_SECONDS(1)
#define BLE_MESH_BEACON_INTERVAL K_SECONDS(5)
#define BLE_MESH_ADVERTISEMENT_RETRY_LOG_INTERVAL 10U

struct ble_mesh_peer {
    uint8_t node_id[BLE_MESH_NODE_ID_LEN];
    uint32_t last_seen_ms;
    bool valid;
};

struct ble_mesh_rx_item {
    struct ble_mesh_packet packet;
    int8_t rssi;
};

struct ble_mesh_scan_context {
    int8_t rssi;
};

K_MSGQ_DEFINE(rx_queue, sizeof(struct ble_mesh_rx_item),
              BLE_MESH_RX_QUEUE_DEPTH, 4);
K_MSGQ_DEFINE(relay_queue, sizeof(struct ble_mesh_packet),
              BLE_MESH_RELAY_QUEUE_DEPTH, 4);

static struct ble_mesh_dedup_cache dedup_cache;
#if defined(CONFIG_HYBRID_ROUTER_POLICY) && CONFIG_HYBRID_ROUTER_POLICY
/* No-radio, no-callback router; only the serialized RX worker touches it. */
static struct hybrid_router hybrid_router_state;
#endif
static struct ble_mesh_peer peers[BLE_MESH_MAX_PEERS];
static uint8_t local_node_id[BLE_MESH_NODE_ID_LEN];
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
static uint8_t relay_b_node_id[BLE_MESH_NODE_ID_LEN];
static enum ble_mesh_topology_role local_topology_role;
#endif
static uint16_t next_sequence;
static struct k_mutex state_mutex;
static bool message_advertisement_active;
static struct k_work_delayable advertising_work;
static struct k_work rx_work;
static uint8_t advertisement_retry_log_count;
static struct ble_mesh_evidence evidence;
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
static struct ble_mesh_rx_diagnostic_counters rx_diagnostics;
static struct ble_mesh_rx_diagnostic_snapshot reported_rx_diagnostics;
static struct k_work_delayable heartbeat_work;
static uint32_t heartbeat_index;
#endif

#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
static char topology_role_name(enum ble_mesh_topology_role role)
{
    switch (role) {
    case BLE_MESH_TOPOLOGY_ROLE_A:
        return 'A';
    case BLE_MESH_TOPOLOGY_ROLE_B:
        return 'B';
    case BLE_MESH_TOPOLOGY_ROLE_C:
        return 'C';
    default:
        return '?';
    }
}

static const char *topology_packet_type_name(
    enum ble_mesh_packet_type type)
{
    switch (type) {
    case BLE_MESH_PACKET_TEXT:
        return "TEXT";
    case BLE_MESH_PACKET_BEACON:
        return "BEACON";
    default:
        return "UNKNOWN";
    }
}

static void log_topology_frame(const char *event,
                               const struct ble_mesh_packet *packet,
                               int result)
{
    printk("TEST topology event=%s role=%c type=%s "
           "source=%02X%02X%02X%02X%02X%02X seq=%u ttl=%u "
           "uptime_ms=%u rc=%d\n",
           event, topology_role_name(local_topology_role),
           topology_packet_type_name(packet->type),
           packet->source_id[0], packet->source_id[1], packet->source_id[2],
           packet->source_id[3], packet->source_id[4], packet->source_id[5],
           (unsigned int)packet->sequence, (unsigned int)packet->ttl,
           (unsigned int)k_uptime_get_32(), result);
}

static void report_rx_diagnostics(void)
{
    struct ble_mesh_rx_diagnostic_snapshot snapshot;
    ble_mesh_rx_diagnostics_snapshot(&rx_diagnostics, &snapshot);
    if (snapshot.manufacturer_decode_failures ==
            reported_rx_diagnostics.manufacturer_decode_failures &&
        snapshot.rx_queue_drops ==
            reported_rx_diagnostics.rx_queue_drops) {
        return;
    }

    printk("TEST topology event=RX_COUNTERS role=%c "
           "decode_failures=%u rx_queue_drops=%u uptime_ms=%u\n",
           topology_role_name(local_topology_role),
           (unsigned int)snapshot.manufacturer_decode_failures,
           (unsigned int)snapshot.rx_queue_drops,
           (unsigned int)k_uptime_get_32());
    reported_rx_diagnostics = snapshot;
}

/*
 * Cumulative counters let a host measure USB console line loss: a gap in
 * n shows dropped lines, and the totals still show scan decode activity.
 */
static void heartbeat_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    struct ble_mesh_rx_diagnostic_snapshot snapshot;
    ble_mesh_rx_diagnostics_snapshot(&rx_diagnostics, &snapshot);
    struct ble_mesh_heartbeat heartbeat = {
        .role = topology_role_name(local_topology_role),
        .index = heartbeat_index++,
        .decoded = snapshot.decoded_frames,
        .text = snapshot.decoded_text_frames,
        .decode_fail = snapshot.manufacturer_decode_failures,
        .rx_drop = snapshot.rx_queue_drops,
        .uptime_ms = k_uptime_get_32(),
    };
    char line[BLE_MESH_HEARTBEAT_MAX_LEN];
    if (ble_mesh_heartbeat_format(&heartbeat, line, sizeof(line)) > 0) {
        printk("%s", line);
    }
    k_work_reschedule(&heartbeat_work, K_SECONDS(5));
}
#endif

static int evidence_read(uint8_t *buffer, size_t capacity)
{
    char role = '?';
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
    role = topology_role_name(local_topology_role);
#endif
    return ble_mesh_evidence_encode(&evidence, role, buffer, capacity);
}

static void submit_rx_work(void)
{
    int err = k_work_submit(&rx_work);
    if (err < 0) {
        printk("BLE mesh receive work submission failed: %d\n", err);
    }
}

static void schedule_advertising(k_timeout_t delay)
{
    int err = k_work_reschedule(&advertising_work, delay);
    if (err < 0) {
        printk("BLE mesh advertising work scheduling failed: %d\n", err);
    }
}

static int advertise_packet(const struct ble_mesh_packet *packet)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t frame_length;
    int err = ble_mesh_packet_encode(packet, frame, sizeof(frame),
                                     &frame_length);
    if (err != 0) {
        printk("BLE mesh packet encode failed: %d\n", err);
        return err;
    }

    return ble_gatt_update_mesh_advertisement(frame, frame_length);
}

static void log_advertisement_retry(const struct ble_mesh_packet *packet,
                                    int err)
{
    if (advertisement_retry_log_count == 0U ||
        advertisement_retry_log_count >=
            BLE_MESH_ADVERTISEMENT_RETRY_LOG_INTERVAL) {
        if (err == -EAGAIN) {
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
            log_topology_frame("ADV_DATA_API_FAIL", packet, err);
#else
            printk("BLE mesh text advertisement deferred; packet retained for retry\n");
#endif
        } else {
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
            log_topology_frame("ADV_DATA_API_FAIL", packet, err);
#else
            printk("BLE mesh text advertisement failed (%d); packet retained for retry\n",
                   err);
#endif
        }
        advertisement_retry_log_count = 0U;
    }
    ++advertisement_retry_log_count;
}

static int enqueue_relay(const struct ble_mesh_packet *packet)
{
    k_mutex_lock(&state_mutex, K_FOREVER);
    int err = k_msgq_put(&relay_queue, packet, K_NO_WAIT);
    if (err == 0 && !message_advertisement_active) {
        schedule_advertising(K_NO_WAIT);
    }
    k_mutex_unlock(&state_mutex);

    if (err != 0) {
        printk("BLE mesh relay queue full; packet dropped: %d\n", err);
    }
    return err;
}

static int update_packet_advertisement(
    const struct ble_mesh_packet *packet, void *context)
{
    ARG_UNUSED(context);
    return advertise_packet(packet);
}

static void advertising_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    struct ble_mesh_packet packet;
    k_mutex_lock(&state_mutex, K_FOREVER);
    int err = k_msgq_peek(&relay_queue, &packet);
    if (err == 0) {
        message_advertisement_active = true;
    } else if (err == -ENOMSG) {
        message_advertisement_active = false;
        packet = (struct ble_mesh_packet) {
            .type = BLE_MESH_PACKET_BEACON,
            .sequence = next_sequence++,
            .ttl = 1U,
            .payload_length = 0U,
        };
        memcpy(packet.source_id, local_node_id, sizeof(packet.source_id));
        schedule_advertising(BLE_MESH_BEACON_INTERVAL);
    }
    k_mutex_unlock(&state_mutex);

    if (err != 0 && err != -ENOMSG) {
        printk("BLE mesh transmit queue inspection failed: %d\n", err);
        schedule_advertising(BLE_MESH_ADVERTISEMENT_RETRY);
        return;
    }

    if (packet.type == BLE_MESH_PACKET_TEXT) {
        int update_err = -EINPROGRESS;
        int queue_err = ble_mesh_relay_queue_update_head(
            &relay_queue, update_packet_advertisement, NULL, &packet,
            &update_err);
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
        if (update_err == 0) {
            log_topology_frame("ADV_DATA_API_OK", &packet, 0);
        }
#endif
        if (queue_err != 0) {
            printk("BLE mesh advertised packet could not be removed from queue: %d\n",
                   queue_err);
            schedule_advertising(BLE_MESH_ADVERTISEMENT_RETRY);
            return;
        }
        if (update_err != 0) {
            log_advertisement_retry(&packet, update_err);
            schedule_advertising(BLE_MESH_ADVERTISEMENT_RETRY);
            return;
        }

        advertisement_retry_log_count = 0U;
        schedule_advertising(BLE_MESH_ADVERTISEMENT_HOLD);
        return;
    }

    err = advertise_packet(&packet);
    if (err != 0) {
        printk("BLE mesh beacon advertisement update failed: %d\n", err);
    }
}

static bool peer_record(const uint8_t node_id[BLE_MESH_NODE_ID_LEN])
{
    uint32_t now_ms = k_uptime_get_32();
    size_t replace_index = 0U;
    uint32_t oldest_age = 0U;

    k_mutex_lock(&state_mutex, K_FOREVER);
    for (size_t i = 0U; i < BLE_MESH_MAX_PEERS; ++i) {
        struct ble_mesh_peer *peer = &peers[i];
        if (peer->valid &&
            memcmp(peer->node_id, node_id, BLE_MESH_NODE_ID_LEN) == 0) {
            peer->last_seen_ms = now_ms;
            k_mutex_unlock(&state_mutex);
            return false;
        }

        if (!peer->valid) {
            replace_index = i;
            oldest_age = UINT32_MAX;
            break;
        }

        uint32_t age = now_ms - peer->last_seen_ms;
        if (age >= BLE_MESH_PEER_TIMEOUT_MS) {
            replace_index = i;
            oldest_age = UINT32_MAX;
            break;
        }
        if (age > oldest_age) {
            oldest_age = age;
            replace_index = i;
        }
    }

    struct ble_mesh_peer *peer = &peers[replace_index];
    memcpy(peer->node_id, node_id, BLE_MESH_NODE_ID_LEN);
    peer->last_seen_ms = now_ms;
    peer->valid = true;
    k_mutex_unlock(&state_mutex);
    return true;
}

static void publish_peer_event(const uint8_t node_id[BLE_MESH_NODE_ID_LEN])
{
    char event[BLE_GATT_MAX_VALUE_LEN];
    int length = snprintf(event, sizeof(event), "PEER:%02X%02X%02X%02X%02X%02X",
                          node_id[0], node_id[1], node_id[2],
                          node_id[3], node_id[4], node_id[5]);
    if (length < 0 || (size_t)length >= sizeof(event)) {
        printk("BLE mesh peer event formatting failed\n");
        return;
    }

    (void)ble_gatt_publish_discovery_event((const uint8_t *)event,
                                           (size_t)length);
}

static void process_received_packets(struct k_work *work)
{
    ARG_UNUSED(work);
    struct ble_mesh_rx_item rx_item;

    while (k_msgq_get(&rx_queue, &rx_item, K_NO_WAIT) == 0) {
        struct ble_mesh_packet packet = rx_item.packet;
        int8_t rssi = rx_item.rssi;
        ARG_UNUSED(rssi);

        if (memcmp(packet.source_id, local_node_id,
                   BLE_MESH_NODE_ID_LEN) == 0) {
            continue;
        }
#if defined(CONFIG_HYBRID_ROUTER_POLICY) && CONFIG_HYBRID_ROUTER_POLICY
        hybrid_router_ble_peer_seen(&hybrid_router_state, k_uptime_get_32(),
                                    rssi);
#endif
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
        bool should_relay = false;
        if (packet.type == BLE_MESH_PACKET_TEXT) {
            k_mutex_lock(&state_mutex, K_FOREVER);
            enum ble_mesh_topology_filter_result filter_result =
                ble_mesh_topology_filter_text(
                    local_topology_role, &packet, &dedup_cache,
                    k_uptime_get_32(), &should_relay);
            k_mutex_unlock(&state_mutex);
            if (filter_result == BLE_MESH_TOPOLOGY_REJECTED) {
                ble_mesh_evidence_record_text(
                    &evidence, BLE_MESH_EVIDENCE_EVENT_REJECT,
                    packet.source_id, packet.sequence, packet.ttl);
                log_topology_frame("RX_REJECT", &packet, 0);
                continue;
            }
            if (filter_result == BLE_MESH_TOPOLOGY_DUPLICATE) {
                ble_mesh_evidence_record_text(
                    &evidence, BLE_MESH_EVIDENCE_EVENT_DUPLICATE,
                    packet.source_id, packet.sequence, packet.ttl);
                log_topology_frame("RX_DUPLICATE", &packet, 0);
                continue;
            }
            if (filter_result == BLE_MESH_TOPOLOGY_ACCEPTED) {
                ble_mesh_evidence_record_text(
                    &evidence, BLE_MESH_EVIDENCE_EVENT_ACCEPT,
                    packet.source_id, packet.sequence, packet.ttl);
                log_topology_frame("RX_ACCEPT", &packet, 0);
            }
        } else
#endif
        {
            k_mutex_lock(&state_mutex, K_FOREVER);
            bool is_new = ble_mesh_dedup_accept(
                &dedup_cache, packet.source_id, packet.sequence,
                k_uptime_get_32());
            k_mutex_unlock(&state_mutex);
            if (!is_new) {
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
                if (packet.type == BLE_MESH_PACKET_TEXT) {
                    log_topology_frame("RX_DUPLICATE", &packet, 0);
                }
#endif
                continue;
            }
        }

        if (peer_record(packet.source_id)) {
            publish_peer_event(packet.source_id);
        }

        if (packet.type != BLE_MESH_PACKET_TEXT) {
            continue;
        }

        int publish_err = ble_gatt_publish_text_message(
            packet.payload, packet.payload_length);
#if defined(CONFIG_BLE_INBOX) && CONFIG_BLE_INBOX
        /* Only frames that passed filter/dedup and are remote TEXT. */
        (void)ble_inbox_service_record(packet.source_id, packet.payload,
                                       packet.payload_length);
#endif
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
        log_topology_frame("GATT_PUBLISH", &packet, publish_err);
        if (should_relay) {
            struct ble_mesh_packet relay_packet = packet;
            if (ble_mesh_packet_prepare_relay(&relay_packet)) {
                int relay_err = enqueue_relay(&relay_packet);
                log_topology_frame(
                    relay_err == 0 ? "RELAY_QUEUE_OK" : "RELAY_QUEUE_FAIL",
                    &relay_packet, relay_err);
            } else {
                log_topology_frame("RELAY_TTL_FAIL", &packet, -EINVAL);
            }
        }
#else
        ARG_UNUSED(publish_err);
        if (ble_mesh_packet_prepare_relay(&packet)) {
            (void)enqueue_relay(&packet);
        }
#endif
    }
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
    report_rx_diagnostics();
#endif
}

static bool advertisement_parse(struct bt_data *data, void *user_data)
{
    const struct ble_mesh_scan_context *context = user_data;
    if (data->type != BT_DATA_MANUFACTURER_DATA) {
        return true;
    }
    struct ble_mesh_packet packet;
    int8_t validated_rssi;
    enum ble_mesh_rssi_validation_result validation =
        ble_mesh_rssi_validation_decode(
            data->data, data->data_len, context->rssi, &packet,
            &validated_rssi);
    if (validation == BLE_MESH_RSSI_VALIDATION_NOT_MESH) {
        return true;
    }
    if (validation != BLE_MESH_RSSI_VALIDATION_VALID) {
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
        ble_mesh_rx_diagnostics_record_decode_failure(&rx_diagnostics);
        submit_rx_work();
#endif
        return true;
    }
    if (memcmp(packet.source_id, local_node_id, BLE_MESH_NODE_ID_LEN) == 0) {
        return true;
    }

    ble_mesh_evidence_record_decoded(&evidence,
                                     packet.type == BLE_MESH_PACKET_TEXT);
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
    ble_mesh_rx_diagnostics_record_decoded(
        &rx_diagnostics, packet.type == BLE_MESH_PACKET_TEXT);
#endif
    struct ble_mesh_rx_item rx_item = {
        .packet = packet,
        .rssi = validated_rssi,
    };
    int err = k_msgq_put(&rx_queue, &rx_item, K_NO_WAIT);
    if (err != 0) {
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
        ble_mesh_rx_diagnostics_record_queue_drop(&rx_diagnostics);
        submit_rx_work();
#endif
        printk("BLE mesh receive queue full; packet dropped: %d\n", err);
        return true;
    }
    submit_rx_work();
    return true;
}

static void scan_receive(const bt_addr_le_t *address, int8_t rssi,
                         uint8_t advertising_type,
                         struct net_buf_simple *advertising_data)
{
    ARG_UNUSED(address);
    ARG_UNUSED(advertising_type);
    struct ble_mesh_scan_context context = {
        .rssi = rssi,
    };
    bt_data_parse(advertising_data, advertisement_parse, &context);
}

static void request_discovery(void)
{
    uint8_t peer_ids[BLE_MESH_MAX_PEERS][BLE_MESH_NODE_ID_LEN];
    size_t peer_count = 0U;
    uint32_t now_ms = k_uptime_get_32();

    k_mutex_lock(&state_mutex, K_FOREVER);
    for (size_t i = 0U; i < BLE_MESH_MAX_PEERS; ++i) {
        if (peers[i].valid &&
            (uint32_t)(now_ms - peers[i].last_seen_ms) <
                BLE_MESH_PEER_TIMEOUT_MS) {
            memcpy(peer_ids[peer_count], peers[i].node_id,
                   BLE_MESH_NODE_ID_LEN);
            ++peer_count;
        } else {
            peers[i].valid = false;
        }
    }
    k_mutex_unlock(&state_mutex);

    if (peer_count == 0U) {
        static const uint8_t no_peers[] = "BLE: no peers";
        (void)ble_gatt_publish_discovery_event(no_peers,
                                               sizeof(no_peers) - 1U);
        return;
    }

    for (size_t i = 0U; i < peer_count; ++i) {
        publish_peer_event(peer_ids[i]);
    }
}

int ble_mesh_relay_handle_phone_write(const uint8_t *value, size_t length)
{
    if (value == NULL || length == 0U) {
        return -EINVAL;
    }

    static const uint8_t discover_command[] = "DISCOVER";
    if (length == sizeof(discover_command) - 1U &&
        memcmp(value, discover_command, length) == 0) {
        request_discovery();
#if defined(CONFIG_BLE_GATT_DISCOVER_NO_ECHO) && CONFIG_BLE_GATT_DISCOVER_NO_ECHO
        return BLE_GATT_RX_CONTROL_HANDLED;
#else
        return 0;
#endif
    }
    if (length > BLE_MESH_MAX_TEXT_LEN) {
        return -EMSGSIZE;
    }
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
    if (!ble_mesh_topology_can_originate_text(local_topology_role)) {
        return -EACCES;
    }
#endif

    struct ble_mesh_packet packet = {
        .type = BLE_MESH_PACKET_TEXT,
        .ttl = BLE_MESH_MAX_HOPS,
        .payload_length = (uint8_t)length,
    };
    memcpy(packet.source_id, local_node_id, sizeof(packet.source_id));
    memcpy(packet.payload, value, length);

    k_mutex_lock(&state_mutex, K_FOREVER);
    packet.sequence = next_sequence++;
    (void)ble_mesh_dedup_accept(&dedup_cache, packet.source_id,
                                packet.sequence, k_uptime_get_32());
    int err = k_msgq_put(&relay_queue, &packet, K_NO_WAIT);
    if (err == 0 && !message_advertisement_active) {
        schedule_advertising(K_NO_WAIT);
    }
    k_mutex_unlock(&state_mutex);

    if (err != 0) {
        printk("BLE mesh transmit queue full: %d\n", err);
    }
    return err;
}

int ble_mesh_relay_init(void)
{
    uint32_t device_id[2] = {
        nrf_ficr_deviceid_get(NRF_FICR, 0U),
        nrf_ficr_deviceid_get(NRF_FICR, 1U),
    };
    sys_put_le32(device_id[0], &local_node_id[0]);
    sys_put_le16((uint16_t)device_id[1], &local_node_id[4]);

#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
#if defined(CONFIG_BLE_MESH_TEST_B_ID_0C13) && \
    CONFIG_BLE_MESH_TEST_B_ID_0C13
    static const uint8_t configured_b_id[BLE_MESH_NODE_ID_LEN] = {
        0x0cU, 0x13U, 0xdaU, 0xe1U, 0x25U, 0xc8U,
    };
#elif defined(CONFIG_BLE_MESH_TEST_B_ID_EE07) && \
    CONFIG_BLE_MESH_TEST_B_ID_EE07
    static const uint8_t configured_b_id[BLE_MESH_NODE_ID_LEN] = {
        0xeeU, 0x07U, 0x14U, 0x5aU, 0x5eU, 0x02U,
    };
#else
#error "Select the immutable mesh ID assigned to test relay B"
#endif
    memcpy(relay_b_node_id, configured_b_id, sizeof(relay_b_node_id));
    local_topology_role = ble_mesh_topology_get_role(local_node_id,
                                                      relay_b_node_id);
    if (local_topology_role == BLE_MESH_TOPOLOGY_ROLE_UNKNOWN) {
        printk("TEST topology enabled but local mesh ID is not A/B/C; "
               "test policy cannot initialize\n");
        return -EINVAL;
    }
#endif

    k_mutex_init(&state_mutex);
    ble_mesh_evidence_init(&evidence);
    int evidence_err = ble_gatt_set_evidence_provider(evidence_read);
    if (evidence_err != 0) {
        printk("BLE evidence provider registration failed: %d\n",
               evidence_err);
        return evidence_err;
    }
    ble_mesh_dedup_init(&dedup_cache);
#if defined(CONFIG_HYBRID_ROUTER_POLICY) && CONFIG_HYBRID_ROUTER_POLICY
    {
        /* Zeroed ops: no LoRa callback; init leaves LoRa TX unauthorized. */
        static const struct hybrid_router_ops no_radio_ops;
        int router_err = hybrid_router_init(&hybrid_router_state,
                                            &dedup_cache, &no_radio_ops);
        if (router_err != 0) {
            printk("Hybrid router init failed: %d\n", router_err);
            return router_err;
        }
    }
#endif
    k_work_init_delayable(&advertising_work, advertising_work_handler);
    k_work_init(&rx_work, process_received_packets);
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
    ble_mesh_rx_diagnostics_init(&rx_diagnostics);
    reported_rx_diagnostics = (struct ble_mesh_rx_diagnostic_snapshot) {0};
    k_work_init_delayable(&heartbeat_work, heartbeat_handler);
    k_work_schedule(&heartbeat_work, K_SECONDS(5));
#endif

    printk("Custom BLE advertising relay node ID %02X%02X%02X%02X%02X%02X; "
           "not Bluetooth SIG Mesh\n",
           local_node_id[0], local_node_id[1], local_node_id[2],
           local_node_id[3], local_node_id[4], local_node_id[5]);
#if defined(CONFIG_BLE_MESH_TEST_TOPOLOGY) && CONFIG_BLE_MESH_TEST_TOPOLOGY
    printk("TEST topology ENABLED role=%c local=%02X%02X%02X%02X%02X%02X "
           "B=%02X%02X%02X%02X%02X%02X; TTL2 is not authenticated\n",
           topology_role_name(local_topology_role),
           local_node_id[0], local_node_id[1], local_node_id[2],
           local_node_id[3], local_node_id[4], local_node_id[5],
           relay_b_node_id[0], relay_b_node_id[1], relay_b_node_id[2],
           relay_b_node_id[3], relay_b_node_id[4], relay_b_node_id[5]);
#endif

    struct bt_le_scan_param scan_param;
    ble_mesh_scan_param_init(&scan_param);
    int err = bt_le_scan_start(&scan_param, scan_receive);
    if (err != 0) {
        printk("BLE peer scan start failed: %d\n", err);
        return err;
    }

    printk("BLE mesh passive scan started; legacy connectable advertising remains enabled\n");
    schedule_advertising(K_NO_WAIT);
    return 0;
}
