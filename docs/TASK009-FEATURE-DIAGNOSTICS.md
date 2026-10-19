# Task 009 feature-level diagnostics and focused tests

## Scope and outcome

Added test-topology-only BLE mesh diagnostics that correlate TEXT receive,
relay queue, advertising-data API, and GATT publish outcomes by packet type,
source, sequence, TTL, and firmware uptime. RX decode and queue-drop counters
are atomic and reported from receive work context; the Bluetooth callback
does not log each advertisement. Accepted beacon records were removed from
topology diagnostics to avoid per-beacon log traffic.

The advertising event is explicitly named `ADV_DATA_API_OK` or
`ADV_DATA_API_FAIL`: it reports the result of updating advertising data and
does not establish that a packet went over the air. The one-second retry,
500 ms text hold, five-second beacon cadence, topology policy, and production
default-off configuration remain unchanged.

Added focused C tests for retry queue retention/removal, receive diagnostics,
and GATT TX storage, plus host parsing tests that accept only complete C
receive records. The existing topology-policy tests remain the source of
coverage for B A/TTL3 to TTL2, C A/TTL2 acceptance without relay, direct A/TTL3
rejection, deduplication, and beacon behavior; they were not duplicated.

**Outcome:** host tests and bytecode compilation passed. Clean default and
topology-enabled firmware builds passed, and the C ztest application compiled
and linked. Twister selected no native_sim test configurations, so no C ztest
case was runtime-executed. No board was flashed, reset, or otherwise accessed;
no live BLE behavior or delivery is claimed.

## Files changed

- `CMakeLists.txt` — included the relay queue, RX diagnostic counter, and
  GATT value helper sources in the firmware.
- `src/features/ble_mesh_relay/ble_mesh_relay.c` — added correlated
  topology-gated receive, queue, advertising API, and GATT publish records;
  added deferred counter snapshots; retained retry rate limiting.
- `src/features/ble_mesh_relay/ble_mesh_relay_queue.c` and `.h` — factored
  queue-head advertisement update so failed updates retain the head and only
  successful updates remove it.
- `src/features/ble_mesh_relay/ble_mesh_rx_diagnostics.c` and `.h` — added
  atomic manufacturer decode-failure and RX queue-drop counters.
- `src/features/ble_gatt/ble_gatt.c` — routes TX value storage through the
  feature helper before attempting notification.
- `src/features/ble_gatt/ble_gatt_value.c` and `.h` — added bounded GATT TX
  value storage.
- `tests/ble_mesh_packet/CMakeLists.txt` and `tests/ble_mesh_packet/src/main.c`
  — compiled the feature helpers into the existing ztest app and added three
  focused C tests.
- `tests/task009_capture_c_console.py` and
  `tests/test_task009_capture_c_console.py` — added strict complete-record
  parsing and joined/truncated-record tests.
- `docs/PROGRESS.md` — recorded this implementation and verification.
- `docs/TASK009-FEATURE-DIAGNOSTICS.md` — this task report.

## Commands executed and actual results

1. `Set-Location C:\zephyrproject; & .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-task009-diagnostics-clean C:\zephyrproject\hybrid-mesh-poc --pristine=always`
   - **PASS**, clean production-default build.
   - Generated configuration has `# CONFIG_BLE_MESH_TEST_TOPOLOGY is not set`.
   - FLASH 133,472 B; RAM 30,408 B; UF2 267,264 B.
   - `zephyr.uf2` SHA-256:
     `20039A78FB135B567667EC2BB2A3CC836710C092974B9021CE4280428CA47AD0`.
2. `Set-Location C:\zephyrproject; & .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-task009-diagnostics-topology C:\zephyrproject\hybrid-mesh-poc --pristine=always -- -DCONFIG_BLE_MESH_TEST_TOPOLOGY=y -DCONFIG_BLE_MESH_TEST_B_ID_0C13=y`
   - **PASS**, clean topology-enabled build.
   - Generated configuration confirms `CONFIG_BLE_MESH_TEST_TOPOLOGY=y` and
     `CONFIG_BLE_MESH_TEST_B_ID_0C13=y`.
   - FLASH 134,916 B; RAM 30,408 B; UF2 270,336 B.
   - `zephyr.uf2` SHA-256:
     `04170F01F5BE435219D12E69702DCEC60CA3909CC604AF1ACD0E7CAC87B7D142`.
3. `Set-Location C:\zephyrproject; & .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-task009-ztest C:\zephyrproject\hybrid-mesh-poc\tests\ble_mesh_packet --pristine=always`
   - **PASS**, clean ztest app compile/link, 203/203 build steps; `CONFIG_ZTEST=y`.
   - FLASH 77,828 B; RAM 16,840 B. This is compile evidence, not test execution.
4. `& C:\zephyrproject\.venv\Scripts\python.exe -m unittest discover -s C:\zephyrproject\hybrid-mesh-poc\tests -p 'test_*.py' -v`
   - **PASS**, 11/11 tests.
5. `& C:\zephyrproject\.venv\Scripts\python.exe -m py_compile C:\zephyrproject\hybrid-mesh-poc\tests\task009_capture_c_console.py C:\zephyrproject\hybrid-mesh-poc\tests\test_task009_capture_c_console.py`
   - **PASS**.
6. `Set-Location C:\zephyrproject; & .\.venv\Scripts\west.exe twister -T C:\zephyrproject\hybrid-mesh-poc\tests\ble_mesh_packet -p native_sim -v`
   - Command completed, but Twister statically filtered the only configuration:
     0 test configurations executed and 0 C cases run.
7. `Set-Location C:\zephyrproject; & .\.venv\Scripts\west.exe twister -T C:\zephyrproject\hybrid-mesh-poc\tests\ble_mesh_packet -p native_sim -K -v --report-filtered`
   - Same outcome with forced platform selection: 0 configurations executed
     and 0 C cases run.

No flash, erase, reset, serial access, or live BLE test was performed.

## Review notes and issues

- `CONFIG_BLE_MESH_TEST_TOPOLOGY` remains `default n`; the production build
  confirmed the option is unset.
- A first ztest configuration attempt assigned project-only topology symbols
  in the standalone test app's `prj.conf`; Kconfig correctly rejected those
  undefined symbols. The assignments were removed because the helper unit
  tests do not require the routing-policy Kconfig, and the pristine ztest
  compile then passed.
- Twister's static filter prevented C ztest runtime execution, including when
  forced. Treat the three new C tests as present and compile-verified, not
  runtime-passed.
- Firmware `printk` output can still interleave across contexts; this change
  does not alter the global logging/console configuration. The host parser
  rejects joined and incomplete topology records rather than reconstructing
  them. If a controlled future capture still interleaves complete-device
  output, investigate feature-level serialization and console throughput
  before changing the firmware-wide logging system.
- An advertising-data API success is not independent over-air evidence.
  Physical radio delivery and GATT reception remain unverified by these
  builds and unit tests.

## Rollback

Restore the relay worker to its prior inline advertise/queue-removal path and
remove `ble_mesh_relay_queue.[ch]` and `ble_mesh_rx_diagnostics.[ch]` along
with their CMake entries. Restore the previous GATT TX buffer handling and
remove `ble_gatt_value.[ch]` and its CMake entry. Remove the added C and host
tests/parser, then remove this report and its latest-verification entry from
`docs/PROGRESS.md`. No hardware rollback is needed because no device was
changed.

## Next action

Use these diagnostics in a new, unique-payload physical Task 009 capture only
after explicit user approval for the required board interaction. Flashing
requires separate explicit approval. Preserve the distinction between
`ADV_DATA_API_OK` and actual on-air observation; require complete C
`RX_ACCEPT` TEXT identity records and C GATT TX readback before asserting
delivery. Phase 3 remains open.
