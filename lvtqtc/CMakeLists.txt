AddTargetLibrary(
    LIBRARY_NAME
        lvtqtc
    SOURCES
        # Undo system:
        ct_lvtqtc_undo_add_package.cpp
        ct_lvtqtc_undo_add_component.cpp
        ct_lvtqtc_undo_add_logicalentity.cpp
        ct_lvtqtc_undo_add_edge.cpp
        ct_lvtqtc_undo_add_entity_common.cpp
        ct_lvtqtc_undo_expand.cpp
        ct_lvtqtc_undo_notes.cpp
        ct_lvtqtc_iconhelpers.cpp
        ct_lvtqtc_undo_load_entity.cpp
        ct_lvtqtc_undo_manager.cpp
        ct_lvtqtc_undo_move.cpp
        ct_lvtqtc_undo_rename_entity.cpp
        ct_lvtqtc_undo_reparent_entity.cpp
        ct_lvtqtc_repositoryentity.cpp

        # Algorithms:
        ct_lvtqtc_alg_transitive_reduction.cpp
        ct_lvtqtc_alg_level_layout.cpp

        # Components:
        ct_lvtqtc_componententity.cpp
        ct_lvtqtc_edgecollection.cpp
        ct_lvtqtc_ellipsistextitem.cpp
        ct_lvtqtc_graphicsrectitem.cpp
        ct_lvtqtc_graphicsscene.cpp
        ct_lvtqtc_graphicsview.cpp
        ct_lvtqtc_isa.cpp
        ct_lvtqtc_lakosentity.cpp
        ct_lvtqtc_lakosentitypluginutils.cpp
        ct_lvtqtc_lakosrelation.cpp
        ct_lvtqtc_logicalentity.cpp
        ct_lvtqtc_minimap.cpp
        ct_lvtqtc_packagedependency.cpp
        ct_lvtqtc_packageentity.cpp
        ct_lvtqtc_tooltip.cpp
        ct_lvtqtc_usesinnameonly.cpp
        ct_lvtqtc_usesintheimplementation.cpp
        ct_lvtqtc_usesintheinterface.cpp
        ct_lvtqtc_util.cpp
        ct_lvtqtc_inputdialog.cpp
        ct_lvtqtc_inspect_dependency_window.cpp

        # tools
        ct_lvtqtc_itool.cpp
        ct_lvtqtc_edge_based_tool.cpp
        ct_lvtqtc_tool_add_physical_dependency.cpp
        ct_lvtqtc_tool_add_logical_relation.cpp
        ct_lvtqtc_tool_zoom.cpp
        ct_lvtqtc_tool_rubberband.cpp
        ct_lvtqtc_tool_add_package.cpp
        ct_lvtqtc_tool_add_component.cpp
        ct_lvtqtc_tool_add_logical_entity.cpp
        ct_lvtqtc_tool_add_entity.cpp
        ct_lvtqtc_tool_reparent_entity.cpp

        # plugin interface
        ct_lvtqtc_pluginmanagerutils.cpp

    HEADERS
        ct_lvtqtc_alg_level_layout.h
        ct_lvtqtc_alg_transitive_reduction.h
        ct_lvtqtc_componententity.h
        ct_lvtqtc_edge_based_tool.h
        ct_lvtqtc_edgecollection.h
        ct_lvtqtc_ellipsistextitem.h
        ct_lvtqtc_graphicsrectitem.h
        ct_lvtqtc_graphicsscene.h
        ct_lvtqtc_graphicsview.h
        ct_lvtqtc_inputdialog.h
        ct_lvtqtc_inspect_dependency_window.h
        ct_lvtqtc_isa.h
        ct_lvtqtc_itool.h
        ct_lvtqtc_lakosentity.h
        ct_lvtqtc_lakosentitypluginutils.h
        ct_lvtqtc_lakosrelation.h
        ct_lvtqtc_logicalentity.h
        ct_lvtqtc_minimap.h
        ct_lvtqtc_packagedependency.h
        ct_lvtqtc_packageentity.h
        ct_lvtqtc_pluginmanagerutils.h
        ct_lvtqtc_repositoryentity.h
        ct_lvtqtc_tool_add_component.h
        ct_lvtqtc_tool_add_entity.h
        ct_lvtqtc_tool_add_logical_entity.h
        ct_lvtqtc_tool_add_logical_relation.h
        ct_lvtqtc_tool_add_package.h
        ct_lvtqtc_tool_add_physical_dependency.h
        ct_lvtqtc_tool_reparent_entity.h
        ct_lvtqtc_tool_rubberband.h
        ct_lvtqtc_tool_zoom.h
        ct_lvtqtc_tooltip.h
        ct_lvtqtc_undo_add_component.h
        ct_lvtqtc_undo_add_edge.h
        ct_lvtqtc_undo_add_logicalentity.h
        ct_lvtqtc_undo_add_package.h
        ct_lvtqtc_undo_expand.h
        ct_lvtqtc_undo_load_entity.h
        ct_lvtqtc_undo_manager.h
        ct_lvtqtc_undo_move.h
        ct_lvtqtc_undo_notes.h
        ct_lvtqtc_undo_rename_entity.h
        ct_lvtqtc_undo_reparent_entity.h
        ct_lvtqtc_usesinnameonly.h
        ct_lvtqtc_usesintheimplementation.h
        ct_lvtqtc_usesintheinterface.h
        ct_lvtqtc_util.h

    LIBRARIES
        Codethink::lvtldr
        Codethink::lvtmdl
        Codethink::lvtshr
        Codethink::lvtclr
        Codethink::lvtplg
        Codethink::lvtprj
        Codethink::MRichTextEdit
        KF${KF_MAJOR_VERSION}::WidgetsAddons
        Codethink::lakospreferences
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
)

if (COMPILE_TESTS)
    function(ADD_LVTQTC_TESTCASE TC_NAME)
        MESSAGE(STATUS "Adding test ${TC_NAME}")

        add_executable(test_${TC_NAME}
            ${TC_NAME}.t.cpp
            ct_lvtqtc_testing_utils.cpp
            ct_lvtqtc_testing_utils.h
            ct_lvtqtc_nodestorage_testing_helpers.h
        )

        target_link_libraries(test_${TC_NAME}
            Codethink::lvtqtc
            Codethink::lvtmdb
            Codethink::lvttst
            Codethink::lvttst_tmpdir
            Codethink::lvttst_fixture_qt
            Qt::Test
        )

        add_test(NAME test_${TC_NAME} COMMAND test_${TC_NAME})
    endfunction()

    add_lvtqtc_testcase(ct_lvtqtc_alg_transitive_reduction)
    add_lvtqtc_testcase(ct_lvtqtc_graphicsview)
    add_lvtqtc_testcase(ct_lvtqtc_tool_add_package)
    add_lvtqtc_testcase(ct_lvtqtc_tool_add_component)
    add_lvtqtc_testcase(ct_lvtqtc_tool_add_physical_dependency)
    add_lvtqtc_testcase(ct_lvtqtc_tool_add_logical_relation)
    add_lvtqtc_testcase(ct_lvtqtc_tool_add_logical_entity)
    add_lvtqtc_testcase(ct_lvtqtc_lakosentity)
    add_lvtqtc_testcase(ct_lvtqtc_inputdialog)
    add_lvtqtc_testcase(ct_lvtqtc_undo_add_component)
    add_lvtqtc_testcase(ct_lvtqtc_undo_add_logicalentity)
    add_lvtqtc_testcase(ct_lvtqtc_undo_add_package)
    add_lvtqtc_testcase(ct_lvtqtc_undo_move)
    add_lvtqtc_testcase(ct_lvtqtc_undo_notes)
    add_lvtqtc_testcase(ct_lvtqtc_undo_rename_entity)
    add_lvtqtc_testcase(ct_lvtqtc_undo_load_entity)
    add_lvtqtc_testcase(ct_lvtqtc_tool_reparent_entity)
    add_lvtqtc_testcase(ct_lvtqtc_graphicsscene)
    add_lvtqtc_testcase(ct_lvtqtc_ellipsistextitem)
    add_lvtqtc_testcase(ct_lvtqtc_inspect_dependency_window)
endif()
