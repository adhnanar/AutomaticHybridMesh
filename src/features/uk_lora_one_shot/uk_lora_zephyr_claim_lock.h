/*
 * Zephyr claim-lock adapter for the portable one-shot policy.
 *
 * This module supplies only serialization. It does not attach itself to a
 * one-shot context, access flash, or provide a radio/TX path.
 */
#ifndef UK_LORA_ZEPHYR_CLAIM_LOCK_H_
#define UK_LORA_ZEPHYR_CLAIM_LOCK_H_

#include <zephyr/sys/atomic.h>

#include "uk_lora_one_shot.h"

#ifdef __cplusplus
extern "C" {
#endif

struct frurt_os_zephyr_claim_lock {
	atomic_t state;
	struct frurt_os_lock_ops ops;
};

/*
 * Initialize a zero-initialized lock exactly once. A second or racing init
 * fails; it never resets a held or previously initialized lock.
 */
int frurt_os_zephyr_claim_lock_init(struct frurt_os_zephyr_claim_lock *lock);

/* Return the stable ops table after successful initialization, otherwise NULL. */
const struct frurt_os_lock_ops *
frurt_os_zephyr_claim_lock_ops(struct frurt_os_zephyr_claim_lock *lock);

#ifdef __cplusplus
}
#endif

#endif /* UK_LORA_ZEPHYR_CLAIM_LOCK_H_ */
