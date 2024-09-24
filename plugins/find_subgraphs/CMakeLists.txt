#####

# KCoreAddons reads BUILD_SHARED_LIBS  to find out what to do.
# we need to set it to true here.
# I don't know why I need this.'

set(BUILD_SHARED_LIBS ON)
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

kcoreaddons_add_plugin(
    findsubgraphs
SOURCES
    findsubgraphsplugin.cpp
    subgraphtreeview.cpp
INSTALL_NAMESPACE
    "codevis_plugins_v2"
)

target_include_directories(findsubgraphs PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

generate_export_header(findsubgraphs
    BASE_NAME findsubgraphs
)

target_link_libraries(findsubgraphs
    Qt${QT_MAJOR_VERSION}::Core
    Codethink::PluginSystemHeaders
    Codethink::lvtqtc
    Codethink::lakospreferences
)
