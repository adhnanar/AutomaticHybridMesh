/*
 * Frurt UK SX1262 transmitter PRE-FLIGHT / NO-TRANSMIT image.
 * 2026-10-19. Gate is compile-locked, no LoRa send API calls exist here.
 * Use ONLY for isolated inspection of radio readiness and UK868 policy.
 */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "uk_lora_regulatory.h"

#define SX1262_NODE DT_NODELABEL(sx1262)
BUILD_ASSERT(DT_NODE_EXISTS(SX1262_NODE), "SX1262 overlay required");
BUILD_ASSERT(DT_NODE_HAS_STATUS(SX1262_NODE, okay), "SX1262 must be enabled");
BUILD_ASSERT(UK868_ALLOW_GATE_OPEN == 0, "Transmission MUST remain compile-locked");
static const struct device *const radio = DEVICE_DT_GET(SX1262_NODE);

int main(void)
{
    struct uk868_profile profile;
    struct uk868_budget budget;
    struct uk868_gate gate = {0};
    struct uk868_authorization authorization = {
        .operator_confirmed_uk = true,
        .erp_assessment_documented = false,
        .equipment_compliance_reviewed = false,
        .worst_case_erp_mdbm = 0,
    };
    struct uk868_token tok = {0};

    uk868_profile_default(&profile);
    uk868_budget_init_cold_boot(&budget);
    const int gate_rc = uk868_gate_open(&gate, &authorization);
    const uint64_t airtime_us = uk868_airtime_us(&profile, 10);
    const int reserve_rc = uk868_budget_reserve(&budget, &gate, k_uptime_get_32(),
                                               UK868_TX_NEW, airtime_us, &tok);

    printk("FRURT_TASK023_UK_TX_LOCKED_PREFLIGHT_V1\n");
    printk("TASK023_RADIO_READY=%u\n", device_is_ready(radio) ? 1U : 0U);
    printk("TASK023_PROFILE_RC=%d\n", uk868_profile_validate(&profile));
    printk("TASK023_REGION_FREQ_HZ=%u BW_HZ=%u SF=%u\n",
           profile.center_hz, profile.bandwidth_hz, profile.sf);
    printk("TASK023_TEN_BYTE_ESTIMATED_AIRTIME_US=%llu\n",
           (unsigned long long)airtime_us);
    printk("TASK023_TX_GATE_OPEN=%u GATE_RC=%d RESERVE_RC=%d\n",
           uk868_gate_is_open(&gate) ? 1U : 0U, gate_rc, reserve_rc);
    printk("TASK023_ACTUAL_LORA_SEND_CALLS=0\n");
    if (uk868_gate_is_open(&gate) || reserve_rc == UK868_OK) {
        printk("TASK023_RESULT=FAIL_UNEXPECTED_TX_AUTH\n");
        return -1;
    }
    printk("TASK023_RESULT=TX_LOCKED_PRE_FLIGHT_ONLY\n");
    for (;;) {
        k_sleep(K_SECONDS(30));
    }
}
