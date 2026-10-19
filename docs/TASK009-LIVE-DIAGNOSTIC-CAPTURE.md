# Task 009 live diagnostic capture review

## Scope and outcome

Review the newly attached independent capture, distinguish this run from
earlier Task 009 evidence, locate any supported logging/timing defect, and
make a narrow diagnostic-only correction with regression tests.

**Outcome:** this run confirms the A GATT write/echo, no PC-scanner observation
of the fresh A or B text frame, and C TX still reading `Ready`. It does not
confirm that the new text was transmitted over the air or received by C. The
C console excerpts cannot identify the accepted packet sequence or type. The
capture harness could emit partial UART records as if they were complete
because `readline()` returned at its 300 ms timeout; that capture defect is
fixed and covered by tests. No firmware/radio behavior was changed.

## Exact evidence and interpretation

Inputs:

- [task009-live-with-console.log](./task009-live-with-console.log)
- [task009-kit3-console.log](./task009-kit3-console.log)

The fresh PC test payload was `HOP0095F2CC5ED`. In the combined live output:

- `A_GATT_WRITE_OK` and `A_SOURCE_GATT_ECHO=PASS` report that exact payload.
- The acceptance failure lists both `A on-air TEXT with TTL3 and the unique
  payload` and `B on-air forwarding of the same A sequence at TTL2`. There is
  no corresponding `A_SOURCE_ON_AIR` or `B_RELAY_ON_AIR` record for this fresh
  payload.
- `C_GATT_TX_DELIVERY=FAIL value=b'Ready'` reports that the final C TX read
  did not contain the fresh payload.
- C's console includes truncated/interleaved `TEST topology ACCEPT role=C`
  fragments containing only partial sources `0C13DAE125` and `EB34ED25A5`.
  Neither fragment provides a complete source ID, packet type, sequence, TTL,
  payload, or timestamp.
- C reports a GATT connection, `BLE mesh beacon advertisement update failed:
  -11`, and a disconnect with reason `0x13`; a later C GATT connection is
  logged. The raw capture has no timestamps with which to align these events
  to the A write or final C read.

Do not carry forward the prior run's A/B sequence 33 or its different unique
payload as evidence for this run. The fresh payload above is the only observed
message identifier in this capture; its on-air sequence is unknown because
the PC scanner reported no matching frames. The existing acceptance predicate
already requires the exact fresh payload, A source, TTL 3, and a matching A/B
sequence, so observations from a previous run cannot pass the predicate. A
regression test now explicitly guards against prior-run frames.

The topology log's `ACCEPT` label is not sufficient to identify a text frame:
the receive worker uses the same label for non-text packets such as beacons.
In particular, if the partial `0C13...` source is B, it cannot be an accepted
test TEXT packet under C's configured A-source predicate; it may be a beacon.
The partial `EB34...` source is likewise not classifiable without its omitted
fields. These fragments therefore do not demonstrate receipt of the fresh
message.

`-11` is `-EAGAIN`. The GATT advertising helper returns `-EAGAIN` when an
active GATT connection prevents an advertising-data update, consistent with
the nearby C connection log. This is a beacon-update diagnostic, not proof of
a failed fresh text transmission. The beacon work is scheduled again on its
five-second cadence, and disconnect recovery restarts advertising with the
latest cached payload. No advertising retry/payload-loss defect is established
by this line.

## Narrow change

- `tests/task009_capture_c_console.py` now accumulates serial bytes until an
  actual newline instead of converting timeout-delimited partial reads into
  separate records. It flushes a final unterminated fragment only at shutdown
  and prefixes completed console records with host monotonic milliseconds.
- `tests/ble_pc_controlled_two_hop.py` prints the same monotonic clock on
  fresh-frame, write/echo, and final C-read observations so the combined
  diagnostic output can be correlated with the console capture.
- `tests/test_task009_capture_c_console.py` covers partial lines split across
  reads, multiple records in one read, CRLF, and final-fragment flushing.
- `tests/test_ble_pc_controlled_two_hop.py` verifies that prior-run A/B frames
  with a different payload cannot satisfy this run's fresh-payload check.

No C firmware, BLE timing, packet acceptance, or advertising retry behavior
was changed: this evidence supports improving capture fidelity, but does not
locate a device-side root cause.

## Files changed

- `tests/task009_capture_c_console.py`
- `tests/test_task009_capture_c_console.py`
- `tests/ble_pc_controlled_two_hop.py`
- `tests/test_ble_pc_controlled_two_hop.py`
- `docs/TASK009-LIVE-DIAGNOSTIC-CAPTURE.md`
- `docs/copilot/010-task009-diagnostic-recapture.prompt.md`
- `docs/PROGRESS.md`

## Commands executed and actual results

- `python -m unittest -v test_task009_capture_c_console
  test_ble_pc_controlled_two_hop test_ble_pc_air_message` from the tests
  directory using `C:\zephyrproject\.venv\Scripts\python.exe`: **PASS, 10/10**.
- Python `-m py_compile` for the changed capture/acceptance scripts and their
  tests: **PASS**.
- `west build -b xiao_ble/nrf52840 -d
  C:\zephyrproject\hybrid-mesh-poc\build
  C:\zephyrproject\hybrid-mesh-poc`: **exit 0**; Ninja reported `no work to
  do` because no firmware source changed.
- The IDE test-runner tool did not discover these Python `unittest` files;
  the explicit `unittest` command above ran them successfully.
- No board was accessed, flashed, erased, or reset. No new BLE scan, GATT
  transaction, or physical acceptance test was run.

## Issues and remaining evidence

- This capture cannot establish whether A's fresh text advertisement reached
  the radio, whether the PC scanner missed it, whether B forwarded it, or
  whether C received it.
- Capture timestamps improve the next combined run but do not retroactively
  timestamp this input.
- A repeat using the corrected harness is needed to capture complete C
  topology records and correlate them with timestamped A/B observations.
  Phase 3 acceptance remains open; no live pass is claimed.

## Rollback

Restore the previous versions of
`tests/task009_capture_c_console.py` and
`tests/ble_pc_controlled_two_hop.py`; remove
`tests/test_task009_capture_c_console.py`; revert the additional regression
test in `tests/test_ble_pc_controlled_two_hop.py`; and remove this report,
`docs/copilot/010-task009-diagnostic-recapture.prompt.md`, and the associated
`docs/PROGRESS.md` entry. No firmware or board rollback is needed.

## Next action

Use the corrected diagnostic harness for one explicitly approved live
acceptance retry. Keep the fresh payload unique, retain the separate C console
file and the combined timestamped output, then decide whether further
diagnostics or a firmware fix are supported by complete source/sequence/type
records. Do not flash without fresh user approval.
