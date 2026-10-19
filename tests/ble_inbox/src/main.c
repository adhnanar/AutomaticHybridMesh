#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "ble_inbox.h"

static const uint8_t src_a[BLE_INBOX_SOURCE_ID_LEN] = {1, 2, 3, 4, 5, 6};
static const uint8_t src_b[BLE_INBOX_SOURCE_ID_LEN] = {0xa, 0xb, 0xc, 0xd, 0xe, 0xf};

static void push_n(struct ble_inbox *in, unsigned int n)
{
    for (unsigned int i = 0; i < n; i++) {
        uint8_t t = (uint8_t)('A' + i);

        zassert_equal(ble_inbox_push(in, src_a, &t, 1), 0);
    }
}

ZTEST(ble_inbox, test_empty_image)
{
    struct ble_inbox in;
    uint8_t out[8];

    ble_inbox_init(&in);
    zassert_equal(ble_inbox_image_len(&in), BLE_INBOX_HEADER_LEN);
    zassert_equal(ble_inbox_read(&in, 0, out, sizeof(out)), 4);
    zassert_equal(out[0], BLE_INBOX_IMAGE_VERSION);
    zassert_equal(out[1], 0);
    zassert_equal(ble_inbox_read(&in, 4, out, sizeof(out)), 0);
}

ZTEST(ble_inbox, test_newest_first_layout)
{
    struct ble_inbox in;
    uint8_t out[64];
    const uint8_t hi[] = "hi";
    const uint8_t yo[] = "yo!";

    ble_inbox_init(&in);
    zassert_equal(ble_inbox_push(&in, src_a, hi, 2), 0);
    zassert_equal(ble_inbox_push(&in, src_b, yo, 3), 0);
    zassert_equal(ble_inbox_read(&in, 0, out, sizeof(out)), 4 + 11 + 12);
    zassert_equal(out[1], 2);
    zassert_equal(out[4], 1);               /* newest seq */
    zassert_mem_equal(&out[6], src_b, 6);
    zassert_equal(out[12], 3);
    zassert_mem_equal(&out[13], "yo!", 3);
    zassert_equal(out[16], 0);              /* older seq */
    zassert_mem_equal(&out[18], src_a, 6);
    zassert_equal(out[24], 2);
}

ZTEST(ble_inbox, test_overflow_drops_oldest)
{
    struct ble_inbox in;
    uint8_t out[BLE_INBOX_IMAGE_MAX_LEN];

    ble_inbox_init(&in);
    push_n(&in, BLE_INBOX_SLOTS + 3);
    zassert_equal(ble_inbox_read(&in, 0, out, sizeof(out)),
                  4 + BLE_INBOX_SLOTS * 10);
    zassert_equal(out[1], BLE_INBOX_SLOTS);
    zassert_equal(out[2], 3);
    zassert_equal(out[3], 0);
    zassert_equal(out[4], BLE_INBOX_SLOTS + 2);               /* newest seq */
    zassert_equal(out[4 + 9], 'A' + BLE_INBOX_SLOTS + 2);
    zassert_equal(out[4 + 7 * 10], 3);                        /* oldest kept */
}

ZTEST(ble_inbox, test_mtu23_chunked_read_matches_full)
{
    struct ble_inbox in;
    uint8_t full[BLE_INBOX_IMAGE_MAX_LEN];
    uint8_t chunked[BLE_INBOX_IMAGE_MAX_LEN];
    size_t off = 0;
    int n;

    ble_inbox_init(&in);
    push_n(&in, 5);
    n = ble_inbox_read(&in, 0, full, sizeof(full));
    zassert_true(n > 22);
    while ((n = ble_inbox_read(&in, off, &chunked[off], 22)) > 0) {
        zassert_true(n <= 22);
        off += (size_t)n;
    }
    zassert_equal(off, ble_inbox_image_len(&in));
    zassert_mem_equal(full, chunked, off);
}

ZTEST(ble_inbox, test_invalid_inputs_store_nothing)
{
    struct ble_inbox in;
    uint8_t big[BLE_INBOX_MAX_TEXT_LEN + 1] = {0};
    uint8_t out[4];

    ble_inbox_init(&in);
    zassert_equal(ble_inbox_push(NULL, src_a, big, 1), -EINVAL);
    zassert_equal(ble_inbox_push(&in, NULL, big, 1), -EINVAL);
    zassert_equal(ble_inbox_push(&in, src_a, NULL, 1), -EINVAL);
    zassert_equal(ble_inbox_push(&in, src_a, big, 0), -EINVAL);
    zassert_equal(ble_inbox_push(&in, src_a, big, sizeof(big)), -EINVAL);
    zassert_equal(ble_inbox_push(&in, src_a, big, BLE_INBOX_MAX_TEXT_LEN), 0);
    zassert_equal(in.count, 1);
    zassert_equal(ble_inbox_read(NULL, 0, out, 4), -EINVAL);
    zassert_equal(ble_inbox_read(&in, 0, NULL, 4), -EINVAL);
    zassert_equal(ble_inbox_read(&in, 1000, out, 4), 0);
}

ZTEST(ble_inbox, test_reset_clears_and_seq_wraps)
{
    struct ble_inbox in;
    uint8_t t = 'x';
    uint8_t out[8];

    ble_inbox_init(&in);
    in.next_seq = UINT16_MAX;
    in.dropped = UINT16_MAX;
    in.count = BLE_INBOX_SLOTS;
    zassert_equal(ble_inbox_push(&in, src_a, &t, 1), 0);
    zassert_equal(in.next_seq, 0);
    zassert_equal(in.dropped, UINT16_MAX); /* saturates */
    ble_inbox_init(&in);
    zassert_equal(ble_inbox_read(&in, 0, out, sizeof(out)), 4);
    zassert_equal(out[1], 0);
    zassert_equal(in.next_seq, 0);
}

ZTEST(ble_inbox, test_snapshot_frozen_across_blob_reads_with_concurrent_push)
{
    struct ble_inbox in;
    struct ble_inbox_snapshot snap;
    uint8_t full[BLE_INBOX_IMAGE_MAX_LEN];
    uint8_t got[BLE_INBOX_IMAGE_MAX_LEN];
    size_t off = 0;
    uint8_t t = 'Z';
    int n;

    ble_inbox_init(&in);
    ble_inbox_snapshot_clear(&snap);
    push_n(&in, 5);
    zassert_equal(ble_inbox_read(&in, 0, full, sizeof(full)) > 22, true);

    n = ble_inbox_snapshot_att_read(&in, &snap, 0, got, 22);
    zassert_equal(n, 22);
    off = 22;
    zassert_equal(ble_inbox_push(&in, src_b, &t, 1), 0); /* mid-sequence */
    while ((n = ble_inbox_snapshot_att_read(&in, &snap, off, &got[off], 22)) > 0) {
        off += (size_t)n;
    }
    zassert_equal(n, 0);
    zassert_equal(off, snap.length);
    zassert_mem_equal(full, got, off);

    /* A new offset-0 read refreshes and sees the new entry. */
    n = ble_inbox_snapshot_att_read(&in, &snap, 0, got, 22);
    zassert_equal(n, 22);
    zassert_equal(got[1], 6);
}

ZTEST(ble_inbox, test_snapshot_policy_errors)
{
    struct ble_inbox in;
    struct ble_inbox_snapshot snap;
    uint8_t out[8];

    ble_inbox_init(&in);
    ble_inbox_snapshot_clear(&snap);
    zassert_equal(ble_inbox_snapshot_att_read(&in, &snap, 4, out, 8), -ENODATA);
    zassert_equal(ble_inbox_snapshot_att_read(&in, &snap, 0, out, 8), 4);
    zassert_equal(ble_inbox_snapshot_att_read(&in, &snap, 4, out, 8), 0);
    zassert_equal(ble_inbox_snapshot_att_read(&in, &snap, 5, out, 8), -EINVAL);
    ble_inbox_snapshot_clear(&snap);
    zassert_equal(ble_inbox_snapshot_read(&snap, 0, out, 8), -ENODATA);
    zassert_equal(ble_inbox_snapshot_att_read(NULL, &snap, 0, out, 8), -EINVAL);
    zassert_equal(ble_inbox_snapshot_att_read(&in, NULL, 0, out, 8), -EINVAL);
    zassert_equal(ble_inbox_snapshot_take(&in, NULL), -EINVAL);
}

ZTEST(ble_inbox, test_full_snapshot_fits_fixed_buffer)
{
    struct ble_inbox in;
    struct ble_inbox_snapshot snap;
    uint8_t big[BLE_INBOX_MAX_TEXT_LEN] = {0};

    ble_inbox_init(&in);
    for (unsigned int i = 0; i < BLE_INBOX_SLOTS + 2; i++) {
        zassert_equal(ble_inbox_push(&in, src_a, big, sizeof(big)), 0);
    }
    zassert_equal(ble_inbox_snapshot_take(&in, &snap), 0);
    zassert_equal(snap.length, BLE_INBOX_IMAGE_MAX_LEN);
}

ZTEST_SUITE(ble_inbox, NULL, NULL, NULL, NULL, NULL);
