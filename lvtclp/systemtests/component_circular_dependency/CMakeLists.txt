cmake_minimum_required(VERSION 3.10)

project(demo-project CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    groups/one/onepkg
    groups/two/twodmo
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
endif()

add_library(demo-project
    groups/one/onepkg/ct_onepkg_circle.cpp
    groups/one/onepkg/ct_onepkg_thing.cpp
    groups/two/twodmo/ct_twodmo_top.cpp

    groups/one/onepkg/ct_onepkg_circle.h
    groups/one/onepkg/ct_onepkg_thing.h
    groups/two/twodmo/ct_twodmo_top.h
)
