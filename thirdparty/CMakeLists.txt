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

set(QUAZIP_HEADERS
  quazip/quazip/ioapi.h
  quazip/quazip/quaadler32.h
  quazip/quazip/quagzipfile.h
  quazip/quazip/quazipfile.h
  quazip/quazip/quazip.h
  quazip/quazip/unzip.h
  quazip/quazip/JlCompress.h
  quazip/quazip/quachecksum32.h
  quazip/quazip/quaziodevice.h
  quazip/quazip/quazipfileinfo.h
  quazip/quazip/quazipnewinfo.h
  quazip/quazip/zip.h
  quazip/quazip/minizip_crypt.h
  quazip/quazip/quacrc32.h
  quazip/quazip/quazipdir.h
  quazip/quazip/quazip_global.h
  quazip/quazip/quazip_qt_compat.h
)

set (QUAZIP_SOURCES
  quazip/quazip/unzip.c
  quazip/quazip/zip.c
  quazip/quazip/JlCompress.cpp
  quazip/quazip/quaadler32.cpp
  quazip/quazip/quacrc32.cpp
  quazip/quazip/quaziodevice.cpp
  quazip/quazip/quazipdir.cpp
  quazip/quazip/quazipfileinfo.cpp
  quazip/quazip/qioapi.cpp
  quazip/quazip/quachecksum32.cpp
  quazip/quazip/quagzipfile.cpp
  quazip/quazip/quazip.cpp
  quazip/quazip/quazipfile.cpp
  quazip/quazip/quazipnewinfo.cpp
)

qt5_wrap_cpp(QUAZIP_MOC ${QUAZIP_HEADERS})

add_library(quazip SHARED ${QUAZIP_HEADERS} ${QUAZIP_SOURCES} ${QUAZIP_MOC})
generate_export_header(quazip)

target_link_libraries(quazip Qt5::Core)

target_include_directories(quazip
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/quazip/quazip
  $<INSTALL_INTERFACE:include/lakos/lvtqtw>
)

if (APPLE)
    # On MacOS we need to tell CMake where to look for bison so that we
    # find Bison 3.x from brew instead of Bison 2.x from the system
    set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
    set(CMAKE_SYSTEM_PREFIX_PATH /usr/local/opt/bison:/usr)
    set(CMAKE_SYSTEM_PROGRAM_PATH /usr/local/opt/bison/bin;/usr/bin)
endif()
