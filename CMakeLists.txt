cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

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
option(USE_QT_WEBENGINE "If set, will compile using Qt Webengine (preferred) to render HTML reports" ON)
option(IGNORE_BROKEN_TESTS_KDE_CI "If set, ignores some broken tests on LLVM 16 / KDE CI." ON)
option(WARNINGS_AS_ERRORS "If set, will force warnings as errors" OFF)
option(ENABLE_FORTRAN_SCANNER "If set, will compile lvtclp_fortran module" OFF)
option(ENABLE_PLUGINS "Compile with plugins support" ON)
option(BUILD_PLUGINS "Compile all application plugins in the plugins/ directory" ON)
option(ENABLE_PYTHON_PLUGINS "If set, will consider plugins implemented in Python" ON)

set(PREFERRED_LLVM_VERSION "" CACHE STRING "If the system has many LLVM versions, provide a preferred one")

if (NOT ENABLE_PLUGINS)
    set(BUILD_PLUGINS OFF)
    set(ENABLE_PYTHON_PLUGINS OFF)
endif()

set(QT_MIN_VERSION "5.12.0")
set(KF5_MIN_VERSION "5.68.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (ENABLE_FORTRAN_SCANNER)
    add_compile_definitions(CT_ENABLE_FORTRAN_SCANNER=1)
endif()

LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/submodules/configuration-parser/cmake)

find_package(ECM ${KF5_MIN_VERSION} QUIET REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
if (ECM_VERSION_MAJOR LESS 5 OR (ECM_VERSION_MAJOR EQUAL 5 AND ECM_VERSION_MINOR LESS 90))
    set(ECM_IS_OLD TRUE)
endif()

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
if (NOT ECM_IS_OLD)
    include(KDEGitCommitHooks)
    kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
endif()

include(ECMOptionalAddSubdirectory)
include(ECMAddAppIcon)
include(ECMInstallIcons)
if (NOT ECM_IS_OLD)
    include(ECMDeprecationSettings)
endif()
include(ECMEnableSanitizers) 

include(AddTargetLibrary)
include(GenerateExportHeader)
include(GetGitVersion)
include(GetDate)
include(CodevisDeployPlugin)

include(FeatureSummary)
find_package(Doxygen)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

foreach(LLVM_VER ${PREFERRED_LLVM_VERSION} 18.1.4 18.1.3 18.1.2 18.1.1 17 16 15 14)
    find_package(LLVM ${LLVM_VER})
    if (LLVM_FOUND)
        break()
    endif()
endforeach()

if (NOT LLVM_FOUND)
    MESSAGE(FATAL "LLVM Not Found")
endif()

find_package(Clang REQUIRED PATHS "${LLVM_LIBRARY_DIR}/cmake/clang/" NO_DEFAULT_PATH)

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
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/lib")
endif()

set(SOCI_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/soci)

# Required here so that the version comparison below works
find_package(Qt${QT_MAJOR_VERSION}Core ${QT_MIN_VERSION} CONFIG REQUIRED)
message("-- Qt Version: ${QT_MAJOR_VERSION}")
message("-- LLVM Version: ${LLVM_VERSION}")

if (BUILD_DESKTOP_APP)
    find_package(Qt${QT_MAJOR_VERSION} CONFIG REQUIRED COMPONENTS Gui Widgets Svg)
    find_package(KF${KF_MAJOR_VERSION} ${KF5_MIN_VERSION} REQUIRED COMPONENTS
        Archive
        Config
        Crash
        CoreAddons
        WidgetsAddons
        TextEditor
        XmlGui
        NewStuffCore
        NewStuff
        Notifications
        KCMUtils
        I18n
        OPTIONAL_COMPONENTS
        DocTools
    )
    # KF5 doesn't have split version, so we build it here
    string(REPLACE "." ";" VERSION_LIST "${KF${KF_MAJOR_VERSION}_VERSION}")
    list(GET VERSION_LIST 0 KF_VERSION_MAJOR)
    list(GET VERSION_LIST 1 KF_VERSION_MINOR)
    list(GET VERSION_LIST 2 KF_VERSION_PATCH)
    if (KF_VERSION_MAJOR LESS 5 OR (KF_VERSION_MAJOR EQUAL 5 AND KF_VERSION_MINOR LESS 90))
        set(KDE_FRAMEWORKS_IS_OLD TRUE)
        add_compile_definitions(KDE_FRAMEWORKS_IS_OLD=1)
    else()
        set(KDE_FRAMEWORKS_IS_OLD FALSE)
    endif()

    if (USE_QT_WEBENGINE)
        find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS WebEngineWidgets)
        add_compile_definitions(USE_WEB_ENGINE=1)
    endif()
    if (UNIX AND NOT APPLE)
        find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS DBus)
    endif()
else()
    find_package(Qt${QT_MAJOR_VERSION} CONFIG REQUIRED COMPONENTS Core)
    find_package(KF${KF_MAJOR_VERSION} ${KF5_MIN_VERSION} REQUIRED COMPONENTS Archive)
endif()

find_package(Catch2)
if (NOT Catch2_FOUND)
    set(COMPILE_TESTS OFF)
endif()

if (COMPILE_TESTS)
    enable_testing()
    find_package(Qt${QT_MAJOR_VERSION} CONFIG REQUIRED COMPONENTS Test)

    set(TEST_PRJ_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lvtclp/systemtests)
    set(TEST_PLG_PATH ${CMAKE_BINARY_DIR}/lvtplg/testplugins)
    set(TEST_QTW_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lvtqtw/testfiles)
    set(LAKOSDIAGRAM_PYSCRIPTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/python/codegeneration)

    configure_file(
        "test-project-paths.h.in"
        "test-project-paths.h"
    )

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
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        SET(SKIP_CLANG_WARNINGS "${SKIP_CLANG_WARNINGS}  -Wno-error=maybe-uninitialized")
    endif()
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments ${SKIP_CLANG_WARNINGS}")
    if (WARNINGS_AS_ERRORS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    endif()
endif()

if (APPLE)
    SET(MACOSX_BUNDLE_ICON_FILE iconset.icns)
endif()

cmake_language(CALL
    qt${QT_MAJOR_VERSION}_add_resources
    resources_SRCS
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

if (KDE_FRAMEWORKS_IS_OLD)
    set(KDE_INSTALL_TARGETS_DEFAULT_ARGS $CMAKE_INSTALL_PREFIX)
    set(KDE_INSTALL_KNSRCDIR $CMAKE_INSTALL_PREFIX/share/knsrcfiles)
    set(KDE_INSTALL_METAINFODIR $CMAKE_INSTALL_PREFIX/share/metainfo)
    set(KDE_INSTALL_APPDIR $CMAKE_INSTALL_PREFIX/share/applications)
    set(KDE_INSTALL_KNOTIFYRCDIR $CMAKE_INSTALL_PREFIX/share/knotifications5)
endif()

add_subdirectory(lvtcgn)
add_subdirectory(lvtclp)
add_subdirectory(lvtldr)
add_subdirectory(lvtmdb)
add_subdirectory(lvtprj)
add_subdirectory(lvtshr)
add_subdirectory(lvttst)
add_subdirectory(project_helpers)
add_subdirectory(cliapps)

if (BUILD_DESKTOP_APP)
    kconfig_add_kcfg_files(CONFIGURATION_SRCS GENERATE_MOC preferences.kcfgc)
    add_library(lakospreferences SHARED ${CONFIGURATION_SRCS})
    add_library(Codethink::lakospreferences ALIAS lakospreferences)
    generate_export_header(lakospreferences)

    target_link_libraries(lakospreferences
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR_VERSION}::ConfigCore
        KF${KF_MAJOR_VERSION}::ConfigGui
    )

    target_include_directories(lakospreferences
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/lakos/preferences>
    )

    install(TARGETS
        lakospreferences
        DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
    )

    add_subdirectory(doc)

    if (ENABLE_PLUGINS)
        add_subdirectory(lvtplg)
        add_subdirectory(plugins)
    endif()

    add_subdirectory(lvtmdl)
    add_subdirectory(lvtclr)
    add_subdirectory(lvtqtc)
    add_subdirectory(lvtqtd)
    add_subdirectory(lvtqtw)

    add_subdirectory(desktopapp)
endif()


if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND ENABLE_CODE_COVERAGE)
    # Warning: Not setting up any TARGET below. Assume ctest will be run before running `make coverage` so that we dont
    # need to keep track of which tests are dependencies for the coverage (all of them are).
    setup_target_for_coverage_gcovr_html(
        NAME coverage
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif()

feature_summary(INCLUDE_QUIET_PACKAGES WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

