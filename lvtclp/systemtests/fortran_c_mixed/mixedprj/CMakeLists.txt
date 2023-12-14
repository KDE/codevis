add_executable(mixedprj
    a.f
    c.c
    main.c

    # b.f is included in a.f, but omitted here on purpose, to check if the parser will find it.

    # other.h is included in main.c, but omitted here on purpose. It has a Fortran function declaration
    # without any definition (The definition is on Fortran code). The parser must be able to find it.
)
