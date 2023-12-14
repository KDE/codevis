cmake_minimum_required(VERSION 3.10)
project(FortranAndCMixed LANGUAGES Fortran C)
enable_language(Fortran)
enable_language(C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(mixedprj)
