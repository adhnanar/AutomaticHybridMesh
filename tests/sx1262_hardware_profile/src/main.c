#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SX1262_NODE DT_NODELABEL(sx1262)

static const struct device *const sx1262 = DEVICE_DT_GET(SX1262_NODE);

int main(void)
{
	if (!device_is_ready(sx1262)) {
		printk("SX1262 device initialization failed\n");
	} else {
		printk("SX1262 device initialized; no LoRa API configuration or send called\n");
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}
