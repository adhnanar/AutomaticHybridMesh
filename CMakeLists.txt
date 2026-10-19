cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hybrid_mesh_poc)
target_sources(app PRIVATE
  src/main.c
  src/features/system_status/system_status.c
  src/features/ble_gatt/ble_gatt.c
  src/features/ble_gatt/ble_gatt_android_name.c
  src/features/ble_gatt/ble_gatt_value.c
  src/features/ble_mesh_relay/ble_mesh_packet.c
  src/features/ble_mesh_relay/ble_mesh_rssi_validation.c
  src/features/ble_mesh_relay/ble_mesh_topology.c
  src/features/ble_mesh_relay/ble_mesh_relay_queue.c
  src/features/ble_mesh_relay/ble_mesh_rx_diagnostics.c
  src/features/ble_mesh_relay/ble_mesh_heartbeat.c
  src/features/ble_mesh_relay/ble_mesh_evidence.c
  src/features/ble_mesh_relay/ble_mesh_relay.c
)

target_sources_ifdef(CONFIG_BLE_INBOX app PRIVATE
  src/features/ble_inbox/ble_inbox.c
  src/features/ble_inbox/ble_inbox_service.c
)

target_sources_ifdef(CONFIG_HYBRID_ROUTER_POLICY app PRIVATE
  src/features/hybrid_router/hybrid_router.c
)

if(CONFIG_HYBRID_ROUTER_POLICY)
  target_include_directories(app PRIVATE
    src/features/ble_mesh_relay
  )
endif()
