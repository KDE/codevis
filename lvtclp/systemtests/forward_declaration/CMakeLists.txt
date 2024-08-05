cmake_minimum_required(VERSION 3.18)
project(FortranAndCMixed LANGUAGES CXX)

add_executable(test_exec ForwardDeclared.cpp ClassDefinition.cpp main.cpp)
