cmake_minimum_required(VERSION 3.10)

project(basicplugin CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2")

include_directories($ENV{LVTPLG_HDRS})

add_library(basicplugin
    SHARED
    plugin.cpp
)
set_target_properties(basicplugin PROPERTIES PREFIX "")
target_compile_options(basicplugin PRIVATE "-fvisibility=default")
set_property(TARGET basicplugin PROPERTY POSITION_INDEPENDENT_CODE ON)
add_custom_command(TARGET basicplugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/README.md README.md
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/metadata.json metadata.json
)
add_dependencies(basicplugin generate_lvtclp_plugin_headers)
