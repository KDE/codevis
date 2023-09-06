cmake_minimum_required(VERSION 3.10)

project(basicpythonplugin)

set(PLUGIN_FILES
    README.md
    basicpythonplugin.py
)

set(copiedFiles "")
foreach(FILENAME ${PLUGIN_FILES})
    set(SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
    set(TARGET_FILE "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}")
    add_custom_command(
        OUTPUT ${TARGET_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE_FILE} ${TARGET_FILE}
        DEPENDS ${SOURCE_FILE}
        COMMENT "[${PROJECT_NAME}] Copying ${FILENAME}..."
        VERBATIM
    )
    list(APPEND copiedFiles ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME})
endforeach()
add_custom_target(
    basicpythonplugin ALL
    DEPENDS ${copiedFiles}
)
