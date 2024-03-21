include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty)
include_directories(.)

AddTargetLibrary(
    LIBRARY_NAME
        codevis_project_helpers
    SOURCES
        merge_project_databases.cpp
        merge_project_databases.h
    LIBRARIES
        Codethink::lvtshr
        Codethink::lvtmdb
        Codethink::lvtprj
)

if (COMPILE_TESTS)
    add_executable(test_codevis_project_helpers merge_project_databases.t.cpp)
    target_link_libraries(test_codevis_project_helpers Codethink::lvtprj
        lvttst
        Codethink::codevis_project_helpers
        Qt${QT_MAJOR_VERSION}::Core
    )
    add_test(NAME test_codevis_project_helpers COMMAND test_codevis_project_helpers)
endif()
