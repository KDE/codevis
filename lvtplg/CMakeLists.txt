find_package(Python COMPONENTS Interpreter Development REQUIRED)

AddTargetLibrary(
  LIBRARY_NAME
    lvtplg
  SOURCES
    ct_lvtplg_pluginmanager.cpp
    ct_lvtplg_sharedlibrarydispatcher.cpp
    ct_lvtplg_pythonlibrarydispatcher.cpp
  QT_HEADERS
    ct_lvtplg_handlercontextmenuaction.h
    ct_lvtplg_handlercodeanalysis.h
    ct_lvtplg_handlersetup.h
    ct_lvtplg_pluginmanager.h
    ct_lvtplg_sharedlibrarydispatcher.h
    ct_lvtplg_pythonlibrarydispatcher.h
    ct_lvtplg_librarydispatcherinterface.h
  LIBRARIES
    ${SYSTEM_EXTRA_LIBRARIES}
    Codethink::lvtshr
    Qt::Core
    Python::Python
)

target_include_directories(lvtplg
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../thirdparty/
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>/plugin-headers/
)

find_program(CLANG_FORMAT NAMES
  clang-format
  clang-format-10
  clang-format-11
  clang-format-12
  clang-format-13
  clang-format-14
  clang-format-15
  clang-format-16
  clang-format-17)

set(PLUGIN_HEADERS_DIR ${CMAKE_BINARY_DIR}/plugin-headers)
file(GLOB lvtclp_handler_headers ${CMAKE_CURRENT_SOURCE_DIR}/ct_lvtplg_handler*.h)
add_custom_command(
    OUTPUT ${PLUGIN_HEADERS_DIR}/ct_lvtplg_basicpluginhooks.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUGIN_HEADERS_DIR}/
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_plugin_hooks.py ${PLUGIN_HEADERS_DIR}/
    COMMAND ${CLANG_FORMAT} -i ${PLUGIN_HEADERS_DIR}/ct_lvtplg_basicpluginhooks.h
    COMMAND ${CMAKE_COMMAND} -E copy ${lvtclp_handler_headers} ${PLUGIN_HEADERS_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ct_lvtplg_plugindatatypes.h ${PLUGIN_HEADERS_DIR}/
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_plugin_hooks.py
    COMMENT "[lvtplg] Populating plugin-headers folder..."
    VERBATIM
)
add_custom_target(
    generate_lvtclp_plugin_headers ALL
    DEPENDS "${PLUGIN_HEADERS_DIR}/ct_lvtplg_basicpluginhooks.h"
)
add_dependencies(lvtplg generate_lvtclp_plugin_headers)

if (COMPILE_TESTS)
    add_subdirectory(testplugins)

    function(add_lvtplg_test TESTNAME)
        add_executable(test_${TESTNAME} ${TESTNAME}.t.cpp)
        target_link_libraries(test_${TESTNAME} lvtplg lvttst lvttst_tmpdir)
        add_test(NAME test_${TESTNAME} COMMAND test_${TESTNAME})
        add_dependencies(test_${TESTNAME} basicplugin basicpythonplugin)
    endfunction()

    add_lvtplg_test(ct_lvtplg_pythonlibrarydispatcher)
    add_lvtplg_test(ct_lvtplg_pluginmanager)
endif()
