# Task011 — Rigorous BLE two-hop RF-isolation proof

**Status: procedure and PC controls prepared; no physical test performed.**
Task010's controlled two-hop acceptance passed 2/2, but did not exclude a
direct A-to-C RF path. This task prepares a repeatable test of that missing
claim using the existing three XIAO nRF52840 Sense kits. It does not change
firmware, access USB devices, flash, reset, power down, or physically move
anything.

## Audit and what the current evidence means

- In [`ble_mesh_topology.c`](../src/features/ble_mesh_relay/ble_mesh_topology.c),
  C is configured to accept A-source TEXT only at TTL 2; B accepts A-source
  TEXT at TTL 3 and relays it. The topology filter rejects before deduplication.
- In [`ble_mesh_relay.c`](../src/features/ble_mesh_relay/ble_mesh_relay.c),
  the scan callback explicitly discards advertiser address and RSSI. The
  firmware diagnostics include role, packet type, mesh source, sequence, TTL,
  board uptime and API result, but not the radio advertiser identity.
- In [`ble_gatt.c`](../src/features/ble_gatt/ble_gatt.c), a GATT connection
  increments the active connection count; advertising restarts after
  disconnect. The PC script keeps its scanner active through the A write and
  the evaluator requires the unique A frame after the A GATT disconnect
  marker. C pre/post GATT reads are outside the packet-observation interval.
- A emits unique TEXT with source ID `eb34ed25a5a4`, a 16-bit sequence and
  TTL 3. B accepts A/TTL 3, queues the same source/sequence/payload at TTL 2,
  and advertises it. C accepts A-origin TEXT only at TTL 2; a direct A/TTL 3
  packet is rejected before deduplication and GATT publication.
- The scan callback receives BLE advertiser address and RSSI, but
  `scan_receive()` explicitly discards both. The queued packet contains the
  original mesh source ID, sequence and TTL, not the BLE advertiser address.
  Therefore C's `RX_ACCEPT` cannot itself identify which radio advertised the
  packet. In a three-node setup, matching C's unique source/sequence/TTL 2
  acceptance with B's receive/relay console events and B's observed TTL 2
  advertisement is strong correlated evidence, not authenticated provenance.
- The C GATT TX value proves that the application published/stored the
  payload. It does not prove whether C heard A directly. In the test topology,
  direct A/TTL 3 should be rejected, while B/TTL 2 is accepted.
- The Task010 PC acceptance observes A/TTL 3, B/TTL 2 with the same source,
  sequence and payload, and C GATT TX. It did not establish that C could not
  hear A. Task010's console lines are not sufficiently intact to infer C's
  receive path from fragments.
- Task010 firmware remains configured for continuous passive BLE scanning
  and connectable advertising. A PC GATT connection temporarily suspends that
  board's connectable advertising; firmware restarts advertising after
  disconnect. The new runner keeps scanning during A's write, logs A's
  connect/disconnect interval, and waits for fresh on-air evidence after
  disconnect. It reads C's GATT value before and after the RF observation
  window, not during it. The C connection can pause C advertising, but it is
  outside the observation window and does not intentionally stop C's firmware
  mesh scan.

## Files changed and PC-only controls

- `tests/ble_pc_controlled_two_hop.py`: added `--mode baseline` alongside the
  existing relay-positive mode. Baseline requires B to be absent from BLE
  discovery (manually powered off), requires the unique A/TTL 3 frame on air,
  rejects a matching B/TTL 2 frame, and requires C's GATT TX value to remain
  unchanged. The result is deliberately named
  `B_OFF_PC_BASELINE=PASS RF_ISOLATION=NOT_ESTABLISHED_CHECK_C_USB_EVIDENCE`.
  This PC check alone is never called isolation proof. The script does not
  turn off, reset, or otherwise control any board.
- `tests/task009_capture_both_consoles.py`: generalized the existing
  diagnostic capture runner to timestamp A/C serial logs for baseline and
  A/B/C for relay trials. It refuses to start the BLE trial if any requested
  serial capture fails; unique run IDs and exclusive log creation prevent
  accidental evidence overwrite. It captures raw PC output as well as console
  lines, emits integrity summaries, and runs the evidence evaluator below.
  DTR/RTS are held low before opening serial ports to avoid reset signaling;
  no reset/power command is issued.
- `tests/test_ble_pc_controlled_two_hop.py`: negative/positive assertions for
  direct-only two-hop rejection, the baseline's required A evidence,
  unchanged C value, and rejection of a B relay or C value change.
- `tests/task011_isolation_evidence.py` and
  `tests/test_task011_isolation_evidence.py`: offline, unit-tested
  cross-checks for exact A/B/C source/sequence/TTL evidence, C heartbeat
  deltas, complete C receive records, sensitivity control, B relay path and
  direct-frame rejection during the positive trial. The capture wrapper exits
  nonzero if evidence is incomplete or fails.
- `docs/PROGRESS.md`: current outcome and next physical step.

## Test phases and acceptance thresholds

Keep A and C in fixed positions through all phases. Position B between them
only after the B-off isolation baseline passes. Keep Android/nRF Connect
disconnected from all three kits during each trial so it cannot introduce
another GATT connection or advertising pause. Run at least **five valid
trials in each phase**, each with a newly generated payload and distinct
sequence. Save every PC and serial log; do not count a failed or incomplete
capture as a passing repeat.

### 1. Direct-path sensitivity control (B physically off)

Put A and C close together with an unobstructed path. Run the baseline command
below for at least five unique messages. For each message, require:

1. PC sees A's fresh TTL 3 advertisement for that exact payload after A's
   GATT disconnect; a pre-connection or stale A frame cannot satisfy this.
2. C console has an intact `RX_REJECT role=C type=TEXT` for A's exact source
   ID and sequence, TTL 3. C's valid `HB r=C` text-decode counter increases
   across the trial.
3. C GATT TX remains at its exact pre-trial value.
4. C heartbeat indices and counters are monotonic. The captured interval has
   no missing heartbeat indices and no damaged topology/heartbeat records.

This positive control proves the C receiver/log path can detect a direct
frame and that C's policy rejects TTL 3. If it fails, the isolation phase is
**inconclusive**; do not treat an empty baseline as proof.

### 2. B-off A-to-C isolation baseline

Physically power B off (for example, disconnect B's USB cable yourself) and
leave A/C in their intended test positions. The PC runner also checks that B's
known BLE address is absent from discovery; this is a guard, not proof that B
is powered off. For at least five unique messages, require all of:

1. A's matching TTL 3 payload is observed on air.
2. B is not discoverable and no matching B-address TTL 2 frame is observed.
3. C has complete valid heartbeat records spanning the interval from before
   A's GATT connection through after the fresh A frame; decoded-text
   counters do not increase during the trial. There is no exact C
   `RX_REJECT`, `RX_ACCEPT` or `RX_DUPLICATE` for the test's A
   source/sequence.
4. C GATT TX remains exactly equal to the pre-trial value.
5. No missing C heartbeat indices or damaged relevant C receive/heartbeat
   records occur in the capture interval.

Any matching C reject, an increase in C's decoded-text counter, changed GATT
value, missing/corrupt C diagnostic coverage, or failure to observe A makes
that repeat a failure or **inconclusive**, never a pass. The text counter is
cumulative over decoded radio frames (before topology policy); with B off and
unique payloads, a zero delta supports no detected direct A TEXT. It is not
an RF energy detector, and loss of console evidence prevents a claim.

### 3. B-on relay positive control (keep A/C placement unchanged)

Reconnect B manually and place it between A and C. Repeat at least five times
with unique payloads. Every counted repeat must have:

1. PC-observed A TTL 3 and B TTL 2 with identical A source ID, sequence and
   exact payload.
2. Intact B console `RX_ACCEPT role=B` for that source/sequence at TTL 3 and
   `RELAY_QUEUE_OK` at TTL 2.
3. Intact C console `RX_ACCEPT role=C` and `GATT_PUBLISH rc=0` for that same
   source/sequence at TTL 2; final C GATT TX equals the payload.
4. No C `RX_REJECT` for the same source/sequence at TTL 3. Such a reject
   means C also decoded a direct A frame during the positive phase; the
   delivery may still be via B, but the physical-isolation criterion failed.
5. Complete, monotonic B/C heartbeat coverage; no missing heartbeat indices
   or damaged relevant receive records.

Accept the **physically isolated two-hop PoC** only if the direct-path
sensitivity control passes, all five (or more) B-off isolation baselines pass,
and all five (or more) B-on relay trials pass while A/C placement is unchanged.
Any baseline failure, inconclusive capture, or direct C/TTL 3 reject during a
relay trial means isolation is not accepted. Report the actual counts and
exceptions; do not average away a failed repeat.

## Placement / attenuation

Preferred: use a screened 2.4 GHz enclosure or RF test chamber for C, with
properly shielded USB feed-throughs, and put B outside between A and C. Keep
the enclosure, cables, board orientation and A/C locations unchanged between
baseline and relay phases. A calibrated 2.4 GHz attenuator/chamber is better
than distance alone if suitable RF access is available; do not modify antenna
feeds or boards for this task.

If no enclosure is available, first demonstrate the direct-path sensitivity
control, then increase A-C separation and use intervening walls/obstructions
while holding A and C fixed. Put B where both A-B and B-C links work. Distance,
walls, human bodies, a drawer, or an uncharacterized metal container alone do
not certify isolation. Only the repeated C-side no-decode baseline under the
same geometry, with a valid sensitivity control, supports this PoC-level
claim.

## Copy-ready commands (run later, only with physical-test go-ahead)

Each run ID must be unique. These commands open read-only USB console capture
and BLE GATT/scan sessions; they do **not** reset, power down, flash, or move
boards. Baseline requires B already powered off by the operator. Do not run
these now; the physical test was not performed for Task011.

```powershell
& C:\zephyrproject\.venv\Scripts\python.exe C:\zephyrproject\hybrid-mesh-poc\tests\task009_capture_both_consoles.py --mode baseline --control sensitivity --a-port COM14 --c-port COM16 --scan-seconds 20 --run-id task011-sensitivity-01
& C:\zephyrproject\.venv\Scripts\python.exe C:\zephyrproject\hybrid-mesh-poc\tests\task009_capture_both_consoles.py --mode baseline --control isolation --a-port COM14 --c-port COM16 --scan-seconds 20 --run-id task011-isolation-01
& C:\zephyrproject\.venv\Scripts\python.exe C:\zephyrproject\hybrid-mesh-poc\tests\task009_capture_both_consoles.py --mode relay --control relay --a-port COM14 --b-port COM15 --c-port COM16 --scan-seconds 25 --run-id task011-relay-01
```

Repeat the relevant command with unique run IDs until there are at least five
valid sensitivity, isolation and relay trials. The runner logs both UTC and
host monotonic nanoseconds; firmware records carry board-local `uptime_ms`.
Do not compare those independent clocks as if synchronized. Correlate on the
unique payload, source ID, sequence, event order and per-board uptime.

## Acceptance and limitations

No Task011 physical command has been run; there are no new BLE, isolation,
relay or delivery results to claim. Existing Task010's 2/2 acceptance remains
valid only as the previously reported controlled three-board relay result,
not as physically isolated two-hop acceptance. This procedure does not prove
cryptographic relay provenance, immunity to a fourth transmitter, a calibrated
RF attenuation value, reliability at scale, LoRa routing, hybrid routing or
Android notification behavior. Firmware has no advertiser-origin metadata;
adding that would require a separate firmware change, clean build, approval
and deployment and is intentionally out of Task011 scope.

## Implementation record

- Scope: added a PC-observed B-off baseline mode, timestamped A/B/C console
  capture runner, and offline evidence evaluator; documented source audit,
  repeatable positive/negative controls, criteria, limitations and later
  physical assistance.
- Files: `tests/ble_pc_controlled_two_hop.py`,
  `tests/test_ble_pc_controlled_two_hop.py`,
  `tests/task009_capture_both_consoles.py`,
  `tests/task011_isolation_evidence.py`,
  `tests/test_task011_isolation_evidence.py`, this file and
  `docs/PROGRESS.md`.
- Commands executed and actual results:
  - `& C:\zephyrproject\.venv\Scripts\python.exe -m unittest discover -s C:\zephyrproject\hybrid-mesh-poc\tests -p "test_*.py"` — PASS, 27 tests.
  - `& C:\zephyrproject\.venv\Scripts\python.exe -m py_compile C:\zephyrproject\hybrid-mesh-poc\tests\ble_pc_controlled_two_hop.py C:\zephyrproject\hybrid-mesh-poc\tests\test_ble_pc_controlled_two_hop.py C:\zephyrproject\hybrid-mesh-poc\tests\task009_capture_both_consoles.py C:\zephyrproject\hybrid-mesh-poc\tests\task011_isolation_evidence.py C:\zephyrproject\hybrid-mesh-poc\tests\test_task011_isolation_evidence.py` — PASS, exit 0.
  - `task009_capture_both_consoles.py --help` and
    `ble_pc_controlled_two_hop.py --help` — PASS, usage printed without
    connecting to or enumerating any device.
  - No firmware sources changed; no firmware build was run. No live test,
    USB serial capture, BLE scan, device connection, reset, power operation
    or flash was run.
- Issues: USB console corruption is a known risk; the strict evidence gate
  rejects incomplete/damaged relevant captures rather than treating them as
  proof. The baseline PC script alone cannot establish isolation.
- Rollback: revert only this task's changes to the two Python scripts, their
  focused test additions, this document and the Task011 progress entry. No
  firmware or device state needs rollback.
- Next action: obtain physical assistance to perform the later test steps
  below. Do not run until the operator has confirmed the requested setup.

## Physical assistance required later

Ask the operator to confirm Android/nRF Connect is disconnected; identify
COM14=A, COM15=B and COM16=C by USB serial; place A/C and choose an RF
enclosure/attenuation or fixed obstructed geometry; manually power off B for
the direct-sensitivity and five baseline trials; manually restore B and place
it between unchanged A/C for five relay trials; run each command above with
unique IDs; and preserve all PC/A/B/C logs. No reset or firmware change is
requested.
