/*
 * UK 868.3 MHz LoRa regulatory profile + TX gate + airtime budget (TASK019).
 *
 * Opt-in, pure C, no Zephyr/radio dependency. Software enforcement here is an
 * engineering safeguard ONLY; it is NOT a regulatory certification. See
 * docs/TASK019-UK-LORA-REGULATORY-PROFILE.md.
 */
#ifndef UK_LORA_REGULATORY_H_
#define UK_LORA_REGULATORY_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Band plan constants (Ofcom IR 2030, IR2030/1/16) ------------------- */
#define UK868_BAND_LOW_HZ           868000000u
#define UK868_BAND_HIGH_HZ          868600000u
#define UK868_PROPOSED_CENTER_HZ    868300000u
#define UK868_PROPOSED_BW_HZ        125000u
/* Extra keep-out inside each band edge for frequency tolerance / emission skirt. */
#define UK868_EDGE_GUARD_HZ         25000u

/* ERP cap 25 mW = 13.979 dBm, floored. This is ERP, NOT SX1262 register dBm. */
#define UK868_ERP_LIMIT_MDBM        13979
/* Required headroom between estimated worst-case ERP and the cap. */
#define UK868_ERP_MARGIN_MDBM       2000
/* Dipole reference: ERP = EIRP - 2.15 dB. */
#define UK868_DIPOLE_REF_MDB        2150

/* ---- Duty budget: rolling hour, deliberately below the 1% ceiling -------- */
#define UK868_WINDOW_US             3600000000ull
#define UK868_DUTY_CEILING_PPM      10000u                /* 1% legal ceiling */
#define UK868_BUDGET_US             30000000ull           /* 30 s/h = 0.833% */
#define UK868_BUCKET_MS             60000u
#define UK868_BUCKET_COUNT          61u                   /* >= 60 min kept */
/* Airtime estimate inflation to cover model uncertainty (permille). */
#define UK868_AIRTIME_MARGIN_PERMILLE 50u
#define UK868_MAX_PAYLOAD           255u
#define UK868_PENDING_SLOTS         4u

#ifndef UK868_ALLOW_GATE_OPEN
/* Default 0: the gate CANNOT be opened in a normal build. */
#define UK868_ALLOW_GATE_OPEN 0
#endif

/* ---- LoRa profile ------------------------------------------------------- */
struct uk868_profile {
	uint32_t center_hz;
	uint32_t bandwidth_hz;     /* 125000, 250000 or 500000 */
	uint8_t sf;                /* 7..12 (formula valid range) */
	uint8_t cr;                /* 1..4 => 4/5..4/8 */
	uint16_t preamble_symbols; /* >= 6 */
	bool crc_on;
	bool explicit_header;
};

enum uk868_status {
	UK868_OK = 0,
	UK868_ERR_PARAM = -1,
	UK868_ERR_BAND = -2,        /* channel would leave 868.0-868.6 (with guard) */
	UK868_ERR_GATE_DENIED = -3, /* TX gate closed (default) */
	UK868_ERR_BUDGET = -4,      /* would exceed rolling-hour budget */
	UK868_ERR_CLOCK = -5,       /* clock went backwards: fail closed */
	UK868_ERR_NO_SLOT = -6,
	UK868_ERR_BAD_TOKEN = -7,
	UK868_ERR_RADIO = -8,
	UK868_ERR_NOT_AUTHORIZED = -9, /* authorization record rejected / build locked */
};

void uk868_profile_default(struct uk868_profile *p);
/* UK868_OK only if every parameter valid and the whole channel is in-band. */
int uk868_profile_validate(const struct uk868_profile *p);
/* Apply channel only if the result validates; *p is unchanged on failure. */
int uk868_profile_set_channel(struct uk868_profile *p, uint32_t center_hz, uint32_t bw_hz);

/* Conservative time-on-air (us, rounded up, +margin). 0 on invalid input. */
uint64_t uk868_airtime_us(const struct uk868_profile *p, size_t payload_len);

/* ---- ERP helper (integer milli-dB). Does not prove compliance. ---------- */
int32_t uk868_erp_mdbm(int32_t conducted_mdbm, int32_t antenna_gain_mdbi,
		       int32_t losses_mdb);

/* ---- TX gate: zero-initialised == DENIED --------------------------------- */
struct uk868_authorization {
	bool operator_confirmed_uk;
	bool erp_assessment_documented;
	bool equipment_compliance_reviewed;
	int32_t worst_case_erp_mdbm; /* includes tolerances, from assessment */
};

struct uk868_gate {
	bool open;
};

int uk868_gate_open(struct uk868_gate *g, const struct uk868_authorization *a);
void uk868_gate_close(struct uk868_gate *g);
bool uk868_gate_is_open(const struct uk868_gate *g);

/* ---- Airtime budget ------------------------------------------------------ */
enum uk868_tx_class {
	UK868_TX_NEW = 0,       /* BLE->LoRa new message */
	UK868_TX_RETRY,
	UK868_TX_RELAY,
	UK868_TX_DUPLICATE_FWD,
	UK868_TX_OTHER,
	UK868_TX_CLASS_COUNT
};

enum uk868_outcome {
	UK868_OUTCOME_NOT_STARTED = 0, /* radio never keyed: reservation refunded */
	UK868_OUTCOME_UNKNOWN,         /* may have radiated: keep full reservation */
	UK868_OUTCOME_COMPLETED,       /* actual airtime supplied */
};

struct uk868_token {
	uint32_t id;   /* 0 == invalid */
	uint8_t slot;
};

struct uk868_pending {
	uint32_t id;
	uint64_t reserved_us;
	uint64_t bucket_idx;
	enum uk868_tx_class cls;
};

struct uk868_budget {
	uint64_t bucket_us[UK868_BUCKET_COUNT];
	uint64_t bucket_id[UK868_BUCKET_COUNT]; /* absolute bucket index + 1; 0 unused */
	uint64_t now_ms;      /* extended monotonic ms */
	uint32_t last_raw_ms; /* last raw 32-bit tick */
	bool started;
	bool precharge_on_start; /* cold boot: assume budget already spent */
	bool clock_fault;
	uint32_t next_id;
	struct uk868_pending pending[UK868_PENDING_SLOTS];
	/* Lifetime counters; each is monotonic non-decreasing (saturating). */
	uint64_t reserved_total_us;
	uint64_t refunded_total_us;
	uint64_t overrun_total_us;
	uint64_t class_reserved_us[UK868_TX_CLASS_COUNT];
	uint32_t denied_budget;
	uint32_t denied_gate;
};

/* Initialize a genuinely new budget object only; do not use this to recover
 * or reset state during a running process. */
void uk868_budget_init(struct uk868_budget *b);
/* Volatile state is lost on reboot: this variant pre-charges the full budget
 * at the first tick so a reboot cannot grant a fresh hour of airtime. */
void uk868_budget_init_cold_boot(struct uk868_budget *b);
/* Feed wrap-safe 32-bit ms tick (e.g. k_uptime_get_32()). A backwards or
 * half-range-ambiguous update permanently latches clock_fault. Only initialize
 * a new budget with uk868_budget_init(); to recover after a restart, use the
 * conservative cold-boot initializer, never reinitialize live faulted state. */
int uk868_budget_advance(struct uk868_budget *b, uint32_t raw_ms);
/* Airtime currently charged inside the (conservative, >=1h) window. */
uint64_t uk868_budget_used_us(const struct uk868_budget *b);
uint64_t uk868_budget_remaining_us(const struct uk868_budget *b);

/* Reserve BEFORE TX. Every class (new/retry/relay/duplicate) uses this. */
int uk868_budget_reserve(struct uk868_budget *b, const struct uk868_gate *g,
			 uint32_t raw_ms, enum uk868_tx_class cls,
			 uint64_t airtime_us, struct uk868_token *tok);
/* Settle after TX attempt. actual_us used only for COMPLETED. */
int uk868_budget_finish(struct uk868_budget *b, struct uk868_token tok,
			enum uk868_outcome outcome, uint64_t actual_us);

/* ---- Guarded send: the only intended path to the radio ------------------ */
/* Radio callback. *started=true once the transmitter may have keyed.
 * Never invoked unless gate open, profile valid, and budget reserved. */
typedef int (*uk868_radio_tx_fn)(void *user, const uint8_t *data, size_t len,
				 bool *started, uint64_t *actual_us);

int uk868_guarded_send(struct uk868_budget *b, const struct uk868_gate *g,
		       const struct uk868_profile *p, uint32_t raw_ms,
		       enum uk868_tx_class cls, const uint8_t *data, size_t len,
		       uk868_radio_tx_fn tx, void *user);

#ifdef __cplusplus
}
#endif

#endif /* UK_LORA_REGULATORY_H_ */
