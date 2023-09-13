cmake_minimum_required(VERSION 3.10)

set(PLUGIN_NAME lakosian_test_rules_plugin)
project(${PLUGIN_NAME} CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O2")

MESSAGE("LVTPLG_HDRS = $ENV{LVTPLG_HDRS}")
include_directories($ENV{LVTPLG_HDRS})

add_library(${PLUGIN_NAME}
    SHARED
    plugin.cpp
    utils.h
)
target_compile_options(${PLUGIN_NAME} PRIVATE "-fvisibility=default")
set_property(TARGET ${PLUGIN_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/README.md
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/README.md ${CMAKE_CURRENT_BINARY_DIR}/README.md
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    COMMENT "[${PLUGIN_NAME}] Copying README.md file..."
    VERBATIM
)
add_custom_target(
    ${PLUGIN_NAME}_extra_files ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/README.md
)
add_dependencies(${PLUGIN_NAME} ${PLUGIN_NAME}_extra_files)
