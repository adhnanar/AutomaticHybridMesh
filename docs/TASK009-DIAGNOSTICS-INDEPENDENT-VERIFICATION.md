# Task009 diagnostics — independent verification

Date: 2026-10-18

## Scope
Independently validate Copilot's Task009 diagnostics implementation and its intended deployment artifact, without changing or flashing hardware.

## Evidence
- Independently ran Python unittest discovery from project root: **11/11 PASS**, exit 0.
- Independently ran py_compile on changed diagnostic capture, parser tests, and PC acceptance runner: **PASS**, exit 0.
- Independently inspected BLE mesh relay queue helper, counter helper, GATT TX storage helper, relay advertisement/receive flow, and associated CMake integration. The new event labels distinguish API advertising update from on-air transmission. No change to the A/B/C routing policy or advertising hold is claimed.
- Verified all three existing board USB serials present: Kit1 COM14 1D5EA4A525ED34EB; Kit2 COM15 224BC825E1DA130C; Kit3 COM16 B13E025E5A1407EE.
- Verified Copilot's production-default build has CONFIG_BLE_MESH_TEST_TOPOLOGY unset; Copilot's topology-enabled build has CONFIG_BLE_MESH_TEST_TOPOLOGY=y and CONFIG_BLE_MESH_TEST_B_ID_0C13=y.
- Independently performed a **clean** Zephyr build, 271/271 steps PASS, exit 0, target xiao_ble/nrf52840, flags CONFIG_BLE_MESH_TEST_TOPOLOGY=y and CONFIG_BLE_MESH_TEST_B_ID_0C13=y. Build log: docs/task009-diagnostics-independent-build.log. FLASH 134916 B; RAM 30408 B; UF2 270336 bytes.
- SHA256 of independently built diagnostic-topology UF2: **04170F01F5BE435219D12E69702DCEC60CA3909CC604AF1ACD0E7CAC87B7D142**, identical to Copilot's topology build hash. Deployment candidate: build-task009-diagnostics-independent/zephyr/zephyr.uf2.
- Copilot reported separate production and C ztest clean builds passed; **C ztests have not been run**, only compiled. The host tests do not establish live BLE delivery.

## Assessment and outstanding issue
Implementation is build-verified, but Task009 controlled A->B->C physical acceptance remains **OPEN**. Old image on all three devices remains unchanged. New diagnostic firmware must be flashed to appropriate devices only with fresh, explicit user approval. A subsequent controlled test should capture B COM15 and C COM16 console output simultaneously; distinguish RX_ACCEPT, RELAY_QUEUE_OK, ADV_DATA_API_OK/FAIL, GATT_PUBLISH and GATT TX readback correlated by A source + sequence + TTL. An ADV_DATA_API_OK result alone is not proof of over-the-air relay. C console records may still interleave; malformed joined records are not evidence.

## Changes and rollback
This review adds only docs/TASK009-DIAGNOSTICS-INDEPENDENT-VERIFICATION.md, independent build directory/log, and a PROGRESS.md entry. No firmware source changes, hardware resets, or flashes. Rollback: remove verification report, independent build and log, and progress entry.
