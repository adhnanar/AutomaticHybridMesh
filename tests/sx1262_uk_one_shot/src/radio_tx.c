/*
 * The ONLY file in this repository (TASK024) that calls lora_send().
 * CMake adds it to the target exclusively when FRURT_UK_ONE_SHOT_TX_ALLOW=ON.
 * It is reached only via frurt_os_fire() -> uk868_guarded_send().
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/lora.h>
#include <errno.h>
#include "radio_tx.h"
#include "uk_lora_one_shot.h"

#if !FRURT_UK_ONE_SHOT_TX_ALLOW
#error "radio_tx.c must not be built in a locked configuration"
#endif

#define RADIO_NODE DT_NODELABEL(sx1262)
static const struct device *const radio = DEVICE_DT_GET(RADIO_NODE);

int frurt_os_radio_tx(void *user, const uint8_t *data, size_t len, bool *started,
		      uint64_t *actual_us)
{
	(void)user;
	/* Conservative: from here on the transmitter may have keyed, even on error. */
	*started = true;
	*actual_us = 0;

	if (!device_is_ready(radio) || len != FRURT_OS_PAYLOAD_LEN) {
		return -ENODEV;
	}

	/* Same modem settings as tests/sx1262_uk_pair RX; only .tx and .tx_power differ. */
	const struct lora_modem_config cfg = {
		.frequency = FRURT_OS_FREQ_HZ,
		.bandwidth = BW_125_KHZ,
		.datarate = SF_9,
		.coding_rate = CR_4_5,
		.preamble_len = FRURT_OS_PREAMBLE,
		.tx_power = FRURT_OS_TX_POWER_DBM,
		.tx = true,
		.public_network = false,
		.iq_inverted = false,
	};
	int rc = lora_config(radio, &cfg);

	if (rc != 0) {
		return rc;
	}
	/* lora_send is blocking and sends exactly once; no retry anywhere. */
	return lora_send(radio, (uint8_t *)data, (uint32_t)len);
}
