# Task016B — PC GATT peer discovery, 3/3 successful

Date: 2026-10-19. **Discovery notification protocol acceptance: 3/3 PASS.** PC used Windows BLE only; no phone, flashing, board reset, LoRa transmission, power actions or physical changes.

## Mechanism
The current Task012 A/C BLE mesh app registers `ble_mesh_relay_handle_phone_write` as the GATT RX handler. The ASCII command `DISCOVER` triggers `request_discovery()` before test-topology restrictions and calls `ble_gatt_publish_discovery_event` for each recent peer in its 8-entry table. Notifications are available through UUID `7b3e0004-6f2a-4c91-9a3d-2f5b6c7d8e00`. Current peer age timeout: 60 s.

`tests/task016_pc_discovery_notifications.py` connects to Kit3/C `E2:DB:6A:54:BB:71`, subscribes to discovery and text TX CCCs, writes `DISCOVER` to GATT RX UUID `7b3e0002-6f2a-4c91-9a3d-2f5b6c7d8e00` and reads back the discovery characteristic. Raw logs `docs/task016-pc-discovery-01..03.log`.

## Results
Each of three independent trials returned **exactly two discovery notifications**:
- `PEER:0C13DAE125C8` — Kit2/B mesh node identifier.
- `PEER:EB34ED25A5A4` — Kit1/A mesh node identifier.

All discovery GATT readbacks matched the final peer notification, all commands returned successful write, each run exited zero and printed `DISCOVERY_RESULT=PASS`.

**Issue identified:** every `DISCOVER` command also generates a text TX notification with value `b'DISCOVER'`, because `ble_gatt.c/write_rx()` unconditionally calls `ble_gatt_publish_text_message(buf, len)` after the handler returns success, even when `ble_mesh_relay_handle_phone_write()` treats `DISCOVER` as a control command. This is not a discovery protocol failure, but pollutes chat notifications and replaces latest text TX with `DISCOVER`. Proposed separate control-command result API (handler indicates successful command with no TX text echo), new host tests and clean Zephyr build. **Do not deploy without separate explicit flash approval.**

## Boundaries
This validates nearby BLE **node-ID** discovery and BLE GATT subscription while connected; not matching social interests, Android nRF Connect UI behavior or background Android notifications. No LoRa discovery or hybrid routing involved. Discovery reports only recent BLE mesh peer IDs, not radio signal strength or range. Only PC verified.

## Next
Implement a feature-scoped no-echo control outcome for DISCOVER command, unit tests, clean default/topology builds, no flashing. When operator awake, test on Android nRF Connect manually (no custom app).
