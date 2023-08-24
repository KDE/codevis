cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "23")
set(RELEASE_SERVICE_VERSION_MINOR "11")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(CodeVis LANGUAGES CXX C VERSION ${RELEASE_SERVICE_VERSION})

option(USE_LLVM_STATIC_LIBS "Prefer the static llvm libs instead of the shared ones" OFF)
option(COMPILE_TESTS "Compile tests (Needs Catch2 dependency installed)" ON)
option(ENABLE_CODE_COVERAGE "Compile the app with code coverage support" OFF)
option(BUILD_DESKTOP_APP "Compile the desktop app, and not just command line" ON)
option(BUILD_PLUGINS "Compile all application plugins in the plugins/ directory" ON)
option(USE_QT_WEBENGINE "If set, will compile using Qt Webengine (preferred) to render HTML reports" ON)
option(ENABLE_PYTHON_PLUGINS "If set, will consider plugins implemented in Python" ON)
option(IGNORE_BROKEN_TESTS_KDE_CI "If set, ignores some broken tests on LLVM 16 / KDE CI." ON)

set(QT_MIN_VERSION "5.15.2")
set(KF5_DEP_VERSION "5.90.0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/submodules/configuration-parser/cmake)

find_package(ECM ${KF5_DEP_VERSION} QUIET REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# This Breaks Soci.
# For the future we should migrate to QtSql or something.
# Soci does *not* have a good policy regarding releases,
# and the current codebase is full of smaller c++ antipatterns
# include(KDECompilerSettings NO_POLICY_SCOPE)

# This Breaks Catch2
# Catch2 Broke it's API on a minor release, and this made
# me quite sad. It's also sad to see that it doesn't
# use export headers, too, so it's hard to use it on a
# shared library.
#include(KDECMakeSettings)

include(KDEInstallDirs)
include(KDEClangFormat)
include(KDEGitCommitHooks)

include(ECMOptionalAddSubdirectory)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMDeprecationSettings)

include(AddTargetLibrary)
include(ConfigurationCompiler)
include(GenerateExportHeader)
include(GetGitVersion)
include(GetDate)
include(FeatureSummary)

find_package(Doxygen)
find_package(SQLite3 REQUIRED)
find_package(Clang REQUIRED)

set(CMAKE_AUTOMOC FALSE)
set(CMAKE_AUTOUIC FALSE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
    set(KF_MIN_VERSION "5.240.0")
    set(KF_MAJOR_VERSION "6")
else()
    set(KF_MAJOR_VERSION "5")
endif()

# Set a default build type if none was specified
set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

if (UNIX)
    if (APPLE)
        set(CMAKE_MACOSX_RPATH 1)
    endif()
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

add_subdirectory(submodules/configuration-parser)
set(SOCI_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/soci)

# Required here so that the version comparison below works
find_package(Qt${QT_MAJOR_VERSION}Core ${QT_MIN_VERSION} CONFIG REQUIRED)
if (QT_MAJOR_VERSION STREQUAL "6")
    find_package(Qt6Core5Compat)
    find_package(QuaZip-Qt6 REQUIRED)
else()
    find_package(QuaZip-Qt5 REQUIRED)
endif()
message("Found ${QT_MAJOR_VERSION}")

if (BUILD_DESKTOP_APP)
    find_package(Qt${QT_MAJOR_VERSION} CONFIG REQUIRED COMPONENTS Gui Widgets Svg)
    if (USE_QT_WEBENGINE)
        find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS WebEngineWidgets)
        add_compile_definitions(USE_WEB_ENGINE=1)
    endif()
    if (UNIX AND NOT APPLE)
        find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS DBus)
    endif()
endif()


if (COMPILE_TESTS)
    enable_testing()
    find_package(Qt${QT_MAJOR_VERSION} CONFIG REQUIRED COMPONENTS Test)

    set(TEST_PRJ_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lvtclp/systemtests)
    set(LAKOSDIAGRAM_PYSCRIPTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/python/codegeneration)

    configure_file(
        "test-project-paths.h.in"
        "test-project-paths.h"
    )

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
    if (NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(SYSTEM_EXTRA_LIBRARIES stdc++fs)
    endif()
endif()


if (APPLE)
    SET(MACOSX_BUNDLE_ICON_FILE iconset.icns)
endif()

qt_add_resources(resources_SRCS
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
        qt_wrap_cpp(CONFIGURATION_SRCS preferences.h)
    else()
        compile_configuration(CONFIGURATION_SRCS EXPORT_HEADER lakospreferences preferences.conf)
    endif()

    add_library(lakospreferences SHARED ${CONFIGURATION_SRCS})
    generate_export_header(lakospreferences)

    target_link_libraries(lakospreferences Qt::Core Qt::Gui Qt::Widgets)

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

include(KDEGitCommitHooks)
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND ENABLE_CODE_COVERAGE)
    # Warning: Not setting up any TARGET below. Assume ctest will be run before running `make coverage` so that we dont
    # need to keep track of which tests are dependencies for the coverage (all of them are).
    setup_target_for_coverage_gcovr_html(
        NAME coverage
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif()

feature_summary(INCLUDE_QUIET_PACKAGES WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/database-spec/cad_db.sql
    ${CMAKE_CURRENT_SOURCE_DIR}/database-spec/codebase_db.sql
  DESTINATION
    ${KDE_INSTALL_DATAROOTDIR}/codevis/database-spec
)
