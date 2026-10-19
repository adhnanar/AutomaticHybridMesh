#include <errno.h>

#include <zephyr/kernel.h>

#include "ble_inbox.h"
#include "ble_inbox_service.h"

static struct ble_inbox inbox;
static struct ble_inbox_snapshot snapshots[CONFIG_BT_MAX_CONN];
static K_MUTEX_DEFINE(inbox_mutex);

int ble_inbox_service_record(const uint8_t *source_id, const uint8_t *text,
                             size_t length)
{
    int err;

    k_mutex_lock(&inbox_mutex, K_FOREVER);
    err = ble_inbox_push(&inbox, source_id, text, length);
    k_mutex_unlock(&inbox_mutex);
    return err;
}

ssize_t ble_inbox_service_read(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr,
                               void *buf, uint16_t len, uint16_t offset)
{
    uint8_t index;
    int n;

    ARG_UNUSED(attr);
    if (conn == NULL) {
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }
    index = bt_conn_index(conn);
    if (index >= ARRAY_SIZE(snapshots)) {
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }

    k_mutex_lock(&inbox_mutex, K_FOREVER);
    n = ble_inbox_snapshot_att_read(&inbox, &snapshots[index], offset, buf,
                                    len);
    k_mutex_unlock(&inbox_mutex);

    if (n == -ENODATA || n == -EINVAL) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    if (n < 0) {
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }
    return n;
}

void ble_inbox_service_disconnected(struct bt_conn *conn)
{
    uint8_t index = bt_conn_index(conn);

    if (index < ARRAY_SIZE(snapshots)) {
        k_mutex_lock(&inbox_mutex, K_FOREVER);
        ble_inbox_snapshot_clear(&snapshots[index]);
        k_mutex_unlock(&inbox_mutex);
    }
}
