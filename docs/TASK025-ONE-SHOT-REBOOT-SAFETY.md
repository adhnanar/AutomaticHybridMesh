# TASK025 — Reboot-safe one-shot TX authorization (PC-only)

Date: 2026-10-19. **Status: design + pure-C implementation + tests WRITTEN; NOT compiled, NOT executed, NOT built in this session (see Blockers). TX allow path remains HARD-DISABLED.** No flash, reset, USB, power or RF action was taken.

## Problem
Task024 `frurt_os_ctx.consumed` and `uk868_budget` are RAM-only, so an allowed image could re-arm after every MCU reset/crash and transmit repeatedly, exceeding the duty cycle.

## Design (fail-closed)
New module [uk_lora_one_shot_store.h](../src/features/uk_lora_one_shot/uk_lora_one_shot_store.h) / [.c](../src/features/uk_lora_one_shot/uk_lora_one_shot_store.c): pure C, storage behind `read`/`write` ops.

* 24-byte record: magic, version, external arm token, CRC32 (bytes 0..11), **consume word** (erased `0xFFFFFFFF` = armed), spare.
* **Firmware can never arm.** Arming = external provisioning (erase + program 16-byte image from `frurt_os_store_arm_image(token)`). Blank / bad magic / bad version / bad CRC / unreadable / consumed => **DENY**. Init never writes.
* Firmware performs exactly **one 32-bit program** of the consume word and **never erases** -> no wear, no erase-interrupted state. NOR programming only clears bits, so a torn/interrupted write yields a value != `0xFFFFFFFF`, which reads as CONSUMED (never ARMED).
* `frurt_os_store_consume()` succeeds only after exact read-back of the marker. `frurt_os_fire()` order: gate/evidence checks -> store must read ARMED -> RF cfg/airtime checks -> **durable consume** -> `uk868_guarded_send` (radio). Any write/verify failure returns `FRURT_OS_ERR_STORE` with **no radio call**.
* Consumed stays consumed if the radio fails, airtime is unknown or the MCU resets after commit and before TX (accepted trade: possible lost shot, never a second shot). A failed-with-no-change write leaves ARMED, which is safe because nothing was transmitted.
* `frurt_os_attach_store()` is the only binding; no store attached => `FRURT_OS_ERR_NOT_ARMED`.
* Compile-time locked default kept. Additionally: `FRURT_UK_ONE_SHOT_TX_ALLOW=1` now needs `FRURT_UK_ONE_SHOT_STORE_ACCEPTED=1` (default 0, `#error` otherwise) and the production CMake allow option is `FATAL_ERROR`. Only the host test CMake / host runner define STORE_ACCEPTED (simulated flash).

## Files changed / added
Added: `src/features/uk_lora_one_shot/uk_lora_one_shot_store.{h,c}`, `tests/uk_lora_one_shot/src/{sim_flash.h,store_tests.c,reboot_tests.c}`, this doc.
Modified: `uk_lora_one_shot.{h,c}` (store pointer, `ERR_NOT_ARMED=-11`, `ERR_STORE=-12`, attach, gate), `tests/uk_lora_one_shot/{CMakeLists.txt,src/main.c,host/run_host_tests.py}`, `tests/sx1262_uk_one_shot/CMakeLists.txt` (store.c; allow FATAL_ERROR), `tests/task024_static_tx_check.py` + its unittest (store-gate checks and a tamper test), `docs/PROGRESS.md`.
Not touched: BLE/Task012 sources, overlays, any UF2.

## Regression tests written (C, simulated NOR flash with fault injection)
store_tests: blank denied/no write; armed->consume->reboot stays consumed; arm-image params + CRC32 vector; every header byte corruption denied; unknown version denied; read error fail-closed; write failure no-change; torn write (error and silent) never ARMED; stuck write fails verify; every single-bit consume pattern = consumed.
reboot_tests (allow/host): commit durable before radio callback; no duplicate TX after restart; unknown airtime / radio error still consumed after reboot; crash after commit before radio no rearm; missing/blank/unattached/unreadable store denies; write failure => no TX; power-cut sweep over 4 fault modes x 10 boots => <=1 TX and zero erases; 50-reset loop => 1 TX, 1 write. Locked build: armed store, zero writes, zero radio calls.

## Commands and ACTUAL results
| Command | Result |
|---|---|
| `python tests\task024_static_tx_check.py` | `TASK024_STATIC_TX_CHECK=PASS` |
| `python -m unittest discover -s tests -p "test_*.py"` | **45/45 OK** (44 previous + 1 new tamper test) |
| `python -m unittest tests.hybrid_router_sim.test_hybrid_router_ref` | **13/13 OK** |
| `west build -b xiao_ble/nrf52840 -d build-task025-locked tests\sx1262_uk_one_shot ...` | **NOT RUN SUCCESSFULLY**: sandbox denied executing `C:\zephyrproject\.venv\Scripts\west.exe` ("not recognized" though the file exists). No new image/hash exists. Task024 locked UF2 hash `D2EC03A6...9803439` was built *before* these edits and is now stale. |
| Host C compile/run (`tests/uk_lora_one_shot/host/run_host_tests.py`) | **NOT RUN**: no gcc/clang/cl/zig/tcc on PATH, `ziglang` module not installed, Zephyr SDK `gnu/` directory is sandbox-denied (so even the ARM cross compiler and `gdb` simulator could not be probed), QEMU `qemu-system-arm.exe` not executable (previously 0xC0000135). |

**No C test pass is claimed.** The C code was only desk-checked (struct field order, macro shim ordering, torn-write arithmetic, single-bit corruption test uses XOR so it always changes the byte).

## Non-invasive runtime options discovered (none installed/changed)
1. `python -m ziglang cc` (existing host runner) — needs `pip install ziglang` (download; needs approval).
2. Zephyr SDK `arm-zephyr-eabi-gcc` can cross-build the ztest app, but SDK dir is sandbox-blocked here; running needs QEMU (missing DLL) — or arm-zephyr-eabi-gdb `target sim` (unverified).
3. Fixing QEMU: identify missing DLL (e.g. MSYS2 runtime libs) — needs user approval.

## Acceptance gate before ANY TX enable (open)
1. Real flash adapter (Zephyr `flash_area`/direct nRF52840 NVMC) with: word-granular program, no hidden erase, committed-before-return semantics (NVMC READY wait), page reserved outside app/settings/bootloader and surviving DFU/UF2 reflash (**UF2 reflash of the app must not erase the page, or provisioning must be re-done deliberately**), UICR/ReadBack-protection decision.
2. Hardware evidence that an interrupted word program cannot read back as `0xFFFFFFFF` after power returns (nRF52 datasheet NVMC write-cycle + power-fail behaviour; consider writing consume word twice/second flag word for margin).
3. Provisioning procedure + tool for the arm token (one token per authorized shot, recorded in docs), plus a design for erased-page detection after full chip erase (blank => deny, already so).
4. C tests compiled and executed (host or QEMU) with 0 failures; locked XIAO image rebuilt and hash recorded.
5. Antenna gain / ERP / equipment compliance evidence (unchanged Task024 gate) and explicit user flash approval.
6. Independent review. Then only may `FRURT_UK_ONE_SHOT_STORE_ACCEPTED` be set by a reviewed adapter build rule.

## Issues / residual risks
* Duty-cycle budget is still RAM-only; protection is "at most one TX per provisioning token", not a persistent airtime counter. Repeated provisioning needs an external airtime ledger.
* Durable consume precedes the regulatory budget check, so a budget refusal also burns the shot (conservative).
* Task024's allow compile-check build no longer compiles by design.

## Rollback
Revert the files listed above (new files can simply be deleted); Task024 behaviour returns. No device state changed.

## Next action
**WAITING FOR: independent review, plus user decision to (a) lift the sandbox block on `C:\zephyrproject\.venv\Scripts\west.exe` / `C:\Users\adhna\zephyr-sdk-1.0.1\gnu` so the locked image can be rebuilt and (b) approve installing a host C compiler (e.g. `pip install ziglang`) or fixing QEMU DLLs so the C tests can run.**
