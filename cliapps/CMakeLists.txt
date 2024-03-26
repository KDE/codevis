include(AddCodevisExecutable)

include_directories(SYSTEM ${LLVM_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty)

if (ENABLE_FORTRAN_SCANNER)
    set(CTFortranLibsIfAvailable "lvtclp_fortran")
else()
    set(CTFortranLibsIfAvailable "")
endif()

include_directories(
    ${SQLite3_INCLUDE_DIRS}
)

AddTargetLibrary(
  LIBRARY_NAME
    commandlineprogressbar
    QT_HEADERS
        commandlineprogressbar.h
    SOURCES 
        commandlineprogressbar.cpp
        
    LIBRARIES Qt${QT_MAJOR_VERSION}::Core
)


add_codevis_exec(
    NAME codevis_create_codebase_db
    SOURCES 
        codevis_create_codebase_db.m.cpp
        
    LIBRARIES Qt${QT_MAJOR_VERSION}::Core
        Codethink::lvtclp
        Codethink::lvtshr
        commandlineprogressbar
        ${CTFortranLibsIfAvailable}
)

add_codevis_exec(
    NAME codevis_merge_databases
    SOURCES codevis_merge_database.m.cpp
    LIBRARIES Qt${QT_MAJOR_VERSION}::Core
        Threads::Threads
        Codethink::codevis_project_helpers
        Codethink::lvtclp
        Codethink::lvtshr
)

add_codevis_exec(
    NAME codevis_dump_database
    SOURCES codevis_dump_database.m.cpp
    LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Codethink::lvtclp
        Codethink::lvtshr

    )

add_codevis_exec(
    NAME codevis_create_prj_from_db
    SOURCES codevis_create_prj_from_db.m.cpp
    LIBRARIES Qt${QT_MAJOR_VERSION}::Core
        Codethink::lvtclp
        Codethink::lvtldr
        Codethink::lvtshr
)

add_codevis_exec(
    NAME codevis_physical_loader
    SOURCES codevis_physical_loader.m.cpp
    LIBRARIES Qt${QT_MAJOR_VERSION}::Core
        Codethink::lvtclp
        Codethink::lvtshr
        Codethink::lvtldr
    )
