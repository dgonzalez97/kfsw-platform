add_library(kfsw_platform STATIC
    src/hardware.c
    src/reset.c
    src/time.c
    src/wallclock.c
)

target_sources_ifdef(CONFIG_KFSW_STORAGE kfsw_platform PRIVATE
    src/storage.c
)

target_sources_ifdef(CONFIG_KFSW_WATCHDOG kfsw_platform PRIVATE
    src/watchdog.c
)

if(CONFIG_KFSW_LASTWORDS)
    target_sources(kfsw_platform PRIVATE src/lastwords.c)

    # One of the two always compiles, so a caller never has to ask whether the
    # SoC has a detector before calling: it asks the function, which answers.
    if(CONFIG_KFSW_LASTWORDS_SUPPLY_WATCH AND CONFIG_SOC_SERIES_STM32L4X)
        target_sources(kfsw_platform PRIVATE src/lastwords_stm32.c)
    else()
        target_sources(kfsw_platform PRIVATE src/lastwords_nosupply.c)
    endif()
endif()

add_library(kfsw::platform ALIAS kfsw_platform)

target_include_directories(kfsw_platform
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(kfsw_platform
    PUBLIC
        zephyr_generated_headers
        zephyr_interface
    PRIVATE
        kernel
)

target_link_libraries_ifdef(CONFIG_KFSW_STORAGE kfsw_platform PUBLIC
    FS
)
