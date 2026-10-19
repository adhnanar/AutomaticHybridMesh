#include "ble_mesh_evidence.h"

#include <errno.h>

void ble_mesh_evidence_init(struct ble_mesh_evidence *evidence)
{
    atomic_set(&evidence->decoded, 0);
    atomic_set(&evidence->text, 0);
    atomic_set(&evidence->reject, 0);
    atomic_set(&evidence->accept, 0);
    atomic_set(&evidence->duplicate, 0);
    evidence->last_seq = 0U;
    evidence->last_ttl = 0U;
    evidence->last_event = BLE_MESH_EVIDENCE_EVENT_NONE;
    evidence->last_source_tail[0] = 0U;
    evidence->last_source_tail[1] = 0U;
    evidence->last_source_tail[2] = 0U;
}

void ble_mesh_evidence_record_decoded(struct ble_mesh_evidence *evidence,
                                      int is_text)
{
    atomic_inc(&evidence->decoded);
    if (is_text) {
        atomic_inc(&evidence->text);
    }
}

void ble_mesh_evidence_record_text(struct ble_mesh_evidence *evidence,
                                   enum ble_mesh_evidence_event event,
                                   const uint8_t source_id[6],
                                   uint16_t sequence, uint8_t ttl)
{
    switch (event) {
    case BLE_MESH_EVIDENCE_EVENT_REJECT:
        atomic_inc(&evidence->reject);
        break;
    case BLE_MESH_EVIDENCE_EVENT_ACCEPT:
        atomic_inc(&evidence->accept);
        break;
    case BLE_MESH_EVIDENCE_EVENT_DUPLICATE:
        atomic_inc(&evidence->duplicate);
        break;
    default:
        return;
    }

    k_spinlock_key_t key = k_spin_lock(&evidence->lock);
    evidence->last_seq = sequence;
    evidence->last_ttl = ttl;
    evidence->last_event = (uint8_t)event;
    evidence->last_source_tail[0] = source_id[3];
    evidence->last_source_tail[1] = source_id[4];
    evidence->last_source_tail[2] = source_id[5];
    k_spin_unlock(&evidence->lock, key);
}

static void put_u16(uint8_t *out, atomic_val_t value)
{
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8) & 0xFFU);
}

int ble_mesh_evidence_encode(struct ble_mesh_evidence *evidence, char role,
                             uint8_t *buffer, size_t capacity)
{
    if (evidence == NULL || buffer == NULL) {
        return -EINVAL;
    }
    if (capacity < BLE_MESH_EVIDENCE_LEN) {
        return -EMSGSIZE;
    }

    buffer[0] = BLE_MESH_EVIDENCE_VERSION;
    buffer[1] = (uint8_t)role;
    put_u16(&buffer[2], atomic_get(&evidence->decoded));
    put_u16(&buffer[4], atomic_get(&evidence->text));
    put_u16(&buffer[6], atomic_get(&evidence->reject));
    put_u16(&buffer[8], atomic_get(&evidence->accept));
    put_u16(&buffer[10], atomic_get(&evidence->duplicate));

    k_spinlock_key_t key = k_spin_lock(&evidence->lock);
    buffer[12] = (uint8_t)(evidence->last_seq & 0xFFU);
    buffer[13] = (uint8_t)(evidence->last_seq >> 8);
    buffer[14] = evidence->last_ttl;
    buffer[15] = evidence->last_event;
    buffer[16] = evidence->last_source_tail[0];
    buffer[17] = evidence->last_source_tail[1];
    buffer[18] = evidence->last_source_tail[2];
    k_spin_unlock(&evidence->lock, key);

    uint8_t checksum = 0U;
    for (size_t i = 0; i < BLE_MESH_EVIDENCE_LEN - 1U; ++i) {
        checksum ^= buffer[i];
    }
    buffer[19] = checksum;
    return (int)BLE_MESH_EVIDENCE_LEN;
}
