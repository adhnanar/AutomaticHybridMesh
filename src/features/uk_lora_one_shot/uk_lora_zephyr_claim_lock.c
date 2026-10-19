#include "uk_lora_zephyr_claim_lock.h"

#include <errno.h>

enum {
	CLAIM_LOCK_UNINITIALIZED = 0,
	CLAIM_LOCK_INITIALIZING,
	CLAIM_LOCK_AVAILABLE,
	CLAIM_LOCK_HELD,
};

static int claim_lock_try_acquire(void *ctx)
{
	struct frurt_os_zephyr_claim_lock *lock = ctx;

	if (lock == NULL) {
		return -EINVAL;
	}
	return atomic_cas(&lock->state, CLAIM_LOCK_AVAILABLE, CLAIM_LOCK_HELD) ? 0 : -EBUSY;
}

static void claim_lock_release(void *ctx)
{
	struct frurt_os_zephyr_claim_lock *lock = ctx;

	if (lock != NULL) {
		/* A bad/double release must not make the lock available. */
		(void)atomic_cas(&lock->state, CLAIM_LOCK_HELD, CLAIM_LOCK_AVAILABLE);
	}
}

int frurt_os_zephyr_claim_lock_init(struct frurt_os_zephyr_claim_lock *lock)
{
	if (lock == NULL) {
		return -EINVAL;
	}
	if (!atomic_cas(&lock->state, CLAIM_LOCK_UNINITIALIZED, CLAIM_LOCK_INITIALIZING)) {
		return -EALREADY;
	}

	lock->ops.try_acquire = claim_lock_try_acquire;
	lock->ops.release = claim_lock_release;
	lock->ops.ctx = lock;
	atomic_set(&lock->state, CLAIM_LOCK_AVAILABLE);
	return 0;
}

const struct frurt_os_lock_ops *
frurt_os_zephyr_claim_lock_ops(struct frurt_os_zephyr_claim_lock *lock)
{
	if (lock == NULL || atomic_get(&lock->state) < CLAIM_LOCK_AVAILABLE) {
		return NULL;
	}
	return &lock->ops;
}
