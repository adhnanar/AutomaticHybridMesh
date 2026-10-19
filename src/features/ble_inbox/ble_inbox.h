#ifndef HYBRID_MESH_BLE_INBOX_H
#define HYBRID_MESH_BLE_INBOX_H

#include <stddef.h>
#include <stdint.h>

/* Pure C, no Zephyr dependency, no dynamic allocation, not thread-safe:
 * the caller serialises access (e.g. one mutex or one work context).
 */
#define BLE_INBOX_SLOTS 8U
#define BLE_INBOX_SOURCE_ID_LEN 6U
#define BLE_INBOX_MAX_TEXT_LEN 14U /* == BLE_MESH_MAX_TEXT_LEN */
#define BLE_INBOX_IMAGE_VERSION 1U
#define BLE_INBOX_HEADER_LEN 4U
/* seq(2) + source(6) + len(1) */
#define BLE_INBOX_RECORD_FIXED_LEN 9U
#define BLE_INBOX_IMAGE_MAX_LEN \
    (BLE_INBOX_HEADER_LEN + BLE_INBOX_SLOTS * \
     (BLE_INBOX_RECORD_FIXED_LEN + BLE_INBOX_MAX_TEXT_LEN))

struct ble_inbox_entry {
    uint16_t seq;
    uint8_t source_id[BLE_INBOX_SOURCE_ID_LEN];
    uint8_t length;
    uint8_t text[BLE_INBOX_MAX_TEXT_LEN];
};

struct ble_inbox {
    struct ble_inbox_entry entries[BLE_INBOX_SLOTS];
    uint8_t head;     /* next slot to write */
    uint8_t count;    /* valid entries, <= BLE_INBOX_SLOTS */
    uint16_t next_seq;
    uint16_t dropped; /* entries overwritten when full, saturating */
};

/* Clears all state, including seq and dropped (RAM only; lost on reset). */
void ble_inbox_init(struct ble_inbox *inbox);

/*
 * Stores a copy. Returns 0, or -EINVAL for null args, empty text or
 * length > BLE_INBOX_MAX_TEXT_LEN (nothing stored, no truncation). When
 * full the oldest entry is overwritten and dropped is incremented.
 */
int ble_inbox_push(struct ble_inbox *inbox,
                   const uint8_t source_id[BLE_INBOX_SOURCE_ID_LEN],
                   const uint8_t *text, size_t length);

/*
 * Serialises the inbox, newest entry first, as a read-only image:
 *   [0] version  [1] count  [2..3] dropped (LE)
 *   then count x { seq(LE u16), source_id[6], len u8, text[len] }
 * Copies image bytes starting at offset into out (at most out_cap) and
 * returns the number copied, 0 at/after end of image, or -EINVAL for null
 * args. Suits ATT Read / Read Blob at any MTU (22 bytes per read at 23).
 */
int ble_inbox_read(const struct ble_inbox *inbox, size_t offset,
                   uint8_t *out, size_t out_cap);

/* Total image length for the current contents. */
size_t ble_inbox_image_len(const struct ble_inbox *inbox);

/*
 * Fixed-size frozen copy of the image, one per GATT connection, so a
 * multi-ATT Read Blob sequence never mixes bytes from different inbox states.
 */
struct ble_inbox_snapshot {
    uint8_t valid;
    uint8_t length;
    uint8_t image[BLE_INBOX_IMAGE_MAX_LEN];
};

/* Marks the snapshot empty/invalid (e.g. on disconnect). */
void ble_inbox_snapshot_clear(struct ble_inbox_snapshot *snap);

/* Freezes the current image into snap. Caller holds the inbox lock. */
int ble_inbox_snapshot_take(const struct ble_inbox *inbox,
                            struct ble_inbox_snapshot *snap);

/*
 * Reads from the frozen image. -EINVAL for null args, -ENODATA when no
 * snapshot is valid, -EINVAL when offset > image length; otherwise bytes
 * copied (0 at offset == length). Needs no inbox lock.
 */
int ble_inbox_snapshot_read(const struct ble_inbox_snapshot *snap,
                            size_t offset, uint8_t *out, size_t out_cap);

/*
 * ATT Read policy: offset 0 starts a new sequence (re-freezes the snapshot);
 * offset > 0 continues the frozen snapshot and fails with -ENODATA if none
 * exists. Caller holds the inbox lock around the whole call.
 */
int ble_inbox_snapshot_att_read(const struct ble_inbox *inbox,
                                struct ble_inbox_snapshot *snap,
                                size_t offset, uint8_t *out, size_t out_cap);

#endif
