# BDE Codebase Database Creation Tool

# Introduction

Parses the classes in C++ source and creates a SQLite database for the application to visualise.

The parser learns about a project through a `compile_commands.json` file,
generated by CMake. For a simple CMake project, this can be generated by adding
the `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to your cmake invocation.

Code is parsed using clang so it is best to configure your build to use clang.
This can be configured in CMake using the `CC` and `CXX` environment variables.
Bugs parsing code configured for other compilers have not been observed for bde
but this is possible where compiler-specific features are used.

## Steps to parse BDE (or any project built using bde-tools)
See doc/command_line_codebase_generation.md
