project(FortranAndCMixed LANGUAGES Fortran)
enable_language(Fortran)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(helloworld
    a.f
    b.f
)
