# TASK044 — Blocked RSSI Assertions
- Scope: Assert the exact blocked/queued result, one pending item, and zero BLE/LoRa sends for stale BLE with unauthorized LoRa.
- Files changed: `tests/hybrid_router/src/main.c`, this report, and one entry in `docs/PROGRESS.md`.
- Commands/results/issues: No builds or tests run; native_sim not retried (TASK043 review records missing host gcc); C runtime acceptance remains open.
- Rollback/next action: Revert the assertions, authorization setting, and task documentation entries if needed; stop for independent review (no kit, flash, reset, or LoRa TX).
