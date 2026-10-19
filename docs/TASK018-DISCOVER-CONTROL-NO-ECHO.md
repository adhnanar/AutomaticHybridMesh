# Task 018 — DISCOVER control/no-echo, independent review

2026-10-19. **PASS: software builds and PC artifact verification. Not deployed or hardware-tested.**

## Behavior
In the previously deployed firmware, a `DISCOVER` write produced correct BLE GATT discovery notifications (PC test 3/3) but also echoed the command into the text TX characteristic. An optional feature `CONFIG_BLE_GATT_DISCOVER_NO_ECHO=y`, default OFF, now lets the handler return `BLE_GATT_RX_CONTROL_HANDLED`. When ON, the GATT service treats this as a successful ATT write without publishing or storing a text message. Ordinary text writes and the discovery events retain their existing paths. When OFF, the GATT handler uses its exact pre-Task018 compile-time code path, not the new classification branch.

Code: root `Kconfig`, `src/features/ble_gatt/ble_gatt.h`, `ble_gatt.c`, `ble_gatt_value.h/.c`, `src/features/ble_mesh_relay/ble_mesh_relay.c`, and `tests/ble_mesh_packet/src/main.c` (new classification C ztest).

## Evidence
- First six-target pristine PC CI run: **all six Zephyr builds succeeded**, but default and topology images differed from Task012 and two DTS checks were false negatives because the parser read only 900 bytes after the I2C1 node start.
- Corrected default-OFF RX handler guard. Then incrementally rebuilt both images, exit0. **Task012 UF2 hashes reproduced exactly**: default `2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14`; topology `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8`. Logs `docs/task018-off-default-rebuild.log` and `docs/task018-off-topology-rebuild.log`.
- Separate **pristine opt-in** `west build ... -DCONFIG_BLE_GATT_DISCOVER_NO_ECHO=y`: PASS, confirmed ON in generated `.config`; log `docs/task018-noecho-independent.log`. UF2 generated, **not flashed**.
- Independent **pristine C policy ztest build**: PASS, 205/205 steps, 83,696 bytes FLASH, 17,032 bytes RAM. Log `docs/task018-gatt-policy-ztest-independent.log`. **C tests were compiled/linked, not executed.**
- `python tests/task018_verify_artifacts.py` audited compiled images, native SX1262 overlay/driver artifacts and no-echo config: **15/15 PASS**. Existing Python unittests **31/31 PASS**, hybrid Python reference-model tests **13/13 PASS**.
- Improved `tests/pc_ci.py` generated-DTS checker to inspect the complete I2C1 node (longer source window); rechecked compiled artifacts. The full six-target **pristine** CI was not rerun after these changes; the report distinguishes fresh earlier builds from later incremental rebuilds.

## Remaining gates
No firmware was flashed, reset or transmitted via LoRa. Actual no-echo GATT behavior has NOT been verified on a board or Android. Existing flashed Task012 A/C and Task010 B continue to echo DISCOVER. Hardware tests require a later approved flash and PC GATT notification check. Native Windows Zephyr C ztest execution remains blocked; compile-link is not runtime evidence. Region/channel for LoRa still needs explicit confirmation.

Rollback: keep feature disabled by default, which now exactly reproduces validated Task012 BLE images. To remove, revert only the Task018 optional handler result and classification changes.
