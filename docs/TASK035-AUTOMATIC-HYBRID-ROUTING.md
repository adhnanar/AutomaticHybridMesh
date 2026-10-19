# TASK035 – Automatic BLE/LoRa hybrid transport-selection policy (PC-only simulation)

## Scope
Pure-logic Python policy + 3-node/Android-GATT simulator. **Nothing is implemented in firmware.** No flash, reset, USB/device, RF, install/download, PATH change, git stage/commit. No LoRa TX authorization exists; the simulated "LoRa air" is a Python list and no TX-enabled firmware was built. Existing BLE defaults, `src/`, CMake/prj.conf and all locked LoRa paths are untouched (verified: no new/changed files outside `tests/hybrid_policy_sim` and docs).

## Files changed
- `tests/hybrid_policy_sim/hybrid_policy.py` (new) – policy + `Sim3` simulator.
- `tests/hybrid_policy_sim/test_hybrid_policy.py` (new) – 16 tests incl. randomized invariants.
- `docs/TASK035-AUTOMATIC-HYBRID-ROUTING.md` (this), `docs/PROGRESS.md` entry.

## Policy (simulated)
1. **BLE** is chosen iff a peer is *verified near*: beacon seen ≤10 s ago AND RSSI ≥ −85 dBm.
2. Otherwise **LoRa** only if `LoraAuthorization.tx_authorized` is set by an independent source (default False; the router never sets it) and hardware present.
3. No usable route → explicit state, never false delivery: `BLOCKED_LORA_NOT_AUTHORIZED` (only LoRa could carry it; held in bounded queue, never transmitted), `QUEUED_NO_ROUTE` (transports present but failed / LoRa hardware absent; retried every 1 s), `QUEUE_FULL` (4 slots). Pending packets are dropped after three unsuccessful retries and logged as `RETRY_EXHAUSTED`, matching the C router; there is no 60-second expiry.
4. **Failover** both directions (BLE send failure → authorized LoRa; none → queue and retry when BLE recovers).
5. **Dedup** (src+seq, 16 entries, 30 s) shared across BLE and LoRa ingress; **TTL** 3 max, decremented once per relay, TTL 1 delivered locally and not forwarded.
6. **Size**: 14-byte text max (24-byte frame, shared by both media). Oversize → `REJECTED_TOO_LARGE`, empty → `REJECTED_EMPTY`; never truncated. Malformed frames → `REJECTED_INVALID`.
7. Observable states: `SENT_BLE`, `SENT_LORA` (= handed to transport, **not** end-to-end ACK), `DELIVERED_LOCAL` (phone GATT inbox feed), plus those above, kept in a per-node audit log.

## Implemented vs simulated
| Item | Status |
|---|---|
| Policy logic, dedup/TTL/failover/queue states | Python model only (new); retry limit aligned with C by TASK039 |
| 3 nodes A–B–C (BLE A-B, B-C; A-C out of BLE range; virtual LoRa A-C) and a per-node Android GATT write/inbox endpoint | Simulated |
| Port to C / hook into firmware, `hybrid_router` C code | TASK038 adds a default-off C router foundation with BLE-first selection, explicit external LoRa authorization, and shared C/Python policy vectors. It is not connected to the default app callbacks; see [TASK038](./TASK038-HYBRID-POLICY-C-FOUNDATION.md) and [TASK039](./TASK039-C-POLICY-RUNTIME-TESTS.md). |
| Real RSSI/beacon verification, GATT write→router, inbox notify | Not done |
| Any LoRa TX | Forbidden; not built |

## Commands and actual results
- `python -m unittest discover -s tests\hybrid_policy_sim -p "test_*.py" -q`: **16/16 OK** (2026-10-19; includes shared C policy vectors and retry exhaustion).
- `python -m unittest discover -s tests -p "test_*.py"`: 55/55 OK (existing; does not recurse into the sim folder). `python tests\task024_static_tx_check.py`: PASS.
- `python -m unittest discover -s tests\hybrid_router_sim -p "test_*.py" -q`: **15/15 OK** (2026-10-19).
- These execute Python models only. See [TASK039](./TASK039-C-POLICY-RUNTIME-TESTS.md): C ztest build access was sandbox-blocked before compilation, and no C runtime pass is claimed.

## Issues / limitations
- Model is broadcast-flood (matches the current packet format: no destination, no ACK). Per-destination routing and ACKs do not exist.
- BLE "verified near" is a policy assumption (thresholds are placeholders to be tuned with real RSSI data).
- BLE relay echoes back to the ingress neighbour (deduped); wasteful but safe.
- C stores aggregate BLE reachability while this simulator tracks multiple peers. Shared vectors align policy thresholds, but do not establish C runtime behavior.

## Rollback
Delete `tests/hybrid_policy_sim`, this doc and the PROGRESS entry.

## PoC acceptance matrix and next minimal hardware tests
| # | Acceptance check | Evidence today | Next minimal real test | Authorization needed |
|---|---|---|---|---|
| 1 | Phone GATT discovery of node | Earlier PC GATT tests; inbox compiled only | nRF Connect finds Kit2/3 by name, reads 7b3e0006 | Flash of Task034 opt-in image to Kit2/3 (user approval) |
| 2 | BLE peer message A→B | Passed earlier (A,B,C controlled) | Kit2↔Kit3 text via GATT write, other phone reads inbox | Same flash approval; no restored A |
| 3 | BLE 3-hop multi-hop | Passed earlier only with restored A | Needs 3 BLE nodes: **blocked** until a third BLE node (restored A) is approved | User approval to restore A |
| 4 | Phone GATT + BLE multi-hop coexist | Coexistence PC test earlier | Repeat with inbox image | Flash approval |
| 5 | Policy: BLE when near | Sim only | Port policy into firmware, observe `SENT_BLE` log | Code task + flash |
| 6 | Policy: LoRa when far | Sim only | Kit1 LoRa **RX** observe only; a TX is impossible without authorization | LoRa TX authorization + antenna/regulatory sign-off (not granted) |
| 7 | Unauthorized LoRa → BLOCKED/QUEUED, no RF | Sim + static TX check PASS | Same on hardware with locked build; verify via RX that nothing is emitted | Kit1 RX capture (listen-only) approval |
| 8 | Failover/dedup/TTL across transports | Sim only | After TX authorization, one-shot A→C test | TX authorization |

## Next action
Independent review of Task039. C runtime ztests remain unexecuted due sandbox denial to the installed Zephyr SDK; see [TASK039](./TASK039-C-POLICY-RUNTIME-TESTS.md). Any flashing or Kit1 RX capture needs explicit user approval.
