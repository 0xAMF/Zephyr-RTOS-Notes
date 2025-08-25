# check if the configuration is set in Kconfig
if (CONFIG_SAY_HELLO)
    # include the say_hello header for zephyr
    zephyr_include_directories(.)
    zephyr_library_sources(say_hello.c)

endif()
