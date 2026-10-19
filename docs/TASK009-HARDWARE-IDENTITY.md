# Task 009 hardware identity mapping and deployment preparation

## Live evidence
2026-10-18: Windows BLE scan via `C:\zephyrproject\.venv\Scripts\python.exe tests\ble_pc_air_probe.py` observed manufacturer beacon frames carrying mesh source IDs:
- Kit1 USB COM14, serial `1D5EA4A525ED34EB` -> beacon E5:12:76:D2:56:EA ID `eb34ed25a5a4`.
- Kit2 USB COM15, serial `224BC825E1DA130C` -> beacon C8:51:08:6E:08:C0 ID `0c13dae125c8`.
- Kit3 USB COM16, serial `B13E025E5A1407EE` -> beacon E2:DB:6A:54:BB:71 ID `ee07145a5e02`.

Each USB serial's final 12 hex digits are the corresponding 6-byte FICR-derived node ID in reverse byte order; firmware constructs the mesh ID from FICR DEVICEID little-endian. This establishes the physical mapping without resetting or flashing devices.

## Correct test configuration
- A = Kit1 `eb34ed25a5a4`.
- B = Kit2 `0c13dae125c8` -> `CONFIG_BLE_MESH_TEST_B_ID_0C13=y`.
- C = Kit3 `ee07145a5e02`.
- Test-only `CONFIG_BLE_MESH_TEST_TOPOLOGY=y` applies the same A/B/C mapping to all three boards.
- Clean verified-topology build initiated at `build-test-topology-verified`; output log `docs/task009-verified-topology-build.log`. Check exit code and SHA256 before any flashing.
- Physical acceptance runner: `python tests/ble_pc_controlled_two_hop.py --source E5:12:76:D2:56:EA --relay C8:51:08:6E:08:C0 --destination E2:DB:6A:54:BB:71`.

## Acceptance and authorization
This is hardware identification and build preparation only. No Task009 firmware flashed. The physical A->B->C acceptance test remains pending separate explicit user authorization to flash all three kits. Do not claim true RF isolation based on TTL filtering alone.
