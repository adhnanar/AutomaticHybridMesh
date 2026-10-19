cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_mesh_packet_tests)

target_sources(app PRIVATE
  src/main.c
  ../../src/features/ble_gatt/ble_gatt_android_name.c
  ../../src/features/ble_gatt/ble_gatt_value.c
  ../../src/features/ble_mesh_relay/ble_mesh_packet.c
  ../../src/features/ble_mesh_relay/ble_mesh_rssi_validation.c
  ../../src/features/ble_mesh_relay/ble_mesh_relay_queue.c
  ../../src/features/ble_mesh_relay/ble_mesh_rx_diagnostics.c
  ../../src/features/ble_mesh_relay/ble_mesh_heartbeat.c
  ../../src/features/ble_mesh_relay/ble_mesh_evidence.c
  ../../src/features/ble_mesh_relay/ble_mesh_topology.c
)
target_include_directories(app PRIVATE
  ../../src/features/ble_gatt
  ../../src/features/ble_mesh_relay
)
