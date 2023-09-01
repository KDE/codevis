# Plugin library
find_package(Python COMPONENTS Interpreter Development REQUIRED)

AddTargetLibrary(
  LIBRARY_NAME
    lvtplg
  SOURCES
    ct_lvtplg_pluginmanager.cpp
    ct_lvtplg_sharedlibrarydispatcher.cpp
    ct_lvtplg_pythonlibrarydispatcher.cpp
  QT_HEADERS
    ct_lvtplg_basicpluginhooks.h
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

# TODO [#438]: Move thirdparty dependencies to Conan (Using 'cpp:result' and 'pybind11' here)
target_include_directories(lvtplg
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../thirdparty/
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

# Todo: Fix this and automatically generate the hooks file on the build folder.
#
# add_custom_command(
#    PRE_BUILD
#    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/lvtplg/generate_plugin_hooks.py ${CMAKE_SOURCE_DIR}/lvtplg/
#    COMMAND ${CLANG_FORMAT} -i ${CMAKE_SOURCE_DIR}/lvtplg/ct_lvtplg_basicpluginhooks.h
#    DEPENDS
#        ${CMAKE_SOURCE_DIR}/lvtplg/generate_plugin_hooks.py
#    OUTPUT
#        ${CMAKE_SOURCE_DIR}/lvtplg/ct_lvtplg_basicpluginhooks.h
#)

function(export_plugin_header FILENAME)
    MESSAGE("[lvtplg] Adding custom command for plugin header '${FILENAME}'")
    add_custom_command(
            TARGET lvtplg POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/lvtplg/${FILENAME}
                ${CMAKE_BINARY_DIR}/plugin-headers/${FILENAME}
            DEPENDS
                ${CMAKE_SOURCE_DIR}/lvtplg/${FILENAME}
    )
endfunction()

export_plugin_header(ct_lvtplg_basicpluginhooks.h)
export_plugin_header(ct_lvtplg_handlersetup.h)
export_plugin_header(ct_lvtplg_handlercontextmenuaction.h)
