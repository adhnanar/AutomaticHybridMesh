cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(frurt_sx1262_uk_one_shot)

# DEFAULT OFF. Enabling is a separate, explicit, assessed decision (see TASK024 doc).
option(FRURT_UK_ONE_SHOT_TX_ALLOW "Build the one-shot image able to transmit" OFF)

target_sources(app PRIVATE
  src/main.c
  ../../src/features/uk_lora_one_shot/uk_lora_one_shot.c
  ../../src/features/uk_lora_one_shot/uk_lora_one_shot_store.c
  ../../src/features/uk_lora_regulatory/uk_lora_regulatory.c
)
target_include_directories(app PRIVATE
  ../../src/features/uk_lora_one_shot
  ../../src/features/uk_lora_regulatory
  src
)

if(FRURT_UK_ONE_SHOT_TX_ALLOW)
  message(FATAL_ERROR "TASK025: TX allow is HARD-DISABLED. No reviewed durable flash store adapter exists; see docs/TASK025-ONE-SHOT-REBOOT-SAFETY.md acceptance gate.")
  message(WARNING "FRURT_UK_ONE_SHOT_TX_ALLOW=ON: image can emit ONE RF packet. Needs documented antenna/ERP evidence and explicit approval.")
  target_sources(app PRIVATE src/radio_tx.c)
  target_compile_definitions(app PRIVATE FRURT_UK_ONE_SHOT_TX_ALLOW=1 UK868_ALLOW_GATE_OPEN=1)
else()
  target_compile_definitions(app PRIVATE FRURT_UK_ONE_SHOT_TX_ALLOW=0 UK868_ALLOW_GATE_OPEN=0)
endif()
