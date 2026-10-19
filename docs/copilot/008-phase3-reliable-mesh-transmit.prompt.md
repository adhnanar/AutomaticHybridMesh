# Task 008 — Phase 3 reliable BLE mesh message transmission

Follow AGENTS.md and docs/ROADMAP.md. Work in C:\zephyrproject\hybrid-mesh-poc only. Do not flash any board. Preserve the successful Task 007 connection recovery behavior.

## Evidence
All three boards advertise beacon manufacturer data and passed 9/9 reconnections. PC wrote `AIRPROBE` via GATT RX to C8:51:08:6E:08:C0, but a simultaneous PC scanner observed zero BLE text frames and remote boards' TX remained `Ready`. In `src/features/ble_gatt/ble_gatt.c`, `ble_gatt_update_mesh_advertisement` returns -EAGAIN if active_connections>0. In `src/features/ble_mesh_relay/ble_mesh_relay.c`, `advertising_work_handler` dequeues and drops the message irrespective of advertise_packet return, scheduling 500ms hold even if advertising failed. Advertising restart after disconnect may temporarily replace manufacturer data with base advertisement.

## Your task
1. Analyze this path and fix the dropped-message/advertising lifecycle issue with minimal, concurrency-safe changes. Do not block the system workqueue or introduce a busy retry loop. Keep pending text until actual successful advertising; distinguish transient -EAGAIN and other errors, ensure periodic beacon behavior and reliable message transmission after GATT disconnect. Consider races with connected callback and restart worker. Keep packet TTL, dedup, GATT notification semantics intact.
2. Add useful bounded logging and host-side/unit tests where practical; update or create PC BLE test to capture manufacturer TEXT advertisements and distinguish on-air send from source GATT echo and remote delivery.
3. Build clean with west for xiao_ble/nrf52840, fix build issues. Record exact commands/results and design decisions in docs/PROGRESS.md and a Task 008 document.
4. Do not flash any kit. Do not claim inter-device relay works until measured on real boards. Leave hardware flashing for explicit user approval.
