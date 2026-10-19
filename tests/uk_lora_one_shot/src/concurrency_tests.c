/* TASK028 claim-lock regression tests: contention, re-entry, storage error, restart.
 * These are DETERMINISTIC interleavings injected at store/radio callbacks on a single thread.
 * They do not prove behaviour under real preemption or on other cores. */
#include <string.h>
#include <zephyr/ztest.h>
#include "sim_flash.h"
#include "sim_lock.h"
#include "uk_lora_one_shot.h"

#define CC_TOKEN 0x5A3C1E7Fu

#if FRURT_UK_ONE_SHOT_TX_ALLOW
struct cc_radio {
	unsigned int calls;
	const struct sim_lock *lock;
	int lock_held_in_tx;
	struct frurt_os_ctx *reenter; /* fire() again from inside the radio callback */
	int reenter_rc;
};

static struct cc_radio cc_r;

static int cc_tx(void *user, const uint8_t *d, size_t n, bool *started, uint64_t *actual)
{
	struct cc_radio *r = user;

	(void)d; (void)n;
	r->calls++;
	r->lock_held_in_tx = r->lock != NULL ? r->lock->held : -1;
	if (r->reenter != NULL) {
		struct frurt_os_ctx *again = r->reenter;

		r->reenter = NULL;
		r->reenter_rc = frurt_os_fire(again, 1, cc_tx, r);
	}
	*started = true;
	*actual = 100000;
	return 0;
}

/* Store wrapper that runs a second fire() at the Nth store access of the first one. */
struct cc_hook {
	struct sim_flash *f;
	unsigned int ops;
	unsigned int trigger; /* 1-based store access index; 0 = never */
	bool writes_only;
	bool in_hook;
	struct frurt_os_ctx *other;
	int other_rc;
	bool fired;
};

static void cc_maybe_hook(struct cc_hook *h, bool is_write)
{
	if (h->writes_only && !is_write) {
		return;
	}
	h->ops++;
	if (h->trigger != 0 && h->ops == h->trigger && !h->in_hook) {
		h->in_hook = true;
		h->fired = true;
		h->other_rc = frurt_os_fire(h->other, 2, cc_tx, &cc_r);
		h->in_hook = false;
	}
}

static int cc_read(void *c, uint32_t off, void *buf, size_t len)
{
	cc_maybe_hook(c, false);
	return sim_read(((struct cc_hook *)c)->f, off, buf, len);
}

static int cc_write(void *c, uint32_t off, const void *buf, size_t len)
{
	cc_maybe_hook(c, true);
	return sim_write(((struct cc_hook *)c)->f, off, buf, len);
}

static struct frurt_os_authz cc_authz(void)
{
	return (struct frurt_os_authz){ true, true, true, true, 2000, 0,
					FRURT_OS_DEFAULT_POWER_TOL_MDB };
}

static void cc_ctx(struct frurt_os_ctx *c, const struct frurt_os_store_ops *s,
		   const struct frurt_os_lock_ops *l)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a = cc_authz();

	frurt_os_rf_cfg_default(&cfg);
	zassert_equal(frurt_os_init(c, &cfg, &a), FRURT_OS_SENT_OK);
	frurt_os_attach_store(c, s);
	frurt_os_attach_lock(c, l);
}

ZTEST(os_concurrency, test_missing_or_incomplete_lock_denies_without_store_access)
{
	struct sim_flash f;
	struct frurt_os_ctx c;
	struct frurt_os_store_ops s;
	struct frurt_os_lock_ops none = { NULL, NULL, NULL };
	struct sim_lock lk = { 0 };
	struct frurt_os_lock_ops no_release = { sim_lock_try, NULL, &lk };

	memset(&cc_r, 0, sizeof(cc_r));
	sim_provision(&f, CC_TOKEN);
	s = sim_ops(&f);
	cc_ctx(&c, &s, NULL);
	zassert_equal(frurt_os_fire(&c, 1, cc_tx, &cc_r), FRURT_OS_ERR_NO_LOCK);
	frurt_os_attach_lock(&c, &none);
	zassert_equal(frurt_os_fire(&c, 1, cc_tx, &cc_r), FRURT_OS_ERR_NO_LOCK);
	frurt_os_attach_lock(&c, &no_release);
	zassert_equal(frurt_os_fire(&c, 1, cc_tx, &cc_r), FRURT_OS_ERR_NO_LOCK);
	zassert_equal(lk.acquires, 0);
	zassert_equal(cc_r.calls, 0);
	zassert_equal(f.reads, 0);
	zassert_equal(f.writes, 0);
}

ZTEST(os_concurrency, test_contended_lock_is_busy_and_does_not_burn_the_shot)
{
	struct sim_flash f;
	struct frurt_os_ctx c;
	struct frurt_os_store_ops s;
	struct sim_lock lk = { .fail_acquire = 1 };
	struct frurt_os_lock_ops lo = sim_lock_ops(&lk);

	memset(&cc_r, 0, sizeof(cc_r));
	sim_provision(&f, CC_TOKEN);
	s = sim_ops(&f);
	cc_ctx(&c, &s, &lo);
	zassert_equal(frurt_os_fire(&c, 1, cc_tx, &cc_r), FRURT_OS_ERR_BUSY);
	zassert_equal(cc_r.calls, 0);
	zassert_equal(f.reads, 0);
	zassert_equal(f.writes, 0);
	zassert_false(c.consumed);
	lk.fail_acquire = 0;
	zassert_equal(frurt_os_fire(&c, 1, cc_tx, &cc_r), FRURT_OS_SENT_OK);
	zassert_equal(frurt_os_fire(&c, 2, cc_tx, &cc_r), FRURT_OS_ERR_ALREADY_USED);
	zassert_equal(cc_r.calls, 1);
}

ZTEST(os_concurrency, test_interleave_at_every_store_access_yields_exactly_one_tx)
{
	for (unsigned int trig = 1; trig <= 8; trig++) {
		struct sim_flash f;
		struct cc_hook h = { .f = &f, .trigger = trig };
		struct frurt_os_store_ops s = { cc_read, cc_write, &h };
		struct frurt_os_ctx a, b;
		struct sim_lock lk = { 0 };
		struct frurt_os_lock_ops lo = sim_lock_ops(&lk);

		memset(&cc_r, 0, sizeof(cc_r));
		cc_r.lock = &lk;
		sim_provision(&f, CC_TOKEN);
		cc_ctx(&a, &s, &lo);
		cc_ctx(&b, &s, &lo); /* second caller, same store and same lock */
		h.other = &b;
		zassert_equal(frurt_os_fire(&a, 1, cc_tx, &cc_r), FRURT_OS_SENT_OK, "trig %u", trig);
		if (h.fired) {
			zassert_equal(h.other_rc, FRURT_OS_ERR_BUSY, "trig %u", trig);
		}
		zassert_equal(cc_r.calls, 1, "trig %u", trig);
		zassert_equal(lk.held, 0);
		zassert_equal(lk.acquires, lk.releases);
		zassert_equal(cc_r.lock_held_in_tx, 0, "lock must not be held during radio");
		zassert_equal(f.erases, 0);
		zassert_equal(frurt_os_fire(&b, 3, cc_tx, &cc_r), FRURT_OS_ERR_NOT_ARMED);
		zassert_equal(cc_r.calls, 1);
	}
}

ZTEST(os_concurrency, test_negative_control_ineffective_lock_double_transmits)
{
	/* Proves the interleaving above is sensitive: without real exclusion the store
	 * alone is NOT atomic and a second caller landing between its ARMED check and
	 * its write transmits twice. This documents the residual risk; it is not a pass
	 * criterion for production. */
	struct sim_flash f;
	struct cc_hook h = { .f = &f, .trigger = 1, .writes_only = true };
	struct frurt_os_store_ops s = { cc_read, cc_write, &h };
	struct frurt_os_ctx a, b;
	struct sim_lock lk = { .ineffective = 1 };
	struct frurt_os_lock_ops lo = sim_lock_ops(&lk);

	memset(&cc_r, 0, sizeof(cc_r));
	sim_provision(&f, CC_TOKEN);
	cc_ctx(&a, &s, &lo);
	cc_ctx(&b, &s, &lo);
	h.other = &b;
	(void)frurt_os_fire(&a, 1, cc_tx, &cc_r);
	zassert_true(h.fired);
	zassert_equal(cc_r.calls, 2, "ineffective lock must reproduce the race");
}

ZTEST(os_concurrency, test_reentry_from_radio_callback_is_denied)
{
	struct sim_flash f;
	struct frurt_os_ctx a, b;
	struct frurt_os_store_ops s;
	struct sim_lock lk = { 0 };
	struct frurt_os_lock_ops lo = sim_lock_ops(&lk);

	/* same context */
	memset(&cc_r, 0, sizeof(cc_r));
	cc_r.lock = &lk;
	sim_provision(&f, CC_TOKEN);
	s = sim_ops(&f);
	cc_ctx(&a, &s, &lo);
	cc_r.reenter = &a;
	zassert_equal(frurt_os_fire(&a, 1, cc_tx, &cc_r), FRURT_OS_SENT_OK);
	zassert_equal(cc_r.reenter_rc, FRURT_OS_ERR_ALREADY_USED);
	zassert_equal(cc_r.calls, 1);
	zassert_equal(cc_r.lock_held_in_tx, 0);

	/* different context, same store */
	memset(&cc_r, 0, sizeof(cc_r));
	sim_provision(&f, CC_TOKEN);
	memset(&lk, 0, sizeof(lk));
	cc_ctx(&a, &s, &lo);
	cc_ctx(&b, &s, &lo);
	cc_r.reenter = &b;
	zassert_equal(frurt_os_fire(&a, 1, cc_tx, &cc_r), FRURT_OS_SENT_OK);
	zassert_equal(cc_r.reenter_rc, FRURT_OS_ERR_NOT_ARMED);
	zassert_equal(cc_r.calls, 1);
	zassert_equal(lk.acquires, lk.releases);
}

ZTEST(os_concurrency, test_reentry_from_store_callback_is_busy_with_nonrecursive_lock)
{
	struct sim_flash f;
	struct cc_hook h = { .f = &f, .trigger = 1 };
	struct frurt_os_store_ops s = { cc_read, cc_write, &h };
	struct frurt_os_ctx a;
	struct sim_lock lk = { 0 };
	struct frurt_os_lock_ops lo = sim_lock_ops(&lk);

	memset(&cc_r, 0, sizeof(cc_r));
	sim_provision(&f, CC_TOKEN);
	cc_ctx(&a, &s, &lo);
	h.other = &a;
	zassert_equal(frurt_os_fire(&a, 1, cc_tx, &cc_r), FRURT_OS_SENT_OK);
	zassert_true(h.fired);
	zassert_equal(h.other_rc, FRURT_OS_ERR_BUSY);
	zassert_equal(cc_r.calls, 1);
	zassert_equal(lk.held, 0);
}

ZTEST(os_concurrency, test_storage_error_releases_lock_denies_tx_and_restart_semantics)
{
	struct sim_flash f;
	struct frurt_os_ctx a, b, c;
	struct frurt_os_store_ops s;
	struct sim_lock lk = { 0 };
	struct frurt_os_lock_ops lo = sim_lock_ops(&lk);

	memset(&cc_r, 0, sizeof(cc_r));
	sim_provision(&f, CC_TOKEN);
	s = sim_ops(&f);
	f.fault = SIM_FAIL_NOCHANGE;
	f.fault_at_write = 1;
	cc_ctx(&a, &s, &lo);
	zassert_equal(frurt_os_fire(&a, 1, cc_tx, &cc_r), FRURT_OS_ERR_STORE);
	zassert_equal(cc_r.calls, 0);
	zassert_equal(lk.held, 0);
	zassert_equal(lk.acquires, lk.releases);
	/* RAM latch stays set after a store error: same boot cannot retry */
	zassert_equal(frurt_os_fire(&a, 2, cc_tx, &cc_r), FRURT_OS_ERR_ALREADY_USED);
	zassert_equal(cc_r.calls, 0);
	/* restart: fresh RAM context + fresh lock, flash still ARMED, one TX only */
	memset(&lk, 0, sizeof(lk));
	cc_ctx(&b, &s, &lo);
	zassert_equal(frurt_os_fire(&b, 1, cc_tx, &cc_r), FRURT_OS_SENT_OK);
	memset(&lk, 0, sizeof(lk));
	cc_ctx(&c, &s, &lo);
	zassert_equal(frurt_os_fire(&c, 1, cc_tx, &cc_r), FRURT_OS_ERR_NOT_ARMED);
	zassert_equal(cc_r.calls, 1);
}

ZTEST(os_concurrency, test_torn_silent_write_with_lock_still_at_most_one_tx)
{
	struct sim_flash f;
	struct frurt_os_ctx c;
	struct frurt_os_store_ops s;
	struct sim_lock lk = { 0 };
	struct frurt_os_lock_ops lo = sim_lock_ops(&lk);

	memset(&cc_r, 0, sizeof(cc_r));
	sim_provision(&f, CC_TOKEN);
	s = sim_ops(&f);
	f.fault = SIM_TORN_SILENT;
	f.fault_at_write = 1;
	for (int boot = 0; boot < 5; boot++) {
		memset(&lk, 0, sizeof(lk));
		cc_ctx(&c, &s, &lo);
		(void)frurt_os_fire(&c, 1, cc_tx, &cc_r);
		zassert_equal(lk.held, 0);
	}
	zassert_true(cc_r.calls <= 1);
}
#else
static int lk_tx(void *user, const uint8_t *d, size_t n, bool *started, uint64_t *actual)
{
	(void)user; (void)d; (void)n; (void)started; (void)actual;
	return -1;
}

ZTEST(os_concurrency, test_locked_build_never_acquires_lock_or_touches_store)
{
	struct sim_flash f;
	struct frurt_os_ctx c;
	struct frurt_os_store_ops s;
	struct sim_lock lk = { 0 };
	struct frurt_os_lock_ops lo = sim_lock_ops(&lk);
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a = { true, true, true, true, 2000, 0, FRURT_OS_DEFAULT_POWER_TOL_MDB };

	sim_provision(&f, CC_TOKEN);
	s = sim_ops(&f);
	frurt_os_rf_cfg_default(&cfg);
	(void)frurt_os_init(&c, &cfg, &a);
	frurt_os_attach_store(&c, &s);
	frurt_os_attach_lock(&c, &lo);
	zassert_equal(frurt_os_fire(&c, 1, lk_tx, NULL), FRURT_OS_ERR_LOCKED);
	zassert_equal(lk.acquires, 0);
	zassert_equal(f.reads, 0);
	zassert_equal(f.writes, 0);
}
#endif
ZTEST_SUITE(os_concurrency, NULL, NULL, NULL, NULL, NULL);
