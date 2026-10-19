# Task020 — PC-only UK LoRa policy hardening

**Status: source and ztests updated; rebuild blocked by sandbox access to the installed Zephyr SDK.**
This is isolated experimental software only. No board was powered, reset,
flashed, queried, or used for RF transmission.

## Scope

Harden the standalone UK868 regulatory budget implementation and its focused
Zephyr ztests, without changing the root BLE firmware or wiring a radio driver.
The locked default remains locked: `UK868_ALLOW_GATE_OPEN=0` still makes the
gate check unconditionally false, including when a caller tampers with the
public `g.open` field.

The review found the clock-fault latch was already present in source. This task
documents its reset contract and exercises backward, exactly half-range
ambiguous, and attacker-restored forward updates. `uk868_budget_init()` is for
a genuinely new object only. Restart recovery must use
`uk868_budget_init_cold_boot()`, which precharges the full budget until its
conservative window expires. Do not reinitialize live faulted state. No
separate secure recovery API is provided.

## Files changed

- `src/features/uk_lora_regulatory/uk_lora_regulatory.c`
  - Reject invalid `uk868_budget_finish()` outcomes before removing a pending
    reservation.
  - Allocate nonzero pending token IDs while skipping IDs still in use across
    `uint32_t` wrap.
  - Compute ERP in `int64_t` and clamp to the `int32_t` return range, avoiding
    signed-overflow undefined behavior.
- `src/features/uk_lora_regulatory/uk_lora_regulatory.h`
  - Document first-initialization versus cold-boot reset and sticky-clock fault
    recovery expectations.
- `tests/uk_lora_regulatory/src/main.c`
  - Add invalid-settlement preservation and subsequent valid-settlement test.
  - Add token-wrap collision, ERP extreme arithmetic, ambiguous/backward clock
    and forward-restoration tests.
  - Verify reservation is charged before the radio callback; verify an
    over-budget reported airtime overrun blocks a subsequent send.
  - Retain and exercise the locked build's direct `g.open=true` tampering
    check against both guarded send and reserve.
- `docs/PROGRESS.md`
  - Record task scope, validation status, limits, and next action.

## Airtime reservation and integration limit

The guarded path books the computed airtime estimate, including the existing
5% engineering margin, in the rolling budget **before** invoking the callback.
The ztest checks the callback observes that booking. Completion charges any
reported overrun, and subsequent reservations are denied when charged use is
at or above the budget. Accounting an overrun after TX cannot prevent that
already-started transmission from exceeding the software budget. A pending
transmission also cannot be safely bounded against concurrent callers without
serialization.

The airtime formula and margin are not validated against a radio, calibrated
timing source, or analyser; callback-reported airtime is not an enforceable
hardware TX timeout. Therefore the implementation is **not approved for radio
integration or real transmission**. Keep it feature-isolated and default
locked until an independently justified upper bound for actual radiated
airtime is enforced by the radio path, budget calls are serialized, and the
corresponding hardware/runtime evidence is reviewed. The tests demonstrate
software reservation/accounting behavior only, not a regulatory guarantee.

## Commands executed and actual results

| Command | Actual result |
|---|---|
| `west build -b qemu_cortex_m3 -d build-task020-uk868 tests\uk_lora_regulatory -p always` | Attempted; PowerShell reported `west` is not recognized. |
| `& 'C:\zephyrproject\.venv\Scripts\west.exe' build -b qemu_cortex_m3 -d build-task020-uk868 tests\uk_lora_regulatory -p always` | Attempted; the referenced executable is not present/resolvable (`CommandNotFoundException`). |
| `ninja -C build-task019-independent-mock` | **Blocked by sandbox** while refreshing `build.ninja`: access denied to `C:\Users\adhna\zephyr-sdk-1.0.1\cmake\Zephyr-sdkConfig.cmake`. |
| `ninja -C build-task019-independent-locked` | **Blocked by sandbox** at the same installed SDK configuration path. Both configured builds remain unverified for this task. |
| `Get-Command gcc,clang,cl,arm-zephyr-eabi-gcc,cmake,ninja -ErrorAction SilentlyContinue` | Only `cmake.exe` and `ninja.exe` were available in the command environment; no C compiler command was available. |

No flash, reset, power-up, board interaction, or RF command was run.

## Independent review

The changed C, header, tests, and documentation were inspected against the
Task019 findings. The change remains within the feature-scoped library and
standalone test app; root BLE sources and application configuration are not in
scope. Verdict: **source review acceptable; build/runtime validation and
physical airtime bound remain OPEN**. No evidence of build or ztest execution
is claimed.

## Issues and acceptance evidence

- Zephyr build and ztest execution are not accepted until actually run. The
  available ninja builds were blocked by sandbox access denial to the external
  SDK path; leave C validation OPEN for the independent reviewer. No retry,
  permission change, or physical intervention was requested.
- No hardware airtime bound, RF measurement, regulatory assessment, or radio
  behavior is established.
- No runtime `clock_fault` recovery is allowed other than cold-boot
  initialization of a new budget state with full-budget precharge; persistence
  across power loss remains a system integration responsibility.

## Rollback

Revert only the changes in the three feature/test files listed above and this
Task020 progress/documentation entry. Do not remove or modify unrelated
worktree content.

## Next action

Have the independent reviewer rebuild the unlocked and locked ztest variants
in an environment with Zephyr SDK access, then execute the tests where the
runtime is available. Keep real-radio integration blocked until the airtime
upper bound, serialization contract, build, and executable ztest evidence are
independently accepted.
