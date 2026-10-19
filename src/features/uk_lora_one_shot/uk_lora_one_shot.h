/*
 * Frurt UK868 single-packet ONE-SHOT TX policy (TASK024).
 *
 * Pure C, no Zephyr/radio dependency. This module never touches the radio:
 * the actual transmit is an injected callback that exists only in a build
 * that sets BOTH FRURT_UK_ONE_SHOT_TX_ALLOW=1 and UK868_ALLOW_GATE_OPEN=1
 * (they must be equal: enforced with #error). Default is 0 = locked.
 *
 * Software gating is an engineering safeguard, NOT regulatory certification.
 * See docs/TASK024-ONE-SHOT-TX-SAFETY-REVIEW.md.
 */
#ifndef UK_LORA_ONE_SHOT_H_
#define UK_LORA_ONE_SHOT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "uk_lora_one_shot_store.h"
#include "uk_lora_regulatory.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FRURT_UK_ONE_SHOT_TX_ALLOW
#define FRURT_UK_ONE_SHOT_TX_ALLOW 0
#endif

/* ---- RF configuration that Kit1 RX (tests/sx1262_uk_pair) uses ------------ */
#define FRURT_OS_FREQ_HZ        868300000u
#define FRURT_OS_BW_HZ          125000u
#define FRURT_OS_SF             9u
#define FRURT_OS_CR             1u /* 4/5 (Zephyr CR_4_5 == 1) */
#define FRURT_OS_PREAMBLE       8u

/* One fixed, short payload; the sender cannot choose length at run time. */
#define FRURT_OS_PAYLOAD_LEN    10u
#define FRURT_OS_PAYLOAD        "FRURT-OS01"
/* Hard cap on the airtime a one-shot may reserve (margined). */
#define FRURT_OS_MAX_AIRTIME_US 250000ull
/* PROVISIONAL conducted-power candidate. NOT a permission. */
#define FRURT_OS_TX_POWER_DBM   (-9)
/* Hard ceiling this module will ever accept as the configured power. */
#define FRURT_OS_TX_POWER_CEIL_DBM (-9)

/* Default assumed conducted-power tolerance (mdB); the assessment may raise it. */
#define FRURT_OS_DEFAULT_POWER_TOL_MDB 3000

struct frurt_os_rf_cfg {
	uint32_t frequency_hz;
	uint32_t bandwidth_hz;
	uint8_t sf;
	uint8_t cr;
	uint16_t preamble_symbols;
	bool public_network; /* must be false (private sync, as RX) */
	bool iq_inverted;    /* must be false */
	bool crc_on;
	bool explicit_header;
	int8_t tx_power_dbm;
};

/* Evidence a competent person must supply; zero-initialised == DENIED. */
struct frurt_os_authz {
	bool operator_confirmed_uk;
	bool erp_assessment_documented;
	bool equipment_compliance_reviewed;
	bool antenna_gain_verified; /* true only with a documented measurement/spec */
	int32_t antenna_gain_mdbi;
	int32_t feed_loss_mdb;       /* >= 0 */
	int32_t power_tolerance_mdb; /* >= 0, positive error above nominal */
};

enum frurt_os_result {
	FRURT_OS_SENT_OK = 0,
	FRURT_OS_ERR_PARAM = -1,
	FRURT_OS_ERR_LOCKED = -2,       /* compile-time lock: no TX possible */
	FRURT_OS_ERR_ALREADY_USED = -3, /* the one shot is consumed */
	FRURT_OS_ERR_NOT_AUTHORIZED = -4,
	FRURT_OS_ERR_CFG_MISMATCH = -5, /* differs from RX configuration */
	FRURT_OS_ERR_POWER = -6,
	FRURT_OS_ERR_ERP = -7,
	FRURT_OS_ERR_AIRTIME = -8,
	FRURT_OS_ERR_REGULATORY = -9,   /* uk868 gate/budget/profile denied */
	FRURT_OS_ERR_RADIO = -10,       /* tx callback failed; airtime kept charged */
	FRURT_OS_ERR_NOT_ARMED = -11,   /* durable store absent/blank/corrupt/consumed */
	FRURT_OS_ERR_STORE = -12,       /* durable consume failed: NO radio call made */
	FRURT_OS_ERR_BUSY = -13,        /* another fire() holds the claim lock: denied, shot NOT consumed */
	FRURT_OS_ERR_NO_LOCK = -14,     /* no usable claim lock attached: denied, store untouched */
};

/*
 * Claim lock (TASK028). fire() performs its whole check -> latch -> durable
 * consume sequence while holding this lock, so two callers can never both pass
 * the ARMED check. Contract the integrator MUST satisfy (not provable here):
 *  - try_acquire() never blocks, is NOT recursive, and returns 0 only when the
 *    caller has gained exclusive ownership; any other value means "denied".
 *  - release() is called exactly once per successful try_acquire().
 *  - ONE lock instance must be shared by every context/caller that can reach
 *    the same durable store.
 *  - The lock must be effective against every execution context that can call
 *    fire() (threads, and ISRs if fire() is ever reachable from one). A
 *    k_mutex is recursive and ISR-unsafe and therefore NOT suitable; use an
 *    atomic compare-and-swap flag or a k_sem (count 1, K_NO_WAIT).
 * The lock is released before the radio callback runs.
 */
struct frurt_os_lock_ops {
	int (*try_acquire)(void *ctx);
	void (*release)(void *ctx);
	void *ctx;
};

struct frurt_os_ctx {
	struct uk868_budget budget;
	struct uk868_gate gate;
	struct uk868_profile profile;
	struct frurt_os_rf_cfg cfg;
	const struct frurt_os_store_ops *store; /* durable once-only record; NULL = deny */
	int32_t worst_case_erp_mdbm;
	bool init_ok;          /* all checks passed at init */
	const struct frurt_os_lock_ops *lock; /* claim lock; NULL = deny (fail closed) */
	bool consumed;         /* latched BEFORE any radio call, never cleared; read/written only under lock */
	uint32_t tx_callbacks; /* times the radio callback was invoked */
};

/* Fill cfg with the exact RX-matching configuration. */
void frurt_os_rf_cfg_default(struct frurt_os_rf_cfg *cfg);
/* True only if every field except power equals the RX configuration. */
bool frurt_os_rf_cfg_matches_rx(const struct frurt_os_rf_cfg *cfg);
/* Worst-case ERP (mdBm) = nominal + tolerance + gain - 2.15 dB - loss. */
int32_t frurt_os_worst_case_erp_mdbm(int8_t tx_power_dbm, const struct frurt_os_authz *a);

/* Margined airtime for the fixed payload on cfg; 0 on invalid input. */
uint64_t frurt_os_airtime_us(const struct frurt_os_rf_cfg *cfg);

/* Initialise a NEW context. The gate stays closed unless the build is
 * unlocked AND every check passes; returns FRURT_OS_ERR_* when denied. */
int frurt_os_init(struct frurt_os_ctx *c, const struct frurt_os_rf_cfg *cfg,
		  const struct frurt_os_authz *authz);

/* Attach the durable once-only record (call after init). Never arms anything.
 * Without a store that reads ARMED, fire() denies in every build. */
void frurt_os_attach_store(struct frurt_os_ctx *c, const struct frurt_os_store_ops *store);

/* Attach the claim lock (call after init, before any concurrent use). Without a
 * valid lock fire() returns FRURT_OS_ERR_NO_LOCK in the allow build. */
void frurt_os_attach_lock(struct frurt_os_ctx *c, const struct frurt_os_lock_ops *lock);

/* Radio callback: set *started=true BEFORE keying the transmitter. Report
 * *actual_us=0 when the true on-air time is not known (counted in full). */
typedef int (*frurt_os_tx_fn)(void *user, const uint8_t *data, size_t len,
			      bool *started, uint64_t *actual_us);

/* The only path to tx. Order: checks -> durable consume (verified readback)
 * -> radio. The shot stays consumed across reset even if the radio fails or
 * its airtime is unknown. Contended or re-entrant calls get FRURT_OS_ERR_BUSY
 * (before the claim) or FRURT_OS_ERR_ALREADY_USED (after it); never a second TX. */
int frurt_os_fire(struct frurt_os_ctx *c, uint32_t now_ms, frurt_os_tx_fn tx, void *user);

#ifdef __cplusplus
}
#endif

#endif /* UK_LORA_ONE_SHOT_H_ */
