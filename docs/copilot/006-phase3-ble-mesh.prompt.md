# Copilot Agent task 006 — Phase 3 BLE mesh

Read AGENTS.md, docs/ROADMAP.md, docs/PROGRESS.md and Phase 2 GATT code. Work only in fresh C:\zephyrproject\hybrid-mesh-poc; never import legacy source.

Implement modular Phase 3 BLE peer discovery, text messaging and multi-hop relaying for Seeed XIAO nRF52840 Zephyr 4.5 while preserving Android nRF Connect GATT RX/TX/discovery characteristics and notifications. First research Zephyr Bluetooth SIG Mesh provisioning, AppKeys and coexistence with existing GATT; distinguish standard SIG Mesh from custom connectionless advertising relay and document a justified choice consistent with no custom phone app. Never claim SIG Mesh compliance for custom flooding.

Implement bounded payload, stable node IDs, message sequence IDs, TTL/hop limits, duplicate suppression and loop prevention, relay, peer discovery events forwarded to phone GATT. Handle BLE scanning/advertising/GATT coexistence, callbacks, errors, memory limits and safe buffer lifetimes. Keep main.c minimal; no LoRa routing. Add testable dedup/TTL and packet parsing logic and tests where feasible.

Run pristine west build from C:\zephyrproject targeting xiao_ble/nrf52840; do not flash, reset or erase boards. Update docs/PHASE3-BLE-MESH.md and docs/PROGRESS.md with design, scope, changed files, exact commands, actual evidence, open hardware tests, issues, rollback, next action. Independent reviewer will inspect code and independently rebuild before flashing. If blocked, document blockers rather than pretending completion.
