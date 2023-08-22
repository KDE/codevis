include_directories(SYSTEM ${LLVM_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty)

find_package(Python COMPONENTS Development REQUIRED)
find_package(Threads REQUIRED)

ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

# Windows does not support yet the LLVM libraries as a shared library.
# Specify all of them by hand. =/
if (USE_LLVM_STATIC_LIBS OR MSVC)
    SET(CLANG_EXTRA_LIBRARIES
        clangIndex
        clangFormat
        clangTooling
        clangToolingInclusions
        clangToolingCore
        clangFrontend
        clangParse
        clangSerialization
        clangSema
        clangAST
        clangLex
        clangDriver
        clangBasic
    )
else()
    SET(CLANG_EXTRA_LIBRARIES
        clang-cpp
        LLVM
    )
endif()

AddTargetLibrary(
    LIBRARY_NAME
        lvtclp
    SOURCES
        ct_lvtclp_clputil.cpp
        ct_lvtclp_compilerutil.cpp
        ct_lvtclp_componentutil.cpp
        ct_lvtclp_diagnostic_consumer.cpp
        ct_lvtclp_filesystemscanner.cpp
        ct_lvtclp_fileutil.cpp
        ct_lvtclp_headercallbacks.cpp
        ct_lvtclp_logicaldepscanner.cpp
        ct_lvtclp_logicaldepvisitor.cpp
        ct_lvtclp_logicalpostprocessutil.cpp
        ct_lvtclp_physicaldepscanner.cpp
        ct_lvtclp_staticfnhandler.cpp
        ct_lvtclp_tool.cpp
        ct_lvtclp_toolexecutor.cpp
        ct_lvtclp_visitlog.cpp
    HEADERS
        ct_lvtclp_clputil.h
        ct_lvtclp_compilerutil.h
        ct_lvtclp_componentutil.h
        ct_lvtclp_filesystemscanner.h
        ct_lvtclp_fileutil.h
        ct_lvtclp_headercallbacks.h
        ct_lvtclp_logicaldepscanner.h
        ct_lvtclp_logicaldepvisitor.h
        ct_lvtclp_logicalpostprocessutil.h
        ct_lvtclp_physicaldepscanner.h
        ct_lvtclp_staticfnhandler.h
        ct_lvtclp_toolexecutor.h
        ct_lvtclp_visitlog.h
    QT_HEADERS
        ct_lvtclp_tool.h
    LIBRARIES
        Codethink::lvtmdb
        Codethink::lvtprj
        Codethink::lvtshr
        ${CLANG_EXTRA_LIBRARIES}
        ${SYSTEM_EXTRA_LIBRARIES}
        Qt::Core
        Python::Python
)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_target_properties(lvtclp
        PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} ${SKIP_CLANG_WARNINGS}"
    )
endif()

# add_lvtclp_exec(
#     NAME exec_name
#     SOURCES foo.cpp bar.cpp)
function(add_lvtclp_exec)
    set(options NONE)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES LIBRARIES)
    cmake_parse_arguments(clp "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${clp_NAME} ${clp_SOURCES})

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set_target_properties(${clp_NAME}
            PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} ${SKIP_CLANG_WARNINGS}")
    endif()

    target_link_libraries(${clp_NAME}
        Codethink::lvtclp
        Codethink::lvtshr
        ${clp_LIBRARIES}
    )
endfunction()

add_lvtclp_exec(
    NAME codevis_create_codebase_db
    SOURCES ct_lvtclp_create_codebase_db.m.cpp
    LIBRARIES Qt::Core)

add_lvtclp_exec(
    NAME codevis_merge_databases
    SOURCES ct_lvtclp_merge_databases.m.cpp
    LIBRARIES Qt::Core Threads::Threads
)
install(TARGETS
    codevis_merge_databases
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR})

install(TARGETS
    codevis_create_codebase_db
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR})

add_lvtclp_exec(
    NAME codevis_dump_database
    SOURCES ct_lvtclp_dumpdatabase.m.cpp)

if (COMPILE_TESTS)
    function(add_lvtclp_test)
        set(options NONE)
        set(oneValueArgs NAME)
        set(multiValueArgs SOURCES LIBRARIES)
        cmake_parse_arguments(clp "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        add_lvtclp_exec(NAME ${clp_NAME} SOURCES ${clp_SOURCES} LIBRARIES lvtclp_test ${clp_LIBRARIES} lvttst)
        add_test(NAME ${clp_NAME} COMMAND ${clp_NAME})
    endfunction()

    add_library(lvtclp_test
        ct_lvtclp_testutil.cpp
        ct_lvtclp_testutil.h
    )
    add_library(Codethink::lvtclp_test ALIAS lvtclp_test)
    target_link_libraries(
        lvtclp_test
        Codethink::lvtclp
        Catch2::Catch2
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_relationships
        SOURCES
            ct_lvtclp_testrelationships.t.cpp
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_sourcefile
        SOURCES
            ct_lvtclp_testsourcefile.t.cpp
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_namespace
        SOURCES
            ct_lvtclp_testnamespace.t.cpp
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_function
        SOURCES
            ct_lvtclp_testfunction.t.cpp
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_physicalandtemplates
        SOURCES
            ct_lvtclp_testphysicalandtemplates.t.cpp
    )
    add_lvtclp_test(
        NAME test_ct_lvtclp_udt
        SOURCES
            ct_lvtclp_testudt.t.cpp
    )
    add_lvtclp_test(
        NAME test_ct_lvtclp_field
        SOURCES
            ct_lvtclp_testfield.t.cpp
    )
    add_lvtclp_test(
        NAME test_ct_lvtclp_var
        SOURCES
            ct_lvtclp_testvar.t.cpp
    )
    add_lvtclp_test(
        NAME test_ct_lvtclp_filesystemscanner
        SOURCES
            ct_lvtclp_filesystemscanner.t.cpp
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_fileupdatemgr_physical
        SOURCES
            ct_lvtclp_fileupdatemgr_physical.t.cpp
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_tool
        SOURCES
            ct_lvtclp_tool.t.cpp
    )
    set_property(TEST test_ct_lvtclp_tool PROPERTY ENVIRONMENT "TEST_PRJ_PATH=${CMAKE_CURRENT_SOURCE_DIR}/systemtests/")

    add_lvtclp_test(
        NAME test_ct_lvtclp_logicalpostprocessutil
        SOURCES
            ct_lvtclp_logicalpostprocessutil.t.cpp
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_clputil
        SOURCES
            ct_lvtclp_clputil.t.cpp
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_physicaldepscanner
        SOURCES
            ct_lvtclp_physicaldepscanner.t.cpp
    )

    add_lvtclp_test(
        NAME test_ct_lvtclp_logicaldepscanner
        SOURCES
            ct_lvtclp_logicaldepscanner.t.cpp
    )
endif()
    
if (CMAKE_COMPILER_IS_GNUCXX AND ENABLE_CODE_COVERAGE)
    setup_target_for_coverage_lcov(
        NAME lvtclp_coverage
        EXECUTABLE ../lvtclp/run_tests.sh
        EXCLUDE "/usr/*"
                "*.m.cpp"
                "*.t.cpp"
                "build*/*"
                "desktopapp/*"
                "lvtclr/*"
                "lvtgrps/*"
                "lvtmdl/*"
                "lvtqtc/*"
                "lvtqtd/*"
                "lvtqtw/*"
                "lvtshr/*"
                "lvtwdg/*"
                "submodules/*"
         )
endif()

SET(INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
configure_file(
    parse_bde.sh.in
    ${CMAKE_BINARY_DIR}/parse_bde.sh
)

install(FILES
    ${CMAKE_BINARY_DIR}/parse_bde.sh
    PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS codevis_dump_database DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS lvtclp DESTINATION "${CMAKE_INSTALL_LIBDIR}")

if (APPLE)
    # See desktopapp/CMakeLists.txt for copying the headers into the bundle
    # codevis.app/Contents/MacOS/codevis ->
    # codevis.app/Contents/Resources/include
    add_compile_definitions(CT_CLANG_HEADERS_RELATIVE_DIR="../Resources/include")
elseif (WIN32)
    # Copying the headers into the build folder happens on llvm-build.ba
    # but we need to set this directory.
    # Winbdows threats directories without the first \ as relatives to the current folder.
    add_compile_definitions(CT_CLANG_HEADERS_RELATIVE_DIR="clang")
endif()
