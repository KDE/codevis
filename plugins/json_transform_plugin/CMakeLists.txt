# KCoreAddons reads BUILD_SHARED_LIBS  to find out what to do.
# we need to set it to true here.
# I don't know why I need this.'

set(BUILD_SHARED_LIBS ON)
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

cmake_language(CALL
    qt${QT_MAJOR_VERSION}_add_resources
    jsontransformplugin_QRC_SRCS
    json_transform_plugin.qrc
)

kcoreaddons_add_plugin(
    jsontransformplugin
SOURCES
    ${jsontransformplugin_QRC_SRCS}
    JsonTransformPlugin.cpp
    JsonEditorDialog.cpp
INSTALL_NAMESPACE
    "codevis_plugins_v2"
)

target_include_directories(jsontransformplugin PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

generate_export_header(jsontransformplugin
    BASE_NAME jsontransformplugin
)

target_link_libraries(jsontransformplugin
    Qt${QT_MAJOR_VERSION}::Core
    KF${KF_MAJOR_VERSION}::TextEditor
    Qt${QT_MAJOR_VERSION}::Widgets
    Codethink::PluginSystemHeaders
    Codethink::lvtqtc
)
