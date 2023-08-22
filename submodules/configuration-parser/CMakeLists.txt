#=============================================================================
# SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
# SPDX-FileCopyrightText: Alexander Lohnau <alexander.lohnau@gmx.de>
#
# SPDX-License-Identifier: MIT
#=============================================================================

project(configurationGeneration)
cmake_minimum_required(VERSION 3.10)

if(EXISTS "${CMAKE_BINARY_DIR}/conan_paths.cmake")
    message("Found conan paths")
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Older versions of gcc don't link automatically to stdc++fs.
    if(NOT APPLE)
        set(EXTRA_SYSTEM_LIBRARIES stdc++fs)
    endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/ConfigurationParser")

find_package(Qt6 COMPONENTS Core)
if (NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core)
endif()

include_directories(.)

add_library(confgen_lib STATIC
    parser/statemachine.cpp
    common/string_helpers.cpp
    common/dump_common.cpp
    kconfig/dump_kconfig.cpp
    qobject/dump_qobject.cpp
    qsettings/dump_qsettings.cpp
)
target_link_libraries(confgen_lib Qt::Core)

add_executable(confgen
    main.cpp
)
add_executable(ConfigurationParser::confgen ALIAS confgen)

target_link_libraries(confgen confgen_lib ${EXTRA_SYSTEM_LIBRARIES})
install(TARGETS confgen EXPORT ConfigurationParserTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/ConfigurationParser)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ConfigurationParserConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ConfigurationParserConfig.cmake"
    INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}")

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ConfigurationParserConfigVersion.cmake"
    VERSION "0.0.1"
    COMPATIBILITY ExactVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ConfigurationParserConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ConfigurationParserConfigVersion.cmake"
        "cmake/ConfigurationCompiler.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}")

install(EXPORT ConfigurationParserTargets
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE ConfigurationParserTargets.cmake
        NAMESPACE ConfigurationParser:: )

if (ENABLE_TESTING) 
	enable_testing()
	add_subdirectory(test)
endif()

