add_library(kfsw_platform STATIC
    src/reset.c
    src/time.c
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
