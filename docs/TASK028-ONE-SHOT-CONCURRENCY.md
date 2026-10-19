# TASK028 — One-shot TX concurrency hardening (2026-10-19)

**Status: code and regression tests written; static/Python/syntax checks PASS; C runtime NOT executed; locked firmware rebuild NOT done (sandbox denied `west.exe`).** PC-only. No flash, reset, RF, power or hardware operation, no install/download, no PATH change. Production TX remains hard-disabled (`FRURT_UK_ONE_SHOT_TX_ALLOW=0`; allow build still `#error`s without `FRURT_UK_ONE_SHOT_STORE_ACCEPTED`; production CMake unchanged).

## Scope
Close the Task025 finding: two concurrent `frurt_os_fire()` callers could both pass the RAM `consumed` check and the flash ARMED check before either consume write, and both transmit.

## Design (fail-closed claim lock)
- New injected `struct frurt_os_lock_ops { try_acquire, release, ctx }` and `frurt_os_attach_lock()` in [uk_lora_one_shot.h](../src/features/uk_lora_one_shot/uk_lora_one_shot.h). No Zephyr or compiler-specific primitive is used by the portable core; it relies only on the integrator's lock.
- In the allow build `frurt_os_fire()` now: (1) requires a complete lock, else `FRURT_OS_ERR_NO_LOCK` (store untouched); (2) non-blocking `try_acquire`, failure → `FRURT_OS_ERR_BUSY` (shot **not** consumed, nothing touched); (3) under the lock runs the whole check → `consumed` latch → durable `frurt_os_store_consume()` (`claim_locked()`); (4) releases the lock on every path; (5) only then calls the radio, outside the lock. `consumed` is now read/written only under the lock, so no volatile/atomic/memory-model assumption is needed.
- Re-entry: from the radio callback → `ALREADY_USED` (same ctx) or `NOT_ARMED` (other ctx, same store); from a store callback → `BUSY` (lock is required to be non-recursive).
- Locked build path is unchanged: returns `FRURT_OS_ERR_LOCKED` before touching any lock or store.
- Zephyr/nRF52840 adapter foundation is implemented separately in [uk_lora_zephyr_claim_lock.c](../src/features/uk_lora_one_shot/uk_lora_zephyr_claim_lock.c) and [uk_lora_zephyr_claim_lock.h](../src/features/uk_lora_one_shot/uk_lora_zephyr_claim_lock.h). It uses Zephyr atomic compare-and-set; it is not connected to the production app or radio path. See [TASK029-ZEPHYR-CLAIM-LOCK.md](TASK029-ZEPHYR-CLAIM-LOCK.md) for compile evidence and integration constraints. **Do not use `k_mutex`** (recursive and ISR-unsafe).

## Files changed
- Modified: `src/features/uk_lora_one_shot/uk_lora_one_shot.h`, `uk_lora_one_shot.c`.
- Added: `tests/uk_lora_one_shot/src/sim_lock.h`, `tests/uk_lora_one_shot/src/concurrency_tests.c` (suite `os_concurrency`).
- Modified tests/build lists: `tests/uk_lora_one_shot/src/main.c`, `reboot_tests.c` (attach a lock), `tests/uk_lora_one_shot/CMakeLists.txt`, `host/run_host_tests.py` (add new source).
- Docs: this file, [PROGRESS.md](PROGRESS.md). Store module and production CMake untouched.

## Regression coverage added (written, NOT executed)
Missing/partial lock denies with zero store access; contended lock → BUSY and shot not burned (later fire succeeds once); a second caller injected at **every** store access (1–8) of the first yields exactly one TX, balanced acquire/release, lock free during radio, later caller NOT_ARMED; **negative control** with an ineffective lock injected at the consume write reproduces 2 TX (shows the test is sensitive and that the store alone is not atomic); re-entry from radio callback (same and different ctx) and from store callback; storage write error releases lock, no TX, same-boot retry `ALREADY_USED`, restart sends once then `NOT_ARMED`; torn-silent write with lock ≤1 TX; locked-build test that no lock/store access occurs.

## Commands executed and actual results
- `python tests\task024_static_tx_check.py` → `TASK024_STATIC_TX_CHECK=PASS`.
- `python -m unittest discover -s tests -p "test_*.py"` → 45 tests OK.
- Direct `arm-zephyr-eabi-gcc.exe -fsyntax-only -std=c11 -Wall -Wextra -Werror` on all 8 translation units (host shim include path), for ALLOW=0 and ALLOW=1 → all rc=0 (syntax/type check only; **no object code linked, no test run**).
- `west build -b xiao_ble/nrf52840 -d build-task028-locked tests/sx1262_uk_one_shot ...` → **NOT RUN**: `C:\zephyrproject\.venv\Scripts\west.exe` reported "not recognized" (sandbox block, as in Task025). No new firmware image/hash exists; the Task025 locked UF2 hash is stale w.r.t. these source edits. Not retried via other routes.

## Correctness limits
- Tests are single-threaded deterministic interleavings; they do not prove behaviour under real preemption, multi-core or ISR contexts. Correctness depends on the integrator's lock meeting the documented contract; a broken lock reintroduces the race (negative control).
- Lock only serializes callers sharing the lock; a second MCU/core or external writer to the flash is out of scope. Flash write semantics/real adapter and antenna/regulatory evidence remain open gates from Task025.
- A BUSY loser is not retried automatically. Unlocking is assumed to succeed.
- C runtime (QEMU 0xC0000135, no native compiler) still unexecuted; no C pass is claimed.

## Rollback
Revert the edits to the two source files and the test edits (`main.c`, `reboot_tests.c`, `CMakeLists.txt`, `run_host_tests.py`); delete `sim_lock.h`, `concurrency_tests.c` and this record plus the Task028 line in PROGRESS.md.

## Next action
**WAITING FOR: independent review**, plus unblocking `west.exe` for the locked rebuild and a working C runtime (QEMU DLL fix or approved host compiler) to execute both suites.
