# Phase 3 — BLE peer discovery and relay

## Status

The modular custom advertising relay implementation is in place and the
`xiao_ble/nrf52840` firmware builds cleanly with Zephyr 4.5.0-rc2. Packet,
TTL, and duplicate-suppression tests are written and compile for the target,
but could not be executed because this Windows environment has no native host
`gcc` for `native_sim`.

Phase 3 acceptance is **not complete**. No Phase 3 image was flashed and no
BLE radio, multi-board relay, Android GATT coexistence, or A → B → C behavior
has been observed. Do not treat build evidence as radio evidence.

## SIG Mesh research and architecture decision

Zephyr's Bluetooth Mesh documentation describes the SIG Mesh profile, its
provisioning requirement, AppKey-based access-model authorization, and GATT
Proxy as a path for legacy devices to access an already provisioned Mesh
network. Zephyr's provisioning API supports PB-ADV and PB-GATT for a
provisionee; its provisioner support is limited to the advertising bearer.
Provisioning assigns the node address and network credentials, and model
traffic additionally depends on AppKeys.

The PoC phone interface is the existing custom GATT service in Android
nRF Connect, with no custom mobile app. A generic GATT connection is not a
Mesh provisioner and does not supply Mesh provisioning, configuration, or
AppKey management. Implementing SIG Mesh would therefore require additional
provisioning/application work and a compatible provisioner workflow that is
outside this phase's phone-interface constraint.

**Decision:** use a purpose-built, connectionless advertising-payload flood
relay, while retaining the connectable GATT peripheral for Android. Peer
nodes exchange the broadcast frames without establishing connections; the
same advertising set remains connectable by the phone. This is a custom PoC
protocol, **not Bluetooth SIG Mesh**, is not SIG Mesh compliant, and does not
use SIG Mesh provisioning, Network Keys, AppKeys, or GATT Proxy. The custom
packets are unauthenticated and unencrypted and are suitable only for this
demonstration.

References consulted:

- [Zephyr Bluetooth Mesh provisioning](https://docs.zephyrproject.org/latest/services/connectivity/bluetooth/api/mesh/provisioning.html)
- [Zephyr Bluetooth Mesh access models and AppKey list](https://docs.zephyrproject.org/latest/services/connectivity/bluetooth/api/mesh/access.html)
- [Zephyr Bluetooth Mesh GATT Proxy](https://docs.zephyrproject.org/latest/services/connectivity/bluetooth/api/mesh/proxy.html)
- [Zephyr Bluetooth GAP API](https://docs.zephyrproject.org/latest/services/connectivity/bluetooth/api/gap.html)

The installed Zephyr 4.5.0-rc2 source was also inspected locally:
`doc/services/connectivity/bluetooth/api/mesh/provisioning.rst`,
`doc/services/connectivity/bluetooth/api/mesh/access.rst`,
`doc/services/connectivity/bluetooth/api/mesh/proxy.rst`, and the GAP
advertising-parameter notes in `include/zephyr/bluetooth/bluetooth.h`.

## Design and limits

- Zephyr `CONFIG_BT_PERIPHERAL` and `CONFIG_BT_OBSERVER` are enabled. The
  existing connectable legacy advertiser is reused; it is not replaced by a
  second advertising set. Relay frames are carried in its manufacturer-data
  field and peer scanning is passive. The scan response contains both the
  existing GATT service UUID and the `HybridMesh` device name (30 bytes total),
  so the phone can still identify and connect to the GATT service.
- The local Bluetooth GAP API notes that scanning and advertising with two
  different random addresses is not possible without extended advertising
  support. This design uses the existing advertiser/identity and does not
  request a second random address. The target build proves the role
  configuration compiles; simultaneous scan, advertising, and a live Android
  connection still needs physical verification.
- Advertising data uses manufacturer identifier `0xFFFF` for this
  development-only custom protocol; it is not a registered organization
  identifier. The on-air frame is:

  | Offset | Size | Meaning |
  |---|---:|---|
  | 0 | 1 | `0x11` beacon or `0x12` text packet |
  | 1 | 6 | Stable node ID: first six little-endian bytes from nRF52840 FICR DEVICEID |
  | 7 | 2 | Per-node sequence number, little-endian |
  | 9 | 1 | Remaining TTL |
  | 10 | 0–14 | Text bytes; beacons have no payload |

- The legacy advertising limit is 31 bytes. Flags and manufacturer-data
  overhead leave room for a 24-byte frame: a 10-byte header and at most
  14 text bytes. No fragmentation is implemented. GATT RX still has a
  20-byte characteristic buffer, but non-`DISCOVER` writes over 14 bytes are
  explicitly rejected rather than truncated.
- Text starts with TTL 3. Each receiving relay decrements it and forwards
  only while the incoming TTL is greater than 1, bounding a path to three
  radio hops. Beacons use TTL 1 and are not relayed.
- Duplicate keys are `(source node ID, sequence)` in a 16-entry cache with a
  30-second suppression window. The sequence is 16-bit and wraps. This is
  bounded loop suppression, not cryptographic replay protection.
- At most eight peers are retained and entries expire after 60 seconds.
  Receive and transmit/relay queues are bounded at eight and four packets.
  Full queues drop packets with an explicit log. A text packet is advertised
  for 500 ms before the next queued packet or the next beacon; discovery
  beacons are refreshed every five seconds.
- RX packet parsing copies the packet and payload into a bounded message
  queue before the advertising callback returns. TX and discovery notification
  buffers are copied into local storage and then copied by Zephyr's GATT
  notification path; advertising frame bytes are held in static storage and
  serialized by the Zephyr advertising update call.
- A newly seen peer publishes `PEER:<12 hex digits>` on the existing Discovery
  characteristic. Writing `DISCOVER` reports the current peer table (or the
  truthful `BLE: no peers` result). A received text message updates/notifies
  the existing TX characteristic. The phone's accepted RX write is still
  echoed to TX.

The design is feasible as a single-controller peripheral + observer build,
but compile evidence cannot guarantee that scanning, connectable advertising,
and an active GATT connection coexist on the physical XIAO. If the radio
rejects that combination, do not call the acceptance test passed: record and
resolve the coexistence design before relying on multi-hop forwarding.

## Files changed

- `CMakeLists.txt` — include the BLE mesh packet and relay feature sources.
- `prj.conf` — enable Bluetooth observer role alongside the existing
  peripheral role.
- `src/main.c` — initialize GATT, the relay, and its RX handler with explicit
  startup error reporting.
- `src/features/ble_gatt/ble_gatt.c` and `ble_gatt.h` — preserve the service
  and characteristic UUIDs; add RX-handler registration, text notification,
  mesh-advertisement updates, and keep the service UUID/name in the scan
  response.
- `src/features/ble_mesh_relay/ble_mesh_packet.c` and `.h` — encode/decode,
  bounded validation, relay TTL handling, and duplicate suppression.
- `src/features/ble_mesh_relay/ble_mesh_relay.c` and `.h` — derive node ID,
  beacon/scan, bounded peer discovery, GATT event forwarding, and queued text
  relaying.
- `tests/ble_mesh_packet/` — six ztest cases covering packet round-trip,
  invalid bounds and packet values, relay TTL, and deduplication/expiry.
- `docs/PHASE3-BLE-MESH.md` — this phase record.
- `docs/PHASE2-BLE-GATT.md` — mark the old simulated `DISCOVER` result as
  historical and link to the current behavior.
- `docs/PROGRESS.md` and `docs/ROADMAP.md` — phase status and engineering-gate
  decision.

## Commands executed and results

Commands were run from `C:\zephyrproject`.

1. Final pristine firmware build:

   ```powershell
   & .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always
   ```

   **PASS, exit 0.** Zephyr `4.5.0-rc2`; `CONFIG_BT=y`,
   `CONFIG_BT_PERIPHERAL=y`, `CONFIG_BT_OBSERVER=y`, and device name
   `HybridMesh` are present in the generated `.config`. Final linker report:
   131,268 bytes flash (16.27% of 788 KB) and 30,344 bytes RAM (11.58% of
   256 KB). Generated `zephyr.elf` (2,829,944 bytes) and `zephyr.uf2`
   (262,656 bytes). These are build artifacts only.

2. Target compilation of the packet test application:

   ```powershell
   & .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\build-hybrid-mesh-poc-phase3-packet-xiao C:\zephyrproject\hybrid-mesh-poc\tests\ble_mesh_packet
   ```

   **PASS, exit 0.** All eight incremental build steps completed; test
   application linked for `xiao_ble/nrf52840`. It was not flashed or run.

3. Requested host test attempt:

   ```powershell
   & .\.venv\Scripts\west.exe twister -T C:\zephyrproject\hybrid-mesh-poc\tests\ble_mesh_packet -p native_sim -O C:\zephyrproject\build-hybrid-mesh-poc-phase3-test
   ```

   **Not a test pass.** Twister exited 0 but executed zero configurations;
   one configuration was filtered.

4. Direct `native_sim` build/run attempt:

   ```powershell
   & .\.venv\Scripts\west.exe build -b native_sim -d C:\zephyrproject\build-hybrid-mesh-poc-packet-direct C:\zephyrproject\hybrid-mesh-poc\tests\ble_mesh_packet --pristine=always; if ($LASTEXITCODE -eq 0) { & .\.venv\Scripts\west.exe build -d C:\zephyrproject\build-hybrid-mesh-poc-packet-direct -t run }
   ```

   **Blocked.** CMake could not find the host `gcc` compiler
   (`CMAKE_C_COMPILER`); the conditional `-t run` step was therefore not
   reached. `Get-Command gcc,clang,cl,cc,arm-zephyr-eabi-gcc
   -ErrorAction SilentlyContinue` found no host C compiler.

No flash, erase, bootloader operation, or board interaction was performed.
No BLE packet, phone connection, notification, or routing result is claimed.

## Independent source/config review verdict

- **Scope and modularity: PASS.** Feature-specific source/header and tests are
  isolated; no legacy source, LoRa routing, SIG Mesh configuration, or custom
  phone app was added.
- **Packet and buffer handling: PASS by source inspection.** Frame and payload
  sizes, TTL, queues, peer table, and dedup cache are bounded; malformed frames
  are rejected; scan callback data is copied before return; Zephyr's GATT and
  advertising paths copy submitted values before returning.
- **Build/config: PASS.** Final pristine target firmware build and target test
  application compile both exit 0. No build warning/error was observed.
- **Automated test execution: BLOCKED.** The six ztest cases compile for the
  board but did not run because `native_sim` requires unavailable host `gcc`.
- **Phase acceptance: PENDING.** Peer discovery, Android nRF Connect GATT
  interoperability during scanning/relaying, duplicate/TTL behavior over RF,
  and A → B → C delivery need physical tests.

## Open issues and next action

1. With explicit authorization for a future board flash, deploy this image
   according to the hardware test plan; this task did not flash anything.
2. Verify Android nRF Connect still discovers the service and can connect,
   read/write RX/TX, and subscribe to TX/Discovery while peer scanning and
   relay are active.
3. Verify at least three nodes: discovery events, phone-to-node text, A → B →
   C delivery, one-time handling under duplicate reception, and no forwarding
   once TTL reaches one. Record observed logs/screenshots and actual outcomes.
4. Install/provide a supported native host C compiler only if running the
   current `native_sim` ztests is required; otherwise keep the execution gap
   explicitly open.
5. Do not advance Phase 4 or mark Phase 3 complete until the radio acceptance
   results are recorded.

## Rollback

Remove the Phase 3 source entries from `CMakeLists.txt`, remove
`CONFIG_BT_OBSERVER=y`, restore the Phase 2 versions of `src/main.c` and the
BLE GATT files, and remove `src/features/ble_mesh_relay/`,
`tests/ble_mesh_packet/`, and this phase record. Remove only the Phase 3
verification entry/status wording from `docs/PROGRESS.md` and the Phase 3
engineering-gate note from `docs/ROADMAP.md`. Preserve the Phase 2 GATT
service and its existing historical documentation; remove only the Phase 3
behavior-update note from `docs/PHASE2-BLE-GATT.md`. Preserve unrelated
workspace files.
