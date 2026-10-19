#include <string.h>

#include <zephyr/ztest.h>

#include "uk_lora_regulatory.h"

#define SEC_MS 1000u
#define HOUR_MS 3600000u

struct fake_radio {
	unsigned int calls;
	bool started;
	uint64_t actual_us;
	int rc;
	struct uk868_budget *observed_budget;
	uint64_t used_when_called;
};

static int fake_tx(void *user, const uint8_t *d, size_t n, bool *started, uint64_t *actual)
{
	struct fake_radio *r = user;

	ARG_UNUSED(d);
	ARG_UNUSED(n);
	r->calls++;
	if (r->observed_budget != NULL) {
		r->used_when_called = uk868_budget_used_us(r->observed_budget);
	}
	*started = r->started;
	*actual = r->actual_us;
	return r->rc;
}

#ifndef UK868_LOCKED_TEST

static const struct uk868_authorization good_auth = {
	.operator_confirmed_uk = true,
	.erp_assessment_documented = true,
	.equipment_compliance_reviewed = true,
	.worst_case_erp_mdbm = 10000,
};

static struct uk868_gate open_gate(void)
{
	struct uk868_gate g = { 0 };

	zassert_equal(uk868_gate_open(&g, &good_auth), UK868_OK);
	zassert_true(uk868_gate_is_open(&g));
	return g;
}

ZTEST_SUITE(uk868, NULL, NULL, NULL, NULL, NULL);

ZTEST(uk868, test_default_profile_is_valid_and_in_band)
{
	struct uk868_profile p;

	uk868_profile_default(&p);
	zassert_equal(p.center_hz, 868300000u);
	zassert_equal(p.bandwidth_hz, 125000u);
	zassert_equal(uk868_profile_validate(&p), UK868_OK);
}

ZTEST(uk868, test_band_edges_rejected_and_profile_unchanged)
{
	struct uk868_profile p;

	uk868_profile_default(&p);
	zassert_equal(uk868_profile_set_channel(&p, 868000000u, 125000u), UK868_ERR_BAND);
	zassert_equal(uk868_profile_set_channel(&p, 868600000u, 125000u), UK868_ERR_BAND);
	zassert_equal(uk868_profile_set_channel(&p, 868100000u, 250000u), UK868_ERR_BAND);
	zassert_equal(uk868_profile_set_channel(&p, 867900000u, 125000u), UK868_ERR_BAND);
	zassert_equal(uk868_profile_set_channel(&p, 868700000u, 125000u), UK868_ERR_BAND);
	zassert_equal(uk868_profile_set_channel(&p, 0u, 125000u), UK868_ERR_BAND);
	zassert_equal(uk868_profile_set_channel(&p, UINT32_MAX, 500000u), UK868_ERR_BAND);
	zassert_equal(uk868_profile_set_channel(&p, 868300000u, 123456u), UK868_ERR_PARAM);
	zassert_equal(p.center_hz, 868300000u);
	zassert_equal(p.bandwidth_hz, 125000u);
	/* Exactly at guarded edge is accepted. */
	zassert_equal(uk868_profile_set_channel(&p, 868125000u, 125000u), UK868_OK);
	zassert_equal(uk868_profile_set_channel(&p, 868475000u, 125000u), UK868_OK);
}

ZTEST(uk868, test_sweep_never_crosses_band)
{
	static const uint32_t bws[] = { 125000u, 250000u, 500000u };
	struct uk868_profile p;

	uk868_profile_default(&p);
	for (uint32_t c = 867800000u; c <= 868800000u; c += 5000u) {
		for (size_t i = 0; i < ARRAY_SIZE(bws); i++) {
			p.center_hz = c;
			p.bandwidth_hz = bws[i];
			if (uk868_profile_validate(&p) == UK868_OK) {
				zassert_true(c - bws[i] / 2u >= UK868_BAND_LOW_HZ);
				zassert_true(c + bws[i] / 2u <= UK868_BAND_HIGH_HZ);
			}
		}
	}
}

ZTEST(uk868, test_airtime_known_vectors_and_invalid)
{
	struct uk868_profile p;
	uint64_t t;

	uk868_profile_default(&p);
	p.sf = 7;
	/* Semtech reference 41.216 ms; ours adds 5% margin. */
	t = uk868_airtime_us(&p, 10);
	zassert_true(t >= 41216u && t <= 43278u, "%llu", t);
	p.sf = 12;
	t = uk868_airtime_us(&p, 10); /* reference 991.232 ms */
	zassert_true(t >= 991232u && t <= 1040794u, "%llu", t);
	zassert_equal(uk868_airtime_us(&p, 0), 0u);
	zassert_equal(uk868_airtime_us(&p, 256), 0u);
	p.center_hz = 868000000u;
	zassert_equal(uk868_airtime_us(&p, 10), 0u);
	uk868_profile_default(&p);
	uint64_t prev = 0;

	for (size_t n = 1; n <= 255; n++) {
		t = uk868_airtime_us(&p, n);
		zassert_true(t >= prev);
		prev = t;
	}
}

ZTEST(uk868, test_erp_helper_and_gate_authorization)
{
	/* 14 dBm conducted + 2 dBi - 1 dB loss => 14.85 dBm EIRP-ish => 12.7 dBm ERP */
	zassert_equal(uk868_erp_mdbm(14000, 2000, 1000), 12850);
	zassert_equal(uk868_erp_mdbm(INT32_MAX, INT32_MAX, INT32_MIN), INT32_MAX);
	zassert_equal(uk868_erp_mdbm(INT32_MIN, INT32_MIN, INT32_MAX), INT32_MIN);

	struct uk868_gate g = { 0 };
	struct uk868_authorization a = good_auth;

	a.worst_case_erp_mdbm = UK868_ERP_LIMIT_MDBM - UK868_ERP_MARGIN_MDBM + 1;
	zassert_equal(uk868_gate_open(&g, &a), UK868_ERR_NOT_AUTHORIZED);
	zassert_false(uk868_gate_is_open(&g));
	a = good_auth;
	a.operator_confirmed_uk = false;
	zassert_equal(uk868_gate_open(&g, &a), UK868_ERR_NOT_AUTHORIZED);
	a = good_auth;
	a.erp_assessment_documented = false;
	zassert_equal(uk868_gate_open(&g, &a), UK868_ERR_NOT_AUTHORIZED);
	a = good_auth;
	a.equipment_compliance_reviewed = false;
	zassert_equal(uk868_gate_open(&g, &a), UK868_ERR_NOT_AUTHORIZED);
	zassert_equal(uk868_gate_open(&g, NULL), UK868_ERR_GATE_DENIED);
	zassert_false(uk868_gate_is_open(&g));
	/* A failed re-open attempt closes a previously open gate. */
	g = open_gate();
	a.worst_case_erp_mdbm = 20000;
	zassert_not_equal(uk868_gate_open(&g, &a), UK868_OK);
	zassert_false(uk868_gate_is_open(&g));
}

ZTEST(uk868, test_gate_default_denied_no_radio)
{
	struct uk868_gate g = { 0 };
	struct uk868_budget b;
	struct uk868_profile p;
	struct fake_radio r = { .started = true, .actual_us = 1000 };
	uint8_t d[10] = { 0 };

	uk868_budget_init(&b);
	uk868_profile_default(&p);
	zassert_false(uk868_gate_is_open(&g));
	zassert_equal(uk868_guarded_send(&b, &g, &p, 0, UK868_TX_NEW, d, sizeof(d), fake_tx, &r),
		      UK868_ERR_GATE_DENIED);
	zassert_equal(r.calls, 0u);
	zassert_equal(uk868_budget_used_us(&b), 0u);
	struct uk868_token t;

	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_NEW, 1000, &t), UK868_ERR_GATE_DENIED);
	zassert_equal(t.id, 0u);
	/* Closing the gate again blocks radio. */
	g = open_gate();
	uk868_gate_close(&g);
	zassert_equal(uk868_guarded_send(&b, &g, &p, 0, UK868_TX_NEW, d, sizeof(d), fake_tx, &r),
		      UK868_ERR_GATE_DENIED);
	zassert_equal(r.calls, 0u);
}

ZTEST(uk868, test_budget_limit_and_all_classes_share_it)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_token t;
	const uint64_t q = UK868_BUDGET_US / 5u;

	uk868_budget_init(&b);
	for (int c = 0; c < UK868_TX_CLASS_COUNT; c++) {
		zassert_equal(uk868_budget_reserve(&b, &g, 0, (enum uk868_tx_class)c, q, &t),
			      UK868_OK);
		zassert_equal(uk868_budget_finish(&b, t, UK868_OUTCOME_UNKNOWN, 0), UK868_OK);
		zassert_equal(b.class_reserved_us[c], q);
	}
	zassert_equal(uk868_budget_used_us(&b), UK868_BUDGET_US);
	zassert_equal(uk868_budget_remaining_us(&b), 0u);
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_RETRY, 1, &t), UK868_ERR_BUDGET);
	zassert_equal(b.denied_budget, 1u);
	/* Budget stays strictly below the 1% legal ceiling. */
	zassert_true(UK868_BUDGET_US * 1000000ull / UK868_WINDOW_US < UK868_DUTY_CEILING_PPM);
}

ZTEST(uk868, test_pending_token_id_wrap_skips_live_id)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_token first, second, third;

	uk868_budget_init(&b);
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_NEW, 1000, &first), UK868_OK);
	b.next_id = UINT32_MAX;
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_RETRY, 1000, &second), UK868_OK);
	zassert_equal(second.id, UINT32_MAX);
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_RELAY, 1000, &third), UK868_OK);
	zassert_not_equal(third.id, first.id);
	zassert_not_equal(third.id, second.id);
	zassert_equal(uk868_budget_finish(&b, first, UK868_OUTCOME_NOT_STARTED, 0), UK868_OK);
	zassert_equal(uk868_budget_finish(&b, second, UK868_OUTCOME_NOT_STARTED, 0), UK868_OK);
	zassert_equal(uk868_budget_finish(&b, third, UK868_OUTCOME_NOT_STARTED, 0), UK868_OK);
}

ZTEST(uk868, test_failed_send_accounting)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_token t;

	uk868_budget_init(&b);
	/* Reserved before TX. */
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_NEW, 1000000, &t), UK868_OK);
	zassert_equal(uk868_budget_used_us(&b), 1000000u);
	/* Radio never keyed => refund. */
	zassert_equal(uk868_budget_finish(&b, t, UK868_OUTCOME_NOT_STARTED, 0), UK868_OK);
	zassert_equal(uk868_budget_used_us(&b), 0u);
	zassert_equal(b.refunded_total_us, 1000000u);
	/* Double finish rejected. */
	zassert_equal(uk868_budget_finish(&b, t, UK868_OUTCOME_NOT_STARTED, 0), UK868_ERR_BAD_TOKEN);
	/* Invalid outcome preserves the live reservation for a valid settlement. */
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_NEW, 1000000, &t), UK868_OK);
	zassert_equal(uk868_budget_finish(&b, t, (enum uk868_outcome)99, 0), UK868_ERR_PARAM);
	zassert_equal(b.pending[t.slot].id, t.id);
	zassert_equal(uk868_budget_used_us(&b), 1000000u);
	zassert_equal(uk868_budget_finish(&b, t, UK868_OUTCOME_NOT_STARTED, 0), UK868_OK);
	zassert_equal(uk868_budget_used_us(&b), 0u);
	/* Unknown after keying => keep full reservation. */
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_RETRY, 1000000, &t), UK868_OK);
	zassert_equal(uk868_budget_finish(&b, t, UK868_OUTCOME_UNKNOWN, 0), UK868_OK);
	zassert_equal(uk868_budget_used_us(&b), 1000000u);
	/* Completed shorter => refund difference. */
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_RELAY, 1000000, &t), UK868_OK);
	zassert_equal(uk868_budget_finish(&b, t, UK868_OUTCOME_COMPLETED, 800000), UK868_OK);
	zassert_equal(uk868_budget_used_us(&b), 1800000u);
	/* Completed longer => extra charged. */
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_RELAY, 1000000, &t), UK868_OK);
	zassert_equal(uk868_budget_finish(&b, t, UK868_OUTCOME_COMPLETED, 1500000), UK868_OK);
	zassert_equal(uk868_budget_used_us(&b), 3300000u);
	zassert_equal(b.overrun_total_us, 500000u);
	/* Forged token rejected. */
	struct uk868_token bad = { .id = 999, .slot = 0 };

	zassert_equal(uk868_budget_finish(&b, bad, UK868_OUTCOME_UNKNOWN, 0), UK868_ERR_BAD_TOKEN);
}

ZTEST(uk868, test_pending_slots_exhaust)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_token t[UK868_PENDING_SLOTS + 1];

	uk868_budget_init(&b);
	for (unsigned int i = 0; i < UK868_PENDING_SLOTS; i++) {
		zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_NEW, 1000, &t[i]), UK868_OK);
	}
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_NEW, 1000, &t[UK868_PENDING_SLOTS]),
		      UK868_ERR_NO_SLOT);
	zassert_equal(uk868_budget_used_us(&b), 4000u);
}

ZTEST(uk868, test_rolling_hour_expiry_is_conservative)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_token t;

	uk868_budget_init(&b);
	zassert_equal(uk868_budget_reserve(&b, &g, 5000, UK868_TX_NEW, UK868_BUDGET_US, &t), UK868_OK);
	zassert_equal(uk868_budget_finish(&b, t, UK868_OUTCOME_UNKNOWN, 0), UK868_OK);
	/* One hour later it must still be counted. */
	zassert_equal(uk868_budget_reserve(&b, &g, 5000 + HOUR_MS, UK868_TX_NEW, 1, &t),
		      UK868_ERR_BUDGET);
	/* Fully expired after one hour plus one bucket. */
	zassert_equal(uk868_budget_reserve(&b, &g, 5000 + HOUR_MS + 2 * UK868_BUCKET_MS,
					   UK868_TX_NEW, 1, &t), UK868_OK);
	/* Used never reports less than what was just reserved. */
	zassert_true(uk868_budget_used_us(&b) >= 1u);
}

ZTEST(uk868, test_tick_wraparound_and_backwards_clock)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_token t;
	const uint32_t start = 0xFFFFFF00u;

	uk868_budget_init(&b);
	zassert_equal(uk868_budget_reserve(&b, &g, start, UK868_TX_NEW, UK868_BUDGET_US, &t), UK868_OK);
	/* 32-bit tick wraps to small value 0x100 ms later: still same window. */
	zassert_equal(uk868_budget_reserve(&b, &g, 0x100u, UK868_TX_NEW, 1, &t), UK868_ERR_BUDGET);
	zassert_true(b.now_ms > 0xFFFFFFFFull);
	uint64_t before = b.now_ms;

	/* Apparent backwards step: fail closed, time not moved. */
	zassert_equal(uk868_budget_reserve(&b, &g, 0x80u, UK868_TX_NEW, 1, &t), UK868_ERR_CLOCK);
	zassert_true(b.clock_fault);
	zassert_equal(b.now_ms, before);
	/* An attacker restoring a plausible forward tick cannot clear the latch. */
	zassert_equal(uk868_budget_advance(&b, 0x100u + HOUR_MS + 2 * UK868_BUCKET_MS),
		      UK868_ERR_CLOCK);
	zassert_equal(uk868_budget_reserve(&b, &g, 0x100u + HOUR_MS + 2 * UK868_BUCKET_MS,
					   UK868_TX_NEW, 1, &t), UK868_ERR_CLOCK);
	zassert_true(b.clock_fault);
	uk868_budget_init(&b);
	zassert_equal(uk868_budget_advance(&b, 1000u), UK868_OK);
	zassert_equal(uk868_budget_advance(&b, 1000u + 0x80000000u), UK868_ERR_CLOCK);
	zassert_true(b.clock_fault);
	zassert_equal(uk868_budget_advance(&b, 2000u), UK868_ERR_CLOCK);
	/* Explicit cold-boot reset pessimistically precharges the entire budget. */
	uk868_budget_init_cold_boot(&b);
	zassert_equal(uk868_budget_reserve(&b, &g, 0x100u, UK868_TX_NEW, 1, &t),
			      UK868_ERR_BUDGET);
	zassert_equal(uk868_budget_reserve(&b, &g, 0x100u + HOUR_MS + 2 * UK868_BUCKET_MS,
					   UK868_TX_NEW, 1, &t), UK868_OK);
}

ZTEST(uk868, test_saturation)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_token t;

	uk868_budget_init(&b);
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_NEW, 1000, &t), UK868_OK);
	zassert_equal(uk868_budget_finish(&b, t, UK868_OUTCOME_COMPLETED, UINT64_MAX), UK868_OK);
	zassert_equal(uk868_budget_remaining_us(&b), 0u);
	/* Second overrun must not wrap counters back to small values. */
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_NEW, 1, &t), UK868_ERR_BUDGET);
	uint64_t used = uk868_budget_used_us(&b);

	zassert_true(used >= UK868_BUDGET_US);
	/* Absurd request never overflows the comparison. */
	zassert_equal(uk868_budget_reserve(&b, &g, 0, UK868_TX_NEW, UINT64_MAX, &t), UK868_ERR_BUDGET);
	zassert_true(b.reserved_total_us >= 1000u);
}

ZTEST(uk868, test_lifetime_counters_monotonic)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_token t;
	uint64_t prev_res = 0, prev_ref = 0;

	uk868_budget_init(&b);
	for (int i = 0; i < 50; i++) {
		(void)uk868_budget_reserve(&b, &g, i * 1000u, UK868_TX_NEW, 100000, &t);
		(void)uk868_budget_finish(&b, t, (i & 1) ? UK868_OUTCOME_NOT_STARTED :
			UK868_OUTCOME_COMPLETED, 50000);
		zassert_true(b.reserved_total_us >= prev_res);
		zassert_true(b.refunded_total_us >= prev_ref);
		prev_res = b.reserved_total_us;
		prev_ref = b.refunded_total_us;
	}
}

ZTEST(uk868, test_cold_boot_precharge_blocks_reboot_reset)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_token t;

	uk868_budget_init_cold_boot(&b);
	zassert_equal(uk868_budget_reserve(&b, &g, 100, UK868_TX_NEW, 1, &t), UK868_ERR_BUDGET);
	zassert_equal(uk868_budget_reserve(&b, &g, 100 + HOUR_MS + 2 * UK868_BUCKET_MS,
					   UK868_TX_NEW, 1, &t), UK868_OK);
}

ZTEST(uk868, test_guarded_send_paths)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_profile p;
	struct fake_radio r;
	uint8_t d[20] = { 0 };
	uint64_t at;

	uk868_budget_init(&b);
	uk868_profile_default(&p);
	at = uk868_airtime_us(&p, sizeof(d));

	/* Success: actual shorter than reserved is refunded. */
	r = (struct fake_radio){ .started = true, .actual_us = at - 1000, .rc = 0 };
	zassert_equal(uk868_guarded_send(&b, &g, &p, 0, UK868_TX_NEW, d, sizeof(d), fake_tx, &r), UK868_OK);
	zassert_equal(r.calls, 1u);
	zassert_equal(uk868_budget_used_us(&b), at - 1000);

	/* Failed before keying: refunded. */
	uint64_t used = uk868_budget_used_us(&b);

	r = (struct fake_radio){ .started = false, .rc = -5 };
	zassert_equal(uk868_guarded_send(&b, &g, &p, 10, UK868_TX_RETRY, d, sizeof(d), fake_tx, &r),
		      UK868_ERR_RADIO);
	zassert_equal(uk868_budget_used_us(&b), used);

	/* Failed after keying with unknown airtime: kept. */
	r = (struct fake_radio){ .started = true, .actual_us = 0, .rc = -5 };
	zassert_equal(uk868_guarded_send(&b, &g, &p, 20, UK868_TX_RELAY, d, sizeof(d), fake_tx, &r),
		      UK868_ERR_RADIO);
	zassert_equal(uk868_budget_used_us(&b), used + at);

	/* Out-of-band profile: no radio. */
	struct uk868_profile bad = p;

	bad.center_hz = 868600000u;
	r = (struct fake_radio){ .started = true, .actual_us = at };
	zassert_equal(uk868_guarded_send(&b, &g, &bad, 30, UK868_TX_NEW, d, sizeof(d), fake_tx, &r),
		      UK868_ERR_BAND);
	zassert_equal(r.calls, 0u);

	/* Exhaust budget: further sends never reach the radio. */
	for (int i = 0; i < 2000; i++) {
		r.calls = 0;
		int rc = uk868_guarded_send(&b, &g, &p, 40, UK868_TX_DUPLICATE_FWD, d, sizeof(d),
					    fake_tx, &r);
		if (rc == UK868_ERR_BUDGET) {
			zassert_equal(r.calls, 0u);
			zassert_true(uk868_budget_used_us(&b) <= UK868_BUDGET_US);
			return;
		}
		zassert_equal(rc, UK868_OK);
	}
	zassert_unreachable("budget never exhausted");
}

ZTEST(uk868, test_guarded_send_reserves_before_tx_and_accounts_overrun)
{
	struct uk868_gate g = open_gate();
	struct uk868_budget b;
	struct uk868_profile p;
	struct fake_radio r;
	uint8_t d[20] = { 0 };
	uint64_t estimate;

	uk868_budget_init(&b);
	uk868_profile_default(&p);
	estimate = uk868_airtime_us(&p, sizeof(d));
	r = (struct fake_radio){
		.started = true,
		.actual_us = estimate + 1000,
		.observed_budget = &b,
	};
	zassert_equal(uk868_guarded_send(&b, &g, &p, 0, UK868_TX_NEW, d, sizeof(d),
					   fake_tx, &r), UK868_OK);
	zassert_equal(r.used_when_called, estimate);
	zassert_equal(uk868_budget_used_us(&b), estimate + 1000);
	zassert_equal(b.overrun_total_us, 1000u);

	/* A full-budget overrun is charged, so no second TX can start. */
	uk868_budget_init(&b);
	r = (struct fake_radio){
		.started = true,
		.actual_us = UK868_BUDGET_US + 1,
		.observed_budget = &b,
	};
	zassert_equal(uk868_guarded_send(&b, &g, &p, 0, UK868_TX_NEW, d, sizeof(d),
					   fake_tx, &r), UK868_OK);
	zassert_equal(r.used_when_called, estimate);
	r.calls = 0;
	zassert_equal(uk868_guarded_send(&b, &g, &p, 1, UK868_TX_RETRY, d, sizeof(d),
					   fake_tx, &r), UK868_ERR_BUDGET);
	zassert_equal(r.calls, 0u);
	zassert_true(uk868_budget_used_us(&b) > UK868_BUDGET_US);
}

#else /* UK868_LOCKED_TEST */

ZTEST_SUITE(uk868_locked, NULL, NULL, NULL, NULL, NULL);

ZTEST(uk868_locked, test_default_build_cannot_open_gate)
{
	struct uk868_gate g = { 0 };
	struct uk868_authorization a = {
		.operator_confirmed_uk = true,
		.erp_assessment_documented = true,
		.equipment_compliance_reviewed = true,
		.worst_case_erp_mdbm = 0,
	};
	struct uk868_budget b;
	struct uk868_profile p;
	struct fake_radio r = { .started = true, .actual_us = 1 };
	uint8_t d[4] = { 0 };

	zassert_equal(uk868_gate_open(&g, &a), UK868_ERR_NOT_AUTHORIZED);
	zassert_false(uk868_gate_is_open(&g));
	uk868_budget_init(&b);
	uk868_profile_default(&p);
	zassert_equal(uk868_guarded_send(&b, &g, &p, 0, UK868_TX_NEW, d, sizeof(d), fake_tx, &r),
		      UK868_ERR_GATE_DENIED);
	zassert_equal(r.calls, 0u);
	/* Compile-time TX lock must survive a caller forcing the public flag. */
	g.open = true;
	zassert_false(uk868_gate_is_open(&g));
	zassert_equal(uk868_guarded_send(&b, &g, &p, 1, UK868_TX_NEW,
					   d, sizeof(d), fake_tx, &r), UK868_ERR_GATE_DENIED);
	zassert_equal(r.calls, 0u);
	zassert_equal(uk868_budget_reserve(&b, &g, 2, UK868_TX_NEW, 1, &(struct uk868_token){0}),
		      UK868_ERR_GATE_DENIED);
}

#endif
