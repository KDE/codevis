
set(PLUGIN_NAME lakosian_test_rules_plugin)

add_library(${PLUGIN_NAME}
    SHARED
    plugin.cpp
    utils.h
)
target_include_directories(${PLUGIN_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${PLUGIN_NAME} PRIVATE "-fvisibility=default")
set_property(TARGET ${PLUGIN_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${PLUGIN_NAME} Qt${QT_MAJOR_VERSION}::Core CodevisPluginHeaders)

set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
set_target_properties(${PLUGIN_NAME} PROPERTIES OUTPUT_NAME ${PLUGIN_NAME})

codevis_install_plugin(${PLUGIN_NAME})
