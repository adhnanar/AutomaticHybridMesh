# Task 008 — Phase 3 reliable mesh transmit correction

## Scope

Prevent queued BLE mesh text from being discarded when a GATT connection or
advertising lifecycle transition prevents a manufacturer-data update. Preserve
Task 007 connection recovery, existing GATT notification behavior, packet TTL,
deduplication, and the periodic beacon schedule. Do not flash a board.

## Files changed

- `src/features/ble_mesh_relay/ble_mesh_relay.c` — peek at the transmit queue
  rather than removing a packet before advertising; remove the head only after
  a successful GATT advertising-data update. Retain and retry the packet after
  either transient `-EAGAIN` or another reported advertising error.
- `src/features/ble_gatt/ble_gatt.c` — preserve the latest requested mesh frame
  and use it when restarting advertising after disconnect, preventing the
  recovery worker from replacing a just-updated text payload with base data.
- `tests/ble_pc_air_message.py` — decode matching manufacturer TEXT frames and
  report source GATT echo, source/other-node on-air frames, and remote GATT TX
  delivery as separate observations.
- `tests/test_ble_pc_air_message.py` — host unit tests for the PC advertisement
  frame decoder.
- `docs/PROGRESS.md` — record implementation, review, and verification.
- `docs/PHASE3-RELIABLE-MESH-TRANSMIT.md` — task evidence and acceptance status.

## Design and independent review

The relay worker leaves the head packet in `relay_queue` while attempting to
advertise it. `k_msgq_get` occurs only after `advertise_packet` returns zero.
On `-EAGAIN` (including an active GATT connection) and all other non-zero
results, the packet remains queued and the delayable work retries after one
second. This avoids blocking the system workqueue and avoids a busy retry loop.
The retry log is emitted for the first failure and then at most once per ten
consecutive retry failures. The GATT update helper returns errors without
logging each retry; the relay worker is the bounded diagnostic point for text
updates. Queue inspection errors are reported and deferred.

After a successful update, the existing 500 ms message hold is kept before
processing the next queued item. When there is no message queued, a beacon is
still scheduled every five seconds. Task 007's disconnect callback and restart
worker remain deferred from callback context with the existing bounded retry
behavior. The GATT layer snapshots the latest requested manufacturer frame
under its value mutex and includes it in disconnect advertising restarts. This
closes the ordering where a restart could otherwise replace a successful text
update with base advertising. If advertising is stopped and an update fails,
the queue still retains the packet for its delayed retry. The beacon tick
restores beacon data when the queue is idle.

The PC test now treats the manufacturer-data dictionary key `0xFFFF` as the
company ID and checks the contained packet's TEXT type, source ID, sequence,
TTL, and exact payload. It reports the source GATT TX read/notification
separately from matching source/other-node advertisements and from remote GATT
TX reads. A source TX echo is not considered evidence of an on-air send or
remote delivery.

Independent source review verdict: **PASS for bounded queue/retry logic and
scope**. The queue head cannot be consumed by another code path; success-only
dequeue preserves it on `-EAGAIN` and other advertising errors. The disconnect
restart uses a mutex-protected snapshot of the latest requested mesh payload.
Existing connection recovery scheduling, GATT notification code, TTL, and
deduplication remain intact. Firmware build and PC decoder tests pass below. A
successful Zephyr advertising-data update/start is not proof that a radio
frame was received; on-air send and relay delivery remain open hardware
acceptance checks.

## Commands executed

From `C:\zephyrproject`:

```powershell
Set-Location C:\zephyrproject
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always
```

```powershell
& C:\zephyrproject\.venv\Scripts\python.exe -m unittest discover -s C:\zephyrproject\hybrid-mesh-poc\tests -p test_ble_pc_air_message.py -v
& C:\zephyrproject\.venv\Scripts\python.exe -m py_compile C:\zephyrproject\hybrid-mesh-poc\tests\ble_pc_air_message.py C:\zephyrproject\hybrid-mesh-poc\tests\test_ble_pc_air_message.py
```

## Actual results

- Pristine Zephyr build for `xiao_ble/nrf52840`: **PASS**, 267/267 build steps,
  exit code 0. FLASH 133,344 B; RAM 30,408 B; generated UF2 266,752 B.
- PC decoder host tests: **PASS**, 3 tests (matching frame decode, unrelated
  frame rejection, invalid bounds/TTL rejection).
- Python bytecode compilation: **PASS**, both PC script and unit-test file.
- No board was flashed, erased, reset, or otherwise changed.
- The PC BLE scan/GATT script was not run against physical boards during this
  task. Hardware on-air transmission, remote delivery, and multi-hop remain
  unverified; Phase 3 is not marked complete.

## Issues and open acceptance evidence

- `ble_gatt_update_mesh_advertisement` returning zero confirms that Zephyr
  accepted the advertising-data update, not that a peer actually received the
  frame. The new PC test is intended to collect that independent evidence.
- The BLE connection/advertising coexistence, recovery timing, peer relay, and
  Android nRF Connect behavior need another controlled hardware test after the
  user explicitly approves flashing.
- Native Zephyr scheduling behavior is compile-verified, not exercised by the
  host decoder tests; no host Zephyr scheduler test was added.

## Rollback

Restore `src/features/ble_mesh_relay/ble_mesh_relay.c` to the Task 007 relay
implementation and restore the previous `tests/ble_pc_air_message.py`; remove
`tests/test_ble_pc_air_message.py` and this task document if reverting Task
008. `docs/PROGRESS.md` should also be restored to remove the Task 008 entry.
No board rollback is needed because no board was flashed.

## Next action

With explicit user approval, flash the built firmware to the selected test
board(s), then run `tests/ble_pc_air_message.py` with a unique message while
capturing source and remote observations. Record GATT echo, matching on-air
TEXT frames, and remote GATT delivery separately. Keep Phase 3 acceptance open
until the radio and multi-hop evidence is measured.
