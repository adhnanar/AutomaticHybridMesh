/* Host-only non-recursive try-lock double for the TASK028 claim-lock tests.
 * Single-threaded: it models exclusion state, it does not provide it. */
#ifndef SIM_LOCK_H_
#define SIM_LOCK_H_

#include "uk_lora_one_shot.h"

struct sim_lock {
	int held;
	unsigned int acquires;
	unsigned int releases;
	unsigned int denied;
	int fail_acquire; /* force try_acquire to report "not acquired" */
	int ineffective;  /* broken-lock negative control: always "acquires" */
};

static inline int sim_lock_try(void *c)
{
	struct sim_lock *l = c;

	if (l->fail_acquire || (l->held && !l->ineffective)) {
		l->denied++;
		return -1;
	}
	l->held++;
	l->acquires++;
	return 0;
}

static inline void sim_lock_release(void *c)
{
	struct sim_lock *l = c;

	l->held--;
	l->releases++;
}

static inline struct frurt_os_lock_ops sim_lock_ops(struct sim_lock *l)
{
	return (struct frurt_os_lock_ops){ sim_lock_try, sim_lock_release, l };
}

#endif
