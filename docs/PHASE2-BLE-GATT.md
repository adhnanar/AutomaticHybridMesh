# Phase 2 — Modular BLE GATT

## Scope and status

Implement a small Zephyr BLE peripheral service for nRF Connect on Android. The service supports a writable RX characteristic, readable/notifiable TX echo characteristic, and a readable/notifiable discovery-event characteristic. The discovery command is a demo only; it does not scan for peers or perform mesh discovery.

The modular BLE GATT implementation has a successful pristine compile for `xiao_ble/nrf52840`. No image has been flashed as part of this phase; Android validation remains pending.

### Phase 3 behavior update

The `DISCOVER` demonstration response described below was the Phase 2 behavior.
Phase 3 replaces it with the current custom BLE peer-discovery event and
message-relay behavior. GATT RX still has a 20-byte characteristic buffer,
but the custom relay accepts at most 14 text bytes per packet and rejects
longer non-`DISCOVER` writes rather than truncating them. See
[PHASE3-BLE-MESH.md](./PHASE3-BLE-MESH.md) for the current protocol, status,
and outstanding hardware acceptance tests.

## Files changed

- `CMakeLists.txt` — include the isolated BLE GATT feature source.
- `prj.conf` — enable Zephyr Bluetooth peripheral support and set the advertised name.
- `src/main.c` — initialize the feature and report startup errors.
- `src/features/ble_gatt/ble_gatt.c` — implement GATT service, advertising, RX echo, and demo event.
- `src/features/ble_gatt/ble_gatt.h` — expose feature initialization and event publishing.
- `docs/PHASE2-BLE-GATT.md` — this phase record.
- `docs/PROGRESS.md` — phase status and verification log.

## GATT UUIDs

All UUIDs are 128-bit custom UUIDs:

| Attribute | UUID | Properties |
|---|---|---|
| Hybrid Mesh service | `7b3e0001-6f2a-4c91-9a3d-2f5b6c7d8e00` | Primary service |
| RX | `7b3e0002-6f2a-4c91-9a3d-2f5b6c7d8e00` | Write, write without response |
| TX | `7b3e0003-6f2a-4c91-9a3d-2f5b6c7d8e00` | Read, notify |
| Discovery event | `7b3e0004-6f2a-4c91-9a3d-2f5b6c7d8e00` | Read, notify |

The peripheral advertises the service UUID and exposes the device name `HybridMesh` in its scan response.

## Android nRF Connect test procedure

1. Build the image and, only after separately approving a flash, connect the board and start nRF Connect scanning.
2. Connect to `HybridMesh`, then open the custom service UUID above.
3. Enable notifications on TX and on Discovery event using each characteristic's CCC control.
4. Read TX; the initial value is `Ready`.
5. Write a short UTF-8 or byte payload to RX. TX should become a notification containing the same bytes and should also be readable with that value.
6. Historical Phase 2 behavior: writing the exact ASCII text `DISCOVER` to RX echoed it on TX and notified/read `DEMO: no mesh peers`. This was a simulated event with no radio scan. Phase 3 replaces it with the real peer-table response described in [PHASE3-BLE-MESH.md](./PHASE3-BLE-MESH.md).

The default payload limit is 20 bytes, appropriate for the default ATT MTU. Larger payloads are rejected rather than silently truncated.

## Build commands and actual results

Final command executed (absolute paths; valid from any working directory):

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always
```

Results:

- Initial command executed from the default shell directory:

  ```powershell
  & .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always
  ```

  It failed because the shell started in `C:\Users\adhna`, so the relative path did not resolve to the installed west executable.
- First corrected pristine build command used the environment's west executable, `C:\zephyrproject\.venv\Scripts\west.exe`, and configured Zephyr 4.5.0-rc2, but compilation failed: advertising data declared inside `ble_gatt_init` produced a non-constant initializer, and `BT_LE_ADV_CONN` is not defined by this Zephyr version. The advertising data was moved to file scope and the supported `BT_LE_ADV_CONN_FAST_1` parameters are used.
- Final pristine build command:

  ```powershell
  & C:\zephyrproject\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always
  ```

- Final result: exit code `0`; all `262/262` build steps completed using Zephyr `4.5.0-rc2`. The build reports 123,040 bytes of flash used (15.25%) and 28,944 bytes of RAM used (11.04%). Verified `build/zephyr/.config` contains `CONFIG_BT=y`, `CONFIG_BT_PERIPHERAL=y`, and `CONFIG_BT_DEVICE_NAME="HybridMesh"`.
- Generated artifacts: `build/zephyr/zephyr.uf2` (246,272 bytes), `build/zephyr/zephyr.elf` (2,718,164 bytes), and `build/zephyr/zephyr.hex` (346,265 bytes).
- No unit tests are configured for this feature. The Zephyr pristine build is compile/link validation only.

No flash, erase, bootloader operation, phone connection, or over-the-air test was performed for this phase.

## Known limits and issues

- GATT and advertising behavior are compile-checked only until a board is connected and the user approves a flash.
- At Phase 2 completion, `DISCOVER` emitted a demonstration event only; Phase 3 replaces that behavior with custom BLE peer discovery and relay. See [PHASE3-BLE-MESH.md](./PHASE3-BLE-MESH.md).
- BLE multi-hop coexistence with phone GATT remains an engineering gate before Phase 3.
- Notification attempts without an enabled subscriber are logged; the characteristic value remains readable.

## Rollback

Revert only the Phase 2 changes listed above: remove the feature source from `CMakeLists.txt`, remove the three Bluetooth settings from `prj.conf`, restore the prior `src/main.c`, and remove the `src/features/ble_gatt` feature directory and this phase record. Do not revert unrelated workspace edits or remove the existing build directory.

## Next action

Request separate user approval before flashing the Phase 2 image. After approved physical testing, record the actual Android connection/read/write/notification results here and update `docs/PROGRESS.md`. Before Phase 3, verify whether BLE peer relay can coexist with phone GATT or document a feasible alternative. Do not mark Phase 2 complete on build evidence alone.

## Independent review and user-approved Kit 1 flash — 2026-10-18
- Independently inspected `src/features/ble_gatt/ble_gatt.c`, `src/main.c`, `prj.conf` and `CMakeLists.txt`: modular BLE peripheral GATT RX, TX echo, demo discovery event, advertising; no real mesh discovery yet.
- First independent west attempt failed outside workspace (`west: unknown command build`); reran from `C:\zephyrproject` with pristine flag. Independent build completed 262/262, exit code 0, UF2 246272 bytes, 123040 B flash and 28944 B RAM. No automated runtime test exists.
- User explicitly approved Phase 2 flashing, then double-pressed reset. Verified `D:\INFO_UF2.TXT`: Board-ID `Seeed_XIAO_nRF52840_Sense`, UF2 Bootloader 0.6.1. Copied verified UF2 to `D:\zephyr.uf2` successfully. USB CDC COM14 returned; serial showed Zephyr 4.5.0-rc2 boot banner and first three characters `Hyb`. BLE advertising and phone GATT interaction NOT YET verified.
- Review verdict: source/build/UF2 flash PASS; functional BLE/Android acceptance PENDING. Next: scan for `HybridMesh` with Android nRF Connect, connect, enable TX and discovery notifications, write test payload to RX and `DISCOVER`, record actual results. No additional flash needed for these tests.

## Android nRF Connect screenshot verification — 2026-10-18
- User supplied Android nRF Connect screenshot showing CONNECTED / NOT BONDED and custom primary service `7b3e0001-6f2a-4c91-9a3d-2f5b6c7d8e00`.
- Screenshot shows RX characteristic `7b3e0002-6f2a-4c91-9a3d-2f5b6c7d8e00` with WRITE and WRITE NO RESPONSE; TX `7b3e0003-6f2a-4c91-9a3d-2f5b6c7d8e00` with NOTIFY and READ.
- Result: Android BLE GATT connection and service discovery PASS. Notification subscription, RX write/echo, and simulated discovery notification NOT YET verified. Next: subscribe TX, write UTF-8 Hello to RX and capture result.

## Android message echo screenshot — 2026-10-18
- User screenshot shows Android nRF Connect connected to custom service, RX `0002` value `Hello` (hex `48-65-6c-6c-6f`) and TX `0003` value `Hello`, with TX notifications enabled.
- PASS: phone-to-board RX write and echoed TX characteristic value. Screenshot does not conclusively prove a notification event was delivered; explicit notification log/event and discovery `DISCOVER`/`0004` test remain pending.

## Android DISCOVER screenshot evidence — 2026-10-18
- User screenshot shows connected `HYBRIDMESH`, RX characteristic `7b3e0002` value ASCII `DISCOVER` (44-49-53-43-4F-56-45-52), TX characteristic `7b3e0003` value `DISCOVER`, and discovery characteristic `7b3e0004` value `DEMO: no mesh peers`.
- Both TX and discovery CCC descriptors visibly state `Notifications enabled`.
- PASS: Android GATT connectivity, write/read echo, discovery demo command-response, and CCC notification subscription configuration. A distinct notification event in the nRF Connect log was not supplied, so notification delivery itself is not independently proven by screenshot; request log capture if strict evidence is needed.
- Phase 2 phone interface functionally demonstrated; real BLE peer discovery, BLE mesh and LoRa remain unimplemented. Do not confuse demo event with mesh discovery.
