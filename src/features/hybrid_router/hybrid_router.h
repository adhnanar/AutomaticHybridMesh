#ifndef HYBRID_MESH_HYBRID_ROUTER_H
#define HYBRID_MESH_HYBRID_ROUTER_H

/*
 * Software-only hybrid BLE + LoRa routing policy.
 *
 * The router owns no radio. BLE/LoRa transmit and local delivery are injected
 * operations, time is an injected uint32 millisecond monotonic value (wrap
 * safe for spans < 2^31 ms), and BLE reachability is whatever the caller last
 * reported via hybrid_router_ble_peer_seen()/hybrid_router_ble_peer_lost().
 * It never touches GATT, BLE or SX1262 drivers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ble_mesh_packet.h"

/*
 * Threading: all router entry points (send_local, receive, poll, setters) and
 * the shared dedup cache must be serialized by the caller (one thread or one
 * external mutex). Ops callbacks must not re-enter the router. The deliver
 * callback returns void, so it cannot report errors; delivered/
 * outcome.delivered_locally mean "deliver() was invoked", not "consumer
 * accepted it". With a NULL deliver, nothing is counted as delivered but
 * relay still follows TTL and dedup rules.
 *
 * Local QUEUE_FULL: the packet was neither sent nor retained, so its dedup
 * admission is rolled back (guarded) and the same source+sequence may be
 * resubmitted once room exists. Packets already admitted are never cleared.
 *
 * BLE reachability is reported only after the integration verifies a peer,
 * including its RSSI policy. The router applies the freshness limit itself.
 * BLE is preferred for local sends and relays; authorized LoRa is a fallback.
 * This remains a transport-choice heuristic, not destination-aware routing.
 */
#define HYBRID_ROUTER_PENDING_MAX 4U
#define HYBRID_ROUTER_RETRY_DELAY_MS 1000U
#define HYBRID_ROUTER_MAX_RETRIES 3U
#define HYBRID_ROUTER_BLE_FRESH_MS 10000U
#define HYBRID_ROUTER_BLE_MIN_RSSI_DBM (-85)

enum hybrid_router_link {
    HYBRID_ROUTER_LINK_NONE = 0,
    HYBRID_ROUTER_LINK_BLE,
    HYBRID_ROUTER_LINK_LORA,
};

enum hybrid_router_result {
    HYBRID_ROUTER_SENT = 0,       /* sent on the first-choice link */
    HYBRID_ROUTER_SENT_FALLBACK,  /* first choice failed, other link sent */
    HYBRID_ROUTER_QUEUED_RETRY,   /* every attempted send failed, pending */
    HYBRID_ROUTER_NO_LINK_QUEUED, /* no link usable now, pending */
    HYBRID_ROUTER_LORA_BLOCKED_QUEUED, /* queued; LoRa TX is unauthorized */
    HYBRID_ROUTER_QUEUE_FULL,     /* could not send nor retain; dropped */
    HYBRID_ROUTER_DUPLICATE,      /* suppressed by shared dedup cache */
    HYBRID_ROUTER_INVALID,        /* malformed packet / bad arguments */
    HYBRID_ROUTER_NOT_FORWARDED,  /* valid, delivered, TTL1 or non-text */
};

struct hybrid_router_ops {
    int (*ble_send)(void *ctx, const uint8_t *frame, size_t length);
    int (*lora_send)(void *ctx, const uint8_t *frame, size_t length);
    void (*deliver)(void *ctx, const struct ble_mesh_packet *packet);
    void *ctx;
};

struct hybrid_router_outcome {
    enum hybrid_router_result result;
    enum hybrid_router_link sent_via; /* link that carried it, else NONE */
    bool delivered_locally;
};

struct hybrid_router_stats {
    uint32_t delivered;
    uint32_t duplicates;
    uint32_t invalid;
    uint32_t sent_ble;
    uint32_t sent_lora;
    uint32_t fallbacks;
    uint32_t retries_ok;
    uint32_t retries_dropped;
    uint32_t queue_full;
};

struct hybrid_router_pending {
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN]; /* final on-air bytes, TTL fixed */
    uint8_t length;
    uint8_t attempts;
    enum hybrid_router_link ingress;
    uint32_t next_retry_ms;
    bool valid;
};

struct hybrid_router {
    struct ble_mesh_dedup_cache *cache; /* caller-owned, shared across media */
    struct hybrid_router_ops ops;
    bool lora_available;
    bool lora_tx_authorized;
    bool allow_same_medium_relay;
    bool ble_reachable;
    uint32_t ble_seen_ms;
    struct hybrid_router_pending pending[HYBRID_ROUTER_PENDING_MAX];
    struct hybrid_router_stats stats;
};

/* Returns 0, or -EINVAL when router/cache/ops are unusable. */
int hybrid_router_init(struct hybrid_router *router,
                       struct ble_mesh_dedup_cache *shared_cache,
                       const struct hybrid_router_ops *ops);

void hybrid_router_set_lora_available(struct hybrid_router *router,
                                      bool available);
/* Authorization is external; init always leaves LoRa TX unauthorized. */
void hybrid_router_set_lora_tx_authorized(struct hybrid_router *router,
                                          bool authorized);
/* Reports below the RSSI threshold do not establish BLE reachability. */
void hybrid_router_ble_peer_seen(struct hybrid_router *router, uint32_t now_ms,
                                 int8_t rssi_dbm);
void hybrid_router_ble_peer_lost(struct hybrid_router *router);
bool hybrid_router_ble_is_fresh(const struct hybrid_router *router,
                                uint32_t now_ms);

/* Originate a TEXT packet; TTL is sent as given (not decremented). */
struct hybrid_router_outcome hybrid_router_send_local(
    struct hybrid_router *router, const struct ble_mesh_packet *packet,
    uint32_t now_ms);

/* Handle a received frame from the given ingress medium. */
struct hybrid_router_outcome hybrid_router_receive(
    struct hybrid_router *router, enum hybrid_router_link ingress,
    const uint8_t *frame, size_t length, uint32_t now_ms);

/* Retry due pending records. Returns the number successfully sent. */
size_t hybrid_router_poll(struct hybrid_router *router, uint32_t now_ms);

size_t hybrid_router_pending_count(const struct hybrid_router *router);

#endif
