# include required subdirectories
add_subdirectory(drivers)

# add subdirectories to the compiler's include search path
zephyr_include_directories(drivers)
