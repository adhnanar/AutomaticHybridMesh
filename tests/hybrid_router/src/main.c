#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "hybrid_router.h"
#include "hybrid_router_local_acceptance.h"
#include "../policy_vectors.h"

#define MAX_FRAMES 8U

struct mock {
    int ble_rc;
    int lora_rc;
    unsigned int ble_calls;
    unsigned int lora_calls;
    uint8_t ble_frame[MAX_FRAMES][BLE_MESH_PACKET_MAX_LEN];
    size_t ble_len[MAX_FRAMES];
    uint8_t lora_frame[MAX_FRAMES][BLE_MESH_PACKET_MAX_LEN];
    size_t lora_len[MAX_FRAMES];
    unsigned int delivered;
    uint16_t last_delivered_seq;
};

static int mock_ble(void *ctx, const uint8_t *frame, size_t length)
{
    struct mock *m = ctx;

    if (m->ble_calls < MAX_FRAMES) {
        memcpy(m->ble_frame[m->ble_calls], frame, length);
        m->ble_len[m->ble_calls] = length;
    }
    ++m->ble_calls;
    return m->ble_rc;
}

static int mock_lora(void *ctx, const uint8_t *frame, size_t length)
{
    struct mock *m = ctx;

    if (m->lora_calls < MAX_FRAMES) {
        memcpy(m->lora_frame[m->lora_calls], frame, length);
        m->lora_len[m->lora_calls] = length;
    }
    ++m->lora_calls;
    return m->lora_rc;
}

static void mock_deliver(void *ctx, const struct ble_mesh_packet *packet)
{
    struct mock *m = ctx;

    ++m->delivered;
    m->last_delivered_seq = packet->sequence;
}

struct fixture {
    struct ble_mesh_dedup_cache cache;
    struct hybrid_router router;
    struct mock mock;
};

static struct fixture f;

#define POLICY_VECTOR_ROW(name_, age_, rssi_, available_, authorized_, ble_rc_, \
                          lora_rc_, result_, link_) \
    {name_, age_, rssi_, available_, authorized_, ble_rc_, lora_rc_, result_, \
     link_},
struct policy_vector {
    const char *name;
    uint32_t ble_age_ms;
    int8_t rssi_dbm;
    bool lora_available;
    bool lora_authorized;
    int ble_rc;
    int lora_rc;
    enum hybrid_router_result expected_result;
    enum hybrid_router_link expected_link;
};

static const struct policy_vector policy_vectors[] = {
    HYBRID_POLICY_VECTORS(POLICY_VECTOR_ROW)
};
#undef POLICY_VECTOR_ROW

static void setup(void)
{
    memset(&f, 0, sizeof(f));
    ble_mesh_dedup_init(&f.cache);
    struct hybrid_router_ops ops = {
        .ble_send = mock_ble,
        .lora_send = mock_lora,
        .deliver = mock_deliver,
        .ctx = &f.mock,
    };
    zassert_ok(hybrid_router_init(&f.router, &f.cache, &ops));
}

static void make_packet(struct ble_mesh_packet *p, uint16_t seq, uint8_t ttl)
{
    memset(p, 0, sizeof(*p));
    p->type = BLE_MESH_PACKET_TEXT;
    memcpy(p->source_id, "\x01\x02\x03\x04\x05\x06", 6);
    p->sequence = seq;
    p->ttl = ttl;
    memcpy(p->payload, "hi", 2);
    p->payload_length = 2U;
}

static size_t make_frame(uint8_t *frame, uint16_t seq, uint8_t ttl)
{
    struct ble_mesh_packet p;
    size_t len = 0U;

    make_packet(&p, seq, ttl);
    zassert_ok(ble_mesh_packet_encode(&p, frame, BLE_MESH_PACKET_MAX_LEN,
                                      &len));
    return len;
}

ZTEST(hybrid_router, test_local_acceptance_result_classification)
{
    static const struct {
        enum hybrid_router_result result;
        bool accepted;
    } cases[] = {
        {HYBRID_ROUTER_QUEUED_RETRY, true},
        {HYBRID_ROUTER_NO_LINK_QUEUED, true},
        {HYBRID_ROUTER_LORA_BLOCKED_QUEUED, true},
        {HYBRID_ROUTER_SENT, true},
        {HYBRID_ROUTER_SENT_FALLBACK, true},
        {HYBRID_ROUTER_QUEUE_FULL, false},
        {HYBRID_ROUTER_INVALID, false},
        {HYBRID_ROUTER_DUPLICATE, false},
        {HYBRID_ROUTER_NOT_FORWARDED, false},
    };

    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        zassert_equal(
            hybrid_router_result_is_locally_accepted(cases[i].result),
            cases[i].accepted, "result %d", cases[i].result);
    }
}

ZTEST(hybrid_router, test_local_acceptance_gatt_status_mapping)
{
    static const struct {
        enum hybrid_router_result result;
        int status;
    } cases[] = {
        {HYBRID_ROUTER_SENT, 0},
        {HYBRID_ROUTER_SENT_FALLBACK, 0},
        {HYBRID_ROUTER_QUEUED_RETRY, 0},
        {HYBRID_ROUTER_NO_LINK_QUEUED, 0},
        {HYBRID_ROUTER_LORA_BLOCKED_QUEUED, 0},
        {HYBRID_ROUTER_QUEUE_FULL, -ENOSPC},
        {HYBRID_ROUTER_INVALID, -EINVAL},
        {HYBRID_ROUTER_DUPLICATE, -EALREADY},
        {HYBRID_ROUTER_NOT_FORWARDED, -ENOTSUP},
        {(enum hybrid_router_result)99, -EINVAL},
    };

    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        zassert_equal(hybrid_router_result_to_gatt_status(cases[i].result),
                      cases[i].status, "result %d", cases[i].result);
    }
}

ZTEST(hybrid_router, test_local_send_prefers_fresh_ble)
{
    struct ble_mesh_packet p;
    setup();
    hybrid_router_ble_peer_seen(&f.router, 1000U, -60);
    hybrid_router_set_lora_available(&f.router, true);
    make_packet(&p, 1U, 3U);

    struct hybrid_router_outcome o = hybrid_router_send_local(&f.router, &p, 1500U);
    zassert_equal(o.result, HYBRID_ROUTER_SENT);
    zassert_equal(o.sent_via, HYBRID_ROUTER_LINK_BLE);
    zassert_equal(f.mock.ble_calls, 1U);
    zassert_equal(f.mock.lora_calls, 0U);
    zassert_equal(f.mock.ble_frame[0][9], 3U, "origination keeps TTL");
}

ZTEST(hybrid_router, test_policy_vectors_match_shared_expected_outcomes)
{
    for (size_t i = 0U; i < sizeof(policy_vectors) / sizeof(policy_vectors[0]);
         ++i) {
        const struct policy_vector *v = &policy_vectors[i];
        struct ble_mesh_packet p;
        const uint32_t now_ms = 20000U + v->ble_age_ms;

        setup();
        if (v->rssi_dbm >= HYBRID_ROUTER_BLE_MIN_RSSI_DBM) {
            hybrid_router_ble_peer_seen(&f.router, 20000U, v->rssi_dbm);
        }
        hybrid_router_set_lora_available(&f.router, v->lora_available);
        hybrid_router_set_lora_tx_authorized(&f.router, v->lora_authorized);
        f.mock.ble_rc = v->ble_rc;
        f.mock.lora_rc = v->lora_rc;
        make_packet(&p, (uint16_t)(i + 1U), 3U);

        struct hybrid_router_outcome out =
            hybrid_router_send_local(&f.router, &p, now_ms);
        zassert_equal(out.result, v->expected_result, "%s result", v->name);
        zassert_equal(out.sent_via, v->expected_link, "%s link", v->name);
        if (!v->lora_authorized) {
            zassert_equal(f.mock.lora_calls, 0U,
                          "%s must not invoke LoRa send", v->name);
        }
    }
}

ZTEST(hybrid_router, test_rssi_threshold_and_freshness_boundaries)
{
    struct ble_mesh_packet p;

    setup();
    hybrid_router_ble_peer_seen(&f.router, 1000U, -86);
    zassert_false(hybrid_router_ble_is_fresh(&f.router, 1000U),
                  "-86 dBm rejected");

    hybrid_router_ble_peer_seen(&f.router, 1000U, -85);
    zassert_true(hybrid_router_ble_is_fresh(&f.router, 1000U),
                 "-85 dBm accepted");
    zassert_true(hybrid_router_ble_is_fresh(
                     &f.router, 1000U + HYBRID_ROUTER_BLE_FRESH_MS),
                 "10000 ms inclusive");
    zassert_false(hybrid_router_ble_is_fresh(
                      &f.router, 1000U + HYBRID_ROUTER_BLE_FRESH_MS + 1U),
                  "10001 ms stale");

    /* Stale BLE and unauthorized LoRa must queue without sending. */
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, false);
    make_packet(&p, 200U, 3U);
    struct hybrid_router_outcome o = hybrid_router_send_local(
        &f.router, &p, 1000U + HYBRID_ROUTER_BLE_FRESH_MS + 1U);
    zassert_equal(o.result, HYBRID_ROUTER_LORA_BLOCKED_QUEUED);
    zassert_equal(hybrid_router_pending_count(&f.router), 1U);
    zassert_equal(f.mock.ble_calls, 0U, "stale BLE: no BLE send");
    zassert_equal(f.mock.lora_calls, 0U, "unauthorized: no LoRa send");
}

ZTEST(hybrid_router, test_unauthorized_lora_remains_blocked_on_retries)
{
    struct ble_mesh_packet p;

    setup();
    hybrid_router_set_lora_available(&f.router, true);
    make_packet(&p, 99U, 3U);
    zassert_equal(hybrid_router_send_local(&f.router, &p, 0U).result,
                  HYBRID_ROUTER_LORA_BLOCKED_QUEUED);
    for (uint32_t now_ms = 1000U; now_ms <= 3000U; now_ms += 1000U) {
        zassert_equal(hybrid_router_poll(&f.router, now_ms), 0U);
        zassert_equal(f.mock.lora_calls, 0U);
    }
    zassert_equal(hybrid_router_pending_count(&f.router), 0U);
}

ZTEST(hybrid_router, test_payload_limit_rejected_without_truncation)
{
    struct ble_mesh_packet p;

    setup();
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);
    make_packet(&p, 100U, 3U);
    p.payload_length = BLE_MESH_MAX_TEXT_LEN;
    zassert_equal(hybrid_router_send_local(&f.router, &p, 1U).result,
                  HYBRID_ROUTER_SENT);
    zassert_equal(f.mock.ble_len[0], BLE_MESH_PACKET_MAX_LEN);

    make_packet(&p, 101U, 3U);
    p.payload_length = BLE_MESH_MAX_TEXT_LEN + 1U;
    zassert_equal(hybrid_router_send_local(&f.router, &p, 2U).result,
                  HYBRID_ROUTER_INVALID);
    zassert_equal(f.mock.ble_calls, 1U);
}

ZTEST(hybrid_router, test_local_send_uses_lora_without_ble_peer)
{
    struct ble_mesh_packet p;
    setup();
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    make_packet(&p, 2U, 3U);

    struct hybrid_router_outcome o = hybrid_router_send_local(&f.router, &p, 10U);
    zassert_equal(o.result, HYBRID_ROUTER_SENT);
    zassert_equal(o.sent_via, HYBRID_ROUTER_LINK_LORA);
    zassert_equal(f.mock.ble_calls, 0U);
}

ZTEST(hybrid_router, test_ble_failure_falls_back_to_lora_once_same_ttl)
{
    struct ble_mesh_packet p;
    setup();
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    f.mock.ble_rc = -EIO;
    make_packet(&p, 3U, 3U);

    struct hybrid_router_outcome o = hybrid_router_send_local(&f.router, &p, 5U);
    zassert_equal(o.result, HYBRID_ROUTER_SENT_FALLBACK);
    zassert_equal(o.sent_via, HYBRID_ROUTER_LINK_LORA);
    zassert_equal(f.mock.ble_calls, 1U);
    zassert_equal(f.mock.lora_calls, 1U);
    zassert_mem_equal(f.mock.ble_frame[0], f.mock.lora_frame[0], f.mock.ble_len[0]);
    zassert_equal(f.mock.lora_frame[0][9], 3U);
    zassert_equal(hybrid_router_pending_count(&f.router), 0U);
}

ZTEST(hybrid_router, test_ble_to_lora_relay_decrements_once)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 10U, 3U);
    setup();
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);

    struct hybrid_router_outcome o =
        hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 10U);
    zassert_equal(o.result, HYBRID_ROUTER_SENT);
    zassert_true(o.delivered_locally);
    zassert_equal(o.sent_via, HYBRID_ROUTER_LINK_LORA,
                  "authorized LoRa is the only available route");
    zassert_equal(f.mock.delivered, 1U);
    zassert_equal(f.mock.ble_calls, 0U);
    zassert_equal(f.mock.lora_frame[0][9], 2U);
}

ZTEST(hybrid_router, test_lora_to_ble_relay)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 11U, 3U);
    setup();
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);

    struct hybrid_router_outcome o =
        hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_LORA, frame, len, 10U);
    zassert_equal(o.result, HYBRID_ROUTER_SENT);
    zassert_equal(o.sent_via, HYBRID_ROUTER_LINK_BLE);
    zassert_equal(f.mock.lora_calls, 0U);
    zassert_equal(f.mock.ble_frame[0][9], 2U);
}

ZTEST(hybrid_router, test_ble_ingress_relays_on_ble_when_no_lora)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 12U, 3U);
    setup();
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);

    struct hybrid_router_outcome o =
        hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 10U);
    zassert_equal(o.result, HYBRID_ROUTER_SENT);
    zassert_equal(o.sent_via, HYBRID_ROUTER_LINK_BLE);

    /* With same-medium relay disabled the ingress is never reused. */
    setup();
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);
    f.router.allow_same_medium_relay = false;
    o = hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 10U);
    zassert_equal(o.result, HYBRID_ROUTER_NO_LINK_QUEUED);
    zassert_equal(f.mock.ble_calls, 0U);
}

ZTEST(hybrid_router, test_ttl_chain_3_2_1)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 20U, 3U);
    uint8_t second[BLE_MESH_PACKET_MAX_LEN];

    setup();
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    struct hybrid_router_outcome o =
        hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 1U);
    zassert_equal(o.result, HYBRID_ROUTER_SENT);
    zassert_equal(f.mock.lora_frame[0][9], 2U);
    memcpy(second, f.mock.lora_frame[0], len);

    /* Next node (fresh state) receives TTL2 and emits TTL1. */
    setup();
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    o = hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, second, len, 2U);
    zassert_equal(o.result, HYBRID_ROUTER_SENT);
    zassert_equal(f.mock.lora_frame[0][9], 1U);
    memcpy(second, f.mock.lora_frame[0], len);

    /* Third node delivers TTL1 but never forwards it. */
    setup();
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);
    o = hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_LORA, second, len, 3U);
    zassert_equal(o.result, HYBRID_ROUTER_NOT_FORWARDED);
    zassert_true(o.delivered_locally);
    zassert_equal(f.mock.delivered, 1U);
    zassert_equal(f.mock.ble_calls + f.mock.lora_calls, 0U);
}

ZTEST(hybrid_router, test_duplicate_across_media_delivers_and_relays_once)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 30U, 3U);
    setup();
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);

    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame,
                                        len, 10U).result, HYBRID_ROUTER_SENT);
    struct hybrid_router_outcome o =
        hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_LORA, frame, len, 20U);
    zassert_equal(o.result, HYBRID_ROUTER_DUPLICATE);
    zassert_false(o.delivered_locally);
    o = hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 30U);
    zassert_equal(o.result, HYBRID_ROUTER_DUPLICATE);
    zassert_equal(f.mock.delivered, 1U);
    zassert_equal(f.mock.ble_calls + f.mock.lora_calls, 1U);
    zassert_equal(f.router.stats.duplicates, 2U);
}

ZTEST(hybrid_router, test_own_packet_echo_is_suppressed)
{
    struct ble_mesh_packet p;
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 31U, 3U);
    setup();
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    make_packet(&p, 31U, 3U);
    zassert_equal(hybrid_router_send_local(&f.router, &p, 1U).result,
                  HYBRID_ROUTER_SENT);
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_LORA, frame,
                                        len, 2U).result, HYBRID_ROUTER_DUPLICATE);
    zassert_equal(f.mock.delivered, 0U);
}

ZTEST(hybrid_router, test_both_sends_fail_then_retry_preserves_identity_and_ttl)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 40U, 3U);
    uint8_t first[BLE_MESH_PACKET_MAX_LEN];
    setup();
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    f.mock.ble_rc = -EIO;
    f.mock.lora_rc = -EIO;

    struct hybrid_router_outcome o =
        hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 100U);
    zassert_equal(o.result, HYBRID_ROUTER_QUEUED_RETRY);
    zassert_equal(hybrid_router_pending_count(&f.router), 1U);
    memcpy(first, f.mock.lora_frame[0], len);
    zassert_equal(first[9], 2U);

    zassert_equal(hybrid_router_poll(&f.router, 200U), 0U, "not due yet");
    zassert_equal(f.mock.lora_calls, 1U);

    f.mock.lora_rc = 0;
    hybrid_router_ble_peer_seen(&f.router, 1000U, -60);
    zassert_equal(hybrid_router_poll(&f.router, 1100U), 1U);
    zassert_equal(hybrid_router_pending_count(&f.router), 0U);
    zassert_mem_equal(f.mock.lora_frame[f.mock.lora_calls - 1U], first, len);

    /* The retry did not re-run dedup and a later copy is still a duplicate. */
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_LORA, frame,
                                        len, 1200U).result, HYBRID_ROUTER_DUPLICATE);
    zassert_equal(f.mock.delivered, 1U);
}

ZTEST(hybrid_router, test_retry_exhaustion_is_bounded)
{
    struct ble_mesh_packet p;
    setup();
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    f.mock.lora_rc = -EIO;
    make_packet(&p, 50U, 2U);
    zassert_equal(hybrid_router_send_local(&f.router, &p, 0U).result,
                  HYBRID_ROUTER_QUEUED_RETRY);

    for (uint32_t t = 1000U; t <= 3000U; t += 1000U) {
        zassert_equal(hybrid_router_poll(&f.router, t), 0U);
    }
    zassert_equal(hybrid_router_pending_count(&f.router), 0U);
    zassert_equal(f.router.stats.retries_dropped, 1U);
    zassert_equal(f.mock.lora_calls, 1U + HYBRID_ROUTER_MAX_RETRIES);
}

ZTEST(hybrid_router, test_no_link_queues_then_link_returns)
{
    struct ble_mesh_packet p;
    setup();
    make_packet(&p, 60U, 3U);

    struct hybrid_router_outcome o = hybrid_router_send_local(&f.router, &p, 0U);
    zassert_equal(o.result, HYBRID_ROUTER_NO_LINK_QUEUED);
    zassert_equal(f.mock.ble_calls + f.mock.lora_calls, 0U);

    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    zassert_equal(hybrid_router_poll(&f.router, 1000U), 1U);
    zassert_equal(f.mock.lora_frame[0][9], 3U);
}

ZTEST(hybrid_router, test_queue_saturation_is_explicit)
{
    struct ble_mesh_packet p;
    setup();

    for (uint16_t i = 0U; i < HYBRID_ROUTER_PENDING_MAX; ++i) {
        make_packet(&p, 100U + i, 3U);
        zassert_equal(hybrid_router_send_local(&f.router, &p, 0U).result,
                      HYBRID_ROUTER_NO_LINK_QUEUED);
    }
    make_packet(&p, 200U, 3U);
    zassert_equal(hybrid_router_send_local(&f.router, &p, 0U).result,
                  HYBRID_ROUTER_QUEUE_FULL);
    zassert_equal(hybrid_router_pending_count(&f.router), HYBRID_ROUTER_PENDING_MAX);
    zassert_equal(f.router.stats.queue_full, 1U);
}

ZTEST(hybrid_router, test_local_queue_full_allows_same_seq_retry)
{
    struct ble_mesh_packet p;
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 200U, 3U);
    setup();

    for (uint16_t i = 0U; i < HYBRID_ROUTER_PENDING_MAX; ++i) {
        make_packet(&p, 100U + i, 3U);
        zassert_equal(hybrid_router_send_local(&f.router, &p, 0U).result,
                      HYBRID_ROUTER_NO_LINK_QUEUED);
    }
    make_packet(&p, 200U, 3U);
    zassert_equal(hybrid_router_send_local(&f.router, &p, 1U).result,
                  HYBRID_ROUTER_QUEUE_FULL);

    /* Earlier admissions must stay suppressed after the rollback. */
    make_packet(&p, 100U, 3U);
    zassert_equal(hybrid_router_send_local(&f.router, &p, 2U).result,
                  HYBRID_ROUTER_DUPLICATE);

    /* Clear queued slots; the rejected sequence can now be resent. */
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    zassert_equal(hybrid_router_poll(&f.router, 1000U),
                  HYBRID_ROUTER_PENDING_MAX);
    make_packet(&p, 200U, 3U);
    zassert_equal(hybrid_router_send_local(&f.router, &p, 1100U).result,
                  HYBRID_ROUTER_SENT);

    /* Now admitted: resend and own echo are suppressed. */
    zassert_equal(hybrid_router_send_local(&f.router, &p, 1200U).result,
                  HYBRID_ROUTER_DUPLICATE);
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_LORA,
                                        frame, len, 1300U).result,
                  HYBRID_ROUTER_DUPLICATE);
    zassert_equal(f.mock.delivered, 0U);
}

ZTEST(hybrid_router, test_null_deliver_not_counted_but_still_relays)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    uint8_t ttl1[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 70U, 3U);
    size_t len1 = make_frame(ttl1, 71U, 1U);
    setup();
    f.router.ops.deliver = NULL;
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);

    struct hybrid_router_outcome o =
        hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 1U);
    zassert_equal(o.result, HYBRID_ROUTER_SENT);
    zassert_false(o.delivered_locally);
    zassert_equal(f.router.stats.delivered, 0U);
    zassert_equal(f.mock.lora_frame[0][9], 2U);
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_LORA, frame,
                                        len, 2U).result, HYBRID_ROUTER_DUPLICATE);

    o = hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, ttl1, len1, 3U);
    zassert_equal(o.result, HYBRID_ROUTER_NOT_FORWARDED);
    zassert_false(o.delivered_locally);
    zassert_equal(f.router.stats.delivered, 0U);
}

ZTEST(hybrid_router, test_deliver_callback_sets_delivered_flag)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 72U, 2U);
    setup();
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);

    struct hybrid_router_outcome o =
        hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 1U);
    zassert_true(o.delivered_locally);
    zassert_equal(f.router.stats.delivered, 1U);
    zassert_equal(f.mock.delivered, 1U);
}

ZTEST(hybrid_router, test_relay_prefers_fresh_ble_over_lora)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 80U, 3U);
    setup();
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);

    struct hybrid_router_outcome o =
        hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 1U);
    zassert_equal(o.sent_via, HYBRID_ROUTER_LINK_BLE);
    zassert_equal(f.mock.lora_calls, 0U);
}

ZTEST(hybrid_router, test_stale_ble_neighbor_not_used)
{
    struct ble_mesh_packet p;
    setup();
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);
    zassert_true(hybrid_router_ble_is_fresh(&f.router, HYBRID_ROUTER_BLE_FRESH_MS));
    zassert_false(hybrid_router_ble_is_fresh(&f.router,
                                             HYBRID_ROUTER_BLE_FRESH_MS + 1U));
    make_packet(&p, 70U, 3U);
    zassert_equal(hybrid_router_send_local(&f.router, &p,
                                           HYBRID_ROUTER_BLE_FRESH_MS + 1U).result,
                  HYBRID_ROUTER_NO_LINK_QUEUED);
    zassert_equal(f.mock.ble_calls, 0U);

    hybrid_router_ble_peer_seen(&f.router, 20000U, -60);
    hybrid_router_ble_peer_lost(&f.router);
    zassert_false(hybrid_router_ble_is_fresh(&f.router, 20001U));

    hybrid_router_ble_peer_seen(&f.router, 30000U, -95);
    zassert_false(hybrid_router_ble_is_fresh(&f.router, 30000U));
}

ZTEST(hybrid_router, test_weak_rssi_does_not_refresh_accepted_peer)
{
    setup();
    hybrid_router_ble_peer_seen(&f.router, 1000U, -85);
    zassert_true(hybrid_router_ble_is_fresh(&f.router, 11000U),
                 "the -85 dBm and 10-second limits are inclusive");

    hybrid_router_ble_peer_seen(&f.router, 10000U, -86);
    zassert_true(hybrid_router_ble_is_fresh(&f.router, 11000U),
                 "a weak observation must not replace the accepted timestamp");
    zassert_false(hybrid_router_ble_is_fresh(&f.router, 11001U));

    hybrid_router_ble_peer_lost(&f.router);
    zassert_false(hybrid_router_ble_is_fresh(&f.router, 11000U));
}

ZTEST(hybrid_router, test_uint32_clock_wraparound)
{
    struct ble_mesh_packet p;
    setup();
    hybrid_router_ble_peer_seen(&f.router, 0xFFFFFFF0U, -60);
    zassert_true(hybrid_router_ble_is_fresh(&f.router, 0x00000004U));
    zassert_false(hybrid_router_ble_is_fresh(
        &f.router, 0xFFFFFFF0U + HYBRID_ROUTER_BLE_FRESH_MS + 1U));

    /* Retry scheduled before the wrap fires after it. */
    hybrid_router_ble_peer_lost(&f.router);
    hybrid_router_set_lora_available(&f.router, false);
    make_packet(&p, 80U, 3U);
    zassert_equal(hybrid_router_send_local(&f.router, &p, 0xFFFFFF00U).result,
                  HYBRID_ROUTER_NO_LINK_QUEUED);
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);
    zassert_equal(hybrid_router_poll(&f.router, 0xFFFFFF10U), 0U, "not due");
    zassert_equal(hybrid_router_poll(&f.router, 0xFFFFFF00U +
                                     HYBRID_ROUTER_RETRY_DELAY_MS), 1U);
}

ZTEST(hybrid_router, test_malformed_input_rejected)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 90U, 3U);
    uint8_t bad[BLE_MESH_PACKET_MAX_LEN + 1U] = {0};
    struct ble_mesh_packet p;
    setup();
    hybrid_router_ble_peer_seen(&f.router, 0U, -60);
    hybrid_router_set_lora_available(&f.router, true);
    hybrid_router_set_lora_tx_authorized(&f.router, true);

    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, NULL, 0U, 0U).result,
                  HYBRID_ROUTER_INVALID);
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, 5U, 0U).result,
                  HYBRID_ROUTER_INVALID);
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, bad,
                                        sizeof(bad), 0U).result, HYBRID_ROUTER_INVALID);
    frame[9] = 0U; /* TTL 0 */
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 0U).result,
                  HYBRID_ROUTER_INVALID);
    frame[9] = 9U; /* TTL above max */
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 0U).result,
                  HYBRID_ROUTER_INVALID);
    frame[9] = 3U;
    frame[0] = 0x55U; /* unknown type */
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 0U).result,
                  HYBRID_ROUTER_INVALID);
    frame[0] = BLE_MESH_PACKET_TEXT;
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_NONE, frame, len, 0U).result,
                  HYBRID_ROUTER_INVALID);

    make_packet(&p, 91U, 0U);
    zassert_equal(hybrid_router_send_local(&f.router, &p, 0U).result, HYBRID_ROUTER_INVALID);
    zassert_equal(hybrid_router_send_local(&f.router, NULL, 0U).result, HYBRID_ROUTER_INVALID);

    /* Nothing leaked into delivery, transmit or dedup state. */
    zassert_equal(f.mock.delivered, 0U);
    zassert_equal(f.mock.ble_calls + f.mock.lora_calls, 0U);
    frame[9] = 3U;
    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 1U).result,
                  HYBRID_ROUTER_SENT);
}

ZTEST(hybrid_router, test_beacon_is_ignored_and_not_forwarded)
{
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t len = make_frame(frame, 95U, 3U);
    setup();
    hybrid_router_set_lora_available(&f.router, true);
    frame[0] = BLE_MESH_PACKET_BEACON;
    len = BLE_MESH_PACKET_HEADER_LEN;

    zassert_equal(hybrid_router_receive(&f.router, HYBRID_ROUTER_LINK_BLE, frame, len, 0U).result,
                  HYBRID_ROUTER_NOT_FORWARDED);
    zassert_equal(f.mock.delivered, 0U);
    zassert_equal(f.mock.lora_calls, 0U);
}

ZTEST(hybrid_router, test_init_rejects_null)
{
    struct hybrid_router_ops ops = {0};
    struct ble_mesh_dedup_cache cache;
    struct hybrid_router r;

    zassert_equal(hybrid_router_init(NULL, &cache, &ops), -EINVAL);
    zassert_equal(hybrid_router_init(&r, NULL, &ops), -EINVAL);
    zassert_equal(hybrid_router_init(&r, &cache, NULL), -EINVAL);
}

ZTEST_SUITE(hybrid_router, NULL, NULL, NULL, NULL, NULL);
