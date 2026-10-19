#include "uk_lora_one_shot.h"

#include <string.h>

/* The two compile-time locks must move together: one cannot open without the other. */
#if FRURT_UK_ONE_SHOT_TX_ALLOW != UK868_ALLOW_GATE_OPEN
#error "FRURT_UK_ONE_SHOT_TX_ALLOW and UK868_ALLOW_GATE_OPEN must be set identically"
#endif
#if (FRURT_UK_ONE_SHOT_TX_ALLOW != 0) && (FRURT_UK_ONE_SHOT_TX_ALLOW != 1)
#error "FRURT_UK_ONE_SHOT_TX_ALLOW must be 0 or 1"
#endif
/* TASK025: the unlocked path is hard-disabled until a durable store adapter passes the gate. */
#if FRURT_UK_ONE_SHOT_TX_ALLOW && !FRURT_UK_ONE_SHOT_STORE_ACCEPTED
#error "TX allow requires a reviewed durable store (FRURT_UK_ONE_SHOT_STORE_ACCEPTED); see docs/TASK025"
#endif

void frurt_os_attach_store(struct frurt_os_ctx *c, const struct frurt_os_store_ops *store)
{
	if (c != NULL) {
		c->store = store;
	}
}

void frurt_os_attach_lock(struct frurt_os_ctx *c, const struct frurt_os_lock_ops *lock)
{
	if (c != NULL) {
		c->lock = lock;
	}
}

void frurt_os_rf_cfg_default(struct frurt_os_rf_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->frequency_hz = FRURT_OS_FREQ_HZ;
	cfg->bandwidth_hz = FRURT_OS_BW_HZ;
	cfg->sf = FRURT_OS_SF;
	cfg->cr = FRURT_OS_CR;
	cfg->preamble_symbols = FRURT_OS_PREAMBLE;
	cfg->crc_on = true;
	cfg->explicit_header = true;
	cfg->tx_power_dbm = FRURT_OS_TX_POWER_DBM;
}

bool frurt_os_rf_cfg_matches_rx(const struct frurt_os_rf_cfg *cfg)
{
	return cfg != NULL && cfg->frequency_hz == FRURT_OS_FREQ_HZ &&
	       cfg->bandwidth_hz == FRURT_OS_BW_HZ && cfg->sf == FRURT_OS_SF &&
	       cfg->cr == FRURT_OS_CR && cfg->preamble_symbols == FRURT_OS_PREAMBLE &&
	       !cfg->public_network && !cfg->iq_inverted && cfg->crc_on &&
	       cfg->explicit_header;
}

int32_t frurt_os_worst_case_erp_mdbm(int8_t tx_power_dbm, const struct frurt_os_authz *a)
{
	int64_t cond = (int64_t)tx_power_dbm * 1000 + a->power_tolerance_mdb;

	if (cond > INT32_MAX) {
		cond = INT32_MAX;
	}
	return uk868_erp_mdbm((int32_t)cond, a->antenna_gain_mdbi, a->feed_loss_mdb);
}

static void to_profile(const struct frurt_os_rf_cfg *cfg, struct uk868_profile *p)
{
	memset(p, 0, sizeof(*p));
	p->center_hz = cfg->frequency_hz;
	p->bandwidth_hz = cfg->bandwidth_hz;
	p->sf = cfg->sf;
	p->cr = cfg->cr;
	p->preamble_symbols = cfg->preamble_symbols;
	p->crc_on = cfg->crc_on;
	p->explicit_header = cfg->explicit_header;
}

uint64_t frurt_os_airtime_us(const struct frurt_os_rf_cfg *cfg)
{
	struct uk868_profile p;

	if (cfg == NULL) {
		return 0;
	}
	to_profile(cfg, &p);
	return uk868_airtime_us(&p, FRURT_OS_PAYLOAD_LEN);
}

int frurt_os_init(struct frurt_os_ctx *c, const struct frurt_os_rf_cfg *cfg,
		  const struct frurt_os_authz *authz)
{
	if (c == NULL) {
		return FRURT_OS_ERR_PARAM;
	}
	memset(c, 0, sizeof(*c));
	uk868_budget_init(&c->budget); /* fresh: cold-boot precharge would forbid even one packet */
	if (cfg == NULL || authz == NULL) {
		return FRURT_OS_ERR_PARAM;
	}
	c->cfg = *cfg;
	to_profile(cfg, &c->profile);

#if !FRURT_UK_ONE_SHOT_TX_ALLOW
	return FRURT_OS_ERR_LOCKED;
#else
	if (!frurt_os_rf_cfg_matches_rx(cfg)) {
		return FRURT_OS_ERR_CFG_MISMATCH;
	}
	if (cfg->tx_power_dbm > FRURT_OS_TX_POWER_CEIL_DBM) {
		return FRURT_OS_ERR_POWER;
	}
	if (!authz->antenna_gain_verified || authz->feed_loss_mdb < 0 ||
	    authz->power_tolerance_mdb < 0) {
		return FRURT_OS_ERR_NOT_AUTHORIZED;
	}
	c->worst_case_erp_mdbm = frurt_os_worst_case_erp_mdbm(cfg->tx_power_dbm, authz);

	uint64_t at = frurt_os_airtime_us(cfg);

	if (at == 0 || at > FRURT_OS_MAX_AIRTIME_US) {
		return FRURT_OS_ERR_AIRTIME;
	}

	struct uk868_authorization ua = {
		.operator_confirmed_uk = authz->operator_confirmed_uk,
		.erp_assessment_documented = authz->erp_assessment_documented,
		.equipment_compliance_reviewed = authz->equipment_compliance_reviewed,
		.worst_case_erp_mdbm = c->worst_case_erp_mdbm,
	};

	if (uk868_gate_open(&c->gate, &ua) != UK868_OK) {
		return c->worst_case_erp_mdbm >
				       UK868_ERP_LIMIT_MDBM - UK868_ERP_MARGIN_MDBM
			       ? FRURT_OS_ERR_ERP
			       : FRURT_OS_ERR_NOT_AUTHORIZED;
	}
	c->init_ok = true;
	return FRURT_OS_SENT_OK;
#endif
}

#if FRURT_UK_ONE_SHOT_TX_ALLOW
static const uint8_t payload_bytes[FRURT_OS_PAYLOAD_LEN] = FRURT_OS_PAYLOAD;

struct wrap {
	struct frurt_os_ctx *c;
	frurt_os_tx_fn tx;
	void *user;
};

static int wrap_tx(void *user, const uint8_t *data, size_t len, bool *started,
		   uint64_t *actual_us)
{
	struct wrap *w = user;

	w->c->tx_callbacks++;
	return w->tx(w->user, data, len, started, actual_us);
}
#endif

#if FRURT_UK_ONE_SHOT_TX_ALLOW
/* Caller holds c->lock. Every state read/write of the claim happens here. */
static int claim_locked(struct frurt_os_ctx *c)
{
	if (c->consumed) {
		return FRURT_OS_ERR_ALREADY_USED;
	}
	if (!c->init_ok || !uk868_gate_is_open(&c->gate)) {
		return FRURT_OS_ERR_NOT_AUTHORIZED;
	}
	if (c->store == NULL || frurt_os_store_state(c->store) != FRURT_OS_STORE_ARMED) {
		return FRURT_OS_ERR_NOT_ARMED;
	}
	/* Latch first: any outcome after this point, including an error, uses the shot. */
	c->consumed = true;

	if (!frurt_os_rf_cfg_matches_rx(&c->cfg)) {
		return FRURT_OS_ERR_CFG_MISMATCH;
	}
	uint64_t at = frurt_os_airtime_us(&c->cfg);

	if (at == 0 || at > FRURT_OS_MAX_AIRTIME_US) {
		return FRURT_OS_ERR_AIRTIME;
	}

	/* Durable commit BEFORE any radio activity; any failure means no TX. */
	if (frurt_os_store_consume(c->store) != FRURT_OS_STORE_OK) {
		return FRURT_OS_ERR_STORE;
	}
	return FRURT_OS_SENT_OK;
}
#endif

int frurt_os_fire(struct frurt_os_ctx *c, uint32_t now_ms, frurt_os_tx_fn tx, void *user)
{
	if (c == NULL || tx == NULL) {
		return FRURT_OS_ERR_PARAM;
	}
#if !FRURT_UK_ONE_SHOT_TX_ALLOW
	(void)now_ms;
	(void)user;
	return FRURT_OS_ERR_LOCKED;
#else
	const struct frurt_os_lock_ops *lk = c->lock;

	if (lk == NULL || lk->try_acquire == NULL || lk->release == NULL) {
		return FRURT_OS_ERR_NO_LOCK;
	}
	if (lk->try_acquire(lk->ctx) != 0) {
		return FRURT_OS_ERR_BUSY;
	}
	int claim = claim_locked(c);

	lk->release(lk->ctx);
	if (claim != FRURT_OS_SENT_OK) {
		return claim;
	}

	struct wrap w = { c, tx, user };
	/* uk868_guarded_send reserves airtime BEFORE calling the radio; a started
	 * transmit with unknown duration keeps the full reservation charged. */
	int rc = uk868_guarded_send(&c->budget, &c->gate, &c->profile, now_ms, UK868_TX_NEW,
				    payload_bytes, FRURT_OS_PAYLOAD_LEN, wrap_tx, &w);

	if (rc == UK868_OK) {
		return FRURT_OS_SENT_OK;
	}
	return rc == UK868_ERR_RADIO ? FRURT_OS_ERR_RADIO : FRURT_OS_ERR_REGULATORY;
#endif
}
