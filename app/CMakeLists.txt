cmake_minimum_required(VERSION 3.20.0)

# Find OpenHaystack module in this repo
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../modules/openhaystack)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(openhaystack-nrf52-zephyr)

target_sources(app PRIVATE src/main.c)
