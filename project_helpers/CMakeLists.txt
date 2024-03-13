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
