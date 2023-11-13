project(FortranAndCMixed LANGUAGES Fortran C)
enable_language(Fortran)
enable_language(C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(helloworld
    a.f
    # b.f is included in a.f, but omitted here on purpose, to check if the parser will find it.
    c.c
    main.c
)
