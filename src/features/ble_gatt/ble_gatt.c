#include <errno.h>
#include <string.h>

#include <hal/nrf_ficr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "ble_gatt.h"
#include "ble_gatt_android_name.h"
#include "ble_gatt_value.h"

#if defined(CONFIG_BLE_INBOX) && CONFIG_BLE_INBOX
#include "../ble_inbox/ble_inbox_service.h"
#endif
#define BT_UUID_HYBRID_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x7b3e0001, 0x6f2a, 0x4c91, 0x9a3d, 0x2f5b6c7d8e00)
#define BT_UUID_HYBRID_RX_VAL \
    BT_UUID_128_ENCODE(0x7b3e0002, 0x6f2a, 0x4c91, 0x9a3d, 0x2f5b6c7d8e00)
#define BT_UUID_HYBRID_TX_VAL \
    BT_UUID_128_ENCODE(0x7b3e0003, 0x6f2a, 0x4c91, 0x9a3d, 0x2f5b6c7d8e00)
#define BT_UUID_HYBRID_DISCOVERY_VAL \
    BT_UUID_128_ENCODE(0x7b3e0004, 0x6f2a, 0x4c91, 0x9a3d, 0x2f5b6c7d8e00)

#define BT_UUID_HYBRID_EVIDENCE_VAL \
    BT_UUID_128_ENCODE(0x7b3e0005, 0x6f2a, 0x4c91, 0x9a3d, 0x2f5b6c7d8e00)
#define BT_UUID_HYBRID_EVIDENCE BT_UUID_DECLARE_128(BT_UUID_HYBRID_EVIDENCE_VAL)

#define BLE_GATT_MESH_FRAME_MAX_LEN 24U
#define BLE_GATT_MESH_COMPANY_ID_LEN 2U

#define BT_UUID_HYBRID_SERVICE BT_UUID_DECLARE_128(BT_UUID_HYBRID_SERVICE_VAL)
#define BT_UUID_HYBRID_RX BT_UUID_DECLARE_128(BT_UUID_HYBRID_RX_VAL)
#define BT_UUID_HYBRID_TX BT_UUID_DECLARE_128(BT_UUID_HYBRID_TX_VAL)
#define BT_UUID_HYBRID_DISCOVERY BT_UUID_DECLARE_128(BT_UUID_HYBRID_DISCOVERY_VAL)

static struct ble_gatt_text_value tx_value = {
    .value = "Ready",
    .length = sizeof("Ready") - 1U,
};
static uint8_t discovery_value[BLE_GATT_MAX_VALUE_LEN] = "discovery inactive";
static uint16_t discovery_length = sizeof("discovery inactive") - 1U;
static uint8_t mesh_manufacturer_data[BLE_GATT_MESH_COMPANY_ID_LEN +
                                      BLE_GATT_MESH_FRAME_MAX_LEN] = {
    0xffU, 0xffU
};
static size_t mesh_frame_length;
static struct k_mutex value_mutex;
static ble_gatt_rx_handler_t rx_handler;
static struct k_work_delayable adv_restart_work;
static atomic_t active_connections = ATOMIC_INIT(0);
static int adv_restart_attempts;

static const struct bt_data advertising[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_HYBRID_SERVICE_VAL),
};

#if defined(CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME) && \
    CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME
static char android_device_name[BLE_GATT_ANDROID_NAME_LEN] = "Mesh-000000";
static struct bt_data scan_response[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_HYBRID_SERVICE_VAL),
    BT_DATA(BT_DATA_NAME_COMPLETE, android_device_name,
            sizeof(android_device_name) - 1U),
};
#else
static const struct bt_data scan_response[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_HYBRID_SERVICE_VAL),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1U),
};
#endif

#if defined(CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME) && \
    CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME
static int initialize_android_discovery_name(void)
{
    uint32_t device_id[2] = {
        nrf_ficr_deviceid_get(NRF_FICR, 0U),
        nrf_ficr_deviceid_get(NRF_FICR, 1U),
    };
    uint8_t node_id[BLE_GATT_ANDROID_NODE_ID_LEN];

    sys_put_le32(device_id[0], &node_id[0]);
    sys_put_le16((uint16_t)device_id[1], &node_id[4]);

    return ble_gatt_android_name_format(android_device_name,
                                        sizeof(android_device_name),
                                        node_id);
}
#endif

static int start_advertising_with_mesh_payload(void)
{
    uint8_t manufacturer_data[BLE_GATT_MESH_COMPANY_ID_LEN +
                              BLE_GATT_MESH_FRAME_MAX_LEN];
    size_t frame_length;

    k_mutex_lock(&value_mutex, K_FOREVER);
    frame_length = mesh_frame_length;
    if (frame_length != 0U) {
        memcpy(manufacturer_data, mesh_manufacturer_data,
               BLE_GATT_MESH_COMPANY_ID_LEN + frame_length);
    }
    k_mutex_unlock(&value_mutex);

    if (frame_length == 0U) {
        return bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, advertising,
                               ARRAY_SIZE(advertising), scan_response,
                               ARRAY_SIZE(scan_response));
    }

    const struct bt_data mesh_advertising[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, manufacturer_data,
                BLE_GATT_MESH_COMPANY_ID_LEN + frame_length),
    };
    return bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, mesh_advertising,
                           ARRAY_SIZE(mesh_advertising), scan_response,
                           ARRAY_SIZE(scan_response));
}

static void restart_advertising_delayable(struct k_work *work)
{
    ARG_UNUSED(work);

    if (atomic_get(&active_connections) > 0) {
        printk("BLE GATT advertising restart deferred: connection still active\n");
        return;
    }

    if (adv_restart_attempts >= 3) {
        printk("BLE GATT advertising restart retries exhausted; leaving last payload intact\n");
        adv_restart_attempts = 0;
        return;
    }

    ++adv_restart_attempts;
    int err = start_advertising_with_mesh_payload();
    if (err == 0) {
        adv_restart_attempts = 0;
        printk("BLE GATT advertising restarted after disconnect\n");
        return;
    }

    if (err == -EALREADY) {
        adv_restart_attempts = 0;
        printk("BLE GATT advertising already active after disconnect\n");
        return;
    }

    printk("BLE GATT advertising restart failed: %d; retrying in 1s\n",
           err);
    k_work_reschedule(&adv_restart_work, K_SECONDS(1));
}

static void schedule_advertising_restart(void)
{
    int err = k_work_reschedule(&adv_restart_work, K_NO_WAIT);
    if (err < 0) {
        printk("BLE GATT advertising restart queued after disconnect: %d\n", err);
    }
}

static void ble_gatt_connected(struct bt_conn *conn, uint8_t err)
{
    if (err != 0) {
        printk("BLE GATT connection failed to %s: %d\n",
               bt_conn_dst_str(conn), err);
        return;
    }

    atomic_inc(&active_connections);
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) == 0) {
        printk("BLE GATT connected to %s\n",
               bt_addr_le_str(info.le.dst != NULL ? info.le.dst : info.le.remote));
    } else {
        printk("BLE GATT connected to %s\n", bt_conn_dst_str(conn));
    }
}

static void ble_gatt_disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("BLE GATT disconnected from %s (reason 0x%02x)\n",
           bt_conn_dst_str(conn), reason);
#if defined(CONFIG_BLE_INBOX) && CONFIG_BLE_INBOX
    ble_inbox_service_disconnected(conn);
#endif

    if (atomic_get(&active_connections) > 0) {
        atomic_dec(&active_connections);
    }
    schedule_advertising_restart();
}

BT_CONN_CB_DEFINE(ble_conn_callbacks) = {
    .connected = ble_gatt_connected,
    .disconnected = ble_gatt_disconnected,
};

static ssize_t write_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, uint16_t len, uint16_t offset,
                        uint8_t flags);

static ssize_t read_tx(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                       void *buf, uint16_t len, uint16_t offset)
{
    k_mutex_lock(&value_mutex, K_FOREVER);
    ssize_t result = bt_gatt_attr_read(conn, attr, buf, len, offset,
                                       tx_value.value, tx_value.length);
    k_mutex_unlock(&value_mutex);
    return result;
}

static ssize_t read_discovery(struct bt_conn *conn,
                              const struct bt_gatt_attr *attr,
                              void *buf, uint16_t len, uint16_t offset)
{
    k_mutex_lock(&value_mutex, K_FOREVER);
    ssize_t result = bt_gatt_attr_read(conn, attr, buf, len, offset,
                                       discovery_value, discovery_length);
    k_mutex_unlock(&value_mutex);
    return result;
}

static ble_gatt_evidence_provider_t evidence_provider;

int ble_gatt_set_evidence_provider(ble_gatt_evidence_provider_t provider)
{
    if (provider == NULL) {
        return -EINVAL;
    }
    evidence_provider = provider;
    return 0;
}

static ssize_t read_evidence(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             void *buf, uint16_t len, uint16_t offset)
{
    uint8_t record[BLE_GATT_MAX_VALUE_LEN];
    int length = 0;

    if (evidence_provider != NULL) {
        length = evidence_provider(record, sizeof(record));
        if (length < 0) {
            return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
        }
    }
    return bt_gatt_attr_read(conn, attr, buf, len, offset, record,
                             (uint16_t)length);
}

static void notifications_changed(const struct bt_gatt_attr *attr,
                                  uint16_t value)
{
    ARG_UNUSED(attr);
    printk("BLE GATT notifications %s\n",
           value == BT_GATT_CCC_NOTIFY ? "enabled" : "disabled");
}

#if defined(CONFIG_BLE_INBOX) && CONFIG_BLE_INBOX
/* Opt-in variant: identical service plus a read-only inbox characteristic. */
BT_GATT_SERVICE_DEFINE(ble_gatt_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_HYBRID_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_HYBRID_RX,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, write_rx, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_HYBRID_TX,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_tx, NULL, NULL),
    BT_GATT_CCC(notifications_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_HYBRID_DISCOVERY,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_discovery, NULL, NULL),
    BT_GATT_CCC(notifications_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_HYBRID_EVIDENCE, BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ, read_evidence, NULL, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_HYBRID_INBOX, BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ, ble_inbox_service_read, NULL,
                           NULL)
);
#else
BT_GATT_SERVICE_DEFINE(ble_gatt_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_HYBRID_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_HYBRID_RX,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, write_rx, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_HYBRID_TX,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_tx, NULL, NULL),
    BT_GATT_CCC(notifications_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_HYBRID_DISCOVERY,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_discovery, NULL, NULL),
    BT_GATT_CCC(notifications_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_HYBRID_EVIDENCE, BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ, read_evidence, NULL, NULL)
);
#endif

static int notify_value(const struct bt_uuid *uuid, const char *label,
                        const uint8_t *value, uint16_t length)
{
    int err = bt_gatt_notify_uuid(NULL, uuid, attr_ble_gatt_service,
                                  value, length);
    if (err != 0) {
        printk("BLE GATT %s notification failed: %d\n", label, err);
    }
    return err;
}

static int notify_text_value(const uint8_t *message, uint16_t length,
                             void *context)
{
    ARG_UNUSED(context);
    return notify_value(BT_UUID_HYBRID_TX, "text", message, length);
}

int ble_gatt_publish_discovery_event(const uint8_t *event, size_t length)
{
    if (event == NULL || length == 0U) {
        return -EINVAL;
    }
    if (length > BLE_GATT_MAX_VALUE_LEN) {
        return -EMSGSIZE;
    }

    uint8_t notification[BLE_GATT_MAX_VALUE_LEN];
    memcpy(notification, event, length);

    k_mutex_lock(&value_mutex, K_FOREVER);
    memcpy(discovery_value, event, length);
    discovery_length = (uint16_t)length;
    k_mutex_unlock(&value_mutex);

    return notify_value(BT_UUID_HYBRID_DISCOVERY, "discovery",
                        notification, (uint16_t)length);
}

int ble_gatt_publish_text_message(const uint8_t *message, size_t length)
{
    return ble_gatt_text_value_publish(&value_mutex, &tx_value, message,
                                       length, notify_text_value, NULL);
}

int ble_gatt_update_mesh_advertisement(const uint8_t *frame, size_t length)
{
    if (frame == NULL || length == 0U) {
        return -EINVAL;
    }
    if (length > BLE_GATT_MESH_FRAME_MAX_LEN) {
        return -EMSGSIZE;
    }

    uint8_t manufacturer_data[BLE_GATT_MESH_COMPANY_ID_LEN +
                              BLE_GATT_MESH_FRAME_MAX_LEN];
    size_t manufacturer_data_length = BLE_GATT_MESH_COMPANY_ID_LEN + length;
    k_mutex_lock(&value_mutex, K_FOREVER);
    memcpy(&mesh_manufacturer_data[BLE_GATT_MESH_COMPANY_ID_LEN],
           frame, length);
    mesh_frame_length = length;
    memcpy(manufacturer_data, mesh_manufacturer_data,
           manufacturer_data_length);
    k_mutex_unlock(&value_mutex);

    if (atomic_get(&active_connections) > 0U) {
        return -EAGAIN;
    }

    const struct bt_data advertising[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, manufacturer_data,
                manufacturer_data_length),
    };

    return bt_le_adv_update_data(advertising, ARRAY_SIZE(advertising),
                                 scan_response, ARRAY_SIZE(scan_response));
}

int ble_gatt_set_rx_handler(ble_gatt_rx_handler_t handler)
{
    if (handler == NULL) {
        return -EINVAL;
    }

    rx_handler = handler;
    return 0;
}

static ssize_t write_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, uint16_t len, uint16_t offset,
                        uint8_t flags)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(attr);

    if ((flags & BT_GATT_WRITE_FLAG_PREPARE) != 0U) {
        return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
    }
    if (offset != 0U) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    if (len > BLE_GATT_MAX_VALUE_LEN) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }
    if (len == 0U) {
        return 0;
    }
    if (rx_handler == NULL) {
        printk("BLE GATT RX write rejected: no message handler registered\n");
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }

    int err = rx_handler(buf, len);
#if defined(CONFIG_BLE_GATT_DISCOVER_NO_ECHO) && CONFIG_BLE_GATT_DISCOVER_NO_ECHO
    switch (ble_gatt_rx_classify(err, true)) {
    case BLE_GATT_RX_ACTION_ERR_LENGTH:
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    case BLE_GATT_RX_ACTION_ERR_FAILED:
        printk("BLE GATT RX message handling failed: %d\n", err);
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    case BLE_GATT_RX_ACTION_CONTROL_NO_TEXT:
        return len;
    case BLE_GATT_RX_ACTION_PUBLISH_TEXT:
    default:
        break;
    }
#else
    /* Keep the previously deployed RX data path when no-echo is disabled. */
    if (err == -EMSGSIZE) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }
    if (err != 0) {
        printk("BLE GATT RX message handling failed: %d\n", err);
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }
#endif
    (void)ble_gatt_publish_text_message(buf, len);
    return len;
}

int ble_gatt_init(void)
{
    k_mutex_init(&value_mutex);
    k_work_init_delayable(&adv_restart_work, restart_advertising_delayable);
    int err;

#if defined(CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME) && \
    CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME
    err = initialize_android_discovery_name();
    if (err != 0) {
        printk("BLE GATT Android discovery name initialization failed: %d\n",
               err);
        return err;
    }
#endif

    err = bt_enable(NULL);
    if (err != 0) {
        return err;
    }

#if defined(CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME) && \
    CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME
    err = bt_set_name(android_device_name);
    if (err != 0) {
        printk("BLE GATT Android device name update failed: %d\n", err);
        return err;
    }
#endif

    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, advertising,
                          ARRAY_SIZE(advertising), scan_response,
                          ARRAY_SIZE(scan_response));
    if (err != 0) {
        printk("BLE GATT advertising start failed: %d\n", err);
        return err;
    }

    printk("BLE GATT peripheral ready; advertising as %s\n",
#if defined(CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME) && \
    CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME
           android_device_name);
#else
           CONFIG_BT_DEVICE_NAME);
#endif
    return 0;
}
