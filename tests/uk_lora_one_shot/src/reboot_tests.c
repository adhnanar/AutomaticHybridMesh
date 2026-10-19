/* TASK025 reboot / power-cut regression tests for frurt_os_fire() + durable store. */
#include <string.h>
#include <zephyr/ztest.h>
#include "sim_flash.h"
#include "sim_lock.h"
#include "uk_lora_one_shot.h"

#define TOKEN 0xA5C3E1F7u

struct rb_radio {
	unsigned int calls;
	bool started;
	uint64_t actual_us;
	int rc;
	struct sim_flash *flash;
	uint8_t consume_byte_at_call;
};

static int rb_tx(void *user, const uint8_t *d, size_t n, bool *started, uint64_t *actual)
{
	struct rb_radio *r = user;

	(void)d; (void)n;
	r->calls++;
	r->consume_byte_at_call = r->flash->mem[FRURT_OS_STORE_OFF_CONSUME];
	*started = r->started;
	*actual = r->actual_us;
	return r->rc;
}

static struct frurt_os_store_ops ops;
static struct sim_lock rb_lock;
static struct frurt_os_lock_ops rb_lock_ops;

static struct frurt_os_authz rb_authz(void)
{
	return (struct frurt_os_authz){ true, true, true, true, 2000, 0,
					FRURT_OS_DEFAULT_POWER_TOL_MDB };
}

/* One "boot": fresh RAM context (consumed flag + budget are RAM-only), same flash. */
static int boot_and_fire(struct sim_flash *f, struct rb_radio *r, bool attach)
{
	static struct frurt_os_ctx c;
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a = rb_authz();

	frurt_os_rf_cfg_default(&cfg);
	ops = sim_ops(f);
	(void)frurt_os_init(&c, &cfg, &a);
	if (attach) {
		frurt_os_attach_store(&c, &ops);
	}
	memset(&rb_lock, 0, sizeof(rb_lock));
	rb_lock_ops = sim_lock_ops(&rb_lock);
	frurt_os_attach_lock(&c, &rb_lock_ops);
	return frurt_os_fire(&c, 1000, rb_tx, r);
}

#if !FRURT_UK_ONE_SHOT_TX_ALLOW
ZTEST(os_reboot, test_locked_build_never_touches_store_or_radio)
{
	struct sim_flash f;
	struct rb_radio r = { .started = true, .flash = &f };

	sim_provision(&f, TOKEN);
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_ERR_LOCKED);
	zassert_equal(r.calls, 0);
	zassert_equal(f.writes, 0);
}
#else
ZTEST(os_reboot, test_consume_is_durable_before_radio_and_blocks_after_reboot)
{
	struct sim_flash f;
	struct rb_radio r = { .started = true, .flash = &f };

	sim_provision(&f, TOKEN);
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_SENT_OK);
	zassert_equal(r.calls, 1);
	zassert_not_equal(r.consume_byte_at_call, 0xFF, "consume must be committed before tx");
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_ERR_NOT_ARMED);
	zassert_equal(r.calls, 1, "no duplicate TX after restart");
}

ZTEST(os_reboot, test_unknown_airtime_or_radio_error_still_consumed_after_reboot)
{
	struct sim_flash f;
	struct rb_radio r = { .started = true, .actual_us = 0, .rc = -5, .flash = &f };

	sim_provision(&f, TOKEN);
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_ERR_RADIO);
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_ERR_NOT_ARMED);
	zassert_equal(r.calls, 1);
}

ZTEST(os_reboot, test_crash_after_commit_before_radio_does_not_rearm)
{
	struct sim_flash f;
	struct rb_radio r = { .started = true, .flash = &f };

	sim_provision(&f, TOKEN);
	ops = sim_ops(&f);
	zassert_equal(frurt_os_store_consume(&ops), FRURT_OS_STORE_OK); /* then power lost */
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_ERR_NOT_ARMED);
	zassert_equal(r.calls, 0);
}

ZTEST(os_reboot, test_missing_blank_or_unattached_store_denies)
{
	struct sim_flash f;
	struct rb_radio r = { .started = true, .flash = &f };

	sim_provision(&f, TOKEN);
	zassert_equal(boot_and_fire(&f, &r, false), FRURT_OS_ERR_NOT_ARMED);
	memset(&f, 0, sizeof(f));
	sim_erase(&f);
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_ERR_NOT_ARMED);
	zassert_equal(r.calls, 0);
	zassert_equal(f.writes, 0);
}

ZTEST(os_reboot, test_write_failure_means_no_tx_and_later_boot_may_try_once)
{
	struct sim_flash f;
	struct rb_radio r = { .started = true, .flash = &f };

	sim_provision(&f, TOKEN);
	f.fault = SIM_FAIL_NOCHANGE;
	f.fault_at_write = 1;
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_ERR_STORE);
	zassert_equal(r.calls, 0, "tx must not happen when commit fails");
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_SENT_OK);
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_ERR_NOT_ARMED);
	zassert_equal(r.calls, 1);
}

ZTEST(os_reboot, test_power_cut_modes_never_allow_two_tx)
{
	const enum sim_fault modes[] = { SIM_FAIL_NOCHANGE, SIM_TORN_ERROR, SIM_TORN_SILENT,
					 SIM_STUCK };

	for (size_t m = 0; m < sizeof(modes) / sizeof(modes[0]); m++) {
		struct sim_flash f;
		struct rb_radio r = { .started = true, .flash = &f };

		sim_provision(&f, TOKEN);
		f.fault = modes[m];
		f.fault_at_write = 1;
		for (int boot = 0; boot < 10; boot++) {
			(void)boot_and_fire(&f, &r, true);
		}
		zassert_true(r.calls <= 1, "mode %d calls %u", (int)m, r.calls);
		zassert_equal(f.erases, 0, "firmware must never erase");
	}
}

ZTEST(os_reboot, test_repeated_reset_loop_transmits_once)
{
	struct sim_flash f;
	struct rb_radio r = { .started = true, .flash = &f };

	sim_provision(&f, TOKEN);
	for (int boot = 0; boot < 50; boot++) {
		(void)boot_and_fire(&f, &r, true);
	}
	zassert_equal(r.calls, 1);
	zassert_equal(f.writes, 1);
}

ZTEST(os_reboot, test_unreadable_store_denies)
{
	struct sim_flash f;
	struct rb_radio r = { .started = true, .flash = &f };

	sim_provision(&f, TOKEN);
	f.read_fail = 1;
	zassert_equal(boot_and_fire(&f, &r, true), FRURT_OS_ERR_NOT_ARMED);
	zassert_equal(r.calls, 0);
}
#endif
ZTEST_SUITE(os_reboot, NULL, NULL, NULL, NULL, NULL);
