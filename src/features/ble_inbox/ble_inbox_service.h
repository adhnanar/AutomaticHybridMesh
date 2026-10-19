#ifndef HYBRID_MESH_BLE_INBOX_SERVICE_H
#define HYBRID_MESH_BLE_INBOX_SERVICE_H

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

/* Zephyr glue for the ble_inbox ring; compiled only with CONFIG_BLE_INBOX. */

/* Read-only inbox characteristic UUID 7b3e0006-6f2a-4c91-9a3d-2f5b6c7d8e00 */
#define BT_UUID_HYBRID_INBOX_VAL \
    BT_UUID_128_ENCODE(0x7b3e0006, 0x6f2a, 0x4c91, 0x9a3d, 0x2f5b6c7d8e00)
#define BT_UUID_HYBRID_INBOX BT_UUID_DECLARE_128(BT_UUID_HYBRID_INBOX_VAL)

/*
 * Records one accepted remote text (source ID + payload). Mutex protected;
 * invalid input (empty or > BLE_INBOX_MAX_TEXT_LEN) returns -EINVAL.
 */
int ble_inbox_service_record(const uint8_t *source_id, const uint8_t *text,
                             size_t length);

/* GATT read callback: per-connection frozen snapshot, see ble_inbox.h. */
ssize_t ble_inbox_service_read(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr,
                               void *buf, uint16_t len, uint16_t offset);

/* Drops the connection's frozen snapshot. */
void ble_inbox_service_disconnected(struct bt_conn *conn);

#endif
