# Task020 independent PC-only review — UK868 policy hardening

2026-10-19. **PASS: C source review and three compile/link targets. Runtime C tests and RF compliance remain OPEN.**

Copilot completed feature-scoped hardening of `src/features/uk_lora_regulatory/uk_lora_regulatory.c/.h` and `tests/uk_lora_regulatory/src/main.c` without changing the root BLE application or physically touching the radios.

- Clock faults are now **latched**: backwards or ambiguous 32-bit millisecond updates set `clock_fault`; subsequent calls refuse to advance, even after a resumed forward timestamp. Recovery requires constructing a new budget and cold-boot precharging a full window. Callers must not invoke the non-precharged `uk868_budget_init` to reset a live budget.
- `uk868_budget_finish` validates outcome before freeing its pending token, so an invalid enum cannot discard a live reservation.
- Pending tokens skip IDs already in use after 32-bit wrap.
- ERP arithmetic uses 64-bit intermediates with a saturated 32-bit return value, rather than undefined signed overflow.
- New C ztests cover the above, plus pre-TX reservation, overrun accounting, and a default-lock bypass test. Locked builds unconditionally reject transmission even if a caller writes `g.open=true`.
- Airtime overrun remains a safety limitation: post-transmission settlement cannot retroactively prevent an over-budget frame. For real RF the radio callback must use validated worst-case airtime bounds, serialized budget operations, and enforce maximum TX duration. This mock code does not provide regulatory certification.

## Independent builds on Abi
1. **XIAO locked** test application `build-task019-independent-locked`: incremental rebuild **PASS exit0**, 8/8 changed steps; log `docs/task020-independent-locked-build.log`.
2. **XIAO mock-gate test** `build-task019-independent-mock`: incremental rebuild **PASS exit0**, 8/8 changed steps; log `docs/task020-independent-mock-build.log`.
3. **QEMU Cortex-M3** test application `build-task020-independent-qemu`: independent **pristine 140/140 build/link PASS**, exit0; log `docs/task020-independent-qemu-build.log`.
4. Existing Python unittests **31/31 PASS**, hybrid Python reference model **13/13 PASS**. These do not execute the UK C ztest.
5. QEMU runtime **not executed successfully**: previous equivalent task019 QEMU launch on Abi failed with Windows `0xc0000135` missing binary runtime dependency, documented `docs/task019-qemu-runtime.log`. No runtime C test pass is claimed. The compile/link results validate syntactic integration only.

## Next hardware/software gates
- Keep source separate from root BLE firmware; the UK TX gate remains denied in default configuration.
- Resolve QEMU host runtime dependency or use a separate validated host C-test environment before declaring C ztests passed.
- Verify current Ofcom UK IR2030/1/16 page-spanning alternative duty-cycle statement visually and full conformity; assess Wio-SX1262 antenna ERP and RF switch/TCXO pins.
- Obtain explicit approval before any test firmware flash. Do not transmit LoRa until hardware and RF/legal power/airtime constraints verified.
- No flash/reset/power/radio RF action occurred in Task020 or independent review.
