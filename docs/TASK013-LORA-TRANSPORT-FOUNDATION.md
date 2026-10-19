# TASK013 — Phase 4 LoRa transport foundation

Date: 2026-10-19. **Status: isolated software foundation implemented and
Zephyr-compiled; no LoRa hardware or radio validation.**

## Scope and hardware status

This task implements only a small, driver-independent frame transport
interface, a bounded in-memory mock, and a caller-shared relay-admission
helper for future hybrid routing. It does not select a transceiver, configure
SPI/devicetree, link LoRa code into the application, or change BLE behavior.
The confirmed hardware is three Seeed XIAO nRF52840 Sense boards; no built-in
LoRa radio or external LoRa module is confirmed.

No physical devices were accessed, powered, reset, flashed, erased, or
otherwise changed. No custom phone app is planned; Android nRF Connect via
BLE GATT remains the phone interface.

## Existing packet, discovery, and relay audit

- `ble_mesh_packet` uses an existing 10-byte header: type (1 byte), source ID
  (6 bytes), little-endian sequence (2 bytes), and TTL (1 byte), followed by
  up to 14 text bytes (24-byte maximum frame). This format is reused without
  changing its encoding. It has no version field, authentication, or
  cryptographic integrity protection.
- BLE beacons have no payload, TTL 1, and are scheduled every five seconds.
  The BLE peer table keeps up to eight peers and expires entries after
  60 seconds. Peer events are published through the existing GATT path.
- BLE text relay uses source ID + sequence dedupe with a 16-entry cache and
  30-second duplicate window; packet TTL is bounded to 1..3 and the relay
  helper decrements it before forwarding. Existing queue retry behavior
  retains a BLE text packet on advertisement update failure.
- The BLE receiver currently owns a private static dedupe cache. Therefore,
  merely adding this LoRa feature cannot provide live cross-radio duplicate
  suppression. The new admission helper accepts a caller-supplied shared
  cache and the unit-test case models BLE and LoRa ingress using that same
  cache. **Live BLE/LoRa cache sharing remains unimplemented and no hybrid
  routing guarantee is claimed.**

## Files changed

- Added `src/features/lora_transport/lora_transport.h` and `.c`: driver
  `send_frame` operation plus packet encode/decode adapters reusing the
  existing packet serialization.
- Added `src/features/lora_transport/lora_transport_mock.h` and `.c`: fixed
  four-frame in-memory mock with deterministic send-error injection.
- Added `src/features/lora_transport/lora_mesh_relay_policy.h` and `.c`:
  caller-cache-based duplicate admission and TTL decrement for unique,
  forwardable text packets. It is not called by the BLE application.
- Added `tests/lora_transport/` Zephyr ztest application and five test cases
  for serialized round-trip, send failure propagation, duplicate cross-radio
  admission with a shared cache, TTL decrement, and non-forwardable TTL.
- Added `docs/copilot/013-phase4-lora-module-confirmation.prompt.md`.
- Updated `docs/PROGRESS.md`.
- Left root `CMakeLists.txt`, BLE sources, BLE configuration, active routing,
  and devicetree unchanged. The new feature is compiled only by its
  standalone test application; the default application does not link it.

## Verification commands and actual results

Run from `C:\zephyrproject`:

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-task013-default C:\zephyrproject\hybrid-mesh-poc --pristine=always
```

**PASS, exit 0.** Fresh default firmware build; FLASH 134,072 bytes, RAM
30,472 bytes; UF2/ELF generated. This validates that the unchanged default
application still builds; the new LoRa feature is not part of that image.

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-task013-topology C:\zephyrproject\hybrid-mesh-poc --pristine=always -- -DCONFIG_BLE_MESH_TEST_TOPOLOGY=y -DCONFIG_BLE_MESH_TEST_B_ID_0C13=y
```

**PASS, exit 0.** Fresh topology-enabled firmware build; FLASH 136,108 bytes,
RAM 30,536 bytes; UF2/ELF generated. No routing behavior was changed.

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-task013-lora-ztest C:\zephyrproject\hybrid-mesh-poc\tests\lora_transport --pristine=always
```

**PASS, exit 0, 202/202 build steps.** The LoRa ztest source compiled and
linked for the XIAO nRF52840 target (FLASH 69,416 bytes, RAM 16,504 bytes).
This is compile/link evidence only, not a test execution or board validation.

```powershell
& .\.venv\Scripts\west.exe twister -T C:\zephyrproject\hybrid-mesh-poc\tests\lora_transport -p native_sim -v
& .\.venv\Scripts\west.exe twister -T C:\zephyrproject\hybrid-mesh-poc\tests\lora_transport -p native_sim -j 1 -v
```

**NOT RUN: both attempts exited 1 before test discovery/execution** with
`FileNotFoundError: [WinError 2]` while Twister attempted to create/access its
Python multiprocessing manager named pipe. No ztest case ran; `-j 1` did not
resolve the host issue.

```powershell
& C:\Users\adhna\AppData\Local\Programs\Python\Python312\python.exe -m unittest discover -s C:\zephyrproject\hybrid-mesh-poc\tests -p 'test_*.py' -q
```

**PASS, 31 Python tests, exit 0.** No Python sources were changed.

## Limitations and independent review

- ztest C sources passed compile/link but could not be executed in this
  environment; all five behaviors are pending runtime test evidence.
- The mock is in-memory simulation only. No LoRa transceiver, antenna, SPI
  pins, RF packet, discovery, range, throughput, or radio reliability was
  tested.
- No exact module part, board wiring, supply, operating region, frequency,
  transmit power, duty-cycle, or antenna is confirmed. No radio driver or
  unsupported devicetree resources are enabled.
- The shared-cache test demonstrates the helper's contract, not current
  application integration. The existing BLE routing path and its private
  cache remain unchanged. Any future cache shared across concurrent BLE and
  LoRa callbacks must be synchronized by the caller.
- Independent code/config review: **PASS for isolated groundwork scope**.
  The LoRa sources are feature-scoped and test-only in CMake; send errors are
  returned to the caller; duplicate packets are rejected before TTL mutation;
  invalid/non-forwardable inputs do not change TTL. Hardware and hybrid
  acceptance remain OPEN.

## Hardware dependency checklist — required before driver selection

- [ ] Exact vendor, product name, orderable module part number, and radio
  silicon/revision confirmed by the user.
- [ ] Module interface and electrical requirements checked against the exact
  XIAO nRF52840 Sense revision: SPI pin mapping; CS; BUSY; DIO1 (and any
  additional IRQ pins); RESET; voltage levels; required pull-ups; and pin
  conflicts with BLE, USB, or board functions.
- [ ] Regulator and decoupling plan verified for receive/transmit peak
  current, module supply range, and available board power budget.
- [ ] Exact antenna, matching/feedline/connector, supported band, gain, and
  placement confirmed. Do not transmit without a suitable antenna.
- [ ] Country/region and permitted frequency plan, channel bandwidth, data
  rate, output power, airtime/duty-cycle/dwell constraints, and applicable
  certification rules confirmed.
- [ ] Compatible Zephyr driver and exact version/configuration confirmed
  against the installed Zephyr release, without importing legacy project
  source/configuration.
- [ ] BLE advertising/scanning plus Android nRF Connect GATT coexistence
  acceptance plan defined for all three boards.

## Rollback

No deployed image was modified. To revert this task, remove only the newly
added `src/features/lora_transport/`, `tests/lora_transport/`, and
`docs/copilot/013-phase4-lora-module-confirmation.prompt.md` files, then
revert this report and its Phase 4 entry in `docs/PROGRESS.md`. The generated
`build-task013-default`, `build-task013-topology`, and
`build-task013-lora-ztest` directories are build outputs, not source changes;
leave them unless their exact contents are intentionally removed.

## Next action

Stop at the hardware dependency gate. Ask the user to confirm an exact LoRa
module part and wiring/power details before proposing a real driver or pin
configuration. Then implement only the confirmed radio backend and host/Zephyr
tests, including intentional shared BLE/LoRa dedupe integration, while
preserving current BLE relay semantics. Do not flash without separate,
explicit approval. Copy-ready prompt:
[TASK013 module confirmation prompt](./copilot/013-phase4-lora-module-confirmation.prompt.md).
