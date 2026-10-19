#ifndef HYBRID_MESH_HYBRID_ROUTER_LOCAL_ACCEPTANCE_H
#define HYBRID_MESH_HYBRID_ROUTER_LOCAL_ACCEPTANCE_H

#include <errno.h>
#include <stdbool.h>

#include "hybrid_router.h"

static inline bool hybrid_router_result_is_locally_accepted(
    enum hybrid_router_result result)
{
    switch (result) {
    case HYBRID_ROUTER_QUEUED_RETRY:
    case HYBRID_ROUTER_NO_LINK_QUEUED:
    case HYBRID_ROUTER_LORA_BLOCKED_QUEUED:
    case HYBRID_ROUTER_SENT:
    case HYBRID_ROUTER_SENT_FALLBACK:
        return true;
    default:
        return false;
    }
}

/*
 * Pure mapping of a router result to a GATT local-write status (0 or -errno).
 * DUPLICATE (-EALREADY) and NOT_FORWARDED (-ENOTSUP) are never new acceptance.
 * Unknown values fail closed with -EINVAL.
 */
static inline int hybrid_router_result_to_gatt_status(
    enum hybrid_router_result result)
{
    if (hybrid_router_result_is_locally_accepted(result)) {
        return 0;
    }

    switch (result) {
    case HYBRID_ROUTER_QUEUE_FULL:
        return -ENOSPC;
    case HYBRID_ROUTER_DUPLICATE:
        return -EALREADY;
    case HYBRID_ROUTER_NOT_FORWARDED:
        return -ENOTSUP;
    case HYBRID_ROUTER_INVALID:
    default:
        return -EINVAL;
    }
}

#endif
