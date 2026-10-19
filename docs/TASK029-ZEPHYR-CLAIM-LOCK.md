# TASK029 — Zephyr/nRF52840 one-shot claim-lock adapter (2026-10-19)

**Status: implementation and compile-only target added; validation results recorded below; STOP for independent review.** PC-only. Production radio TX remains hard-disabled and no real TX path is connected. No flash, reset, RF transmission, USB/power change, installation/download, or global PATH modification was performed.

## Scope

Provide a Zephyr lock implementation for Task028's injected `frurt_os_lock_ops` without changing the portable one-shot policy or connecting a real radio callback. Compile the adapter against the Zephyr nRF52840 target and add Python static regressions for isolation and lock lifecycle.

## Design and constraints

- `frurt_os_zephyr_claim_lock` stores an atomic state and a stable `frurt_os_lock_ops` table. The adapter uses `atomic_cas` to try `AVAILABLE -> HELD`; contention and uninitialized state return nonzero immediately. It does not wait or use a mutex/semaphore.
- Initialization is a single-shot CAS state machine (`UNINITIALIZED -> INITIALIZING -> AVAILABLE`). Repeated/racing init fails; it cannot reset a held lock. Ops are written before publishing `AVAILABLE`.
- The adapter is generic and does not discover the durable store. Integrators MUST initialize exactly one lock object per store and pass its same ops pointer to every context/caller that can reach that store. Multiple lock instances protecting one store do not serialize one another.
- Initialize and attach the lock before concurrent use. The adapter prevents reinitialization of its object, but the existing `frurt_os_attach_lock()` context pointer is not atomic; concurrent attach/rebind versus `frurt_os_fire()` is unsupported. Do not replace/rebind the pointer once callers are published. Any deliberate rebind requires all users to be quiescent.
- Zephyr atomic APIs are intended for ISR/thread-safe atomic operations, so the lock's acquisition itself does not block in an ISR. This is not proof that `frurt_os_fire()` is ISR-safe: the durable store callbacks run while the claim is held and must themselves be ISR-safe/nonblocking. Flash-backed operations may not meet that condition; use a worker thread unless verified. Never call attach/init/rebind from store callbacks. Re-entry from a store callback fails acquisition (BUSY); the lock is released before the radio callback, which may re-enter without recursively holding this lock.
- The adapter does not implement a flash store, does not attach to `frurt_os_ctx`, and has no radio/TX symbols. It is omitted from both root production CMake and `tests/sx1262_uk_one_shot/CMakeLists.txt`. Its test app links only the adapter and a small API-use `main()`.
- This foundation does not close Task025 gates: real reserved-page flash semantics, interruption behavior, provisioning, C runtime execution, hardware/radio acceptance, and regulatory/antenna evidence remain open. No TX enable is authorized.

## Files changed

- Added `src/features/uk_lora_one_shot/uk_lora_zephyr_claim_lock.{h,c}`.
- Added isolated compile app `tests/uk_lora_zephyr_claim_lock/{CMakeLists.txt,prj.conf,src/main.c}`.
- Added static regression suite `tests/test_task029_zephyr_claim_lock.py`; included its test-app directory in Task024's static TX-token scan.
- Updated `docs/TASK028-ONE-SHOT-CONCURRENCY.md` and `docs/PROGRESS.md`.

## Commands and actual results

| Command | Actual result |
|---|---|
| `python -m unittest tests.test_task029_zephyr_claim_lock -v` | **3/3 PASS**. |
| `python -m unittest discover -s tests -p "test_*.py" -q` | **48/48 PASS**. |
| `python tests/task024_static_tx_check.py` | `TASK024_STATIC_TX_CHECK=PASS`. |
| `C:\zephyrproject\.venv\Scripts\python.exe -m west build -b xiao_ble/nrf52840 -d build-task029-claim-lock tests\uk_lora_zephyr_claim_lock --pristine=always` | **192/192 build steps PASS**, exit code 0; compiled and linked for `xiao_ble/nrf52840` with Zephyr 4.5.0-rc2. `zephyr.elf` (1,309,728 bytes) and `zephyr.hex` (126,852 bytes) were generated; the map contains `frurt_os_zephyr_claim_lock_init` and `claim_lock_try_acquire`. `.config` has `CONFIG_LORA` and `CONFIG_ZTEST` unset; `build.ninja` has no `radio_tx.c` or `lora_send` reference. |

This validates compilation/linking only. The app was not run, no C test suite was executed, and this is not the production one-shot app. CMake reported missing optional host Dtc and Ccache tools, but generation/build succeeded. No host C runtime result is claimed.

## Review and acceptance

- Independent code/config/security/logic review: **pending**; this is the required stop point.
- Production TX hard-disabled: unchanged; the Task025 production CMake `FATAL_ERROR` remains in place.
- Runtime concurrency/ISR behavior: **not proven by compile-time validation**.
- Real flash adapter, hardware tests, TX authorization: **not implemented/not authorized**.

## Issues

The adapter's one-lock-per-store relation is an integration invariant, not dynamically enforced because the core accepts opaque lock ops and store callbacks. Rebinding a context lock pointer during concurrent operations remains unsafe and is explicitly unsupported. The store and radio callback execution contexts still require their own review.

## Rollback

Remove the added adapter, compile-only app, and static regression test; remove the new Task024 scan directory; restore the prior Task028 integration guidance and remove the Task029 entry from `docs/PROGRESS.md`. No hardware state or production radio path was changed.

## Next action

**WAITING FOR: independent review.** Do not connect a real TX path, build a TX-allowed image, flash, reset, or transmit.
