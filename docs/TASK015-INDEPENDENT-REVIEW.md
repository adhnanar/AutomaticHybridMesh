# TASK015 / TASK015B — Independent hybrid-router mock review

Reviewed 2026-10-19. **Verdict: PASS as isolated SOFTWARE-ONLY routing state machine and test harness. BLOCKED for real BLE/LoRa integration until radio/link/topology, ACK/retry, synchronization and legal RF parameters are verified.**

## Code-level review
Inspected `src/features/hybrid_router/hybrid_router.h/.c`, `src/features/ble_mesh_relay/ble_mesh_packet.c/.h`, `tests/hybrid_router/src/main.c`, test app CMake/prj.conf/testcase YAML and Python reference simulator/tests. All changes are feature-scoped. Root BLE/GATT firmware build remains independent (no Task015 source linked into root CMake).

- Local messages: inject packet with TTL unchanged, prefer BLE when caller reports fresh neighbor (10s), LoRa otherwise; immediate BLE failure can fallback to LoRa using IDENTICAL encoded source/seq/TTL bytes. This is mock link availability (caller-provided), NOT measured RF distance or end-to-end ACK.
- Ingress from BLE or LoRa: decode/validate, shared source+seq dedup cache, optional local callback, at most one decrement on relay, TTL1 delivered but not forwarded. Default relay policy tries *other* medium first to avoid reflection, then same-medium if allowed, so BLE ingress with LoRa available may use LoRa even if BLE neighbor exists. It is an experimental bridge heuristic, not a destination-aware optimal route; must be tuned before live integration.
- Pending queue: 4 records, retries 1s apart at most 3, preserves on-air bytes across retries rather than re-deduping. Send success means backend callback returned 0, not proof of arrival.
- Independent-review correction Task015B: originally local QUEUE_FULL left source dedup poisoned though nothing was sent/queued. Now local admission caches a snapshot and restores it on QUEUE_FULL *only if no apparent cache index change*, enabling same-sequence retry after a slot frees; existing relay duplicate suppression remains. **Caller must serialize callbacks**: the guarded snapshot test of `next_entry` alone does not detect all reentrancy cases (e.g., expired existing-key timestamp update), so this is NOT thread-safe or production-ready across concurrent callbacks. Future multi-radio integration needs locking/reservation transaction.
- Previously `stats.delivered` and `out.delivered_locally` were set even with NULL delivery callback. Fixed: only indicate delivery when callback actually invoked; void callback cannot report downstream app failure.
- No security/authentication, time sync, packet reassembly, LoRa driver binding, per-destination topology or hybrid RF acceptance in Task015. No source flashed.

## Independent verification
1. `C:\zephyrproject\.venv\Scripts\python.exe -m unittest discover -s tests\hybrid_router_sim -p test_*.py -q`: **13/13 PASS** (Python reference model, not C runtime).
2. `C:\zephyrproject\.venv\Scripts\python.exe -m unittest discover -s tests -p test_*.py -q`: existing **31/31 PASS**.
3. Independently pristine rebuilt Task015 before review changes `build-task015-independent-review`: Zephyr `xiao_ble/nrf52840` 200/200 C ztest compile/link PASS, FLASH 78,788 B, RAM 17,720 B; raw `docs/task015-independent-build.log`.
4. Independently pristine rebuilt after Task015B code updates `build-task015b-independent-review`: **200/200 compile/link PASS**, FLASH 81,116 B, RAM 17,784 B; `zephyr.uf2` 162,304 B generated, NOT FLASHED. Raw `docs/task015b-independent-build.log`.
5. `native_sim` test execution unavailable on current Windows host: Twister pre-execution multiprocessing WinError2, no host GCC/clang/WSL. The **C ztests have NOT executed**; Python reference tests do not substitute for C runtime tests.

## Safety and next steps
- The same three physical BLE boards and current flashed firmware remain intact. Task014 independent pristine default/topology UF2 hashes match previously validated Task012 originals; Task015 libraries remain only in independent ztest app.
- User confirmed SX1262 modules and antennas connected on 3 kits. **Legal operating region/frequency still missing.** Never transmit LoRa or flash without later explicit approval.
- Next software-only development: opt-in verified SX1262 RF switch/TCXO overlay (Task017) and suppress `DISCOVER` control command echo from text GATT path (Task018). Preserve default BLE firmware until explicit flash authorization.
- Rollback Task015: remove `src/features/hybrid_router`, `tests/hybrid_router`, `tests/hybrid_router_sim` and their documentation/build outputs. No deployed image changed.
