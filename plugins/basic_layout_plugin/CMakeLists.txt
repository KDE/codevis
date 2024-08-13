# KCoreAddons reads BUILD_SHARED_LIBS  to find out what to do.
# we need to set it to true here.
# I don't know why I need this.'

set(BUILD_SHARED_LIBS ON)
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

kcoreaddons_add_plugin(
    basiclayoutplugin
SOURCES
    BasicLayoutPlugin.cpp
INSTALL_NAMESPACE
    "codevis_plugins_v2"
)

target_link_libraries(basiclayoutplugin
    Qt6::Core
    Codethink::PluginSystemHeaders
    Codethink::lvtqtc
)
