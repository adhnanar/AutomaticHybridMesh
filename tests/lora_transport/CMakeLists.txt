cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lora_transport_tests)

target_sources(app PRIVATE
  src/main.c
  ../../src/features/ble_mesh_relay/ble_mesh_packet.c
  ../../src/features/lora_transport/lora_transport.c
  ../../src/features/lora_transport/lora_transport_mock.c
  ../../src/features/lora_transport/lora_mesh_relay_policy.c
)
target_include_directories(app PRIVATE
  ../../src/features/ble_mesh_relay
  ../../src/features/lora_transport
)
