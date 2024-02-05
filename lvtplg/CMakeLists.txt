find_package(Python COMPONENTS Interpreter Development REQUIRED)

AddTargetLibrary(
  LIBRARY_NAME
    lvtplg
  SOURCES
    ct_lvtplg_pluginmanager.cpp
    ct_lvtplg_sharedlibrarydispatcher.cpp
    ct_lvtplg_pythonmodule.cpp
    ct_lvtplg_pythonlibrarydispatcher.cpp
  QT_HEADERS
    ct_lvtplg_abstractlibrarydispatcher.h
    ct_lvtplg_pluginmanager.h
    ct_lvtplg_sharedlibrarydispatcher.h
    ct_lvtplg_pythonlibrarydispatcher.h
  LIBRARIES
    ${SYSTEM_EXTRA_LIBRARIES}
    Codethink::lvtshr
    KF${KF_MAJOR_VERSION}::NewStuffCore
    Qt${QT_MAJOR_VERSION}::Core
    Python::Python
)
target_compile_definitions(lvtplg PRIVATE AS_EMBEDDED=1)

target_include_directories(lvtplg
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../thirdparty/
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>/plugin-headers/
)

add_library(pyLksPlugin
    MODULE
        ct_lvtplg_pythonmodule.cpp
)
target_compile_definitions(pyLksPlugin PRIVATE AS_MODULE=1)
target_include_directories(pyLksPlugin
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../thirdparty/
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>/plugin-headers/
)
target_link_libraries(pyLksPlugin
    PRIVATE
        Python::Python
)

if (WIN32)
    set(PYMODULE_EXT ".pyd")
else()
    set(PYMODULE_EXT ".so")
endif()
set_target_properties(pyLksPlugin
    PROPERTIES
    PREFIX ""
    SUFFIX ${PYMODULE_EXT}
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
add_custom_command(
    OUTPUT
        "${PLUGIN_HEADERS_DIR}/ct_lvtplg_basicpluginhooks.h"
        "${PLUGIN_HEADERS_DIR}/ct_lvtplg_basicpluginhandlers.h"
        "${PLUGIN_HEADERS_DIR}/ct_lvtplg_plugindatatypes.h"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUGIN_HEADERS_DIR}/
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_plugin_hooks.py ${PLUGIN_HEADERS_DIR}/
    COMMAND ${CLANG_FORMAT} -i ${PLUGIN_HEADERS_DIR}/ct_lvtplg_basicpluginhooks.h
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_plugin_handlers.py ${PLUGIN_HEADERS_DIR}/
    COMMAND ${CLANG_FORMAT} -i ${PLUGIN_HEADERS_DIR}/ct_lvtplg_basicpluginhandlers.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ct_lvtplg_plugindatatypes.h ${PLUGIN_HEADERS_DIR}/
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/generate_plugin_hooks.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/generate_plugin_handlers.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/ct_lvtplg_plugindatatypes.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/hooks.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/handlers.py"
    COMMENT "[lvtplg] Populating plugin-headers folder..."
    VERBATIM
)
add_custom_target(
    generate_lvtclp_plugin_headers ALL
    DEPENDS
        "${PLUGIN_HEADERS_DIR}/ct_lvtplg_basicpluginhooks.h"
        "${PLUGIN_HEADERS_DIR}/ct_lvtplg_basicpluginhandlers.h"
        "${PLUGIN_HEADERS_DIR}/ct_lvtplg_plugindatatypes.h"
)

add_custom_command(
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/ct_lvtplg_hookbindings.inc.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/ct_lvtplg_handlerbindings.inc.cpp"
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_python_hook_bindings.py ${CMAKE_BINARY_DIR}/lvtplg/
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_python_handler_bindings.py ${CMAKE_BINARY_DIR}/lvtplg/
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/generate_python_hook_bindings.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/generate_python_handler_bindings.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/hooks.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/handlers.py"
    COMMENT "[lvtplg] Generating python bindings..."
    VERBATIM
)
add_custom_target(
    generate_lvtclp_py_bindings ALL
    DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/ct_lvtplg_hookbindings.inc.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/ct_lvtplg_handlerbindings.inc.cpp"
)
add_library(CodevisPluginHeaders INTERFACE)
target_include_directories(CodevisPluginHeaders
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>/plugin-headers/
)

add_dependencies(lvtplg generate_lvtclp_py_bindings generate_lvtclp_plugin_headers)
add_dependencies(pyLksPlugin generate_lvtclp_py_bindings generate_lvtclp_plugin_headers)
add_dependencies(CodevisPluginHeaders generate_lvtclp_plugin_headers)

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
