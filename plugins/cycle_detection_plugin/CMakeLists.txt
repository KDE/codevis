cmake_minimum_required(VERSION 3.10)

set(PLUGIN_NAME cycle-detection-plugin)
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
target_link_libraries(${PLUGIN_NAME} Qt5::Core)
