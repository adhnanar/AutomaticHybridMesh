/* TASK024 host-only (no radio) tests for the one-shot TX policy. */
#include <string.h>
#include <zephyr/ztest.h>
#include "uk_lora_one_shot.h"

/* Independent expected value: tests/task024_airtime_check.py, 10 B, SF9/125k/CR4-5/pre8. */
#define EXPECT_AIRTIME_US 151604ull

struct fake_radio {
	unsigned int calls;
	bool started;
	uint64_t actual_us;
	int rc;
	const struct uk868_budget *budget;
	uint64_t used_at_call;
	uint8_t data[16];
	size_t len;
};

static int fake_tx(void *user, const uint8_t *d, size_t n, bool *started, uint64_t *actual)
{
	struct fake_radio *r = user;

	r->calls++;
	r->used_at_call = r->budget ? uk868_budget_used_us(r->budget) : 0;
	r->len = n;
	memcpy(r->data, d, n < sizeof(r->data) ? n : sizeof(r->data));
	*started = r->started;
	*actual = r->actual_us;
	return r->rc;
}

static struct frurt_os_authz good_authz(void)
{
	return (struct frurt_os_authz){
		.operator_confirmed_uk = true,
		.erp_assessment_documented = true,
		.equipment_compliance_reviewed = true,
		.antenna_gain_verified = true,
		.antenna_gain_mdbi = 2000,
		.feed_loss_mdb = 0,
		.power_tolerance_mdb = FRURT_OS_DEFAULT_POWER_TOL_MDB,
	};
}

ZTEST(os_common, test_rx_config_and_airtime)
{
	struct frurt_os_rf_cfg c;

	frurt_os_rf_cfg_default(&c);
	zassert_true(frurt_os_rf_cfg_matches_rx(&c));
	zassert_equal(c.frequency_hz, 868300000u);
	zassert_equal(c.bandwidth_hz, 125000u);
	zassert_equal(c.sf, 9);
	zassert_equal(c.cr, 1);
	zassert_equal(c.preamble_symbols, 8);
	zassert_false(c.public_network);
	zassert_equal(frurt_os_airtime_us(&c), EXPECT_AIRTIME_US);
	zassert_true(frurt_os_airtime_us(&c) <= FRURT_OS_MAX_AIRTIME_US);
	c.sf = 10;
	zassert_false(frurt_os_rf_cfg_matches_rx(&c));
	frurt_os_rf_cfg_default(&c);
	c.public_network = true;
	zassert_false(frurt_os_rf_cfg_matches_rx(&c));
}

ZTEST(os_common, test_worst_case_erp_arithmetic)
{
	struct frurt_os_authz a = good_authz();

	/* -9 dBm + 3 dB tol + 2 dBi - 2.15 dB = -6.15 dBm */
	zassert_equal(frurt_os_worst_case_erp_mdbm(-9, &a), -6150);
	a.antenna_gain_mdbi = 20000;
	zassert_equal(frurt_os_worst_case_erp_mdbm(-9, &a), 11850);
}

ZTEST(os_common, test_default_authz_denied_and_no_tx)
{
	struct frurt_os_ctx c;
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz none = {0};
	struct fake_radio r = { .started = true };

	frurt_os_rf_cfg_default(&cfg);
	zassert_not_equal(frurt_os_init(&c, &cfg, &none), FRURT_OS_SENT_OK);
	zassert_false(uk868_gate_is_open(&c.gate));
	zassert_not_equal(frurt_os_fire(&c, 1000, fake_tx, &r), FRURT_OS_SENT_OK);
	zassert_equal(r.calls, 0);
	zassert_equal(c.tx_callbacks, 0);
}

ZTEST(os_common, test_direct_guarded_send_cannot_bypass_gate)
{
	struct frurt_os_ctx c;
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz none = {0};
	struct fake_radio r = { .started = true };
	const uint8_t d[10] = {0};

	ARG_UNUSED(d);
	frurt_os_rf_cfg_default(&cfg);
	(void)frurt_os_init(&c, &cfg, &none);
	c.gate.open = true; /* tamper */
#if !FRURT_UK_ONE_SHOT_TX_ALLOW
	zassert_equal(uk868_guarded_send(&c.budget, &c.gate, &c.profile, 1000, UK868_TX_NEW,
					 d, 10, fake_tx, &r), UK868_ERR_GATE_DENIED);
	zassert_equal(r.calls, 0);
#else
	/* Allowed build + tampered gate: still not accepted by one-shot layer (init_ok false). */
	zassert_equal(frurt_os_fire(&c, 1000, fake_tx, &r), FRURT_OS_ERR_NOT_AUTHORIZED);
	zassert_equal(r.calls, 0);
#endif
}

#if !FRURT_UK_ONE_SHOT_TX_ALLOW
ZTEST(os_locked, test_locked_build_denies_even_with_full_authz)
{
	struct frurt_os_ctx c;
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a = good_authz();
	struct fake_radio r = { .started = true, .rc = 0 };
	struct uk868_gate g = {0};
	struct uk868_authorization ua = { true, true, true, -9000 };

	frurt_os_rf_cfg_default(&cfg);
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_LOCKED);
	zassert_equal(frurt_os_fire(&c, 1000, fake_tx, &r), FRURT_OS_ERR_LOCKED);
	zassert_equal(r.calls, 0);
	zassert_equal(c.tx_callbacks, 0);
	zassert_equal(uk868_gate_open(&g, &ua), UK868_ERR_NOT_AUTHORIZED);
	zassert_false(uk868_gate_is_open(&g));
	g.open = true;
	zassert_false(uk868_gate_is_open(&g));
	zassert_equal(uk868_budget_used_us(&c.budget), 0);
}
ZTEST_SUITE(os_locked, NULL, NULL, NULL, NULL, NULL);
#else
static struct frurt_os_ctx c;

/* TASK025: every allowed-build fire needs an ARMED durable store; give each test a fresh one. */
#include "sim_flash.h"
#include "sim_lock.h"
static struct sim_flash t_flash;
static struct frurt_os_store_ops t_ops;
static struct sim_lock t_lock;
static struct frurt_os_lock_ops t_lock_ops;
static int t_init(struct frurt_os_ctx *ctx, const struct frurt_os_rf_cfg *cfg,
		  const struct frurt_os_authz *a)
{
	int rc = (frurt_os_init)(ctx, cfg, a);

	sim_provision(&t_flash, 0xA5C3E1F7u);
	t_ops = sim_ops(&t_flash);
	frurt_os_attach_store(ctx, &t_ops);
	memset(&t_lock, 0, sizeof(t_lock));
	t_lock_ops = sim_lock_ops(&t_lock);
	frurt_os_attach_lock(ctx, &t_lock_ops);
	return rc;
}
#define frurt_os_init(c_, cfg_, a_) t_init(c_, cfg_, a_)
static void setup(struct frurt_os_rf_cfg *cfg, struct frurt_os_authz *a)
{
	frurt_os_rf_cfg_default(cfg);
	*a = good_authz();
}

ZTEST(os_allow, test_success_reserves_before_send_and_counts_full)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a;
	struct fake_radio r = { .started = true, .actual_us = 0, .rc = 0, .budget = &c.budget };

	setup(&cfg, &a);
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_SENT_OK);
	zassert_equal(frurt_os_fire(&c, 5000, fake_tx, &r), FRURT_OS_SENT_OK);
	zassert_equal(r.calls, 1);
	zassert_equal(r.len, FRURT_OS_PAYLOAD_LEN);
	zassert_mem_equal(r.data, FRURT_OS_PAYLOAD, FRURT_OS_PAYLOAD_LEN);
	zassert_equal(r.used_at_call, EXPECT_AIRTIME_US, "reserve must precede tx");
	/* unknown actual airtime => full reservation stays charged */
	zassert_equal(uk868_budget_used_us(&c.budget), EXPECT_AIRTIME_US);
}

ZTEST(os_allow, test_second_fire_denied_one_shot)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a;
	struct fake_radio r = { .started = true };

	setup(&cfg, &a);
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_SENT_OK);
	zassert_equal(frurt_os_fire(&c, 5000, fake_tx, &r), FRURT_OS_SENT_OK);
	zassert_equal(frurt_os_fire(&c, 9000, fake_tx, &r), FRURT_OS_ERR_ALREADY_USED);
	zassert_equal(r.calls, 1);
}

ZTEST(os_allow, test_radio_error_after_start_stays_charged_and_consumed)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a;
	struct fake_radio r = { .started = true, .rc = -5 };

	setup(&cfg, &a);
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_SENT_OK);
	zassert_equal(frurt_os_fire(&c, 5000, fake_tx, &r), FRURT_OS_ERR_RADIO);
	zassert_equal(uk868_budget_used_us(&c.budget), EXPECT_AIRTIME_US);
	zassert_equal(frurt_os_fire(&c, 6000, fake_tx, &r), FRURT_OS_ERR_ALREADY_USED);
	zassert_equal(r.calls, 1);
}

ZTEST(os_allow, test_failure_before_start_is_refunded_but_still_consumed)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a;
	struct fake_radio r = { .started = false, .rc = -5 };

	setup(&cfg, &a);
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_SENT_OK);
	zassert_equal(frurt_os_fire(&c, 5000, fake_tx, &r), FRURT_OS_ERR_RADIO);
	zassert_equal(uk868_budget_used_us(&c.budget), 0);
	zassert_equal(frurt_os_fire(&c, 6000, fake_tx, &r), FRURT_OS_ERR_ALREADY_USED);
}

ZTEST(os_allow, test_unverified_gain_or_missing_evidence_denied)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a;
	struct fake_radio r = { .started = true };

	setup(&cfg, &a);
	a.antenna_gain_verified = false;
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_NOT_AUTHORIZED);
	zassert_equal(frurt_os_fire(&c, 1000, fake_tx, &r), FRURT_OS_ERR_NOT_AUTHORIZED);
	setup(&cfg, &a);
	a.erp_assessment_documented = false;
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_NOT_AUTHORIZED);
	setup(&cfg, &a);
	a.equipment_compliance_reviewed = false;
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_NOT_AUTHORIZED);
	setup(&cfg, &a);
	a.operator_confirmed_uk = false;
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_NOT_AUTHORIZED);
	setup(&cfg, &a);
	a.feed_loss_mdb = -1;
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_NOT_AUTHORIZED);
	zassert_equal(r.calls, 0);
}

ZTEST(os_allow, test_excess_erp_denied)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a;

	setup(&cfg, &a);
	a.antenna_gain_mdbi = 25000; /* -9+3+25-2.15 = 16.85 dBm > 11.979 */
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_ERP);
	zassert_false(uk868_gate_is_open(&c.gate));
}

ZTEST(os_allow, test_power_above_ceiling_and_cfg_mismatch_denied)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a;

	setup(&cfg, &a);
	cfg.tx_power_dbm = -8;
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_POWER);
	setup(&cfg, &a);
	cfg.frequency_hz = 868310000u;
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_CFG_MISMATCH);
	setup(&cfg, &a);
	cfg.public_network = true;
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_ERR_CFG_MISMATCH);
}

ZTEST(os_allow, test_tampered_cfg_after_init_blocked_before_radio)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a;
	struct fake_radio r = { .started = true };

	setup(&cfg, &a);
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_SENT_OK);
	c.cfg.sf = 12;
	zassert_equal(frurt_os_fire(&c, 1000, fake_tx, &r), FRURT_OS_ERR_CFG_MISMATCH);
	zassert_equal(r.calls, 0);
}

ZTEST(os_allow, test_budget_exhausted_blocks_radio)
{
	struct frurt_os_rf_cfg cfg;
	struct frurt_os_authz a;
	struct fake_radio r = { .started = true };
	struct uk868_token t;

	setup(&cfg, &a);
	zassert_equal(frurt_os_init(&c, &cfg, &a), FRURT_OS_SENT_OK);
	zassert_equal(uk868_budget_reserve(&c.budget, &c.gate, 1000, UK868_TX_OTHER,
					   UK868_BUDGET_US - 1000, &t), UK868_OK);
	zassert_equal(frurt_os_fire(&c, 1000, fake_tx, &r), FRURT_OS_ERR_REGULATORY);
	zassert_equal(r.calls, 0);
}
ZTEST_SUITE(os_allow, NULL, NULL, NULL, NULL, NULL);
#endif

ZTEST_SUITE(os_common, NULL, NULL, NULL, NULL, NULL);
