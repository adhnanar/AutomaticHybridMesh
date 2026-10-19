#include "uk_lora_zephyr_claim_lock.h"

static struct frurt_os_zephyr_claim_lock store_claim_lock;

int main(void)
{
	if (frurt_os_zephyr_claim_lock_init(&store_claim_lock) != 0) {
		return -1;
	}

	const struct frurt_os_lock_ops *shared_ops =
		frurt_os_zephyr_claim_lock_ops(&store_claim_lock);

	return shared_ops == NULL || shared_ops->try_acquire == NULL ||
			       shared_ops->release == NULL || shared_ops->ctx != &store_claim_lock
		       ? -1
		       : 0;
}
