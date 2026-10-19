#include "ble_inbox.h"

#include <errno.h>
#include <string.h>

_Static_assert(BLE_INBOX_IMAGE_MAX_LEN <= UINT8_MAX,
               "snapshot length field is one byte");
void ble_inbox_init(struct ble_inbox *inbox)
{
    if (inbox != NULL) {
        memset(inbox, 0, sizeof(*inbox));
    }
}

int ble_inbox_push(struct ble_inbox *inbox,
                   const uint8_t source_id[BLE_INBOX_SOURCE_ID_LEN],
                   const uint8_t *text, size_t length)
{
    struct ble_inbox_entry *entry;

    if (inbox == NULL || source_id == NULL || text == NULL || length == 0U ||
        length > BLE_INBOX_MAX_TEXT_LEN || inbox->head >= BLE_INBOX_SLOTS ||
        inbox->count > BLE_INBOX_SLOTS) {
        return -EINVAL;
    }

    entry = &inbox->entries[inbox->head];
    memset(entry, 0, sizeof(*entry));
    entry->seq = inbox->next_seq++;
    memcpy(entry->source_id, source_id, BLE_INBOX_SOURCE_ID_LEN);
    entry->length = (uint8_t)length;
    memcpy(entry->text, text, length);

    inbox->head = (uint8_t)((inbox->head + 1U) % BLE_INBOX_SLOTS);
    if (inbox->count < BLE_INBOX_SLOTS) {
        inbox->count++;
    } else if (inbox->dropped < UINT16_MAX) {
        inbox->dropped++;
    }
    return 0;
}

static size_t render(const struct ble_inbox *inbox, uint8_t *img)
{
    size_t pos = 0U;
    uint8_t count = inbox->count <= BLE_INBOX_SLOTS ? inbox->count : 0U;

    img[pos++] = BLE_INBOX_IMAGE_VERSION;
    img[pos++] = count;
    img[pos++] = (uint8_t)(inbox->dropped & 0xFFU);
    img[pos++] = (uint8_t)(inbox->dropped >> 8);

    for (uint8_t i = 0U; i < count; i++) {
        uint8_t idx = (uint8_t)((inbox->head + BLE_INBOX_SLOTS - 1U - i) %
                                BLE_INBOX_SLOTS);
        const struct ble_inbox_entry *e = &inbox->entries[idx];
        uint8_t len = e->length <= BLE_INBOX_MAX_TEXT_LEN ?
                      e->length : BLE_INBOX_MAX_TEXT_LEN;

        img[pos++] = (uint8_t)(e->seq & 0xFFU);
        img[pos++] = (uint8_t)(e->seq >> 8);
        memcpy(&img[pos], e->source_id, BLE_INBOX_SOURCE_ID_LEN);
        pos += BLE_INBOX_SOURCE_ID_LEN;
        img[pos++] = len;
        memcpy(&img[pos], e->text, len);
        pos += len;
    }
    return pos;
}

size_t ble_inbox_image_len(const struct ble_inbox *inbox)
{
    uint8_t img[BLE_INBOX_IMAGE_MAX_LEN];

    return inbox == NULL ? 0U : render(inbox, img);
}

void ble_inbox_snapshot_clear(struct ble_inbox_snapshot *snap)
{
    if (snap != NULL) {
        memset(snap, 0, sizeof(*snap));
    }
}

int ble_inbox_snapshot_take(const struct ble_inbox *inbox,
                            struct ble_inbox_snapshot *snap)
{
    if (inbox == NULL || snap == NULL) {
        return -EINVAL;
    }
    snap->length = (uint8_t)render(inbox, snap->image);
    snap->valid = 1U;
    return 0;
}

int ble_inbox_snapshot_read(const struct ble_inbox_snapshot *snap,
                            size_t offset, uint8_t *out, size_t out_cap)
{
    size_t n;

    if (snap == NULL || out == NULL) {
        return -EINVAL;
    }
    if (snap->valid == 0U) {
        return -ENODATA;
    }
    if (offset > snap->length) {
        return -EINVAL;
    }
    n = snap->length - offset;
    if (n > out_cap) {
        n = out_cap;
    }
    memcpy(out, &snap->image[offset], n);
    return (int)n;
}

int ble_inbox_snapshot_att_read(const struct ble_inbox *inbox,
                                struct ble_inbox_snapshot *snap,
                                size_t offset, uint8_t *out, size_t out_cap)
{
    int err;

    if (inbox == NULL || snap == NULL || out == NULL) {
        return -EINVAL;
    }
    if (offset == 0U) {
        err = ble_inbox_snapshot_take(inbox, snap);
        if (err != 0) {
            return err;
        }
    }
    return ble_inbox_snapshot_read(snap, offset, out, out_cap);
}

int ble_inbox_read(const struct ble_inbox *inbox, size_t offset,
                   uint8_t *out, size_t out_cap)
{
    uint8_t img[BLE_INBOX_IMAGE_MAX_LEN];
    size_t len;
    size_t n;

    if (inbox == NULL || out == NULL) {
        return -EINVAL;
    }
    len = render(inbox, img);
    if (offset >= len) {
        return 0;
    }
    n = len - offset;
    if (n > out_cap) {
        n = out_cap;
    }
    memcpy(out, &img[offset], n);
    return (int)n;
}
