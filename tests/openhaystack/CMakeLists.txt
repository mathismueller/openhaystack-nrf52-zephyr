cmake_minimum_required(VERSION 3.20.0)

# Reuse the OpenHaystack module from this repo
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/openhaystack)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(openhaystack_test)

target_sources(app PRIVATE src/openhaystack_test.c)
