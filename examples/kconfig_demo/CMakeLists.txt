cmake_minimum_required(VERSION 3.13.1)

set(ZEPHYR_EXTRA_MODULES "${CMAKE_SOURCE_DIR}/../modules/say_hello")
set(BOARD qemu_x86_64)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(random)

target_sources(app PRIVATE src/main.c)
