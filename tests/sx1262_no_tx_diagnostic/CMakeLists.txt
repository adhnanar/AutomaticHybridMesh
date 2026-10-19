cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(frurt_sx1262_receive_only_diagnostic)
target_sources(app PRIVATE src/main.c)
