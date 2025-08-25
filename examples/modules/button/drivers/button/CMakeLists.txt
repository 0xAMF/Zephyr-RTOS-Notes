# declare the current directory as a zephyr library
# the name is derived from the directory name if no name is given

zephyr_library()

# list the library's source code
zephyr_library_sources(button.c)

# add header files to the CMake search directories
zephyr_include_directories(.)
