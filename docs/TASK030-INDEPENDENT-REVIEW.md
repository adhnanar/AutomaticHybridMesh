# Task030 independent review — 2026-10-19

**Verdict: ACCEPT GATT code inventory with critical deployment-state correction.** Copilot idle, verified workspace `C:\zephyrproject\hybrid-mesh-poc`. Independently read `src/features/ble_gatt/ble_gatt.c` and `docs/PROGRESS.md`.

Critical correction: Task030's 'what is deployed' section relied on old README/Task012 state. Later dated physical hardware evidence proves Kit1/A COM14 was flashed Task022 LoRa RX-only on 2026-10-19 and successfully captured RX_WAIT_NO_PACKET. Kit2/B COM15 retains Task010 BLE topology; Kit3/C COM16 retains Task012 BLE topology. Kit1 is **not currently available for BLE A-originated messaging**. The proposed A->B->C Android demo is NOT presently executable without explicit approval to restore Kit1's original Task012 BLE image, disrupting its LoRa receiver role. No such approval given; do not flash.

Grounded code inventory: Hybrid service UUID `7B3E0001-6F2A-4C91-9A3D-2F5B6C7D8E00`; RX 0002; TX 0003; Discovery 0004; Evidence 0005. nRF Connect may access read/write/notify in foreground, but automatic Android status-bar/background notifications are not established. Mesh is limited to short unaddressed text; no reliable inbox/history, user profiles, delivery acknowledgments, or privacy. Existing deployed topology policy prohibits B/C originating A-like text, and Kit1 LoRa RX image does not support this BLE mesh demo.

PC-only follow-up: design isolated BLE Android usability improvements/testable modules without changing original firmware or flashing. For physical Android demo, request explicit Kit1 restoration approval and a test plan first; do not infer kit state solely from older README.
