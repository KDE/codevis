include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

OPTION(SOCI_SQLITE3 "" ON)
OPTION(SOCI_SHARED "" ON)

OPTION(SOCI_STATIC "" OFF)
OPTION(SOCI_EMPTY "" OFF)
OPTION(SOCI_MYSQL "" OFF)
OPTION(SOCI_ODBC "" OFF)
OPTION(SOCI_POSTGRESQL "" OFF)
OPTION(SOCI_ORACLE "" OFF)
OPTION(SOCI_FIREBIRD "" OFF)
OPTION(SOCI_TESTS "" OFF)

add_subdirectory(soci ${SOCI_BINARY_DIR})

if (BUILD_DESKTOP_APP)
  add_subdirectory(kwidgetsaddons)
  add_subdirectory(MRichTextEditor)
endif()

if (APPLE)
    # On MacOS we need to tell CMake where to look for bison so that we
    # find Bison 3.x from brew instead of Bison 2.x from the system
    set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
    set(CMAKE_SYSTEM_PREFIX_PATH /usr/local/opt/bison:/usr)
    set(CMAKE_SYSTEM_PROGRAM_PATH /usr/local/opt/bison/bin;/usr/bin)
endif()

