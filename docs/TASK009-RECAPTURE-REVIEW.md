# Task 009 corrected recapture review

## Scope and outcome

Review the corrected Task 009 capture against the B advertisement scheduler,
the C receive/GATT path, and the console capture format. Identify the smallest
feature-level diagnostics and tests that would distinguish the remaining
failure modes.

**Outcome:** the fresh A text frame was observed on air; the same frame was
not observed from B, and C's TX value remained `Ready`. The capture does not
show whether B received or accepted A's frame, whether an advertisement-data
update succeeded, or whether C received the corresponding B frame. The host
line buffer did not repair the interleaved C console records. No firmware
change is justified by this evidence alone.

Phase 3 remains **OPEN**. No board was accessed, reset, flashed, or erased.

## Exact evidence

Input: [task009-recapture-verified.log](./task009-recapture-verified.log)

| Observation | Evidence |
|---|---|
| A GATT write and source echo | PASS for `HOP00971595B15` at monotonic ms `318772281` and `318772296`. |
| A fresh source advertisement | E5 / source `eb34ed25a5a4`, sequence `154`, TTL `3`, exact payload, at `318775484`. |
| B forwarding | No matching C8/B observation at TTL `2` for A sequence `154` and the unique payload. This is absence from the PC scan, not proof B did not receive the packet or call the advertising API. |
| C TX readback | FAIL; value remained `Ready` at `318794296`. |
| C console | At `318792875`, the capture contains concatenated fragments beginning with two `TEST topology ACCEPT role=C` records, then a GATT connection line. The first source is truncated at `0C13DAE125`; the second at `EB34ED25A5`. No complete type, source, sequence, TTL, or record boundary is available. |
| Advertising update error | C logs `beacon advertisement update failed: -11` at `318769406`, after a GATT connection at `318766640`. The helper returns `-EAGAIN` while a GATT connection is active. The line concerns a beacon update, not proof of a text-forwarding failure. |

The C-side topology logger uses `ACCEPT` for both policy-accepted TEXT and
deduplicated non-TEXT packets (including beacons), and does not log packet
type. A complete record showing source B accepted by C would therefore be a
beacon, not an accepted test TEXT frame: the C TEXT policy only accepts source
A at TTL 2. The truncated source-A record cannot be classified. Neither
fragment establishes receipt of A sequence 154.

The previous capture's lack of an A on-air observation applies only to that
earlier run. It is superseded for this recapture by the complete A TTL3
observation above. This run does not contain matching B TTL2 evidence.

## Source-path assessment

### B emission scheduling

In [ble_mesh_relay.c](../src/features/ble_mesh_relay/ble_mesh_relay.c), an
accepted B TEXT packet is copied, decremented from TTL 3 to TTL 2, and queued.
A non-empty relay queue schedules the advertising work immediately. The
worker peeks the queue, calls `ble_gatt_update_mesh_advertisement()`, and
removes the packet only after a successful update. On update failure, it
retains the queued packet and schedules a one-second retry. An active GATT
connection makes the GATT helper return `-EAGAIN`; the cached mesh frame is
retained and advertising restart is handled after disconnect.

The same path holds a successfully updated text advertisement for 500 ms
before returning to the five-second beacon cadence. Consequently, an absent
PC-side B observation could be caused by no B receive/accept, a queue or
update/retry outcome, or a scanner miss during the brief text-advertisement
window. Source inspection alone cannot select among them.

Existing test-topology diagnostics can report B `ACCEPT`, `RELAY_ATTEMPT`,
queue failure, and `RELAY_UPDATE_SUCCESS`, but the successful update means
advertising data was updated; it is not an independent on-air observation.
The records omit packet type and firmware uptime, and queue admission/retry
events do not consistently carry the packet identity. No B console from this
run is included.

### C receive and GATT

In [ble_mesh_relay.c](../src/features/ble_mesh_relay/ble_mesh_relay.c), the
scan callback silently ignores manufacturer data that fails mesh decode. It
reports receive-queue overflow and work-submission failure, but those records
do not include packet identity. The worker logs topology outcomes only after
queueing and does not distinguish TEXT from beacon in the record. Accepted
TEXT is then passed to `ble_gatt_publish_text_message()`; its return value is
discarded at this call site.

In [ble_gatt.c](../src/features/ble_gatt/ble_gatt.c),
`ble_gatt_publish_text_message()` stores the TX value before attempting a
notification. Therefore an accepted TEXT followed by a notification error
should still be visible in a subsequent TX characteristic read. A `Ready`
read means this capture did not observe the expected stored value;
notification status alone would not explain it.

### Console integrity

The host capture helper
[task009_capture_c_console.py](../tests/task009_capture_c_console.py) now
accumulates serial bytes until a newline, so a host `readline()` timeout no
longer turns one split record into multiple records. The new capture still
contains joined/interleaved
firmware-originated output. Host buffering cannot reconstruct dropped,
interleaved, or absent device-side bytes. The evidence does not determine
whether the cause is concurrent `printk` output, UART/USB throughput, or
another console transport issue; those remain hypotheses.

## Minimal diagnostics and tests proposed

1. **Add correlated, test-topology-only B state records** in the BLE mesh relay
   feature for TEXT receive/accept, relay queue outcome, and each advertising
   update result. Include role, event/stage, packet type, source, sequence,
   TTL, and firmware uptime. Keep callback logging out of the scan callback
   and rate-limit retries. Label a successful advertising-data update as an
   API update, never as proof of an over-air transmission.
2. **Make C receive outcomes classifiable.** Add packet type and uptime to
   topology records. Count manufacturer-data decode failures and RX queue
   drops in the callback, then emit bounded counter snapshots from work
   context. For an accepted TEXT, log the GATT publish return with the same
   source/sequence/TTL. Do not print one record for every beacon.
3. **Test the B scheduler with a fake advertising-update result.** Assert
   that `-EAGAIN` keeps the queue head and retries, success removes that
   packet once, and queue-full/update-failure diagnostics preserve packet
   identity. Keep the existing 500 ms hold and five-second beacon cadence
   unchanged unless measurements implicate timing.
4. **Test C's receive-to-GATT boundary separately from the policy.** Existing
   ztests already cover B accepting A/TTL3 and decrementing to TTL2, C
   accepting A/TTL2 without relaying, direct A/TTL3 rejection before dedup,
   duplicate suppression, and beacon handling. Add focused tests for
   malformed manufacturer decode and queue-drop accounting, and verify that
   accepted TEXT updates GATT TX even when notification delivery is
   unavailable. Do not duplicate the existing policy tests.
5. **Make the host accept only intact diagnostics.** Add parser tests that
   reject merged/truncated topology records and require a complete
   type/source/sequence/TTL tuple before treating a C record as evidence.
   Keep the independent PC observations (A TTL3, B TTL2, C TX readback)
   separate. If the device still emits corrupted console lines under a
   controlled diagnostic burst, test deferred/serialized feature diagnostics
   and console throughput before changing transport or routing behavior.

No radio hold-time, topology predicate, retry interval, or production logging
change is recommended until the missing device-side stages are captured.

## Files changed

- [TASK009-RECAPTURE-REVIEW.md](./TASK009-RECAPTURE-REVIEW.md) — evidence,
  source-path analysis, and
  proposed minimal diagnostics/tests.
- [PROGRESS.md](./PROGRESS.md) — corrected recapture status and next
  diagnostic step.
- [011-task009-feature-diagnostics.prompt.md](./copilot/011-task009-feature-diagnostics.prompt.md) — copy-ready,
  no-flash implementation task for the next evidence-gathering change.

No firmware, configuration, or test source changed.

## Commands executed and actual results

- Read-only inspection of the captured log, B relay/GATT sources, C packet
  receive/GATT sources, current Python capture and acceptance tests, existing
  C ztests, and related progress/task documentation: **completed**.
- No build or test command was run; this review changed documentation only.
- No serial connection or live BLE test was run.
- No flash, erase, or reset was performed.

## Issues, rollback, and next action

- **Issue:** no B console evidence and no intact C packet record. The exact
  point of loss remains undetermined. The scan's missing B frame and C's
  `Ready` TX read confirm acceptance failure, not a root cause.
- **Rollback:** documentation-only; remove this report and prompt, and remove
  this task's latest-verification bullet from `docs/PROGRESS.md`.
- **Next action:** use the copy-ready prompt to add bounded diagnostics and
  focused tests, then build/test those changes. Use a new unique-payload live
  capture only with user approval for physical interaction; do not flash
  without fresh explicit approval.
