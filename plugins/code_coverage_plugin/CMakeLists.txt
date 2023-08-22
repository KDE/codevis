cmake_minimum_required(VERSION 3.10)

project(code-cov-plugin CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2")

MESSAGE("LVTPLG_HDRS = $ENV{LVTPLG_HDRS}")
include_directories($ENV{LVTPLG_HDRS})

add_library(code-cov-plugin
    SHARED
    plugin.cpp
)
target_compile_options(code-cov-plugin PRIVATE "-fvisibility=default")
set_property(TARGET code-cov-plugin PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(code-cov-plugin Qt::Core)
