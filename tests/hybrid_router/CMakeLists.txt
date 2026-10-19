cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hybrid_router_tests)

target_sources(app PRIVATE
  src/main.c
  ../../src/features/ble_mesh_relay/ble_mesh_packet.c
  ../../src/features/hybrid_router/hybrid_router.c
)
target_include_directories(app PRIVATE
  ../../src/features/ble_mesh_relay
  ../../src/features/hybrid_router
)
