# TASK038 — PC-only hybrid policy C foundation

## Scope

Adapted the existing software-only `hybrid_router` rather than adding a second
router. Local and relay candidates are now BLE-first. BLE is usable only after
a recent report at or above the `-85 dBm` threshold and remains fresh for at
most 10 seconds. LoRa is a fallback only when both the link is available and an
external integration has explicitly granted runtime TX authorization. With
LoRa available but unauthorized and no usable BLE path, the result is
`HYBRID_ROUTER_LORA_BLOCKED_QUEUED`; the injected LoRa send callback is not
called.

The router is opt-in through `CONFIG_HYBRID_ROUTER_POLICY`, default `n`, and
remains disconnected from the default application's transport callbacks.
Enabling the symbol only compiles the software module; it does not connect a
radio or itself grant TX authorization. Existing packet TTL, shared dedup,
24-byte wire-size bound, pending queue, and retry/failover behavior remain in
the C router. Queue retries are bounded to three retries. Task039 aligned the
Python simulator retry limit and extended the shared BLE freshness/RSSI test
vectors; aggregate BLE reachability in C still differs from the simulator's
per-peer model.

## Files changed

- `Kconfig` — default-off router policy option.
- `CMakeLists.txt` — compile the router only when the option is enabled.
- `src/features/hybrid_router/hybrid_router.h` — explicit authorization,
  RSSI threshold, and blocked-queued result API.
- `src/features/hybrid_router/hybrid_router.c` — BLE-first selection, weak
  peer rejection, LoRa authorization enforcement, and explicit blocked state.
- `tests/hybrid_router/policy_vectors.h` — shared deterministic policy cases.
- `tests/hybrid_router/src/main.c` — ztests for vectors, authorization,
  retries, relay preference, RSSI, TTL, and payload bounds.
- `tests/hybrid_router_sim/hybrid_router_ref.py` and
  `tests/hybrid_router_sim/test_hybrid_router_ref.py` — keep the existing C
  router reference model aligned to BLE-first and explicit authorization.
- `tests/hybrid_policy_sim/test_hybrid_policy.py` — compares the Python model
  against the shared C policy vectors.
- `docs/TASK035-AUTOMATIC-HYBRID-ROUTING.md` — current C foundation status.
- `docs/PROGRESS.md` — Task038 record and review gate.

## Safety and integration boundary

- `lora_tx_authorized` is zero after router initialization. Only an external
  caller can change it with `hybrid_router_set_lora_tx_authorized()`.
- Every candidate must also have a non-NULL callback. LoRa callbacks are
  filtered by both `lora_available` and authorization before selection.
- No real LoRa callback was connected, no default app route was changed, and
  no TX-enabled build, radio operation, flash, reset, or device access was
  performed.
- The router stores one aggregate BLE reachability state, not a table of
  peers. The BLE integration must report verified peer RSSI and call the
  router's peer-lost function appropriately.
- Local sends and relays use callbacks only; `SENT` means callback success,
  not end-to-end delivery. Destination-aware routing and ACKs remain out of
  scope.

## Commands and actual results

- `python -m unittest discover -s tests\hybrid_policy_sim -v` — **16/16
  passed**, including parity cases read from `policy_vectors.h`.
- `python -m unittest discover -s tests\hybrid_router_sim -v` — **14/14
  passed** after updating the C router reference model.
- `python -m unittest discover -s tests -p 'test_*.py'` — **55/55 passed**.
- `python tests\task024_static_tx_check.py` — **PASS**.
- `python -m py_compile tests\hybrid_policy_sim\test_hybrid_policy.py
  tests\hybrid_router_sim\hybrid_router_ref.py
  tests\hybrid_router_sim\test_hybrid_router_ref.py` — **PASS**.
- `python -m west --version` using the available system interpreter — **not
  available** (`No module named west`). The workspace `.venv` interpreter was
  not present. Checking the external `C:\zephyrproject\.venv` location was
  denied by the sandbox; no retry or workaround was attempted.
- No `gcc`, `clang`, or `cl` executable was available for a portable native
  test build.
- No C ztest build or execution was possible in the available environment.
  The new Zephyr tests are authored but remain unverified by a compiler.
- The first parity-test run failed because the Python vector reader did not
  account for C macro continuation backslashes; the parser was corrected and
  the focused suite rerun successfully.
- Inspection of `build-task024-ztest-locked\zephyr\.config` confirmed
  `# CONFIG_LORA is not set`. The repository root `Kconfig` contains existing
  BLE app options and the new default-off router option; no LoRa Kconfig or
  TX setting was changed.

## Independent review verdict

Source/configuration review confirms the default-off compile gate, no default
radio wiring, BLE-first order, freshness/RSSI checks, and authorization
filtering before any LoRa callback path. The shared policy vector assertions
and Python tests pass. Review also caught a missing BLE packet include path
under the opt-in root-app build; the path is now added only when the feature
is enabled. **Review is incomplete for build evidence:** the C
ztest suite and default-app configuration could not be compiled here, so no
firmware image or binary-hash preservation claim is made. This task stops at
the requested independent-review gate.

## Issues and limitations

- Zephyr C compilation and ztest execution remain open until the repository's
  authorized venv/west/Zephyr SDK environment is available.
- Task039 compared the existing Task012 and Task038 default UF2 artifacts;
  their SHA-256 values matched. This was an artifact comparison, not a new
  build or runtime test.
- The Python simulator now uses the C three-retry bound. A weak RSSI report is
  ignored; the router represents a verified peer aggregate and cannot
  distinguish concurrent peers or destinations.
- No BLE GATT integration, Android validation, physical acceptance, LoRa RX,
  or LoRa TX was attempted.

## Rollback

Remove the `HYBRID_ROUTER_POLICY` Kconfig option and its conditional CMake
source entry; restore the prior C router ordering/API; remove the new shared
vectors and tests; and remove the Task038 progress/doc entry. No generated
firmware or hardware state needs rollback.

## Next action

Task039 records the PC-only follow-up, the existing-image hash comparison,
and the sandbox-blocked C ztest attempt. Independent review is pending; see
[TASK039-C-POLICY-RUNTIME-TESTS.md](./TASK039-C-POLICY-RUNTIME-TESTS.md).
