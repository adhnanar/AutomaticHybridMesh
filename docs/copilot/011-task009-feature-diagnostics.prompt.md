# Task 009 bounded mesh diagnostics and tests

Implement the smallest test-topology-only diagnostic change needed to
distinguish B relay scheduling outcomes from C receive/GATT outcomes in the
next Task 009 capture. Start from
[TASK009-RECAPTURE-REVIEW.md](../TASK009-RECAPTURE-REVIEW.md) and the current
[seven-phase roadmap](../ROADMAP.md). Do not import or copy
code/configuration from `C:\zephyrproject\hybrid-mesh`.

## Constraints

- Do not flash, erase, reset, or otherwise interact with physical boards.
- Do not change topology policy, packet behavior, B's 500 ms text hold,
  five-second beacon cadence, or one-second retry interval.
- Keep diagnostics within the BLE mesh relay/GATT feature areas, gated by
  `CONFIG_BLE_MESH_TEST_TOPOLOGY` where appropriate.
- Do not log each manufacturer advertisement from the Bluetooth callback.
- Distinguish an advertising-data API update from actual over-air evidence.
- Preserve the existing production default-off topology configuration.
- Do not claim the new diagnostics prove live delivery.

## Implementation

1. Add compact, correlated B records for accepted TEXT, relay queue outcome,
   and advertising update outcome. Include event/stage, packet type, source,
   sequence, TTL, and firmware uptime. Include the failure result for retries
   and retain rate limiting.
2. Make C's topology record include packet type and uptime. Add bounded
   receive counters for manufacturer decode failures and RX queue drops;
   report snapshots from work context. For accepted TEXT, record the GATT
   publish return with the same packet identity. Avoid per-beacon log spam.
3. Add only missing tests:
   - Fake advertisement update returning `-EAGAIN` retains the relay queue
     head and retries; success removes it exactly once.
   - Malformed manufacturer input and RX queue overflow update diagnostic
     counters without false accept events.
   - Accepted C TEXT updates TX storage independently of notification
     availability.
   - Host parsing rejects joined/truncated console records and accepts only
     complete packet type/source/sequence/TTL tuples as C receive evidence.
4. Reuse the existing C topology tests for B A/TTL3 → TTL2, C A/TTL2 accept
   without relay, direct A/TTL3 rejection, deduplication, and beacon handling;
   do not duplicate them.

Prefer a small test seam or pure helper over broad architectural changes.
If reliable serialization requires a wider logging configuration change,
stop and document the evidence and options rather than silently migrating
the whole firmware logging system.

## Verification and reporting

- Run the focused host unit tests and `py_compile` for changed Python files.
- Build the Zephyr application and the existing `ble_mesh_packet` ztest
  configuration; do not claim runtime hardware behavior from a build.
- Review all changed code/configuration for feature ownership, type safety,
  behavior preservation, and whether logs can still interleave.
- Create a task report under `docs/` recording scope, files changed, commands,
  actual results, issues, rollback, and next action; update `docs/PROGRESS.md`.
- Do not run a live BLE test or flash any board. State that a future physical
  acceptance retry needs explicit user approval.
