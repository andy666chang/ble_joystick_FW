# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

target_sources(app PRIVATE main.c)
target_sources(app PRIVATE driver/blink.c)

# target_sources(app PRIVATE driver/ble_nus.c)

target_sources(app PRIVATE driver/ble_hid.c)
target_sources(app PRIVATE driver/hog.c)
