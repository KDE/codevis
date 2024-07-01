include_directories(${LLVM_INCLUDE_DIR} SYSTEM)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../submodules/backward-cpp)

set(CodeVisDesktop_SRCS
    codevis_dbus_interface.cpp
    mainwindow.cpp
    projectsettingsdialog.cpp
)

cmake_language(CALL
    qt${QT_MAJOR_VERSION}_add_resources
    CodeVisDesktop_SRCS
    desktopapp.qrc
)

cmake_language(CALL
    qt${QT_MAJOR_VERSION}_wrap_ui
    CodeVisDesktop_SRCS
    mainwindow.ui
    projectsettingsdialog.ui
)

set(CODEVIS_DESKTOP_QT_LIBRARIES
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::Widgets
)

if (USE_QT_WEBENGINE)
    set(CODEVIS_DESKTOP_QT_LIBRARIES "${CODEVIS_DESKTOP_QT_LIBRARIES}" Qt${QT_MAJOR_VERSION}::WebEngineWidgets)
endif()

# Mac and Windows are currently failing on the CI
if (UNIX AND NOT APPLE)
    set(CodeVisDesktop_DBUS_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/org.codethink.CodeVis.xml
    )

    cmake_language(CALL
        qt${QT_MAJOR_VERSION}_generate_dbus_interface
        codevis_dbus_interface.h
        org.codethink.CodeVis.xml
        # Only parse scriptable properties.
        OPTIONS -p -s -m
    )

    # install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.codethink.CodeVis.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

    cmake_language(CALL
        qt${QT_MAJOR_VERSION}_add_dbus_adaptor
        CodeVisDesktop_DBUS_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/org.codethink.CodeVis.xml
        codevis_dbus_interface.h
        CodeVisDBusInterface # Classname used for the adaptor.
    )

    set(CODEVIS_DESKTOP_QT_LIBRARIES ${CODEVIS_DESKTOP_QT_LIBRARIES} Qt${QT_MAJOR_VERSION}::DBus)
endif()

if (APPLE)
    set(CodeVisIcon_SRCS ${PROJECT_SOURCE_DIR}/iconset.icns)
    set_source_files_properties(${CodeVisIcon_SRCS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    # clang headers
    file(GLOB_RECURSE AppleHeaders_SRCS ${CT_CLANG_HEADERS_DIR}/*.h)
    set_source_files_properties(${AppleHeaders_SRCS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/include")
endif()

if (WIN32)
#    set(CodeVisIcon_SRCS ${PROJECT_SOURCE_DIR}/packaging/windows/applicationicon.rc)
    set(WindowsKeyword WIN32)
endif()

add_library(codevis_lib STATIC
    ${CodeVisDesktop_DBUS_SRCS}
    ${CodeVisIcon_SRCS}
    ${CodeVisDesktop_SRCS}
    ${AppleHeaders_SRCS}
)

if (${KF_VERSION_MINOR} LESS 90)
    message("Warning: Old KDE Framework version doesn't have KFNewStuffWidgets available. Disabling support.")
    set(KFNewStuffWidgetsIfAvailable "") # No such library
else()
    set(KFNewStuffWidgetsIfAvailable KF${KF_MAJOR_VERSION}::NewStuffWidgets)
endif()

target_link_libraries(
    codevis_lib

    KF${KF_MAJOR_VERSION}::Crash
    KF${KF_MAJOR_VERSION}::WidgetsAddons
    KF${KF_MAJOR_VERSION}::XmlGui
    KF${KF_MAJOR_VERSION}::CoreAddons
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::Notifications
    KF${KF_MAJOR_VERSION}::NewStuffCore
    ${KFNewStuffWidgetsIfAvailable}

    Codethink::lvtmdl
    Codethink::lvtprj
    Codethink::lvtqtw
    Codethink::lvtqtc
    Codethink::lvtplg
    Codethink::lvtcgn_adapter
    Codethink::codevis_project_helpers

    resources

    ${CODEVIS_DESKTOP_QT_LIBRARIES}
)

add_executable(codevis_desktop
    MACOSX_BUNDLE
    ${WindowsKeyword}
    desktopapp.m.cpp
)

target_link_libraries(
    codevis_desktop
    codevis_lib
)

ecm_install_icons(ICONS ${CMAKE_SOURCE_DIR}/imgs/256-apps-org.kde.codevis.png DESTINATION ${KDE_INSTALL_ICONDIR})
install(TARGETS codevis_desktop ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES codevis.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.codevis.metainfo.xml
  DESTINATION ${KDE_INSTALL_METAINFODIR}
)
install(FILES org.kde.codevis.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES codevis.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})

file(COPY ${CMAKE_SOURCE_DIR}/python/ DESTINATION ${CMAKE_BINARY_DIR}/desktopapp/python/)

if (COMPILE_TESTS)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testcrash.lks ${CMAKE_CURRENT_BINARY_DIR}/testcrash.lks COPYONLY)

    set(test_desktopapp_SRCS
        testmainwindow.cpp
    )

    macro(ADD_DESKTOP_TEST TEST_NAME FILE_NAME)
        add_executable(${TEST_NAME}
            ${FILE_NAME}
            ${test_desktopapp_SRCS}
            apptesting_fixture.cpp
            apptesting_fixture.h
        )

        target_link_libraries(${TEST_NAME}
            Codethink::lvttst
            Codethink::lvttst_fixture_qt
            Codethink::lvttst_tmpdir
            codevis_lib
            ${SYSTEM_EXTRA_LIBRARIES}
            ${CODEVIS_DESKTOP_QT_LIBRARIES}
            Qt${QT_MAJOR_VERSION}::Test
        )
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endmacro()

    ADD_DESKTOP_TEST(mainwindow_auto_add_edges_from_classes mainwindow_auto_add_edges_from_classes.t.cpp)
    ADD_DESKTOP_TEST(mainwindow_auto_add_edges_from_components mainwindow_auto_add_edges_from_components.t.cpp)
    ADD_DESKTOP_TEST(mainwindow_auto_add_edges_from_packages mainwindow_auto_add_edges_from_packages.t.cpp)
    ADD_DESKTOP_TEST(mainwindow_basic_application_workflow mainwindow_basic_application_workflow.t.cpp)
    ADD_DESKTOP_TEST(mainwindow_load_project_test mainwindow_load_test_crash.t.cpp)
    ADD_DESKTOP_TEST(mainwindow_show_error_on_add_edge_failure mainwindow_show_error_on_add_edge_failure.t.cpp)
    ADD_DESKTOP_TEST(mainwindow_test mainwindow.t.cpp)
    ADD_DESKTOP_TEST(mainwindow_test_autosave mainwindow_test_autosave.t.cpp)
    ADD_DESKTOP_TEST(mainwindow_test_tool_selection mainwindow_test_tool_selection.t.cpp)

    # This test doesn't work on Qt5 because of Bad API
    if (QT_MAJOR_VERSION STREQUAL "6")
        ADD_DESKTOP_TEST(mainwindow_test_element_move mainwindow_test_element_move.t.cpp)
    endif()

    add_executable(test_projectsettingsdialog projectsettingsdialog.t.cpp)
    target_link_libraries(test_projectsettingsdialog
        Codethink::lvttst
        Codethink::lvttst_fixture_qt
        codevis_lib
        ${SYSTEM_EXTRA_LIBRARIES}
        ${CODEVIS_DESKTOP_QT_LIBRARIES}
        Qt${QT_MAJOR_VERSION}::Test
    )
    add_test(NAME test_projectsettingsdialog COMMAND test_projectsettingsdialog)
endif()
