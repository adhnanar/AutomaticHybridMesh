cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uk_lora_regulatory_tests)

# -DUK868_TEST_LOCKED=ON builds the default (gate cannot open) configuration.
option(UK868_TEST_LOCKED "Test default build where the TX gate cannot be opened" OFF)

target_sources(app PRIVATE
  src/main.c
  ../../src/features/uk_lora_regulatory/uk_lora_regulatory.c
)
target_include_directories(app PRIVATE ../../src/features/uk_lora_regulatory)
if(UK868_TEST_LOCKED)
  target_compile_definitions(app PRIVATE UK868_LOCKED_TEST=1)
else()
  target_compile_definitions(app PRIVATE UK868_ALLOW_GATE_OPEN=1)
endif()
