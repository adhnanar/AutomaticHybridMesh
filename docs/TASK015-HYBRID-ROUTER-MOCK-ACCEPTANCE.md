# TASK015 â€“ Hybrid router mock acceptance (software-only)

## Scope
Deterministic hybrid BLE + LoRa routing policy library with **injected mock transports**, opt-in test-only. No GATT, no BLE, no SX1262, no RF, no flashing/reset/power actions. Root `CMakeLists.txt`, `prj.conf`, default devicetree and the deployed BLE app are untouched. No code from the legacy project or Meshtastic.

## Files
- `src/features/hybrid_router/hybrid_router.{h,c}` â€“ router state machine (new).
- `tests/hybrid_router/` â€“ ztest app (CMake, prj.conf, testcase.yaml, `src/main.c`, 19 tests). Reuses `ble_mesh_packet.c` (24-byte source+sequence+TTL packet and shared dedup cache) unchanged.
- `tests/hybrid_router_sim/` â€“ Python **reference model** + unittest/property tests.
- `docs/copilot/015-hybrid-router-mock.prompt.md`, logs `docs/task015-*.log`, this file, `docs/PROGRESS.md`.

## Behavior (as implemented)
- Inputs are caller-driven: `hybrid_router_ble_peer_seen/lost` (BLE reachability, fresh for â‰¤10 s, uint32 wrap-safe), `hybrid_router_set_lora_available`, injected `ble_send`/`lora_send`/`deliver`, explicit `now_ms`. Not RF discovery.
- Local send: BLE if fresh, else LoRa if available. BLE failure â†’ LoRa fallback once, same bytes (TTL not decremented). TTL sent as given.
- Receive: decode/validate (malformed â†’ INVALID, nothing recorded) â†’ one shared dedup decision across both media â†’ deliver once â†’ if TTL>1 decrement exactly once and relay; TTL1 delivered, not forwarded. Beacons ignored.
- Relay link order: the medium other than ingress first; same-medium relay (BLEâ†’BLE multi-hop) only as second choice and can be disabled (`allow_same_medium_relay`).
- All sends failed or no link â†’ bounded pending record (4 slots) holding the final frame; `hybrid_router_poll` retries after 1 s, max 3 retries, then drops (counted). Retry bypasses dedup, so identity/TTL are preserved and the dedup cache cannot suppress it.
- Explicit results: SENT, SENT_FALLBACK, QUEUED_RETRY, NO_LINK_QUEUED, QUEUE_FULL, DUPLICATE, INVALID, NOT_FORWARDED.

## Commands and actual results
1. `west build -b xiao_ble/nrf52840 -d hybrid-mesh-poc\build-task015-hybrid-router-ztest hybrid-mesh-poc\tests\hybrid_router --pristine=always` (from `C:\zephyrproject`): **COMPILE+LINK SUCCEEDED**, 200/200, FLASH 78788 B (9.76 %), RAM 17720 B, `zephyr.uf2` generated (NOT flashed). Log: `docs/task015-xiao-compile.log` (the PowerShell wrapper reported exit 1 because west writes to stderr; output shows a completed link).
2. `west twister -T tests\hybrid_router -p native_sim`: **BLOCKED / NOT EXECUTED** â€“ Twister crashed in `multiprocessing` (`WinError 2` named pipe) on this Windows host before running any test. Not retried. Log: `docs/task015-native-sim-attempt.log`.
3. `python -m unittest discover -s tests\hybrid_router_sim -v`: **10/10 PASS**. This executes only the Python reference model, **not the C code**. Existing 31 Python tests still pass.

**Therefore: the C ztests are compiled/linked for xiao_ble/nrf52840 only. They have NOT been executed.** A remote runner should run `west twister -T tests/hybrid_router -p native_sim` (Linux/WSL) and record results.

## Issues / limitations
- No per-destination topology or ACK; "sent" means the injected op returned 0.
- Relay queue-full still retains shared dedup admission and local delivery where a callback exists, so a dropped relay copy cannot be recovered by a duplicate neighbour copy. Task015B now guards local QUEUE_FULL admission rollback so local callers may retry the same sequence once a pending slot becomes available; **serialized, non-reentrant callbacks are required** (no multithread safety claimed).
- Dedup uses the existing 16-bit sequence, 16 entries, 30 s window; unchanged.
- Signed-diff timing is valid for spans < 2^31 ms. A long link outage can exhaust the 3 retries and drop a record.
- Python model is hand-mirrored; divergence from C is possible until the C tests run.
- Relay prefers cross-medium over same-medium, so with LoRa available a BLE-ingress packet goes to LoRa rather than BLE; real policy to be tuned later.

## Rollback
Delete `src/features/hybrid_router`, `tests/hybrid_router`, `tests/hybrid_router_sim`, `docs/copilot/015-*`, `docs/task015-*.log`, `build-task015-*`, and this doc plus the PROGRESS entry. Nothing else was modified.

## Next actions
1. Remote runner executes native_sim twister; independent review (AGENTS rule 10).
2. After RF country/channel is confirmed and user approves: bind `lora_send` to the SX1262 transport (TASK013/014) in an opt-in app, still without default build changes.
3. Bind `ble_send`/`deliver` to the existing GATT peer-notification path and feed `ble_peer_seen` from scan/connection state; verify phone GATT and BLE peer relay coexist.
4. Hardware acceptance only with explicit user approval.

## Task015B independent-review amendment (2026-10-19)
- Fixed local queue-full dedup poisoning with guarded single-thread cache snapshot rollback and added Python/C unit cases; fixed false local-delivery accounting when deliver callback NULL; pinned experimental cross-medium relay priority in tests.
- Independent Python reference model: **13/13 PASS**; existing Python tests: **31/31 PASS**. Independent pristine Zephyr xiao_ble/nrf52840 C ztest compile/link: **200/200 PASS**, FLASH 81,116 B, RAM 17,784 B. C ztest runtime NOT executed.
- Full review and important concurrency caveat: [TASK015-INDEPENDENT-REVIEW.md](./TASK015-INDEPENDENT-REVIEW.md).

