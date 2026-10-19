# Task019 — UK LoRa regulatory profile independent review

Date: 2026-10-19. **Verdict: PASS compile/link and isolated software foundation, NOT approved for real RF transmissions and NOT yet runtime-verified.**

## Work inspected
Copilot completed `src/features/uk_lora_regulatory/uk_lora_regulatory.h/.c`, separate Zephyr ztest `tests/uk_lora_regulatory/` and `docs/TASK019-UK-LORA-REGULATORY-PROFILE.md`. The source is separate from the existing root BLE firmware: no SX1262 driver or UK transmit gating is wired into the deployed application. Current three devices remain on validated BLE Task012 A/C and Task010 B firmware. No flash or RF TX took place.

The profile proposes center 868.300 MHz, 125 kHz BW, SF9 and 8-symbol preamble, plus a 25 kHz engineering guard from the band edges. UK transmission is **compile locked by default** (`UK868_ALLOW_GATE_OPEN=0`), requires documented authorization inputs in the mock-enabled build, limits max declared ERP conservatively relative to 25 mW ERP, and models shared LoRa airtime consumption across new packets, forwarding and retries with a 30 s/h conservative budget. Includes rolling-minute accounting, cold-start precharge, four pending reservations and a guarded radio-callback interface. None are substitutes for actual Ofcom/UKCA compliance or tested radio airtime.

## Independent code review and correction
**Found a safety defect:** despite the compile-time default lock, the original `uk868_gate_is_open()` trusted a mutable public `g->open` boolean. A caller could have set it to true directly and bypassed the lock if this code were linked to a radio callback. Fixed `uk868_gate_is_open()` so builds with `UK868_ALLOW_GATE_OPEN=0` unconditionally return false, even if the struct is modified. Extended the locked C ztest to force `g.open=true` and assert radio callback is never invoked. Rebuilt both variants afterwards.

Other production gates, not considered validated:
- Airtime estimate and +5% margin are uncalibrated; if actual airtime exceeds the reserved budget, the callback can already have radiated before the overrun is charged. Explicit tested PHY parameters, conservative upper bounds and hardware TX completion measurements are required.
- `uk868_budget_advance` sets `clock_fault` after an apparent backwards tick but does not make that flag sticky; subsequent monotonic input can resume authorizations. Add a deliberate clock-fault recovery policy and tests before live use.
- The budget uses 60-second buckets and a conservative 61-bucket live interval; safe in intent, but concurrent TX requests are not serialized/thread-safe.
- `uk868_budget_finish` removes the pending slot before validating the outcome enum. Invalid enum input can lose reservation accounting (remaining booked airtime stays charged, but tracking is lost); prevent malformed settlement in future.
- Gate authorization is a software data structure, not proof of module/equipment conformity, radiated ERP, antenna gain or license exemption. The effective channel emissions/skirt, SF/BW, RF switch and TCXO remain physical test gates.

## Independent verification
1. `west build -b xiao_ble/nrf52840 ... tests/uk_lora_regulatory` with mock TX-gate-enabled **PASS 199/199** compile/link. Initial evidence `docs/task019-independent-mock-build.log`.
2. `west build -b xiao_ble/nrf52840 ... tests/uk_lora_regulatory -- -DUK868_TEST_LOCKED=ON` locked variant **PASS 199/199** compile/link. Initial evidence `docs/task019-independent-locked-build.log`.
3. After the independent safety patch, **both XIAO variants rebuild PASS**: logs `docs/task019-review-mock-rebuild.log`, `docs/task019-review-locked-rebuild.log`, no flash.
4. Independently built `qemu_cortex_m3` ztest app **PASS 140/140** compile/link and rebuilt after patch **PASS**. Evidence `docs/task019-independent-qemu-build.log` and `docs/task019-review-qemu-rebuild.log`.
5. Tried `west build -d build-task019-independent-qemu -t run` on Abi. **QEMU runtime failed to launch**, Windows exit `0xc0000135` (missing native dependency), so **zero UK C ztest cases have been executed**, despite compiled test sources. Log `docs/task019-qemu-runtime.log`.
6. Existing repo Python unit suite **31/31 PASS** and hybrid Python reference **13/13 PASS**. Not UK C runtime substitutes.

## Regulatory evidence status
The operator explicitly confirmed **United Kingdom**. The official Ofcom IR2030/1/16 row lists non-specific SRD band 868.0–868.6 MHz with 25 mW ERP and spectrum access / interference mitigation requirements, with the alternative <=1% duty-cycle wording appearing at a page break in the extracted PDF. The page-break attribution needs visual verification in the current official document before use on-air; check latest regulations. Proposed 868.3 MHz is a software planning default only. Even with duty-cycle limits, verify antenna, effective radiated power and conformity; this report does not authorize radio operation.

## Next actions
- **No device flash**, no SX1262 RF transmission, no power/reset.
- Fix QEMU missing dependency if practical, then execute unlocked mock tests and locked bypass test.
- Before actual LoRa operation: measure/verify Wio-SX1262 RF switch / TCXO and driver readiness, assess emitted ERP (antenna gain and feeder losses), choose compliant UK profile and get explicit firmware flashing authorization. Perform A/C/B geometry-based baseline/positive LoRa acceptance then mixed BLE/LoRa tests.
- Source and test code may remain in repo as experimental but do not enable TX in default firmware. Rollback can remove just `src/features/uk_lora_regulatory` and `tests/uk_lora_regulatory` without affecting the deployed BLE build.
