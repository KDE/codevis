project(CppGeneralTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(helloworld hello.m.cpp third.cpp)
add_library(other other.cpp)
