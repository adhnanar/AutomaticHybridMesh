cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uk_lora_one_shot_tests)

# Default: locked build. -DONE_SHOT_TEST_ALLOW=ON is a HOST-ONLY policy test with a
# fake callback; it has no radio driver and no lora_* API.
option(ONE_SHOT_TEST_ALLOW "Host-only: compile the unlocked policy path with a fake radio" OFF)

target_sources(app PRIVATE
  src/main.c
  src/store_tests.c
  src/reboot_tests.c
  src/concurrency_tests.c
  ../../src/features/uk_lora_one_shot/uk_lora_one_shot.c
  ../../src/features/uk_lora_one_shot/uk_lora_one_shot_store.c
  ../../src/features/uk_lora_regulatory/uk_lora_regulatory.c
)
target_include_directories(app PRIVATE
  src
  ../../src/features/uk_lora_one_shot
  ../../src/features/uk_lora_regulatory
)
if(ONE_SHOT_TEST_ALLOW)
  # STORE_ACCEPTED=1 is HOST-ONLY (simulated flash). Production CMake never sets it.
  target_compile_definitions(app PRIVATE FRURT_UK_ONE_SHOT_TX_ALLOW=1 UK868_ALLOW_GATE_OPEN=1 FRURT_UK_ONE_SHOT_STORE_ACCEPTED=1)
else()
  target_compile_definitions(app PRIVATE FRURT_UK_ONE_SHOT_TX_ALLOW=0 UK868_ALLOW_GATE_OPEN=0)
endif()
