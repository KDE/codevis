cmake_minimum_required(VERSION 3.10)

project(CodeVis CXX C)

option(USE_LLVM_STATIC_LIBS "Prefer the static llvm libs instead of the shared ones" OFF)
option(COMPILE_TESTS "Compile tests (Needs Catch2 dependency installed)" ON)
option(ENABLE_CODE_COVERAGE "Compile the app with code coverage support" OFF)
option(BUILD_DESKTOP_APP "Compile the desktop app, and not just command line" ON)
option(BUILD_PLUGINS "Compile all application plugins in the plugins/ directory" ON)
option(USE_QT_WEBENGINE "If set, will compile using Qt Webengine (preferred) to render HTML reports" ON)
option(ENABLE_PYTHON_PLUGINS "If set, will consider plugins implemented in Python" ON)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE
        STRING "Minimum OS X deployment version"
        FORCE
    )
endif()

if(EXISTS "${CMAKE_BINARY_DIR}/conan_paths.cmake")
    message("Found conan paths")
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
else()
    message("Couldn't find conan paths")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/submodules/configuration-parser/cmake" ${CMAKE_MODULE_PATH})

enable_testing()

include(GNUInstallDirs)
include(AddTargetLibrary)
include(ConfigurationCompiler)
include(GenerateExportHeader)
include(GetGitVersion)
include(GetDate)

if (UNIX)
    if (APPLE)
        set(CMAKE_MACOSX_RPATH 1)
    endif()
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

add_subdirectory(submodules/configuration-parser)
set(SOCI_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/soci)

find_package(Doxygen)
find_package(SQLite3)

# This is horrible, but Sqlite3 exists on every package,
# so we can force it to be found.
if (NOT SQLite3_FOUND)
    set(SQLite3_LIBRARIES sqlite3)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Test)
if (BUILD_DESKTOP_APP)
    find_package(Qt5 REQUIRED COMPONENTS Gui Widgets Svg)
    if (USE_QT_WEBENGINE)
        find_package(Qt5 REQUIRED COMPONENTS WebEngineWidgets)
        add_compile_definitions(USE_WEB_ENGINE=1)
    endif()
endif()

if (UNIX AND NOT APPLE)
    find_package(Qt5 REQUIRED COMPONENTS DBus)
endif()

find_package(Clang REQUIRED)

if (COMPILE_TESTS)
    find_package(Catch2 REQUIRED)
    if (Catch2_VERSION_MAJOR EQUAL 2)
        SET(CATCH2_INTERNAL_INCLUDE_FILE "catch2/catch.hpp")
        configure_file(
            "${PROJECT_SOURCE_DIR}/catch2-local-includes.h.in"
            "${PROJECT_BINARY_DIR}/catch2-local-includes.h"
        )
    else()
        SET(CATCH2_INTERNAL_INCLUDE_FILE "catch2/catch_all.hpp")
        configure_file(
            "${PROJECT_SOURCE_DIR}/catch2-local-includes.h.in"
            "${PROJECT_BINARY_DIR}/catch2-local-includes.h"
        )
    endif()
    INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
endif()

# find header files for clang tools to use for system includes
if (NOT CT_CLANG_HEADERS_DIR)
    set(CT_CLANG_HEADERS_DIR "${LLVM_LIBRARY_DIR}/clang/${LLVM_PACKAGE_VERSION}/include")
endif()
message("Looking for clang tool headers at ${CT_CLANG_HEADERS_DIR}. You can change this by defining CT_CLANG_HEADERS_DIR")
if (NOT EXISTS "${CT_CLANG_HEADERS_DIR}/stddef.h")
    message("Cannot find clang tool headers at ${CT_CLANG_HEADERS_DIR}, Errors might happen in runtime.")
endif()

if (MSVC)
    # On c++17 msvc deprecated the use of inheriting from std::iterator for
    # implementing iterators. and it spams your build with *too many* warnings
    # that are mostly not userfull, specially if this happens inside of libraries,
    # such as boost.
    add_definitions(/D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

    # Silence most warnings. We have *so many* warnings from clang that's
    # impossible to filter anything.
    add_definitions(/W0)
endif()

set(CMAKE_AUTOMOC FALSE)
set(CMAKE_AUTOUIC FALSE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

option(SCANBUILD "Trimmed build for scan-build analysis" OFF)
if (SCANBUILD)
  add_compile_definitions(CT_SCANBUILD)
endif()

if (DOXYGEN_FOUND)
  add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/doc/reference
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/doc/reference
    COMMENT "Creating output directory ... "
  )
  add_custom_target(doxygen
    ${DOXYGEN_EXECUTABLE}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DEPENDS ${PROJECT_SOURCE_DIR}/doc/reference
    COMMENT "Doxygen ...")
endif (DOXYGEN_FOUND)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # warnings encountered in llvm/clang headers
    set(SKIP_CLANG_WARNINGS "-Wno-unused-parameter -Wno-comment -Wno-strict-aliasing -Wno-nonnull -Wno-deprecated-declarations")

    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror -Wno-gnu-zero-variadic-macro-arguments ${SKIP_CLANG_WARNINGS}")

    # Old versions of gnu don't link to stdfs by default.
    if (NOT APPLE)
        set(SYSTEM_EXTRA_LIBRARIES stdc++fs)
    endif()
endif()


if (APPLE)
    SET(MACOSX_BUNDLE_ICON_FILE iconset.icns)
endif()

qt5_add_resources(resources_SRCS
    resources.qrc
)
add_library(resources STATIC ${resources_SRCS})

get_git_version(GIT_VERSION)
get_git_authors(GIT_AUTHORS)
get_date(CURRENT_DATE)

configure_file (
    "${PROJECT_SOURCE_DIR}/version.h.in"
    "${PROJECT_BINARY_DIR}/version.h"
)

set(SYSTEM_EXTRA_LIBRARIES ${SYSTEM_EXTRA_LIBRARIES} ${SQLite3_LIBRARIES})

add_subdirectory(thirdparty)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if (ENABLE_CODE_COVERAGE)
        # For code coverage, must be in Debug mode with no optimizations
        include(CodeCoverage)
        append_coverage_compiler_flags()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    else()
        if (CMAKE_BUILD_TYPE MATCHES DEBUG)
            set(DEFAULT_BUILD_OPTIMIZATION OFF)
        else()
            set(DEFAULT_BUILD_OPTIMIZATION ON)
        endif()
        option(ENABLE_OPTIMIZATIONS "Enable optimizations" ${DEFAULT_BUILD_OPTIMIZATION})

        if (ENABLE_OPTIMIZATIONS)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
        endif()
    endif()
endif()

if (ENABLE_PYTHON_PLUGINS)
    add_compile_definitions(ENABLE_PYTHON_PLUGINS=1)
endif()

add_subdirectory(lvtcgn)
add_subdirectory(lvtclp)
add_subdirectory(lvtldr)
add_subdirectory(lvtmdb)
add_subdirectory(lvtprj)
add_subdirectory(lvtplg)
add_subdirectory(lvtshr)
add_subdirectory(lvttst)

if (BUILD_DESKTOP_APP)
    if (WIN32)
        set(CONFIGURATION_SRCS preferences.cpp preferences.h)
        qt5_wrap_cpp(CONFIGURATION_SRCS preferences.h)
    else()
        compile_configuration(CONFIGURATION_SRCS EXPORT_HEADER lakospreferences preferences.conf)
    endif()

    add_library(lakospreferences SHARED ${CONFIGURATION_SRCS})
    generate_export_header(lakospreferences)

    target_link_libraries(lakospreferences Qt5::Core Qt5::Gui Qt5::Widgets)

    target_include_directories(lakospreferences
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/lakos/preferences>
    )

    install(TARGETS
        lakospreferences
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    add_subdirectory(lvtmdl)
    add_subdirectory(lvtclr)
    add_subdirectory(lvtqtc)
    add_subdirectory(lvtqtd)
    add_subdirectory(lvtqtw)
    add_subdirectory(desktopapp)
endif()

if (BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

# 5.79.0 is when ECM got GitCommitHooks
find_package(ECM "5.79.0" NO_MODULE)
if (ECM_VERSION VERSION_GREATER_EQUAL 5.79.0)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})
    include(KDEGitCommitHooks)
    kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND ENABLE_CODE_COVERAGE)
    # Warning: Not setting up any TARGET below. Assume ctest will be run before running `make coverage` so that we dont
    # need to keep track of which tests are dependencies for the coverage (all of them are).
    setup_target_for_coverage_gcovr_html(
        NAME coverage
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif()
