# TASK024 — UK SX1262 one-shot TX: safety implementation and review

Date: 2026-10-19. **Result: PC-ONLY implementation complete; locked image builds; policy tests compile in both configs but were NOT executed (no runnable host/QEMU here). NO RF emission, NO flash, NO reset.** Authored by the assistant (Copilot SDK in VS Code), not a separate Copilot Agent task; an independent reviewer should still rerun everything per Rule 10.

## Scope
First single-packet TX role, feature-isolated, **compile-time locked, default OFF**. Does not touch the flashed devices (Kit1 COM14 RX 868.3 MHz/125 kHz/SF9/CR4-5/pre8/private; Kit2 COM15, Kit3 COM16 BLE), the root BLE app, or any legacy project. Separate from the Task023 locked preflight (`tests/sx1262_uk_tx_locked`, untouched).

## Files
New:
- `src/features/uk_lora_one_shot/uk_lora_one_shot.{h,c}` — pure C policy (no radio/Zephyr dependency).
- `tests/sx1262_uk_one_shot/` — Zephyr app (`CMakeLists.txt`, `prj.conf`, `src/main.c`, `src/radio_tx.{h,c}`).
- `tests/uk_lora_one_shot/` — ztest (`src/main.c`, `CMakeLists.txt`, `prj.conf`, `testcase.yaml`) plus `host/` shim runner (`ztest.h`, `runner.c`, `run_host_tests.py`).
- `tests/task024_airtime_check.py` (independent airtime), `tests/task024_static_tx_check.py` (static no-TX proof), `tests/test_task024_static_tx_check.py`.
- Already present from earlier in the session: `tests/task024_erp_margin.py` and `tests/test_task024_erp_margin.py` (unchanged).
- Logs: `docs/task024-one-shot-locked-build.log`, `docs/task024-allow-compilecheck-build.log`, `docs/task024-ztest-{locked,allow}-build.log`.
- This doc, `docs/copilot/024-one-shot-tx-review.prompt.md`, `docs/PROGRESS.md` entry.
Existing `uk_lora_regulatory` is reused unmodified.

## Safety design (defence in depth)
1. **Two compile-time locks, forced equal** (`#error` otherwise): `FRURT_UK_ONE_SHOT_TX_ALLOW` and `UK868_ALLOW_GATE_OPEN`, both default 0. CMake option `FRURT_UK_ONE_SHOT_TX_ALLOW` is `OFF` by default and sets both.
2. **Only one file calls `lora_send`**: `tests/sx1262_uk_one_shot/src/radio_tx.c`, added to the target *only* inside the allow block and `#error`s if compiled locked. Locked `main.c` uses a stub callback and references no `lora_*` API. `tests/task024_static_tx_check.py` enforces this.
3. **Not bypassable via direct `lora_send`** in the shipped app: the only call site is the callback invoked by `uk868_guarded_send()` (gate → profile validate → airtime reserve → callback). Caveat: any developer who adds a new source file calling `lora_send` is outside software control; the static check guards the scanned directories only.
4. **Runtime evidence gate** even in an allow build: UK operator confirmed, ERP assessment documented, equipment reviewed, **antenna gain verified** flag, non-negative loss/tolerance, worst-case ERP ≤ 13.979 − 2.0 dBm. All `-D` evidence macros default to 0 (denied); the supplied-antenna true gain is unverified so the allow image is still denied today.
5. **Config must equal RX exactly** (868.300 MHz, 125 kHz, SF9, CR4/5, preamble 8, private sync, IQ normal, CRC on, explicit header); rechecked at fire time (tamper test).
6. **One shot latch** `consumed` set before any radio call and never cleared; a radio error does not re-arm. No retries, no loop, no continuous TX. Fixed 10-byte payload `FRURT-OS01`.
7. **Airtime reserved BEFORE send** (verified by a test that reads budget inside the callback). The real callback sets `started=true` on entry and `actual_us=0`, so any failure after entry (including `-ENODEV`, `lora_config` error, `lora_send` error) keeps the **full reservation charged** (uncertain ⇒ counted). Refund only if a callback reports `started=false` (the real one never does).
8. Allow image waits 30 s (serial marker `TASK024_TX_IN_30S_UNPLUG_TO_ABORT`) before the single send.

## Airtime (independent)
`tests/task024_airtime_check.py` (float, Semtech formula) vs. C integer model: SF9/125 kHz/CR4-5/pre8/explicit/CRC, 10 B → raw **144.384 ms**, +5 % margin = **151,604 µs**; C returns the same value (asserted in the ztest and printed by the locked app as `TASK024_AIRTIME_US`). That is 0.42 % of the 1 % hourly allowance (36 s); the module budget is 30 s/h and the single-shot cap 250 ms. Low-data-rate optimisation is off (Tsym 4.096 ms < 16 ms).

## Worst-case ERP and power uncertainty
Limit 25 mW = 13.979 dBm ERP; module requires ≥ 2 dB margin (≤ 11.979 dBm). Worst case = nominal + tolerance(default assumed 3 dB) + gain − 2.15 dB − feed loss. For −9 dBm: gain 2 dBi → −6.15 dBm; 10 dBi → 1.85; 20 dBi → 11.85 (2.13 dB margin); the gate would deny above ~20.1 dBi. **−9 dBm is only a provisional candidate, not a permit.** Unverified: true included-antenna gain/efficiency; SX1262 PA/driver output at −9 dBm and its tolerance (the 3 dB tolerance is an assumption, not a datasheet value); RF-switch/matching loss on this kit (assumed 0, which is the pessimistic direction); harmonics/spurious emissions, which ERP-of-carrier arithmetic does not cover. Also unassessed: the Zephyr driver's actual mapping of `tx_power`, and LBT/duty-cycle regime choice (the ≤1 % duty alternative is assumed; this module does not implement listen-before-talk).
**Reboot caveat:** volatile budget; the context uses a fresh (not cold-boot-precharged) budget, because a precharge would forbid even one packet. A reset/crash loop of an allow image could therefore send once per boot. Mitigation: flash only the one-shot image once, then reflash a locked/RX image; never leave an allow image resident.

## Commands and actual results
| Command | Result |
|---|---|
| `west build -b xiao_ble/nrf52840 -d build-task024-one-shot-locked tests\sx1262_uk_one_shot --pristine=always -- -DDTC_OVERLAY_FILE=<rf_frontend overlay>` | **PASS 203/203**, FLASH 56,836 B, RAM 16,888 B, UF2 114,176 B, SHA256 `D2EC03A68E6BB860BC2BDBB22817BD59EFB28D27C46F7DEA78112549F9803439`, no warnings. First attempt failed (my `#endif` typo, then fixed; an earlier PowerShell argument-splitting error also fixed by quoting). **Not flashed.** |
| `arm-zephyr-eabi-nm` on locked ELF | No `frurt_os_radio_tx`; no `lora_config`/`lora_send` undefined reference in any app object. `sx126x_lora_send` exists in the image only as the driver's device-API table entry (unavoidable, never called by app code). |
| `python tests\task024_static_tx_check.py --build build-task024-one-shot-locked` | **PASS** (source + artefact: no `radio_tx` in map/ninja, cache OFF). |
| Same checker against a temporary allow build | **FAIL as intended** (negative control); that allow build compiled (FLASH 58,984 B, no warnings) then the **build directory was deleted** so no TX-capable artefact remains. |
| `west build -b qemu_cortex_m3 tests\uk_lora_one_shot` locked and `-DONE_SHOT_TEST_ALLOW=ON` | Both **compile/link, 0 warnings**. |
| `west build -t run` (QEMU) | **NOT EXECUTED**: `qemu-system-arm.exe` exits 0xC0000135 (missing DLL) in this environment. So the ztest results are **unproven**. |
| Host harness (`run_host_tests.py`, zig cc) | **NOT EXECUTED**: zig cannot create its cache dir in this sandbox (AccessDenied); `ziglang` was uninstalled again. Only `arm-gcc -fsyntax-only -Wall -Wextra -Werror` over the test + module sources for both ALLOW=0/1 passed. Harness accepts `TASK024_HOST_CC` for another host compiler. |
| `python -m unittest tests.test_task024_static_tx_check tests.test_task024_erp_margin` | **13 tests OK** |

## Test coverage written (unexecuted C; names in `tests/uk_lora_one_shot/src/main.c`)
RX-config equality and airtime = 151,604 µs; ERP arithmetic; default authorisation denied; tampered gate cannot reach `uk868_guarded_send`; locked build denies even with full evidence and `g.open=true`; allow: reserve-before-send, one shot, radio error stays charged, not-started refund, unverified-gain/missing-evidence denial, excess ERP, power above ceiling, config mismatch (also tampered after init), budget exhausted ⇒ radio not called.

## Issues / risks
- C-level tests not executed (see above) — the main open acceptance item.
- ERP inputs are assumptions; no regulatory sign-off. Software gate ≠ compliance.
- Timing of single `lora_send` blocking behaviour and Zephyr driver power mapping unverified on hardware.
- `docs/PROGRESS.md` is untracked in a repo with no commits; nothing was staged or committed.

## Rollback
Delete `src/features/uk_lora_one_shot/`, `tests/sx1262_uk_one_shot/`, `tests/uk_lora_one_shot/`, `tests/task024_*`, `tests/test_task024_static_tx_check.py`, `build-task024-*`, `docs/task024-*`, this doc and the PROGRESS entry. No existing source was modified.

## Review verdict (assistant self-review, pending independent review)
Design and locked build: acceptable. Not acceptable to advance until the independent reviewer (a) runs the ztests on a working QEMU/host, (b) re-hashes the locked build, (c) confirms no allow artefact exists. **Blockers before any TX: verified antenna gain/ERP and equipment assessment, plus explicit separate user approval for Kit2 flash and RF emission.**

## Next action
Independent review/test execution; then a documented antenna/power assessment. **WAITING FOR:** nothing physical now; any flash or RF requires explicit user authorisation.
