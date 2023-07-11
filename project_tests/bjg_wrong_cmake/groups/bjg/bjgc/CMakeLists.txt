cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB BJGC_SRCS *.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(bjgc STATIC ${BJGC_SRCS})
target_include_directories(bjgc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(bjgc BlackJack::bjgb)
add_library(BlackJack::bjgc ALIAS bjgc)

# executable targets

# bjgc_dealertable.t
add_executable(bjgc_dealertable.t
               bjgc_dealertable.t.cpp)
target_include_directories(bjgc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(bjgc_dealertable.t PUBLIC BlackJack::bjgc)

# bjgc_playertable.t
add_executable(bjgc_playertable.t
               bjgc_playertable.t.cpp)

target_include_directories(bjgc_playertable.t
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(bjgc_playertable.t PUBLIC BlackJack::bjgc)
