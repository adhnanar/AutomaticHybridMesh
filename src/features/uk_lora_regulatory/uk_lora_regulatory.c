#include "uk_lora_regulatory.h"

#include <string.h>

static bool bw_supported(uint32_t bw)
{
	return bw == 125000u || bw == 250000u || bw == 500000u;
}

static uint64_t sat_add(uint64_t a, uint64_t b)
{
	return (a > UINT64_MAX - b) ? UINT64_MAX : a + b;
}

static uint64_t sat_sub(uint64_t a, uint64_t b)
{
	return (b > a) ? 0 : a - b;
}

void uk868_profile_default(struct uk868_profile *p)
{
	memset(p, 0, sizeof(*p));
	p->center_hz = UK868_PROPOSED_CENTER_HZ;
	p->bandwidth_hz = UK868_PROPOSED_BW_HZ;
	p->sf = 9;
	p->cr = 1;
	p->preamble_symbols = 8;
	p->crc_on = true;
	p->explicit_header = true;
}

int uk868_profile_validate(const struct uk868_profile *p)
{
	if (p == NULL || !bw_supported(p->bandwidth_hz) || p->sf < 7 || p->sf > 12 ||
	    p->cr < 1 || p->cr > 4 || p->preamble_symbols < 6) {
		return UK868_ERR_PARAM;
	}

	int64_t half = (int64_t)(p->bandwidth_hz / 2u);
	int64_t lo = (int64_t)p->center_hz - half;
	int64_t hi = (int64_t)p->center_hz + half;

	if (lo < (int64_t)UK868_BAND_LOW_HZ + (int64_t)UK868_EDGE_GUARD_HZ ||
	    hi > (int64_t)UK868_BAND_HIGH_HZ - (int64_t)UK868_EDGE_GUARD_HZ) {
		return UK868_ERR_BAND;
	}
	return UK868_OK;
}

int uk868_profile_set_channel(struct uk868_profile *p, uint32_t center_hz, uint32_t bw_hz)
{
	if (p == NULL) {
		return UK868_ERR_PARAM;
	}
	struct uk868_profile t = *p;

	t.center_hz = center_hz;
	t.bandwidth_hz = bw_hz;
	int rc = uk868_profile_validate(&t);

	if (rc == UK868_OK) {
		*p = t;
	}
	return rc;
}

uint64_t uk868_airtime_us(const struct uk868_profile *p, size_t payload_len)
{
	if (uk868_profile_validate(p) != UK868_OK || payload_len == 0 ||
	    payload_len > UK868_MAX_PAYLOAD) {
		return 0;
	}

	uint64_t chips = 1ull << p->sf;
	/* Low data-rate optimisation when symbol time > 16 ms. */
	int de = (chips * 1000000ull > 16000ull * p->bandwidth_hz) ? 1 : 0;
	int ih = p->explicit_header ? 0 : 1;
	int64_t num = 8 * (int64_t)payload_len - 4 * (int64_t)p->sf + 28 +
		      (p->crc_on ? 16 : 0) - 20 * ih;
	int64_t den = 4 * ((int64_t)p->sf - 2 * de);
	int64_t blocks = (num > 0) ? (num + den - 1) / den : 0;
	int64_t payload_syms = 8 + blocks * ((int64_t)p->cr + 4);
	/* Quarter-symbols: preamble + 4.25 sync, plus payload symbols. */
	uint64_t q = 4ull * p->preamble_symbols + 17ull + 4ull * (uint64_t)payload_syms;
	/* us = q/4 * chips * 1e6 / bw, rounded up. */
	uint64_t n = q * chips * 250000ull;
	uint64_t us = (n + p->bandwidth_hz - 1u) / p->bandwidth_hz;
	uint64_t inflated = us * (1000ull + UK868_AIRTIME_MARGIN_PERMILLE);

	return (inflated + 999ull) / 1000ull;
}

int32_t uk868_erp_mdbm(int32_t conducted_mdbm, int32_t antenna_gain_mdbi, int32_t losses_mdb)
{
	int64_t erp = (int64_t)conducted_mdbm + antenna_gain_mdbi -
		      UK868_DIPOLE_REF_MDB - losses_mdb;

	if (erp > INT32_MAX) {
		return INT32_MAX;
	}
	if (erp < INT32_MIN) {
		return INT32_MIN;
	}
	return (int32_t)erp;
}

int uk868_gate_open(struct uk868_gate *g, const struct uk868_authorization *a)
{
	if (g == NULL) {
		return UK868_ERR_PARAM;
	}
	g->open = false;
	if (a == NULL) {
		return UK868_ERR_GATE_DENIED;
	}
#if !UK868_ALLOW_GATE_OPEN
	return UK868_ERR_NOT_AUTHORIZED;
#else
	if (!a->operator_confirmed_uk || !a->erp_assessment_documented ||
	    !a->equipment_compliance_reviewed ||
	    a->worst_case_erp_mdbm > UK868_ERP_LIMIT_MDBM - UK868_ERP_MARGIN_MDBM) {
		return UK868_ERR_NOT_AUTHORIZED;
	}
	g->open = true;
	return UK868_OK;
#endif
}

void uk868_gate_close(struct uk868_gate *g)
{
	if (g != NULL) {
		g->open = false;
	}
}

bool uk868_gate_is_open(const struct uk868_gate *g)
{
#if !UK868_ALLOW_GATE_OPEN
	/* Never authorize TX in a locked build, even if a caller sets g->open. */
	(void)g;
	return false;
#else
	return g != NULL && g->open;
#endif
}

void uk868_budget_init(struct uk868_budget *b)
{
	memset(b, 0, sizeof(*b));
	b->next_id = 1;
}

void uk868_budget_init_cold_boot(struct uk868_budget *b)
{
	uk868_budget_init(b);
	b->precharge_on_start = true;
}

static uint64_t cur_idx(const struct uk868_budget *b)
{
	return b->now_ms / UK868_BUCKET_MS;
}

static bool bucket_live(const struct uk868_budget *b, unsigned int s)
{
	if (b->bucket_id[s] == 0) {
		return false;
	}
	uint64_t id = b->bucket_id[s] - 1;
	uint64_t cur = cur_idx(b);

	return id <= cur && cur - id <= 60u;
}

static void charge(struct uk868_budget *b, uint64_t idx, uint64_t us)
{
	unsigned int s = (unsigned int)(idx % UK868_BUCKET_COUNT);

	if (b->bucket_id[s] != idx + 1) {
		b->bucket_id[s] = idx + 1;
		b->bucket_us[s] = 0;
	}
	b->bucket_us[s] = sat_add(b->bucket_us[s], us);
}

int uk868_budget_advance(struct uk868_budget *b, uint32_t raw_ms)
{
	/* A backwards/ambiguous clock is a latched TX safety fault. */
	if (b->clock_fault) {
		return UK868_ERR_CLOCK;
	}
	if (!b->started) {
		b->started = true;
		b->now_ms = raw_ms;
		b->last_raw_ms = raw_ms;
		if (b->precharge_on_start) {
			charge(b, cur_idx(b), UK868_BUDGET_US);
		}
		return UK868_OK;
	}

	uint32_t delta = raw_ms - b->last_raw_ms; /* modular: survives 2^32 wrap */

	if (delta >= 0x80000000u) {
		b->clock_fault = true; /* apparent backwards step: fail closed */
		return UK868_ERR_CLOCK;
	}
	b->last_raw_ms = raw_ms;
	b->now_ms += delta;
	return UK868_OK;
}

uint64_t uk868_budget_used_us(const struct uk868_budget *b)
{
	uint64_t sum = 0;

	for (unsigned int s = 0; s < UK868_BUCKET_COUNT; s++) {
		if (bucket_live(b, s)) {
			sum = sat_add(sum, b->bucket_us[s]);
		}
	}
	return sum;
}

uint64_t uk868_budget_remaining_us(const struct uk868_budget *b)
{
	return sat_sub(UK868_BUDGET_US, uk868_budget_used_us(b));
}

int uk868_budget_reserve(struct uk868_budget *b, const struct uk868_gate *g, uint32_t raw_ms,
			 enum uk868_tx_class cls, uint64_t airtime_us, struct uk868_token *tok)
{
	if (b == NULL || tok == NULL || (unsigned int)cls >= UK868_TX_CLASS_COUNT) {
		return UK868_ERR_PARAM;
	}
	tok->id = 0;

	if (!uk868_gate_is_open(g)) {
		b->denied_gate++;
		return UK868_ERR_GATE_DENIED;
	}
	if (airtime_us == 0) {
		return UK868_ERR_PARAM;
	}
	if (uk868_budget_advance(b, raw_ms) != UK868_OK) {
		return UK868_ERR_CLOCK;
	}
	if (airtime_us > uk868_budget_remaining_us(b)) {
		b->denied_budget++;
		return UK868_ERR_BUDGET;
	}

	unsigned int slot = UK868_PENDING_SLOTS;

	for (unsigned int i = 0; i < UK868_PENDING_SLOTS; i++) {
		if (b->pending[i].id == 0) {
			slot = i;
			break;
		}
	}
	if (slot == UK868_PENDING_SLOTS) {
		return UK868_ERR_NO_SLOT;
	}

	uint32_t id = 0;

	for (unsigned int attempt = 0; attempt <= UK868_PENDING_SLOTS; attempt++) {
		uint32_t candidate = b->next_id;

		if (candidate == 0) {
			candidate = 1;
		}
		b->next_id = candidate + 1;
		if (b->next_id == 0) {
			b->next_id = 1;
		}

		bool in_use = false;

		for (unsigned int i = 0; i < UK868_PENDING_SLOTS; i++) {
			if (b->pending[i].id == candidate) {
				in_use = true;
				break;
			}
		}
		if (!in_use) {
			id = candidate;
			break;
		}
	}
	if (id == 0) {
		return UK868_ERR_NO_SLOT;
	}

	uint64_t idx = cur_idx(b);

	charge(b, idx, airtime_us);
	b->reserved_total_us = sat_add(b->reserved_total_us, airtime_us);
	b->class_reserved_us[cls] = sat_add(b->class_reserved_us[cls], airtime_us);

	b->pending[slot] = (struct uk868_pending){ id, airtime_us, idx, cls };
	tok->id = id;
	tok->slot = (uint8_t)slot;
	return UK868_OK;
}

static uint64_t refund(struct uk868_budget *b, uint64_t idx, uint64_t us)
{
	unsigned int s = (unsigned int)(idx % UK868_BUCKET_COUNT);

	if (b->bucket_id[s] != idx + 1) {
		return 0; /* bucket already recycled: cannot refund */
	}
	uint64_t take = us > b->bucket_us[s] ? b->bucket_us[s] : us;

	b->bucket_us[s] -= take;
	b->refunded_total_us = sat_add(b->refunded_total_us, take);
	return take;
}

int uk868_budget_finish(struct uk868_budget *b, struct uk868_token tok,
			enum uk868_outcome outcome, uint64_t actual_us)
{
	if (b == NULL || tok.id == 0 || tok.slot >= UK868_PENDING_SLOTS ||
	    b->pending[tok.slot].id != tok.id) {
		return UK868_ERR_BAD_TOKEN;
	}
	if (outcome != UK868_OUTCOME_NOT_STARTED && outcome != UK868_OUTCOME_UNKNOWN &&
	    outcome != UK868_OUTCOME_COMPLETED) {
		return UK868_ERR_PARAM;
	}
	struct uk868_pending pd = b->pending[tok.slot];

	memset(&b->pending[tok.slot], 0, sizeof(b->pending[tok.slot]));

	switch (outcome) {
	case UK868_OUTCOME_NOT_STARTED:
		(void)refund(b, pd.bucket_idx, pd.reserved_us);
		break;
	case UK868_OUTCOME_UNKNOWN:
		break;
	case UK868_OUTCOME_COMPLETED:
		if (actual_us < pd.reserved_us) {
			(void)refund(b, pd.bucket_idx, pd.reserved_us - actual_us);
		} else if (actual_us > pd.reserved_us) {
			uint64_t extra = actual_us - pd.reserved_us;

			charge(b, cur_idx(b), extra);
			b->overrun_total_us = sat_add(b->overrun_total_us, extra);
			b->reserved_total_us = sat_add(b->reserved_total_us, extra);
			b->class_reserved_us[pd.cls] =
				sat_add(b->class_reserved_us[pd.cls], extra);
		}
		break;
	default:
		return UK868_ERR_PARAM;
	}
	return UK868_OK;
}

int uk868_guarded_send(struct uk868_budget *b, const struct uk868_gate *g,
		       const struct uk868_profile *p, uint32_t raw_ms, enum uk868_tx_class cls,
		       const uint8_t *data, size_t len, uk868_radio_tx_fn tx, void *user)
{
	if (b == NULL || p == NULL || data == NULL || tx == NULL) {
		return UK868_ERR_PARAM;
	}
	if (!uk868_gate_is_open(g)) {
		b->denied_gate++;
		return UK868_ERR_GATE_DENIED;
	}
	int rc = uk868_profile_validate(p);

	if (rc != UK868_OK) {
		return rc;
	}
	uint64_t at = uk868_airtime_us(p, len);

	if (at == 0) {
		return UK868_ERR_PARAM;
	}

	struct uk868_token tok;

	rc = uk868_budget_reserve(b, g, raw_ms, cls, at, &tok);
	if (rc != UK868_OK) {
		return rc;
	}

	bool started = false;
	uint64_t actual = 0;
	int trc = tx(user, data, len, &started, &actual);

	if (!started) {
		(void)uk868_budget_finish(b, tok, UK868_OUTCOME_NOT_STARTED, 0);
	} else if (actual > 0) {
		(void)uk868_budget_finish(b, tok, UK868_OUTCOME_COMPLETED, actual);
	} else {
		(void)uk868_budget_finish(b, tok, UK868_OUTCOME_UNKNOWN, 0);
	}
	return trc == 0 ? UK868_OK : UK868_ERR_RADIO;
}
