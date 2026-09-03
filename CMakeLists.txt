add_library(kfsw_platform STATIC
    src/reset.c
    src/time.c
)

target_sources_ifdef(CONFIG_KFSW_STORAGE kfsw_platform PRIVATE
    src/storage.c
)

target_sources_ifdef(CONFIG_KFSW_WATCHDOG kfsw_platform PRIVATE
    src/watchdog.c
)

add_library(kfsw::platform ALIAS kfsw_platform)

target_include_directories(kfsw_platform
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(kfsw_platform
    PUBLIC
        zephyr_generated_headers
        zephyr_interface
)

target_link_libraries_ifdef(CONFIG_KFSW_STORAGE kfsw_platform PUBLIC
    FS
)
