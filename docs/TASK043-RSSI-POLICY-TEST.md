# Task043 RSSI/freshness policy test

- Scope: PC-only, test-only. No production, GATT, advertising or dispatch change.
- File changed: `tests/hybrid_router/src/main.c` (new `test_rssi_threshold_and_freshness_boundaries`); this doc; `docs/PROGRESS.md`.
- Checks: -86 dBm rejected; -85 accepted; age 10000 ms fresh (inclusive); 10001 ms stale; stale BLE + unauthorized LoRa performs no LoRa send.
- Commands executed: none (no build/test run per instruction). **Result: not yet verified** — reviewer to run the `tests/hybrid_router` twister/native_sim test.
- Issues: none known; unverified.
- Rollback: `git checkout tests/hybrid_router/src/main.c` and remove this doc/progress entry.
- Next action: independent review and test run. No hardware, flashing, reset, RF or LoRa TX.
