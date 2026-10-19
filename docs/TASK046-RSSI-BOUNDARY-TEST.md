# Task046 — RSSI validation boundary regression

## Scope

Extend the existing BLE mesh RSSI validation ztest with deterministic assertions
that NULL and short manufacturer inputs preserve packet/RSSI outputs, a wrong
manufacturer ID does not refresh them, and valid packet RSSI values of -85 and
-86 are copied exactly by the decoder. RSSI filtering remains the routing
layer's responsibility.

## Files changed

- `tests/ble_mesh_packet/src/main.c` — extended the existing validation test.
- `docs/TASK046-RSSI-BOUNDARY-TEST.md` — this task record.
- `docs/PROGRESS.md` — added one Task046 entry.

## Commands executed and actual results

- Read the Task045 review, RSSI validation implementation, Task046 guidance,
  targeted test, and relevant progress entries.
- Searched the existing test for RSSI validation assertions. Existing coverage
  included wrong manufacturer output preservation and a valid RSSI of -73, but
  not NULL/short input or the -85/-86 boundaries.
- No build or test command was run, as requested.

## Issues

The C ztest edits have not been compiled or executed.

## Rollback

Revert only the Task046 assertions in `tests/ble_mesh_packet/src/main.c` and
remove this task record and its single progress entry.

## Next action

Stop for independent review before any further changes or test execution.
