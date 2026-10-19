# Task 009 — Controlled, instrumented BLE two-hop proof

## Goal
Prove genuine logical A -> B -> C forwarding on the three connected Seeed XIAO nRF52840 Sense kits, despite direct RF reachability. Do not flash firmware; flashing requires separate explicit user approval. Preserve normal production mesh behavior by default.

## Evidence and IDs
Kit 1 COM14 USB serial 1D5EA4A525ED34EB, BLE E5:12:76:D2:56:EA, mesh source ID eb34ed25a5a4. Other BLE addresses C8:51:08:6E:08:C0 mesh source ID 0c13dae125c8 and E2:DB:6A:54:BB:71 mesh source ID ee07145a5e02; physical Kit 2 vs Kit 3 mapping not yet established. All three Task008 running. Previous on-air test saw TTL3 from E5 and TTL2 forwarded from C8, but endpoint delivery intermittent.

## Implementation requirements
1. Introduce a compile-time gated TEST-ONLY topology policy (disabled by default). Make node roles based on existing immutable mesh source IDs, not BLE random addresses. A = eb34ed25a5a4 (Kit1), B/C = the two other IDs; mapping of physical Kit2/3 must be verified first, or expose role assignment as build-time configuration rather than guessing.
2. For TEXT packets only: A originates TTL3; B accepts only original A packets TTL3 and forwards as TTL2; C must reject any directly heard A TTL3 and accept only A packets at TTL2 (or a properly authenticated relay marker). Ensure C cannot forward messages back into a loop. Explicitly document that TTL2 alone does not cryptographically prove which node relayed it; in this three-node restricted topology only B is allowed to relay. Preserve dedup and beacon discovery. Never publish rejected direct packets to GATT TX or dedup cache.
3. Instrument observable evidence: role, local mesh ID, accepted/rejected frame with source/sequence/TTL, and relay attempt/success. Consider that logging must not break BLE timing. Avoid sensitive logs.
4. Add host unit tests for topology predicate and reject-before-dedup, positive A->B->C and negative A->C direct; make tests deterministic. Add a PC-side acceptance script which sends a unique message from A, confirms B forwarding on-air TTL2 and C GATT TX delivery, and clearly fails if only direct A TTL3 was observed. Do not misclassify GATT source echo as mesh delivery.
5. Preserve Task008 retry semantics (do not dequeue on advertising failure), bounded queues, 500ms message advertisement, normal reconnect behavior. Do not claim physical RF isolation or production routing from this test-only logical proof.
6. Build with clean Zephyr build, run host tests, document exact commands, pass/fail and SHA256/UF2 size in docs, and update docs/PROGRESS.md. DO NOT copy UF2 to any drive, reset boards, or flash.

## Workflow
Use VS Code GitHub Copilot Agent as primary coding agent. Inspect code first, implement minimally, run tests and build. Surface any inability to determine Kit2/Kit3 physical role instead of guessing. Request independent review by ChatGPT before firmware flashing.
