cmake_minimum_required(VERSION 3.20.0)

set(DTC_OVERLAY_FILE boards/stm32f4_disco.overlay)
set(BOARD stm32f4_disco)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})


project(
    EmptyApp
    VERSION 0.1
    DESCRIPTION "Empty zephyr application."
    LANGUAGES C
)

target_sources(
    app
    PRIVATE
    src/main.c
)
