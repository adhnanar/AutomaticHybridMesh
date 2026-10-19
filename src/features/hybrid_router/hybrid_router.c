#include <errno.h>
#include <string.h>

#include "hybrid_router.h"

static bool link_usable(const struct hybrid_router *router,
                        enum hybrid_router_link link, uint32_t now_ms)
{
    if (link == HYBRID_ROUTER_LINK_BLE) {
        return router->ops.ble_send != NULL &&
               hybrid_router_ble_is_fresh(router, now_ms);
    }
    return link == HYBRID_ROUTER_LINK_LORA && router->ops.lora_send != NULL &&
           router->lora_available && router->lora_tx_authorized;
}

/*
 * BLE-first ordered candidates. Relays may not reflect onto their ingress
 * medium unless same-medium forwarding has explicitly been enabled.
 */
static size_t build_candidates(const struct hybrid_router *router,
                               enum hybrid_router_link ingress, uint32_t now_ms,
                               enum hybrid_router_link out[2])
{
    static const enum hybrid_router_link order[] = {
        HYBRID_ROUTER_LINK_BLE,
        HYBRID_ROUTER_LINK_LORA,
    };
    size_t n = 0U;

    for (size_t i = 0U; i < sizeof(order) / sizeof(order[0]); ++i) {
        if (order[i] == ingress && !router->allow_same_medium_relay) {
            continue;
        }
        if (link_usable(router, order[i], now_ms)) {
            out[n++] = order[i];
        }
    }
    return n;
}

static int send_on(struct hybrid_router *router, enum hybrid_router_link link,
                   const uint8_t *frame, size_t length)
{
    int rc = (link == HYBRID_ROUTER_LINK_BLE)
                 ? router->ops.ble_send(router->ops.ctx, frame, length)
                 : router->ops.lora_send(router->ops.ctx, frame, length);
    if (rc == 0) {
        if (link == HYBRID_ROUTER_LINK_BLE) {
            ++router->stats.sent_ble;
        } else {
            ++router->stats.sent_lora;
        }
    }
    return rc;
}

/* 0 sent, -ENOLINK no route, -EACCES unauthorized LoRa, -EIO send failures. */
static int transmit(struct hybrid_router *router, const uint8_t *frame,
                    size_t length, enum hybrid_router_link ingress,
                    uint32_t now_ms, enum hybrid_router_link *via,
                    bool *fallback)
{
    enum hybrid_router_link cand[2];
    size_t n = build_candidates(router, ingress, now_ms, cand);

    *via = HYBRID_ROUTER_LINK_NONE;
    *fallback = false;
    if (n == 0U) {
        if (!link_usable(router, HYBRID_ROUTER_LINK_BLE, now_ms) &&
            router->ops.lora_send != NULL && router->lora_available &&
            !router->lora_tx_authorized) {
            return -EACCES;
        }
        return -ENOLINK;
    }
    for (size_t i = 0U; i < n; ++i) {
        if (send_on(router, cand[i], frame, length) == 0) {
            *via = cand[i];
            *fallback = (i > 0U);
            if (*fallback) {
                ++router->stats.fallbacks;
            }
            return 0;
        }
    }
    return -EIO;
}

static struct hybrid_router_pending *pending_alloc(struct hybrid_router *router)
{
    for (size_t i = 0U; i < HYBRID_ROUTER_PENDING_MAX; ++i) {
        if (!router->pending[i].valid) {
            return &router->pending[i];
        }
    }
    return NULL;
}

static struct hybrid_router_outcome route_frame(
    struct hybrid_router *router, const uint8_t *frame, size_t length,
    enum hybrid_router_link ingress, uint32_t now_ms, bool delivered)
{
    struct hybrid_router_outcome out = {
        .result = HYBRID_ROUTER_INVALID,
        .sent_via = HYBRID_ROUTER_LINK_NONE,
        .delivered_locally = delivered,
    };
    bool fallback = false;
    int rc = transmit(router, frame, length, ingress, now_ms, &out.sent_via,
                      &fallback);

    if (rc == 0) {
        out.result = fallback ? HYBRID_ROUTER_SENT_FALLBACK
                              : HYBRID_ROUTER_SENT;
        return out;
    }

    struct hybrid_router_pending *slot = pending_alloc(router);
    if (slot == NULL) {
        ++router->stats.queue_full;
        out.result = HYBRID_ROUTER_QUEUE_FULL;
        return out;
    }

    memcpy(slot->frame, frame, length);
    slot->length = (uint8_t)length;
    slot->attempts = 0U;
    slot->ingress = ingress;
    slot->next_retry_ms = now_ms + HYBRID_ROUTER_RETRY_DELAY_MS;
    slot->valid = true;
    if (rc == -EACCES) {
        out.result = HYBRID_ROUTER_LORA_BLOCKED_QUEUED;
    } else {
        out.result = (rc == -ENOLINK) ? HYBRID_ROUTER_NO_LINK_QUEUED
                                      : HYBRID_ROUTER_QUEUED_RETRY;
    }
    return out;
}

int hybrid_router_init(struct hybrid_router *router,
                       struct ble_mesh_dedup_cache *shared_cache,
                       const struct hybrid_router_ops *ops)
{
    if (router == NULL || shared_cache == NULL || ops == NULL) {
        return -EINVAL;
    }
    memset(router, 0, sizeof(*router));
    router->cache = shared_cache;
    router->ops = *ops;
    router->allow_same_medium_relay = true;
    return 0;
}

void hybrid_router_set_lora_available(struct hybrid_router *router,
                                      bool available)
{
    if (router != NULL) {
        router->lora_available = available;
    }
}

void hybrid_router_set_lora_tx_authorized(struct hybrid_router *router,
                                          bool authorized)
{
    if (router != NULL) {
        router->lora_tx_authorized = authorized;
    }
}

void hybrid_router_ble_peer_seen(struct hybrid_router *router, uint32_t now_ms,
                                 int8_t rssi_dbm)
{
    if (router != NULL && rssi_dbm >= HYBRID_ROUTER_BLE_MIN_RSSI_DBM) {
        router->ble_reachable = true;
        router->ble_seen_ms = now_ms;
    }
}

void hybrid_router_ble_peer_lost(struct hybrid_router *router)
{
    if (router != NULL) {
        router->ble_reachable = false;
    }
}

bool hybrid_router_ble_is_fresh(const struct hybrid_router *router,
                                uint32_t now_ms)
{
    return router != NULL && router->ble_reachable &&
           (uint32_t)(now_ms - router->ble_seen_ms) <=
               HYBRID_ROUTER_BLE_FRESH_MS;
}

struct hybrid_router_outcome hybrid_router_send_local(
    struct hybrid_router *router, const struct ble_mesh_packet *packet,
    uint32_t now_ms)
{
    struct hybrid_router_outcome out = {.result = HYBRID_ROUTER_INVALID};
    uint8_t frame[BLE_MESH_PACKET_MAX_LEN];
    size_t length = 0U;

    if (router == NULL || packet == NULL ||
        packet->type != BLE_MESH_PACKET_TEXT ||
        ble_mesh_packet_encode(packet, frame, sizeof(frame), &length) != 0) {
        if (router != NULL) {
            ++router->stats.invalid;
        }
        return out;
    }

    /*
     * Snapshot so a QUEUE_FULL (neither sent nor retained) can undo exactly
     * this admission. Only entries touched by this call are reverted; a
     * packet admitted earlier is rejected as DUPLICATE before this point and
     * is never cleared.
     */
    struct ble_mesh_dedup_cache before = *router->cache;

    /* Mark our own packet seen so echoes from either medium are dropped. */
    if (!ble_mesh_dedup_accept(router->cache, packet->source_id,
                               packet->sequence, now_ms)) {
        ++router->stats.duplicates;
        out.result = HYBRID_ROUTER_DUPLICATE;
        return out;
    }
    size_t admitted_next = router->cache->next_entry;

    out = route_frame(router, frame, length, HYBRID_ROUTER_LINK_NONE, now_ms,
                      false);
    /*
     * Guarded rollback: only when the cache is still in its post-admission
     * shape (callbacks did not re-enter the router). Inputs are expected to
     * be serialized; otherwise we conservatively keep the entry.
     */
    if (out.result == HYBRID_ROUTER_QUEUE_FULL &&
        router->cache->next_entry == admitted_next) {
        *router->cache = before;
    }
    return out;
}

struct hybrid_router_outcome hybrid_router_receive(
    struct hybrid_router *router, enum hybrid_router_link ingress,
    const uint8_t *frame, size_t length, uint32_t now_ms)
{
    struct hybrid_router_outcome out = {.result = HYBRID_ROUTER_INVALID};
    struct ble_mesh_packet packet;

    if (router == NULL) {
        return out;
    }
    if (ingress != HYBRID_ROUTER_LINK_BLE &&
        ingress != HYBRID_ROUTER_LINK_LORA) {
        ++router->stats.invalid;
        return out;
    }
    if (ble_mesh_packet_decode(frame, length, &packet) != 0) {
        ++router->stats.invalid;
        return out;
    }
    if (packet.type != BLE_MESH_PACKET_TEXT) {
        out.result = HYBRID_ROUTER_NOT_FORWARDED;
        return out;
    }

    /* The one dedup decision, shared by both media. */
    if (!ble_mesh_dedup_accept(router->cache, packet.source_id,
                               packet.sequence, now_ms)) {
        ++router->stats.duplicates;
        out.result = HYBRID_ROUTER_DUPLICATE;
        return out;
    }

    /* Counted only when a callback actually ran; relay proceeds regardless. */
    if (router->ops.deliver != NULL) {
        router->ops.deliver(router->ops.ctx, &packet);
        ++router->stats.delivered;
        out.delivered_locally = true;
    }

    if (!ble_mesh_packet_prepare_relay(&packet)) {
        out.result = HYBRID_ROUTER_NOT_FORWARDED;
        return out;
    }

    uint8_t relay[BLE_MESH_PACKET_MAX_LEN];
    size_t relay_length = 0U;
    if (ble_mesh_packet_encode(&packet, relay, sizeof(relay),
                               &relay_length) != 0) {
        out.result = HYBRID_ROUTER_INVALID;
        return out;
    }
    return route_frame(router, relay, relay_length, ingress, now_ms,
                       out.delivered_locally);
}

size_t hybrid_router_poll(struct hybrid_router *router, uint32_t now_ms)
{
    size_t sent = 0U;

    if (router == NULL) {
        return 0U;
    }
    for (size_t i = 0U; i < HYBRID_ROUTER_PENDING_MAX; ++i) {
        struct hybrid_router_pending *p = &router->pending[i];
        enum hybrid_router_link via;
        bool fallback;

        if (!p->valid || (int32_t)(now_ms - p->next_retry_ms) < 0) {
            continue;
        }
        if (transmit(router, p->frame, p->length, p->ingress, now_ms, &via,
                     &fallback) == 0) {
            p->valid = false;
            ++router->stats.retries_ok;
            ++sent;
            continue;
        }
        if (++p->attempts >= HYBRID_ROUTER_MAX_RETRIES) {
            p->valid = false;
            ++router->stats.retries_dropped;
        } else {
            p->next_retry_ms = now_ms + HYBRID_ROUTER_RETRY_DELAY_MS;
        }
    }
    return sent;
}

size_t hybrid_router_pending_count(const struct hybrid_router *router)
{
    size_t count = 0U;

    if (router != NULL) {
        for (size_t i = 0U; i < HYBRID_ROUTER_PENDING_MAX; ++i) {
            count += router->pending[i].valid ? 1U : 0U;
        }
    }
    return count;
}
