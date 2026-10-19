# Task041 — Preserve BLE RX RSSI in the private queue

## Scope and files

Store scanner RSSI beside each successfully decoded manufacturer mesh packet
in the private RX queue, passing it through the advertisement parser to the RX
worker. No router calls, deduplication, GATT, advertising, or phone behavior
changes. Malformed advertisements retain the existing rejection behavior.

Changed:
- `src/features/ble_mesh_relay/ble_mesh_relay.c`
- `docs/PROGRESS.md`
- `docs/TASK041-BLE-RSSI-QUEUE.md`

## Commands and results

- Read `docs/TASK040-INDEPENDENT-REVIEW.md` and the targeted relay source.
- No build, tests, hardware operations, flash, reset, or RF activity were run.
- Implementation and documentation changes are ready for independent review;
  behavior and build validation remain pending that review.

## Issues

None observed during the scoped source change. No execution evidence is claimed.

## Rollback

Revert the Task041 changes to the RX queue item and the two Task041 Markdown
entries; no configuration or generated artifacts were changed.

## Next action

Independent reviewer to inspect the source change and perform any requested
validation. Keep acceptance open until that review is complete.
