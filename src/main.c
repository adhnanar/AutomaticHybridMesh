#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "features/ble_gatt/ble_gatt.h"
#include "features/ble_mesh_relay/ble_mesh_relay.h"
#include "features/system_status/system_status.h"

int main(void)
{
    system_status_announce();

    int err = ble_gatt_init();
    if (err != 0) {
        printk("BLE GATT initialization failed: %d\n", err);
        return err;
    }

    err = ble_mesh_relay_init();
    if (err != 0) {
        printk("Custom BLE relay initialization failed: %d\n", err);
        return err;
    }

    err = ble_gatt_set_rx_handler(ble_mesh_relay_handle_phone_write);
    if (err != 0) {
        printk("BLE GATT RX handler registration failed: %d\n", err);
        return err;
    }

    return 0;
}
