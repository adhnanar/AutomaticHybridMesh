/* Radio transmit callback. Compiled ONLY when the one-shot TX allow build is selected. */
#ifndef FRURT_ONE_SHOT_RADIO_TX_H_
#define FRURT_ONE_SHOT_RADIO_TX_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int frurt_os_radio_tx(void *user, const uint8_t *data, size_t len, bool *started,
		      uint64_t *actual_us);

#endif
