# Task037 — Windows PC live BLE automated tests, 2026-10-19

User authorized PC BLE connection and all tests. Workspace `C:\zephyrproject\hybrid-mesh-poc`. Used preinstalled `bleak` and Windows WinRT Bluetooth adapter; **no installs, device resets or firmware flashing. No LoRa TX.**

## Discovery and connection
Bleak scan: Kit3 `Mesh-5A5E02` E2:DB:6A:54:BB:71 RSSI −87; Kit2 `Mesh-E125C8` C8:51:08:6E:08:C0 RSSI −98. Both connected independently via Windows BLE. Each exposed service `7b3e0001-6f2a-4c91-9a3d-2f5b6c7d8e00` and characteristics 0002 through 0006. Kit3 GATT 0006 initially held Kit2's prior `HELLO23`, confirming independent PC readback.

## Live bidirectional RF BLE delivery verified
- Kit3 send `BACK32` via RX characteristic 0002 with response: Kit3 local TX GATT `BACK32`. Disconnect Kit3, wait 4 s, connect Kit2. Kit2 inbox 0006 reports v1, count1, dropped0, seq0, source `EE07145A5E02`, text `BACK32`: **PASS**.
- Kit2 send `PC23`, disconnect, wait, connect Kit3. Kit3 inbox reports v1, count2, dropped0, latest seq1 source `0C13DAE125C8` text `PC23` with previous `HELLO23` retained: **PASS**.

## Protocol and notifications
- Both kits accepted GATT write `DISCOVER` but did NOT change TX text to `DISCOVER`, and inbox snapshots unchanged: **PASS**.
- Both rejected 15-byte over-limit text with ATT GATT error 13 Invalid Attribute Value Length: **PASS**.
- PC subscribed to discovery characteristic 0004 and observed actual peer notifications on both kits: **PASS**.
- Simultaneous PC receiver connection to Kit3 with TX 0003 notifications enabled while PC temporarily connected to Kit2 sender. Kit2 wrote `NOTIFY24`, sender disconnected; Kit3 received real notification bytes `4e-4f-54-49-46-59-32-34` = `NOTIFY24` and inbox showed three Kit2 messages (`NOTIFY24`, `PC23`, `HELLO23`): **PASS**. This establishes foreground GATT real-time BLE notify on Windows, not Android OS background notification.

## Remaining limitations
No third BLE node, so cannot run multi-hop on currently deployed Kit1 LoRa RX-only hardware. Automatic BLE↔LoRa switching is still PC-only simulated, LoRa TX not authorized and not tested. Reliability/range/packet-loss/long-run and power-cycle persistence not tested; inbox is RAM-only, no delivery ACK. Weak initial Kit2 PC RSSI -98 dBm requires range testing. BLE message advertisement is released by disconnecting sender client; sending while staying connected is not supported. Temporary scripts: `tests/task037_pc_ble_probe.py`, `tests/task037_pc_ble_gatt_test.py`, `tests/task037_pc_ble_two_way.py`, `tests/task037_pc_ble_protocol.py`, `tests/task037_pc_ble_notify.py`.
