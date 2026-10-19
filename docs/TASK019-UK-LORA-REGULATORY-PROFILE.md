# TASK019 — UK 868.3 MHz LoRa regulatory profile (PC-only, opt-in)

**Status: IMPLEMENTED BUT NOT BUILT / NOT EXECUTED.** The sandbox blocked every available C toolchain and the Zephyr/west environment (see *Issues*). No test result is claimed. Independent review required before anything else.

> **Software enforcement is not a regulatory certification.** Nothing here authorises RF transmission. No LoRa is enabled by default; no radio code is linked into any existing firmware.

## Scope
- Feature-scoped constants/data structures for a **proposed** UK 868.3 MHz / 125 kHz profile.
- TX gate, default **denied** (zero-initialised struct = closed), and compile-time locked: `UK868_ALLOW_GATE_OPEN` defaults to `0`, so in a normal build `uk868_gate_open()` always fails. Only the test build defines it to `1`.
- Monotonic per-node rolling-hour airtime budget: 30 s/h (0.833 %), below the 1 % ceiling. Every class (new BLE→LoRa, retry, relay, duplicate forward, other) draws from the same budget.
- Reserve-before-TX, then settle: not started → refund; started/unknown → keep full reservation; completed → true-up (refund difference or charge overrun).
- Channel constraint: whole occupied channel (centre ± BW/2) must sit inside 868.0–868.6 MHz **with a further 25 kHz guard** at each edge; BW limited to 125/250/500 kHz; SF 7–12.
- Guarded send wrapper: the radio callback is invoked only after gate open + valid in-band profile + successful reservation.
- Out of scope / untouched: root CMake, GATT, existing firmware, flashing, RF, hardware, mobile app.

## Files
- [src/features/uk_lora_regulatory/uk_lora_regulatory.h](../src/features/uk_lora_regulatory/uk_lora_regulatory.h)
- [src/features/uk_lora_regulatory/uk_lora_regulatory.c](../src/features/uk_lora_regulatory/uk_lora_regulatory.c)
- [tests/uk_lora_regulatory/](../tests/uk_lora_regulatory) (`CMakeLists.txt`, `prj.conf`, `testcase.yaml`, `src/main.c`, Zephyr ztest). `-DUK868_TEST_LOCKED=ON` builds the default-locked variant.
- [docs/copilot/019-uk-lora-regulatory-review.prompt.md](copilot/019-uk-lora-regulatory-review.prompt.md)
- This file and [PROGRESS.md](PROGRESS.md).

Tests written (not run): default profile in band; band-edge rejection and no mutation on failure; 5 kHz sweep never crossing the band; airtime vs Semtech reference vectors (SF7 10 B = 41.216 ms, SF12 10 B = 991.232 ms before the 5 % margin) and monotonicity; ERP helper and gate authorisation rejection; gate default denied → radio never called; shared budget across all classes; failed-send accounting; pending-slot exhaustion; rolling-hour expiry (never forgets before 1 h); 32-bit tick wraparound and backwards-clock fail-closed; saturation; counter monotonicity; cold-boot pre-charge; guarded-send paths; locked build cannot open gate.

## Regulatory evidence (what was actually read)
Source: Ofcom IR 2030 PDF (downloaded 2026-10-19, 128 pages), text extracted with pypdf.
- **IR2030/1/16**, non-specific SRD, "Equipment may be used airborne": **868.0 – 868.6 MHz, 25 mW e.r.p.**, "techniques to access spectrum and mitigate interference that provide at least equivalent performance to the techniques described in designated standards specified in the Notices of publication (See Section 4) must be used" — informative reference EN 300 220. Located on PDF pp. 20–21 (document pages 11–12).
- The row is split across pages. The wording "Alternatively, a duty cycle limit of ≤ 1% applies." appears at the top of PDF p. 21, directly before the IR2030/1/17 row, and I attribute it to 1/16. The "0.1 %" wording at the top of p. 20 belongs to the preceding row IR2030/1/15 (868–869.7 MHz analogue audio). **This attribution is my inference from text-extraction order; a human must confirm it against the visual table.**
- Ofcom 2026 update: Ofcom page "Notice of proposals to make Wireless Telegraphy (Exemption) (Amendment) Regulations 2025", *statement published 15 April 2026*, is a **proposal/consultation notice**. I did not find, and did not verify, any change to the 868.0–868.6 MHz entry, and it is not confirmed as in force. Re-check the current Exemption Regulations and IR 2030 revision before any transmission.

## Uncertainties (explicit)
1. **Airtime model**: Semtech SX126x LoRa formula in integers, rounded up, +5 % margin. Not validated against a real radio/analyser; implicit-header, SF5/6, FLRC/FSK excluded. Actual chip timing (ramp-up, CAD, LBT listen time) is not modelled.
2. **Channel mask**: ±BW/2 plus 25 kHz guard is an engineering assumption. Real occupied bandwidth, emission mask and crystal/TCXO frequency tolerance are unmeasured. 868.3 MHz is only a proposed centre.
3. **ERP / antenna gain**: limit is 25 mW **ERP**. `uk868_erp_mdbm()` = conducted + gain(dBi) − 2.15 dB − losses is a helper, not proof; driver dBm is **not** ERP. The gate demands a documented worst-case ERP ≥ 2 dB below 13.979 dBm, but that number is supplied by the operator, unchecked.
4. **Transmit power**: no power setting exists in this module; it must be chosen from an antenna/module assessment.
5. **Equipment compliance**: radio equipment conformity (module/antenna approvals, EN 300 220 access techniques / LBT-AFA if relying on the non-duty-cycle route) is not addressed. Using the ≤ 1 % duty-cycle alternative still requires meeting the other IR 2030 conditions.
6. **Budget is per node and volatile**: reboot loses history (mitigated by `uk868_budget_init_cold_boot`, which pre-charges the whole hour). The 1 % may be assessed over a different period or per device/network in practice; 30 s/h is conservative only under the per-device reading.
7. Refunds for "not started" assume the caller reports honestly whether the radio keyed.

## Commands executed and actual results
| Command | Result |
|---|---|
| `curl` Ir2030 PDF + `pypdf` text extraction | OK; clauses above |
| `west build -b qemu_cortex_m3 ... tests\uk_lora_regulatory` | **Not run**: `C:\zephyrproject\.venv` python/west not executable in sandbox; `.west\config` access denied |
| direct `cmake -G Ninja ... -DBOARD=qemu_cortex_m3` | **Failed**: Zephyr package not found (zephyr tree not readable by cmake under sandbox) |
| `arm-zephyr-eabi-gcc -fsyntax-only` | **Not executable** in sandbox |
| `zig cc` (pip `ziglang`, copied into workspace) | **Failed** ("Unexpected" writing cache; Permission denied in temp) |
| Host `gcc/clang/cl` | Not installed |
**Therefore: compile status unknown, test status unknown.** The code has not even had a syntax check; expect possible trivial errors.

## Issues
- Sandbox policy prevents building/running tests. Leftover, undeletable (access denied) ignorable scratch directories: `build-task019-tools/` (zig + copied site-packages, ~0.7 GB) and `build-task019-uk868/`. Please delete them manually.
- Needed from user: allow execution of `C:\zephyrproject\.venv\Scripts\*`, read of `C:\zephyrproject\.west`, and `C:\Users\adhna\zephyr-sdk-1.0.1\` tools; or provide a host C compiler.

## Rollback
Delete `src/features/uk_lora_regulatory/`, `tests/uk_lora_regulatory/`, this file, the prompt file, and the Task019 line in PROGRESS.md. Nothing else was modified.

## Next action
1. Fix sandbox/toolchain access, then run:
   `west build -b qemu_cortex_m3 -d build-task019-uk868 tests\uk_lora_regulatory -p always` and `ninja -C build-task019-uk868 run`; repeat with `-DUK868_TEST_LOCKED=ON` (or `native_sim`).
2. Independent review (use the prompt in `docs/copilot/`), including human confirmation of the IR 2030 table row reading.
