cmake_minimum_required(VERSION 3.13.0)

set(BOARD stm32f4_disco)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(threads_demo)

target_sources(app PRIVATE src/main.c)
