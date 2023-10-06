cmake_minimum_required(VERSION 3.10)

set(PLUGIN_NAME code_coverage_plugin)
project(${PLUGIN_NAME} CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2")

MESSAGE("LVTPLG_HDRS = $ENV{LVTPLG_HDRS}")
include_directories($ENV{LVTPLG_HDRS})

add_library(${PLUGIN_NAME}
    SHARED
    plugin.cpp
)
target_compile_options(${PLUGIN_NAME} PRIVATE "-fvisibility=default")
set_property(TARGET ${PLUGIN_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${PLUGIN_NAME} Qt::Core)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/README.md
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/metadata.json
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/README.md ${CMAKE_CURRENT_BINARY_DIR}/README.md
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/metadata.json ${CMAKE_CURRENT_BINARY_DIR}/metadata.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/metadata.json
    COMMENT "[${PLUGIN_NAME}] Copying README and metadata files..."
    VERBATIM
)
add_custom_target(
    ${PLUGIN_NAME}_extra_files ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/README.md
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/metadata.json
)
add_dependencies(${PLUGIN_NAME} ${PLUGIN_NAME}_extra_files)

add_dependencies(${PLUGIN_NAME} generate_lvtclp_plugin_headers)
