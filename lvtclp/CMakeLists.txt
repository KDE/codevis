include_directories(SYSTEM ${LLVM_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty)

find_package(Python COMPONENTS Development REQUIRED)

include (AddCodevisExecutable)

ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

LINK_DIRECTORIES(
    ${CLANG_LIBRARY_DIRS}
)

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
elseif(APPLE)
    # Craft - The tool we use to manage builds on windows and apple
    # probably has a bug on the llvm recipe that makes libLLVM.dylib
    # not useful. I'v discovered that by linking to libLLVMSupport we at least
    # have a successfull build.
    SET(CLANG_EXTRA_LIBRARIES
        clang-cpp
        LLVMSupport
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
        ct_lvtclp_cpp_tool.cpp
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
        ct_lvtclp_cpp_tool.h
    LIBRARIES
        Codethink::lvtmdb
        Codethink::lvtprj
        Codethink::lvtshr
        ${CLANG_EXTRA_LIBRARIES}
        ${SYSTEM_EXTRA_LIBRARIES}
        Qt${QT_MAJOR_VERSION}::Core
        Python::Python
)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_target_properties(lvtclp
        PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} ${SKIP_CLANG_WARNINGS}"
    )
endif()

if (ENABLE_FORTRAN_SCANNER)
    set(CTFortranLibsIfAvailable "lvtclp_fortran")
endif()

if (COMPILE_TESTS)
    function(add_lvtclp_test)
        set(options NONE)
        set(oneValueArgs NAME)
        set(multiValueArgs SOURCES LIBRARIES)
        cmake_parse_arguments(clp "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        add_codevis_exec(NAME ${clp_NAME} SOURCES ${clp_SOURCES} LIBRARIES lvtclp_test ${clp_LIBRARIES} lvttst NOINSTALL)
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
        NAME test_ct_lvtclp_compilerutil
        SOURCES
            ct_lvtclp_compilerutil.t.cpp
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
        NAME test_ct_lvtclp_cpp_tool
        SOURCES
            ct_lvtclp_cpp_tool.t.cpp
    )
    set_property(TEST test_ct_lvtclp_cpp_tool PROPERTY ENVIRONMENT "TEST_PRJ_PATH=${CMAKE_CURRENT_SOURCE_DIR}/systemtests/")

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

SET(${CMAKE_CURRENT_SOURCE_DIR}/systemtests/)

configure_file(
    autogen-test-variables.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/autogen-test-variables.h
)

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

if (ENABLE_FORTRAN_SCANNER)
    MESSAGE("Fortran support ENABLED")

    # MLIR is required by flang
    find_package(MLIR REQUIRED PATHS "${LLVM_LIBRARY_DIR}/cmake/mlir/" NO_DEFAULT_PATH)
    find_package(Flang REQUIRED PATHS "${LLVM_LIBRARY_DIR}/cmake/flang/" NO_DEFAULT_PATH)

    AddTargetLibrary(
            LIBRARY_NAME
            lvtclp_fortran
            SOURCES
            fortran/ct_lvtclp_logicalscanner.cpp
            fortran/ct_lvtclp_physicalscanner.cpp
            fortran/ct_lvtclp_fortran_tool.cpp
            fortran/ct_lvtclp_fortran_c_interop.cpp
            HEADERS
            fortran/ct_lvtclp_logicalscanner.h
            fortran/ct_lvtclp_physicalscanner.h
            fortran/ct_lvtclp_fortran_tool.h
            fortran/ct_lvtclp_fortran_c_interop.h
            LIBRARIES
            Codethink::lvtmdb
            flangFrontend
            flangFrontendTool
            ${SYSTEM_EXTRA_LIBRARIES}
    )
    target_include_directories(lvtclp_fortran
        PRIVATE
        ${LLVM_INCLUDE_DIR}
    )
    target_compile_definitions(lvtclp_fortran
        PRIVATE
        FLANG_LITTLE_ENDIAN=1
    )

    if (COMPILE_TESTS)
        function(add_lvtclp_fortran_test)
            set(options NONE)
            set(oneValueArgs NAME)
            set(multiValueArgs SOURCES LIBRARIES)
            cmake_parse_arguments(clp "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

            if (ENABLE_FORTRAN_SCANNER)
                set(CTFortranLibsIfAvailable "lvtclp_fortran")
            else()
                set(CTFortranLibsIfAvailable "")
            endif()
            add_lvtclp_exec(NAME ${clp_NAME} SOURCES ${clp_SOURCES} LIBRARIES lvtclp lvtclp_test lvttst ${CTFortranLibsIfAvailable})
            add_test(NAME ${clp_NAME} COMMAND ${clp_NAME})
        endfunction()

        add_lvtclp_fortran_test(
            NAME test_ct_lvtclp_fortranscanner
            SOURCES
                fortran/ct_lvtclp_fortranscanner.t.cpp
        )
    endif()
else()
    MESSAGE("Fortran support DISABLED")
endif()
