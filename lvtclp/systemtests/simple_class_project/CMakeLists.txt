project(simple_class_project)

cmake_minimum_required(VERSION 3.10)

add_subdirectory(prjact)
add_subdirectory(prjvhc)

add_executable(simple_class_project simple_class_project.m.cpp)
target_link_libraries(simple_class_project
    prjact
    prjvhc
)
