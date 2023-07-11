project(simple_class_project)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(groups)

add_executable(simple_class_project simple_class_project.m.cpp)
target_link_libraries(simple_class_project
    prjact
    prjvhc
)
