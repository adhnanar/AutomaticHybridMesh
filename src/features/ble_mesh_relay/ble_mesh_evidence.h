#ifndef HYBRID_MESH_BLE_MESH_EVIDENCE_H
#define HYBRID_MESH_BLE_MESH_EVIDENCE_H

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

/*
 * Console-independent receive evidence. Cumulative counters are read over
 * a read-only GATT characteristic, so a lossy USB CDC console cannot erase
 * the observation. Wire record (little endian, exactly 20 bytes = one
 * default-MTU read):
 *   0 version(1)  1 role char  2 decoded u16  4 text u16  6 reject u16
 *   8 accept u16  10 duplicate u16  12 last_seq u16  14 last_ttl
 *   15 last_event  16..18 last source id bytes 3..5  19 XOR of bytes 0..18
 * Counters wrap at 65536; compare deltas between two reads.
 */
#define BLE_MESH_EVIDENCE_VERSION 1U
#define BLE_MESH_EVIDENCE_LEN 20U

enum ble_mesh_evidence_event {
    BLE_MESH_EVIDENCE_EVENT_NONE = 0,
    BLE_MESH_EVIDENCE_EVENT_REJECT = 1,
    BLE_MESH_EVIDENCE_EVENT_ACCEPT = 2,
    BLE_MESH_EVIDENCE_EVENT_DUPLICATE = 3,
};

struct ble_mesh_evidence {
    atomic_t decoded;
    atomic_t text;
    atomic_t reject;
    atomic_t accept;
    atomic_t duplicate;
    struct k_spinlock lock;
    uint16_t last_seq;
    uint8_t last_ttl;
    uint8_t last_event;
    uint8_t last_source_tail[3];
};

void ble_mesh_evidence_init(struct ble_mesh_evidence *evidence);
void ble_mesh_evidence_record_decoded(struct ble_mesh_evidence *evidence,
                                      int is_text);
/* Counts a text frame outcome and remembers its sequence/TTL/source tail. */
void ble_mesh_evidence_record_text(struct ble_mesh_evidence *evidence,
                                   enum ble_mesh_evidence_event event,
                                   const uint8_t source_id[6],
                                   uint16_t sequence, uint8_t ttl);
/* Returns BLE_MESH_EVIDENCE_LEN, or -EINVAL/-EMSGSIZE. */
int ble_mesh_evidence_encode(struct ble_mesh_evidence *evidence, char role,
                             uint8_t *buffer, size_t capacity);

#endif
