cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(./)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include(GenerateExportHeader)

file(GLOB BJGB_SRCS *.cpp)

add_library(bjgb STATIC ${BJGB_SRCS})
generate_export_header(bjgb)

target_include_directories(bjgb
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
add_library(BlackJack::bjgb ALIAS bjgb)

# executable targets

# bjgb_dealercount.t
add_executable(bjgb_dealercount.t
               bjgb_dealercount.t.cpp bjgb_dealercount.cpp)

# bjgb_rank.t
add_executable(bjgb_rank.t
               bjgb_rank.t.cpp bjgb_rank.cpp)

# bjgb_shoe.t
add_executable(bjgb_shoe.t
               bjgb_shoe.t.cpp bjgb_shoe.cpp
               bjgb_rank.cpp)

# bjgb_shoeutil.t
add_executable(bjgb_shoeutil.t
               bjgb_shoeutil.t.cpp bjgb_shoeutil.cpp
               bjgb_rank.cpp       bjgb_shoe.cpp)

# bjgb_state.t
add_executable(bjgb_state.t
               bjgb_state.t.cpp bjgb_state.cpp)

# bjgb_types.t
add_executable(bjgb_types.t
               bjgb_types.t.cpp bjgb_types.cpp)
