# Task 009 live acceptance failure investigation

## Scope and outcome

Investigate the first live A → B → C acceptance failure using the supplied
raw log and current firmware, topology policy, and PC acceptance code. The
three kits were flashed under user approval before this investigation. No
hardware was accessed, no firmware/configuration/source was changed, and no
build, test, reset, or flash was performed.

**Outcome: the failure is confirmed, but the present evidence does not locate
the failure within C's receive path.** The log proves A's GATT write/echo,
observes A's TTL 3 advertisement, and observes B forwarding the same source
and sequence at TTL 2. C's TX characteristic still reads `Ready`. Source
inspection shows that correctly configured C should accept precisely that
TTL 2 packet, publish the payload to its TX value, and not advertise/relay the
text onward. There is no C-side radio/decode/dispatch/GATT diagnostic in the
captured PC log to tell which step did not occur.

Phase 3 acceptance remains **FAIL / OPEN** pending diagnosis and a passing
rerun. This report does not claim that a firmware fix has been found or tested.

## Files examined

- `docs/task009-live-acceptance.log`
- `docs/PROGRESS.md`
- `tests/ble_pc_controlled_two_hop.py` and
  `tests/test_ble_pc_controlled_two_hop.py`
- `src/features/ble_mesh_relay/ble_mesh_relay.c`,
  `ble_mesh_packet.c/.h`, and `ble_mesh_topology.c/.h`
- `src/features/ble_gatt/ble_gatt.c/.h`, `src/main.c`, `Kconfig`, and
  `tests/ble_mesh_packet/src/main.c`
- `docs/PHASE3-CONTROLLED-TWO-HOP.md`,
  `docs/TASK009-INDEPENDENT-REVIEW.md`, and
  `docs/PHASE3-RELIABLE-MESH-TRANSMIT.md`

## Evidence from the supplied log

The Windows PowerShell decode of the attached UTF-16 log reported:

| Observation | Result |
|---|---|
| A GATT write | `HOP0098D7082FA` accepted |
| A source GATT echo | PASS, same payload |
| A on-air text | E5 address, source `eb34ed25a5a4`, sequence 33, TTL 3 |
| B on-air text | C8 address, same source/sequence/payload, TTL 2 |
| C GATT TX read | FAIL, still `Ready` |

The payload is 14 bytes, which fits the firmware's 14-byte mesh-text maximum.
The C-side packet expected after B's forwarding has source
`eb34ed25a5a4`, sequence 33, and TTL 2. The PC log has no C UART output,
packet-receive counter, decode/queue status, topology-role report, or GATT
publication/notification result.

The PC scanner's lack of a C on-air text frame is **not a failure signal**:
role C is explicitly configured not to relay. Only C's GATT TX read is an
end-to-end observation in this run.

## Code-path assessment

### Packet acceptance and topology role

- With `CONFIG_BLE_MESH_TEST_TOPOLOGY=y` and B ID `0c13dae125c8`, the known
  physical mapping is A/Kit 1 = `eb34ed25a5a4`, B/Kit 2 = `0c13dae125c8`,
  and C/Kit 3 = `ee07145a5e02`. The role resolver assigns the remaining known
  ID to C. The matching topology ztests cover this mapping.
- The scan callback accepts manufacturer data with company ID `0xffff`,
  decodes the mesh packet, excludes local-origin frames, and enqueues valid
  packets. With a valid frame from A, C's policy accepts TTL 2 after
  deduplication; it rejects direct TTL 3 before deduplication. The existing
  C ztest exercises those policy results.
- On an accepted text packet, the receive worker publishes the payload to
  GATT and queues a relay only when `should_relay` is true. For C,
  `should_relay` is false. Therefore a C TTL 1 advertisement is neither
  expected nor needed.
- No evident mismatch was found between the observed B frame (A source,
  sequence 33, TTL 2, 14-byte payload) and the C acceptance predicate.
  The source and policy appear consistent; that does not prove that the
  flashed C image has the expected build configuration or runtime role.

### Scan and timing

- The firmware starts `BT_LE_SCAN_PASSIVE_CONTINUOUS`; the receive callback
  does not filter by advertiser address, so it can process B's frame.
- B's successful text advertisement is held for 500 ms before the next
  advertising update. The PC runner sees that B frame, proving it was
  observable by the PC scanner, **not** that C received it.
- The PC runner's default 12-second scan window begins before A is connected
  and written. It captures timestamp-free A/B advertisements, then reads C's
  TX value after scanning ends. The PC scan duration is not a measurement of
  C's own scan callback or of when C accepted a packet.
- The existing relay code reports receive-queue overflow, and the controlled
  topology path reports accept/reject/duplicate outcomes over the firmware
  console. Those C-side logs were not included. Decode failures are silently
  ignored in the scan callback, so absence of a topology result alone cannot
  yet distinguish an RF miss from a malformed/unrecognized frame.

### GATT delivery

- The C receive worker calls the GATT text publisher after policy acceptance.
  That publisher updates the TX characteristic's stored value before
  attempting a notification. A notification subscription is therefore not
  required for the runner's subsequent characteristic read.
- The runner reads C's TX characteristic after the 12-second scan. The
  `Ready` result is evidence that the expected payload was not present in
  that read; notification delivery itself was not tested.
- Both the relay worker and the GATT RX handler currently discard the return
  value from text publication. A notification error can thus go unreported
  at those call sites, but by itself it does **not** explain a stored TX
  value of `Ready`: the value is updated before notification is attempted.
- Confirm the C connection address and TX UUID/read result against the mapped
  Kit 3 identity in the next run. The supplied raw log establishes the
  address labels used by the runner, not C's boot role or the firmware
  attached to that address.

## Prioritized feature-level fixes and tests to propose

Do not change the 500 ms hold or packet predicate based only on this log.
First collect enough C-side evidence to select the narrow fix:

1. **Capture existing C console evidence for one retry.** Record C's boot
   line showing `TEST topology ENABLED role=C`, local ID `ee07145a5e02`, and
   B ID `0c13dae125c8`; then capture the result for source
   `eb34ed25a5a4`, sequence 33 (or the new unique sequence), TTL 2. The
   current firmware already emits role initialization and
   `TEST topology ACCEPT/REJECT/DUPLICATE` diagnostics. Include queue-overflow,
   scan-start, and GATT notification-error lines. This differentiates an
   incorrect image/role from an observed packet rejected by policy and from
   no packet reaching the worker.
2. **Add bounded receive diagnostics in the BLE mesh relay feature if the
   console is insufficient.** Count manufacturer frames, decode failures,
   valid packets, RX queue drops, and topology outcomes in the scan/worker
   path; report snapshots from work context, not with per-advertisement
   logging in the Bluetooth callback. Include local ID/role, packet
   source/sequence/TTL, and a monotonic receive timestamp. Add an explicit
   post-GATT-publish result log that distinguishes the stored TX update from
   notification success/failure. Keep this test-only or rate-limited.
3. **Add feature-level receive/GATT tests.** Extend the mesh relay/topology
   tests to cover the exact A/TTL2/C acceptance, direct A/TTL3 reject-before-
   dedup, duplicate suppression, malformed manufacturer frame, RX queue
   overflow/reporting, one TX update for an accepted message, and no relay
   from C. Add GATT feature tests that separately verify TX readback after
   publishing and notification-subscription/error behavior. Existing topology
   predicate ztests do not execute the full scan → queue → worker → GATT path.
4. **Only if C-side evidence shows a missed frame**, test a longer test-only
   B advertisement hold or bounded repeat/acknowledgement and compare
   timestamped B observations with C receive counters over repeated trials.
   Keep the current routing policy and 500 ms production behavior unchanged
   until this timing hypothesis is demonstrated.
5. **Keep acceptance observations distinct.** Improve the PC runner to emit
   monotonic timestamps and label its C observation as TX readback; require a
   C-side receive/accept record (console or test diagnostic) when claiming
   that C received a packet. For notification acceptance, subscribe on C
   before transmission using a separate central/phone or a suitable
   multi-central test setup, then assert both notification payload and
   characteristic readback. Do not use a C on-air text frame as a delivery
   requirement because C is designed not to relay.

### Decision tree for the next evidence

- C boots with role B, unknown role, or wrong IDs: verify the actual flashed
  test-policy image and B-ID selection before packet-path changes.
- C logs a reject for the expected source/sequence/TTL: fix the diagnosed
  predicate/configuration mismatch and add a regression test.
- C logs an accept but the GATT TX read remains `Ready`: instrument the
  receive-to-publish boundary and verify connected device address, service,
  TX UUID, publication return, and any later writes; add a GATT regression.
- C has no receive outcome while B is observed on air: inspect C's scan
  start/counters, frame decode, receive queue, and RF/timing evidence before
  altering the hold interval.

Any firmware/configuration change and subsequent flash require fresh user
approval. No flashing is proposed as part of this investigation.

## Commands executed and actual results

```powershell
Get-Content -LiteralPath 'C:\zephyrproject\hybrid-mesh-poc\docs\task009-live-acceptance.log' -Encoding Unicode
```

- **Completed.** Decoded the attached UTF-16 log; results are summarized
  above. The logged acceptance command failed with
  `ACCEPTANCE_FAIL: C GATT TX must contain the unique payload`; the saved TX
  value was `b'Ready'`.

Read-only source and Markdown inspection was performed for the files listed
above. No build, unit test, serial connection, or hardware command was run;
no new runtime result is claimed.

## Files changed

- `docs/TASK009-LIVE-ACCEPTANCE-INVESTIGATION.md` — this evidence-based
  investigation and proposed targeted remediation/tests.
- `docs/PROGRESS.md` — records the live failure, its evidence boundary, and
  the diagnosis still required.

## Issues, rollback, and next action

- **Issue:** C-side runtime evidence is absent. The available log cannot
  determine whether C missed the advertisement, rejected/dropped it, used a
  different runtime role/image, or failed between receive and GATT
  publication. Existing source and topology tests do not prove the flashed
  kits executed that path.
- **Rollback:** Documentation-only change. Revert the new investigation
  entry in `docs/PROGRESS.md` and remove this report; no firmware or hardware
  rollback is needed.
- **Next action:** Capture C's existing boot and topology console diagnostics
  during a fresh unique-payload acceptance attempt. Select the narrow
  feature-level fix from those observations, add the corresponding tests,
  build and verify it, and obtain fresh user approval before any firmware
  flash.
