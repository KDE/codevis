cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(../bjgb ../bjgc)

# executable targets

# bjg.t -- the original 'main'
add_executable(bjg.t
               bj.m.220207.cpp)

target_link_libraries(bjg.t PUBLIC bjgb bjgc)
