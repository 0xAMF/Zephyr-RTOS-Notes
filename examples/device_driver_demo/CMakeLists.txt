cmake_minimum_required(VERSION 3.20.0)

set(BOARD stm32f4_disco)
set(DTC_OVERLAY_FILE "boards/stm32f4_disco.overlay")
set(ZEPHYR_EXTRA_MODULES "${CMAKE_SOURCE_DIR}/../modules/button")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(custom_button)

target_sources(app PRIVATE src/main.c)
