/*
 * TASK024 Frurt UK SX1262 ONE-SHOT TX app. DEFAULT BUILD IS LOCKED:
 * no lora_* API is referenced, the radio callback is a stub that is never called.
 * Do not build with -DFRURT_UK_ONE_SHOT_TX_ALLOW=ON or flash without separate
 * antenna/power assessment and explicit user approval.
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "uk_lora_one_shot.h"

#if FRURT_UK_ONE_SHOT_TX_ALLOW
#include "radio_tx.h"
#endif

#define RADIO_NODE DT_NODELABEL(sx1262)
BUILD_ASSERT(DT_NODE_EXISTS(RADIO_NODE), "SX1262 overlay required");
BUILD_ASSERT(DT_NODE_HAS_STATUS(RADIO_NODE, okay), "SX1262 must be enabled");

#ifndef FRURT_OS_AUTHZ_GAIN_MDBI
#define FRURT_OS_AUTHZ_GAIN_MDBI 0
#endif
#ifndef FRURT_OS_AUTHZ_LOSS_MDB
#define FRURT_OS_AUTHZ_LOSS_MDB 0
#endif
#ifndef FRURT_OS_AUTHZ_TOL_MDB
#define FRURT_OS_AUTHZ_TOL_MDB FRURT_OS_DEFAULT_POWER_TOL_MDB
#endif
/* Evidence flags default to 0 (denied); only explicit -D overrides can change them. */
#ifndef FRURT_OS_AUTHZ_OPERATOR_UK
#define FRURT_OS_AUTHZ_OPERATOR_UK 0
#endif
#ifndef FRURT_OS_AUTHZ_ERP_DOC
#define FRURT_OS_AUTHZ_ERP_DOC 0
#endif
#ifndef FRURT_OS_AUTHZ_EQUIP_REVIEWED
#define FRURT_OS_AUTHZ_EQUIP_REVIEWED 0
#endif
#ifndef FRURT_OS_AUTHZ_GAIN_VERIFIED
#define FRURT_OS_AUTHZ_GAIN_VERIFIED 0
#endif

#if !FRURT_UK_ONE_SHOT_TX_ALLOW
static int locked_stub_tx(void *user, const uint8_t *d, size_t n, bool *started,
			  uint64_t *actual)
{
	(void)user; (void)d; (void)n;
	*started = false;
	*actual = 0;
	return -1;
}
#define TX_FN locked_stub_tx
#else
#define TX_FN frurt_os_radio_tx
#endif

static struct frurt_os_ctx ctx;

int main(void)
{
	struct frurt_os_rf_cfg cfg;
	const struct frurt_os_authz authz = {
		.operator_confirmed_uk = FRURT_OS_AUTHZ_OPERATOR_UK,
		.erp_assessment_documented = FRURT_OS_AUTHZ_ERP_DOC,
		.equipment_compliance_reviewed = FRURT_OS_AUTHZ_EQUIP_REVIEWED,
		.antenna_gain_verified = FRURT_OS_AUTHZ_GAIN_VERIFIED,
		.antenna_gain_mdbi = FRURT_OS_AUTHZ_GAIN_MDBI,
		.feed_loss_mdb = FRURT_OS_AUTHZ_LOSS_MDB,
		.power_tolerance_mdb = FRURT_OS_AUTHZ_TOL_MDB,
	};

	frurt_os_rf_cfg_default(&cfg);
	int init_rc = frurt_os_init(&ctx, &cfg, &authz);

	printk("FRURT_TASK024_UK_ONE_SHOT_V1\n");
	printk("TASK024_BUILD_TX_ALLOW=%d\n", FRURT_UK_ONE_SHOT_TX_ALLOW);
	printk("TASK024_RF FREQ_HZ=%u BW_HZ=%u SF=%u CR=%u PRE=%u PWR_DBM=%d (provisional)\n",
	       cfg.frequency_hz, cfg.bandwidth_hz, cfg.sf, cfg.cr, cfg.preamble_symbols,
	       cfg.tx_power_dbm);
	printk("TASK024_AIRTIME_US=%llu\n", (unsigned long long)frurt_os_airtime_us(&cfg));
	printk("TASK024_INIT_RC=%d WORST_ERP_MDBM=%d\n", init_rc, ctx.worst_case_erp_mdbm);

#if FRURT_UK_ONE_SHOT_TX_ALLOW
	if (init_rc == FRURT_OS_SENT_OK) {
		printk("TASK024_TX_IN_30S_UNPLUG_TO_ABORT\n");
		k_sleep(K_SECONDS(30));
	}
#endif
	int fire_rc = frurt_os_fire(&ctx, k_uptime_get_32(), TX_FN, NULL);

	printk("TASK024_FIRE_RC=%d TX_CALLBACKS=%u\n", fire_rc, ctx.tx_callbacks);
#if !FRURT_UK_ONE_SHOT_TX_ALLOW
	if (ctx.tx_callbacks != 0 || fire_rc != FRURT_OS_ERR_LOCKED) {
		printk("TASK024_RESULT=FAIL_LOCKED_BUT_TX_PATH_REACHED\n");
		return -1;
	}
	printk("TASK024_RESULT=LOCKED_NO_TX\n");
#else
	printk("TASK024_RESULT=ONE_SHOT_DONE_RC_%d\n", fire_rc);
#endif
	for (;;) {
		k_sleep(K_SECONDS(30));
	}
}
