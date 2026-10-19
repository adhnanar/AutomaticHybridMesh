/*
 * Task022 / Frurt UK-SX1262 RX-ONLY validation image.
 * There are NO LoRa transmit calls in this firmware.
 * Limited to 868.3MHz/125kHz modulation matching a separately assessed TX.
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <string.h>

#define RADIO_NODE DT_NODELABEL(sx1262)
BUILD_ASSERT(DT_NODE_EXISTS(RADIO_NODE), "Wio SX1262 overlay required");
BUILD_ASSERT(DT_NODE_HAS_STATUS(RADIO_NODE, okay), "SX1262 enabled required");
BUILD_ASSERT(DT_NODE_HAS_PROP(RADIO_NODE, dio2_tx_enable), "DIO2 frontend property needed");
BUILD_ASSERT(DT_NODE_HAS_PROP(RADIO_NODE, dio3_tcxo_voltage), "DIO3 TCXO property needed");
BUILD_ASSERT(DT_NODE_HAS_PROP(RADIO_NODE, rx_enable_gpios), "D5 RXEN property needed");

static const struct device *const radio = DEVICE_DT_GET(RADIO_NODE);
static const struct lora_modem_config rx_cfg = {
    .frequency = 868300000U,
    .bandwidth = BW_125_KHZ,
    .datarate = SF_9,
    .coding_rate = CR_4_5,
    .preamble_len = 8,
    .tx_power = -9,
    .tx = false,
    .public_network = false,
    .iq_inverted = false,
};

int main(void)
{
    printk("FRURT_TASK022_UK868_RX_ONLY_V1\n");
    printk("TASK022_RADIO_READY=%u\n", device_is_ready(radio) ? 1U : 0U);
    if (!device_is_ready(radio)) {
        printk("TASK022_RESULT=RADIO_INIT_FAILED\n");
        return -ENODEV;
    }
    int rc = lora_config(radio, &rx_cfg);
    printk("TASK022_RX_CONFIG_RC=%d\n", rc);
    if (rc != 0) {
        return rc;
    }
    printk("TASK022_RX_FREQ_HZ=868300000 BW_HZ=125000 SF=9 CR=4/5\n");
    for (;;) {
        uint8_t buf[32] = {0};
        int16_t rssi = 0;
        int8_t snr = 0;
        rc = lora_recv(radio, buf, sizeof(buf), K_SECONDS(10), &rssi, &snr);
        if (rc > 0) {
            printk("TASK022_RX_N=%d RSSI=%d SNR=%d DATA=", rc, rssi, snr);
            for (int i = 0; i < rc; ++i) {
                printk("%02X", buf[i]);
            }
            printk("\n");
        } else if (rc == -EAGAIN || rc == -ETIMEDOUT) {
            printk("TASK022_RX_WAIT_NO_PACKET\n");
        } else {
            printk("TASK022_RX_ERROR=%d\n", rc);
            k_sleep(K_SECONDS(2));
        }
    }
}
