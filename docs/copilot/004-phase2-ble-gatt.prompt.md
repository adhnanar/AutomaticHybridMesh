# Copilot Agent — Phase 2 BLE GATT design and build

Read AGENTS.md, docs/ROADMAP.md and docs/PHASE1-BASELINE.md. Work ONLY in C:\zephyrproject\hybrid-mesh-poc. Do not copy legacy firmware or configs.

Current verified evidence: fresh Phase 1 firmware compiled and was flashed via UF2 to Kit 1; COM14 emitted Zephyr boot line. User uses Android nRF Connect and wants no custom mobile app.

Implement a minimal Phase 2 BLE peripheral GATT interface using feature-level organization: features/ble_gatt/{ble_gatt.c,ble_gatt.h}, with a stable custom service UUID, writable RX characteristic, readable/notifiable TX characteristic, and a readable/notifiable discovery-event characteristic if supported. Support simple test echo and discovery notifications, without claiming actual mesh discovery yet. Document UUIDs and nRF Connect test procedure.

Use Zephyr Bluetooth APIs for installed Zephyr 4.5.0-rc2 and target xiao_ble/nrf52840. Keep main.c small. Do not enable LoRa, invent SX1262 pins, or implement BLE mesh prematurely. Build with west pristine and fix compilation issues. No flash without explicit user approval for this new image.

MANDATORY: Update at least one Markdown file EVERY time you change code, including docs/PROGRESS.md and a new docs/PHASE2-BLE-GATT.md with exact files, build commands/results, GATT UUIDs, Android steps, known limits, rollback and next actions. Keep all feature code modular. If blocked, document it and stop safely.
