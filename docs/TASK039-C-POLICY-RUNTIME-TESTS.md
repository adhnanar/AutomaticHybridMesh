# TASK039 — PC-only C policy runtime tests and simulator parity

## Scope

Tried to execute the Task038 decision-policy tests without hardware. The
existing Zephyr ztest target could not be built or run in this sandbox, so no
C runtime pass is claimed. To reduce the simulator/C gap while preserving the
Task012 default image, this task aligns the Python simulator's bounded retry
behavior with the C router and expands shared C/Python cases for the BLE RSSI
and freshness boundaries.

No board, USB, serial, power, reset, flash, BLE, LoRa RX, or RF/TX operation
was performed. No real LoRa callback was connected; no TX-enabled image was
built. Existing Task036 BLE source/configuration and the Kit1 RX-only setup
were not changed.

## Files changed

- `tests/hybrid_router/policy_vectors.h` — shared inclusive 10-second freshness
  and below-threshold RSSI vectors consumed by C ztests and the Python policy
  parity test.
- `tests/hybrid_router/src/main.c` — a C ztest for inclusive RSSI/freshness
  boundaries, weak observations not refreshing an accepted peer, and peer-lost
  handling.
- `tests/hybrid_router_sim/hybrid_router_ref.py` and
  `tests/hybrid_router_sim/test_hybrid_router_ref.py` — model the C BLE peer
  report API and test the RSSI/freshness boundary.
- `tests/hybrid_policy_sim/hybrid_policy.py` and
  `tests/hybrid_policy_sim/test_hybrid_policy.py` — use the C router's three
  retry limit instead of 60-second pending expiry; test retry exhaustion.
- `docs/TASK035-AUTOMATIC-HYBRID-ROUTING.md` — update the simulator's queue
  contract and current test evidence.
- `docs/TASK038-HYBRID-POLICY-C-FOUNDATION.md` — record the aligned retry
  behavior and remaining C/simulator boundary.
- `docs/PROGRESS.md` — add the Task039 evidence and pending review/runtime
  status.
- `docs/TASK039-C-POLICY-RUNTIME-TESTS.md` — this record.

## Commands and actual results

All Python commands below used the available system Python 3.12.10:

- `python -m unittest discover -s tests\hybrid_router_sim -p 'test_*.py' -q`
  — **15/15 passed**.
- `python -m unittest discover -s tests\hybrid_policy_sim -p 'test_*.py' -q`
  — **16/16 passed**.
- `python -m unittest discover -s tests -p 'test_*.py' -q` — **55/55 passed**.
- `python -m py_compile tests\hybrid_router_sim\hybrid_router_ref.py
  tests\hybrid_router_sim\test_hybrid_router_ref.py
  tests\hybrid_policy_sim\hybrid_policy.py
  tests\hybrid_policy_sim\test_hybrid_policy.py` — **passed**.
- `python tests\task024_static_tx_check.py` — **PASS**.
- Compared existing
  `build-task012-default\zephyr\zephyr.uf2` and
  `build-task038-independent-default\zephyr\zephyr.uf2` — exact SHA-256 match:
  `2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14`.
  Existing Task038 `.config` has
  `# CONFIG_HYBRID_ROUTER_POLICY is not set` and
  `# CONFIG_LORA is not set`; Task012 `.config` has
  `# CONFIG_LORA is not set`.
- `python -m west --version` with system Python — **unavailable**
  (`No module named west`). A separate initial probe of the requested external
  venv reported West v1.5.0, but a later access to that venv path returned
  sandbox `Access is denied`; no ztest command was run through it.
- `cmake --build build-task015-hybrid-router-ztest --parallel 1` — **blocked
  before compilation** when Ninja attempted to read
  `C:\Users\adhna\zephyr-sdk-1.0.1\cmake\Zephyr-sdkConfig.cmake`; the sandbox
  returned `Access is denied`. No retry or alternate-tool workaround was
  attempted.
- No host C compiler, ARM compiler command, or `qemu-system-arm` executable
  was available on PATH. The existing configured ztest build is for
  `xiao_ble/nrf52840`, not an executable host simulator.
- **C ztests were not executed.** Compilation, link, and runtime behavior of
  the changed C test remain unverified. Python model tests are not evidence
  that C runtime tests passed.

## Parity and remaining divergence

The Python simulation now retries pending packets at one-second intervals
and drops them after three unsuccessful retries, like
`HYBRID_ROUTER_MAX_RETRIES`; it no longer applies its former 60-second
pending expiry. The shared vectors exercise `-85 dBm` inclusive RSSI,
`-86 dBm` rejection, 10,000 ms inclusive freshness, and stale behavior. The C
ztest source additionally verifies that a weak observation does not refresh
an already accepted BLE timestamp and that peer-lost invalidates reachability.
Existing C ztests cover blocked unauthorized LoRa callbacks, BLE-to-LoRa
failover, retry frame/TTL preservation and exhaustion, TTL relay/decrement,
and cross-media dedup.

An intentional model boundary remains: C stores one aggregate BLE
reachability/timestamp, while the network simulator tracks multiple peers.
The simulator is not the C implementation and cannot establish C runtime
success. The added and existing C ztests need to execute in a safe native
Zephyr test runtime before this gap is closed.

## Safety and acceptance

- The only comparison involving firmware images used existing build artifacts;
  this task did not build or replace an image.
- The Task012 default hash matched the existing Task038 default artifact.
- No Task036 BLE production source/configuration, root firmware source/config,
  or LoRa runtime/authorization path was modified.
- No hardware or RF activity occurred.
- **C policy runtime acceptance remains OPEN. Independent review is pending.**

## Issues and limitations

- Zephyr C ztest runtime could not run because the sandbox denied access to
  the external SDK configuration file. Access to the external venv was also
  inconsistent/denied on a later probe.
- Python tests validate the models and shared vector expectations only; they
  do not validate compiled C behavior.
- The current C aggregate-BLE model remains different from the simulator's
  per-peer topology, as described above.
- Hardware acceptance, phone GATT coexistence, LoRa RX, and all RF/TX
  behavior remain outside this PC-only task and unclaimed.

## Rollback

Revert only the listed Task039 test/model/documentation edits to restore the
previous 60-second simulation expiry and the prior vector/test set. No
firmware image, device configuration, or hardware state was changed, so
there is no firmware or hardware rollback.

## Next action

Stop for independent review of the changed tests, Python queue semantics,
documentation, safety gates, and existing-image hash evidence. To execute the
real C ztests afterward, the sandbox must permit the needed local Zephyr SDK
and venv paths; then run only the isolated test application on an executable
Zephyr test runtime. Do not flash, reset, access hardware, or enable LoRa TX.
