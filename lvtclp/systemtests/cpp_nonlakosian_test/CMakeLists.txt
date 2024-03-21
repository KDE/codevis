project(CppProg)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(someprog
    mylibs/lib1/utils/lib1_utils.h
    mylibs/lib1/lib1.h
    mylibs/lib1/lib1.cpp
    mylibs/lib2/lib2.h
    mylibs/lib2/lib2.cpp
    main.cpp
)
