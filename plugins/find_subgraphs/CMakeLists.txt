
set(PLUGIN_NAME find_subgraphs)

add_library(${PLUGIN_NAME}
    SHARED
    plugin.cpp
)
include_directories(${Boost_INCLUDE_DIRS})

target_compile_options(${PLUGIN_NAME} PRIVATE "-fvisibility=default")
set_property(TARGET ${PLUGIN_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${PLUGIN_NAME} Qt${QT_MAJOR_VERSION}::Core)
target_link_libraries(${PLUGIN_NAME} Qt${QT_MAJOR_VERSION}::Core CodevisPluginHeaders)

set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
set_target_properties(${PLUGIN_NAME} PROPERTIES OUTPUT_NAME ${PLUGIN_NAME})

codevis_install_plugin(${PLUGIN_NAME})

if (COMPILE_TESTS)
    add_executable(find_subgraph_test
    add_subgraph_test.t.cpp
    )

    target_link_libraries(find_subgraph_test
        Qt${QT_MAJOR_VERSION}::Test
        Catch2::Catch2
        CodevisPluginHeaders
        Codethink::lvttst
    )
    add_test(NAME find_subgraph_test COMMAND find_subgraph_test)
endif()
