cmake_minimum_required(VERSION 3.13)
project(bjg VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(groups/bjg/bjgb)

add_subdirectory(groups/bjg/bjgc)

add_subdirectory(groups/bjg/test)
