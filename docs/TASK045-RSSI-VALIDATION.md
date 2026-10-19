# TASK045 — BLE RSSI receive validation seam

**Scope:** Add a deterministic, unit-testable gate that pairs RSSI with a successfully decoded BLE mesh manufacturer payload. Non-mesh manufacturer data and malformed mesh frames are rejected before a validated RSSI is produced. The accepted RSSI is the value supplied by Zephyr's BLE scan receive callback.

No routing dispatch, GATT/phone, advertisement behavior, LoRa code, hardware, flash, or reset changes are in scope. The existing routing-policy RSSI call remains behind its existing default-off configuration guard. The receive parser continues to accept and queue the same decoded mesh frames as before.

## Files changed

- `src/features/ble_mesh_relay/ble_mesh_rssi_validation.c` and `.h`: pure validation seam for manufacturer data, packet decode, and received RSSI.
- `src/features/ble_mesh_relay/ble_mesh_relay.c`: use the seam in the existing RX parser and retain the scan callback's RSSI only after validation.
- `tests/ble_mesh_packet/CMakeLists.txt` and `src/main.c`: link the seam into the existing ztest suite and add deterministic valid/non-mesh/malformed assertions.
- `docs/PROGRESS.md`: record Task045 and the review handoff.

## Commands and actual results

- Inspected AGENTS.md item 13, the Task044 independent review, the RSSI parser and its nearby ztests; confirmed the existing RSSI source is `scan_receive()`'s `rssi` argument.
- `git status --short`: completed; worktree was already populated with untracked project files and build directories.
- Final static pass found and corrected the queue-result variable declaration after extracting the decoder.
- No build or tests were run, as requested; the reviewer handles execution. No firmware, flash, hardware or RF action was performed.

## Issues / review status

No broad refactor was needed. No new configuration guard was needed: this change extracts the existing always-on parsing gate without changing which valid packets enter the queue. Router RSSI dispatch remains unchanged and default-off. Test/build results and acceptance evidence are pending independent review.

## Rollback

Remove the two new `ble_mesh_rssi_validation` files, revert only the Task045 hunks in `src/features/ble_mesh_relay/ble_mesh_relay.c`, `tests/ble_mesh_packet/CMakeLists.txt`, and `tests/ble_mesh_packet/src/main.c`, and remove the Task045 documentation and progress entry. Preserve unrelated pre-existing worktree contents.

## Next action

Stop for independent review. The reviewer should inspect the changed parser/helper/tests, run the targeted existing ztest build/test as feasible, and record the execution outcome before accepting the task.
