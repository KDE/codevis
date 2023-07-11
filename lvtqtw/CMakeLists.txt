include_directories(SYSTEM ${LLVM_INCLUDE_DIR})
include_directories(SYSTEM ${Boost_INCLUDE_DIR})
include_directories(SYSTEM ${QUAZIP_INCLUDE_DIRS})

AddTargetLibrary(
    LIBRARY_NAME
        lvtqtw
    SOURCES
        ct_lvtqtw_backgroundeventfilter.cpp
        ct_lvtqtw_configurationdialog.cpp
        ct_lvtqtw_exportmanager.cpp
        ct_lvtqtw_errorview.cpp
        ct_lvtqtw_graphtabelement.cpp
        ct_lvtqtw_cycletreeview.cpp
        ct_lvtqtw_modifierhelpers.cpp
        ct_lvtqtw_namespacetreeview.cpp
        ct_lvtqtw_parse_codebase.cpp
        ct_lvtqtw_statusbar.cpp
        ct_lvtqtw_splitterview.cpp
        ct_lvtqtw_tabwidget.cpp
        ct_lvtqtw_textview.cpp
        ct_lvtqtw_treeview.cpp
        ct_lvtqtw_toolbox.cpp
        ct_lvtqtw_searchwidget.cpp
        ct_lvtqtw_welcomescreen.cpp
    QT_HEADERS
        ct_lvtqtw_backgroundeventfilter.h
        ct_lvtqtw_configurationdialog.h
        ct_lvtqtw_errorview.h
        ct_lvtqtw_exportmanager.h
        ct_lvtqtw_cycletreeview.h
        ct_lvtqtw_modifierhelpers.h
        ct_lvtqtw_namespacetreeview.h
        ct_lvtqtw_tabwidget.h
        ct_lvtqtw_textview.h
        ct_lvtqtw_graphtabelement.h
        ct_lvtqtw_parse_codebase.h
        ct_lvtqtw_statusbar.h
        ct_lvtqtw_splitterview.h
        ct_lvtqtw_treeview.h
        ct_lvtqtw_toolbox.h
        ct_lvtqtw_searchwidget.h
        ct_lvtqtw_welcomescreen.h

    DESIGNER_FORMS
        ct_lvtqtw_graphtabelement.ui
        ct_lvtqtw_errorview.ui
        ct_lvtqtw_configurationdialog.ui
        ct_lvtqtw_parse_codebase.ui
        ct_lvtqtw_searchwidget.ui
        ct_lvtqtw_welcomewidget.ui

    LIBRARIES
        ${SYSTEM_EXTRA_LIBRARIES}
        Codethink::InternalKWidgetsAddons
        Codethink::lvtmdl
        Codethink::lvtclp
        Codethink::lvtqtc
        Codethink::lvtqtd
        Codethink::lvtprj
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Svg
        lakospreferences
)

if (COMPILE_TESTS)
    function(ADD_LVTQTW_TESTCASE TC_NAME)
        MESSAGE(STATUS "Adding test ${TC_NAME}")

        add_executable(test_${TC_NAME}
            ${TC_NAME}.t.cpp
        )
        target_link_libraries(test_${TC_NAME}
            Codethink::lvtqtw
            Codethink::lvttst
            Codethink::lvttst_fixture_qt
            Codethink::lvttst_tmpdir
            Qt5::Test
        )
        add_test(NAME test_${TC_NAME} COMMAND test_${TC_NAME})
    endfunction()

    add_lvtqtw_testcase(ct_lvtqtw_tabwidget)
    add_lvtqtw_testcase(ct_lvtqtw_exportmanager)
    add_lvtqtw_testcase(ct_lvtqtw_statusbar)
    add_lvtqtw_testcase(ct_lvtqtw_textview)
    add_lvtqtw_testcase(ct_lvtqtw_configurationdialog)
endif()
