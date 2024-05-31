include_directories(SYSTEM ${LLVM_INCLUDE_DIR})

if (${KF_VERSION_MAJOR} EQUAL 5 AND ${KF_VERSION_MINOR} LESS 90)
    message("Warning: Old KDE Framework version doesn't have KFNewStuffWidgets available. Disabling support.")
    set(KFNewStuffWidgetsIfAvailable "") # No such library

    # Old KF5 didn't have KNewStuffWidget features, so they are disabled in this .ui file
    set(CONFIGURATION_DIALOG_UI ct_lvtqtw_configurationdialog_oldkf5.ui)
else()
    set(KFNewStuffWidgetsIfAvailable KF${KF_MAJOR_VERSION}::NewStuffWidgets)
    set(CONFIGURATION_DIALOG_UI ct_lvtqtw_configurationdialog.ui)
endif()

if (ENABLE_FORTRAN_SCANNER)
    set(CTFortranLibsIfAvailable "lvtclp_fortran")
else()
    set(CTFortranLibsIfAvailable "")
endif()

AddTargetLibrary(
    LIBRARY_NAME
        lvtqtw
    SOURCES
        ct_lvtqtw_backgroundeventfilter.cpp
        ct_lvtqtw_bulkedit.cpp
        ct_lvtqtw_configurationdialog.cpp
        ct_lvtqtw_exportmanager.cpp
        ct_lvtqtw_errorview.cpp
        ct_lvtqtw_graphtabelement.cpp
        ct_lvtqtw_modifierhelpers.cpp
        ct_lvtqtw_namespacetreeview.cpp
        ct_lvtqtw_parse_codebase.cpp
        ct_lvtqtw_statusbar.cpp
        ct_lvtqtw_splitterview.cpp
        ct_lvtqtw_tabwidget.cpp
        ct_lvtqtw_textview.cpp
        ct_lvtqtw_treeview.cpp
        ct_lvtqtw_plugineditor.cpp
        ct_lvtqtw_toolbox.cpp
        ct_lvtqtw_searchwidget.cpp
        ct_lvtqtw_sqleditor.cpp
        ct_lvtqtw_welcomescreen.cpp
    QT_HEADERS
        ct_lvtqtw_backgroundeventfilter.h
        ct_lvtqtw_bulkedit.h
        ct_lvtqtw_configurationdialog.h
        ct_lvtqtw_errorview.h
        ct_lvtqtw_exportmanager.h
        ct_lvtqtw_modifierhelpers.h
        ct_lvtqtw_namespacetreeview.h
        ct_lvtqtw_tabwidget.h
        ct_lvtqtw_textview.h
        ct_lvtqtw_graphtabelement.h
        ct_lvtqtw_parse_codebase.h
        ct_lvtqtw_statusbar.h
        ct_lvtqtw_splitterview.h
        ct_lvtqtw_treeview.h
        ct_lvtqtw_plugineditor.h
        ct_lvtqtw_toolbox.h
        ct_lvtqtw_searchwidget.h
        ct_lvtqtw_sqleditor.h
        ct_lvtqtw_welcomescreen.h

    DESIGNER_FORMS
        ct_lvtqtw_graphtabelement.ui
        ct_lvtqtw_errorview.ui
        ${CONFIGURATION_DIALOG_UI}
        ct_lvtqtw_parse_codebase.ui
        ct_lvtqtw_searchwidget.ui
        ct_lvtqtw_welcomewidget.ui

    LIBRARIES
        ${SYSTEM_EXTRA_LIBRARIES}
        KF${KF_MAJOR_VERSION}::KCMUtils
        KF${KF_MAJOR_VERSION}::WidgetsAddons
        KF${KF_MAJOR_VERSION}::TextEditor
        KF${KF_MAJOR_VERSION}::NewStuffCore
        KF${KF_MAJOR_VERSION}::Notifications
        ${KFNewStuffWidgetsIfAvailable}
        Codethink::lvtmdl
        Codethink::lvtclp
        ${CTFortranLibsIfAvailable}
        Codethink::lvtqtc
        Codethink::lvtqtd
        Codethink::lvtprj
        Codethink::lvtplg
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Svg
        Codethink::lakospreferences
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
            Codethink::lvtcgn_gui
            Qt::Test
        )
        add_test(NAME test_${TC_NAME} COMMAND test_${TC_NAME})
    endfunction()

    add_lvtqtw_testcase(ct_lvtqtw_tabwidget)
    add_lvtqtw_testcase(ct_lvtqtw_exportmanager)
    add_lvtqtw_testcase(ct_lvtqtw_statusbar)
    add_lvtqtw_testcase(ct_lvtqtw_textview)
    add_lvtqtw_testcase(ct_lvtqtw_configurationdialog)
    add_lvtqtw_testcase(ct_lvtqtw_plugineditor)
    add_lvtqtw_testcase(ct_lvtqtw_parse_codebase)
endif()
