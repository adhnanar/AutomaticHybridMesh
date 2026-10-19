# Task 009 — Phase 3 controlled, instrumented BLE two-hop proof

## Scope and result

Implemented a compile-time gated, test-only A → B → C TEXT topology. It is
disabled by default. Role A is the immutable mesh ID `eb34ed25a5a4`; build
configuration must explicitly assign one of the other known IDs
(`0c13dae125c8` or `ee07145a5e02`) to relay B, and the remaining ID becomes C.
Live BLE manufacturer beacon IDs were independently matched to USB serial
FICR suffixes, resolving the physical mapping: Kit 1 is A
(`eb34ed25a5a4`, BLE E5:12:76:D2:56:EA), Kit 2 is B
(`0c13dae125c8`, BLE C8:51:08:6E:08:C0), and Kit 3 is C
(`ee07145a5e02`, BLE E2:DB:6A:54:BB:71). This establishes which B ID to
select; it does not establish Task 009 firmware deployment or acceptance.

With the test policy enabled:

- Only A may originate text, at TTL 3.
- B accepts only A-sourced TTL 3 text, deduplicates it, publishes it to GATT,
  and queues the TTL 2 relay.
- C rejects direct A TTL 3 before the dedup cache, peer discovery, or GATT TX;
  it accepts A TTL 2 and does not relay it.
- Beacons and discovery retain their existing path. The production path and
  normal mesh relay behavior are unchanged while the Kconfig option is off.

TTL 2 is **not cryptographic proof** that B relayed a packet. This policy only
restricts which configured node is permitted to relay in this three-node test
topology. It does not provide physical RF isolation or establish production
routing.

## Files changed

- `Kconfig` — test-only policy gate, default off, and explicit optional relay-B
  ID choice; enabling the policy without selecting B fails at compile time.
- `src/features/ble_mesh_relay/ble_mesh_topology.h` and
  `src/features/ble_mesh_relay/ble_mesh_topology.c` — immutable-ID role
  resolution and filter-before-dedup policy predicate.
- `src/features/ble_mesh_relay/ble_mesh_relay.c` — test-only origin/receive
  gates and role/frame/relay diagnostics in work context, not the scan callback.
  Rejected direct frames continue before dedup, peer discovery, and GATT TX.
  Existing bounded queues, Task 008 retry/dequeue behavior, 500 ms advertisement
  hold, beacon interval, and reconnect path are retained.
- `CMakeLists.txt` and `tests/ble_mesh_packet/CMakeLists.txt` — compile the
  topology helper into firmware and the packet ztest app.
- `tests/ble_mesh_packet/src/main.c` — deterministic role, allowed/denied TTL,
  A→B→C, direct A→C reject-before-dedup, duplicate, source, and beacon tests.
- `tests/ble_pc_controlled_two_hop.py` — PC acceptance runner requiring a
  matching A TTL 3 advertisement, B TTL 2 advertisement with the same source
  sequence, no C TTL 1 forwarding, and C GATT TX delivery. B and C BLE
  addresses are required arguments; it does not guess the Kit 2/3 mapping.
  Source GATT echo is reported separately and never counts as mesh delivery.
- `tests/test_ble_pc_controlled_two_hop.py` — host tests for positive evidence,
  direct-TTL3-only failure, mismatched sequence, and unexpected C forwarding.
- `docs/PROGRESS.md` — current implementation and evidence status.

## Independent review

**Verdict: PASS for scope, default behavior preservation, and build integration;
runtime C ztests and physical acceptance remain pending.** Reviewed the Kconfig
default and explicit B-ID requirement, test-policy preprocessor gates, role
mapping, packet acceptance order, dedup locking, and transmit queue flow.
Rejected direct A TTL 3 frames return before the shared dedup cache, peer
discovery, or GATT publication. Accepted B frames can only decrement TTL once
and enqueue; C receives with `should_relay == false`. Beacons continue through
the prior dedup/discovery behavior. Diagnostics run in work context and omit
payload contents.

`RELAY_UPDATE_SUCCESS` means the local BLE advertising data update returned
success after the queue dequeue; it is **not** proof that a radio packet was
transmitted or received. The PC acceptance runner is what checks observed
advertisements and destination GATT state.

## Commands and actual results

Executed from `C:\zephyrproject`:

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always
```

- **PASS**, clean default/production configuration, 268/268 build steps.
- Kconfig confirms `CONFIG_BLE_MESH_TEST_TOPOLOGY` is not set.
- FLASH 133,344 B; RAM 30,408 B.
- UF2: 266,752 bytes; SHA256
  `68FCD334628272F092BB094244B67D8B0514B27C88FD736627D96699C3FBA350`.

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-test-topology C:\zephyrproject\hybrid-mesh-poc --pristine=always -- -DCONFIG_BLE_MESH_TEST_TOPOLOGY=y -DCONFIG_BLE_MESH_TEST_B_ID_0C13=y
```

- **PASS**, clean test-policy build, 268/268 steps; Kconfig confirms the test
  policy and `0c13dae125c8` B choice.
- At the time, the physical owner of this B ID had not yet been established.
  Subsequent live identity evidence maps it to Kit 2; the independently
  verified build below uses that now-confirmed mapping.
- FLASH 134,496 B; RAM 30,408 B.
- UF2: 269,312 bytes; SHA256
  `093F3568635F8B60021BA0FC6A140FC5E5DD21D56743F9F5E6E2D1DFB2F54C83`.

The later clean topology build in `build-test-topology-verified` was inspected
after completion. Its captured log shows all 268/268 steps, FLASH 134,496 B,
RAM 30,408 B, and UF2 generation. `zephyr/.config` confirms
`CONFIG_BLE_MESH_TEST_TOPOLOGY=y` and
`CONFIG_BLE_MESH_TEST_B_ID_0C13=y`, which selects the independently mapped
Kit 2 as B. UF2 is 269,312 bytes with SHA256
`093F3568635F8B60021BA0FC6A140FC5E5DD21D56743F9F5E6E2D1DFB2F54C83`.
A non-pristine verification build of this same directory returned exit code
0 (`ninja: no work to do`). The missing-Dtc configure notice in the clean
transcript was non-fatal. These are build/configuration checks only, not
hardware acceptance or flash approval.

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-ztest C:\zephyrproject\hybrid-mesh-poc\tests\ble_mesh_packet --pristine=always
```

- **PASS**, packet/topology ztest application compiled and linked for
  `xiao_ble/nrf52840` (200/200 build steps). This is compile evidence only; it
  does not mean the ztest cases were executed.

```powershell
& C:\zephyrproject\.venv\Scripts\python.exe -m unittest discover -s C:\zephyrproject\hybrid-mesh-poc\tests -p 'test_*.py' -v
& C:\zephyrproject\.venv\Scripts\python.exe -m py_compile C:\zephyrproject\hybrid-mesh-poc\tests\ble_pc_controlled_two_hop.py C:\zephyrproject\hybrid-mesh-poc\tests\test_ble_pc_controlled_two_hop.py
```

- **PASS**, 6 Python unit tests; script and test bytecode compilation passed.

```powershell
& .\.venv\Scripts\west.exe twister -T C:\zephyrproject\hybrid-mesh-poc\tests\ble_mesh_packet -p native_sim
```

- **NOT EXECUTED**: Twister selected the native test scenario but statically
  filtered its only configuration (0 built, 0 executed). No host `gcc`, `clang`,
  or `cl` compiler is available in this environment. The C tests are present
  and the Zephyr ztest app compiles, but their runtime results remain open.

## Physical acceptance and blockers

The BLE acceptance runner was not run. No board was flashed, reset, erased, or
otherwise changed. The firmware on the kits therefore does not contain this
Task 009 policy; there is no new on-air or GATT acceptance evidence from this
implementation. The Kit 2/Kit 3 identity blocker is resolved, but the clean
test-topology build and its configuration/hash have been verified as recorded
above. The separate flash-approval gate remains in force.

After independently reviewing this change and obtaining separate explicit
authorization to flash all three kits, deploy the verified topology image and
run:

```powershell
& C:\zephyrproject\.venv\Scripts\python.exe C:\zephyrproject\hybrid-mesh-poc\tests\ble_pc_controlled_two_hop.py --source E5:12:76:D2:56:EA --relay C8:51:08:6E:08:C0 --destination E2:DB:6A:54:BB:71
```

Run this only after separate explicit authorization to flash all three kits
with the Task 009 firmware. Acceptance fails unless it sees A's TTL 3 frame and
B's TTL 2 frame for the same source sequence, and C GATT TX reads the unique
message. Seeing only direct A TTL 3 is explicitly insufficient. Do not claim
true RF isolation from TTL filtering alone.

## Issues and rollback

- The first clean build exposed that a new app `Kconfig` must source
  `Kconfig.zephyr`; corrected before the final clean builds.
- A string-valued command-line override was rejected by Kconfig quoting. Replaced
  it with an explicit optional choice of known B IDs; test mode without a B
  selection now fails at compile time rather than guessing.
- Host execution of the C ztests and all physical BLE acceptance are blockers,
  not passes. Phase 3 remains incomplete.
- Rollback: remove the test-only Kconfig entries and topology source/header;
  remove their CMake source entries; restore the prior relay receive path and
  packet ztest file; remove the two Task 009 PC test files; and remove this
  report/progress entry. The default production build remains test-policy-off;
  no board rollback is needed.

## Next action

Obtain host-native C test execution or run the ztests in an approved test
environment. Keep Phase 3 acceptance open and do not flash until separate
explicit user authorization. After authorization, run the copy-ready BLE
acceptance command with controlled direct-link exclusion; require B's on-air
TTL 2 and C's GATT delivery.
