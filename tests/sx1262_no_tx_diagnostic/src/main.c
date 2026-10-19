/*
 * Frurt Task021: SX1262 INITIALIZATION-ONLY diagnostics.
 * NO LoRa API configuration, listen, or transmit is invoked by this app.
 * The Zephyr SX126x driver itself initializes the peripheral during boot.
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define RADIO_NODE DT_NODELABEL(sx1262)
BUILD_ASSERT(DT_NODE_EXISTS(RADIO_NODE), "SX1262 overlay required");
BUILD_ASSERT(DT_NODE_HAS_STATUS(RADIO_NODE, okay), "SX1262 node must be enabled");
BUILD_ASSERT(DT_NODE_HAS_PROP(RADIO_NODE, dio2_tx_enable), "RF frontend overlay required");
BUILD_ASSERT(DT_NODE_HAS_PROP(RADIO_NODE, dio3_tcxo_voltage), "TCXO voltage is required");
BUILD_ASSERT(DT_NODE_HAS_PROP(RADIO_NODE, rx_enable_gpios), "RXEN line is required");
static const struct device *const radio = DEVICE_DT_GET(RADIO_NODE);

int main(void)
{
    printk("FRURT_TASK021_NO_TX_DIAGNOSTIC v2\n");
    printk("TASK021_BUILD_RADIO_NODE=%s\n", radio->name);
    printk("TASK021_EXPECTED_TX_CALLS=0\n");
    if (!device_is_ready(radio)) {
        printk("TASK021_SX1262_DEVICE_READY=0\n");
        printk("TASK021_RESULT=DRIVER_INIT_FAILED\n");
    } else {
        printk("TASK021_SX1262_DEVICE_READY=1\n");
        printk("TASK021_RESULT=DRIVER_INIT_OK_NOT_RF_VALIDATED\n");
    }
    /* Periodically re-report the cached boot result: Windows may reconnect
     * USB CDC after the one-time startup lines have already been printed. */
    for (;;) {
        k_sleep(K_SECONDS(10));
        printk("FRURT_TASK021_NO_TX_DIAGNOSTIC v2\n");
        printk("TASK021_SX1262_DEVICE_READY=%d\n", device_is_ready(radio) ? 1 : 0);
        printk("TASK021_ALIVE_NO_TX=1\n");
    }
}
