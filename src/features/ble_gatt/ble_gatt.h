#ifndef HYBRID_MESH_BLE_GATT_H
#define HYBRID_MESH_BLE_GATT_H

#include <stddef.h>
#include <stdint.h>

#define BLE_GATT_MAX_VALUE_LEN 20U

/*
 * RX handler result: 0 = ordinary message accepted (GATT publishes it as TX
 * text), negative = -errno failure. BLE_GATT_RX_CONTROL_HANDLED means a
 * control command was consumed: the ATT write succeeds but no TX text is
 * notified or stored. It is honoured only when
 * CONFIG_BLE_GATT_DISCOVER_NO_ECHO=y; otherwise it is treated as a failure.
 */
#define BLE_GATT_RX_CONTROL_HANDLED 1

typedef int (*ble_gatt_rx_handler_t)(const uint8_t *value, size_t length);

/* Fills buffer with a binary evidence record; returns length or -errno. */
typedef int (*ble_gatt_evidence_provider_t)(uint8_t *buffer, size_t capacity);

int ble_gatt_init(void);
/* Read-only evidence characteristic 7b3e0005-...; reads Empty until set. */
int ble_gatt_set_evidence_provider(ble_gatt_evidence_provider_t provider);
int ble_gatt_set_rx_handler(ble_gatt_rx_handler_t handler);
int ble_gatt_publish_discovery_event(const uint8_t *event, size_t length);
int ble_gatt_publish_text_message(const uint8_t *message, size_t length);
int ble_gatt_update_mesh_advertisement(const uint8_t *frame, size_t length);

#endif
