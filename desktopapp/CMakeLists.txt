include_directories(${LLVM_INCLUDE_DIR} SYSTEM)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../submodules/backward-cpp)

set(CodeVisDesktop_SRCS
    codevis_dbus_interface.cpp
    mainwindow.cpp
    aboutdialog.cpp
    projectsettingsdialog.cpp
)

qt_wrap_cpp(CodeVisDesktop_SRCS
    mainwindow.h
    aboutdialog.h
    projectsettingsdialog.h
    codevis_dbus_interface.h
)

qt_wrap_ui(CodeVisDesktop_SRCS
    mainwindow.ui
    aboutdialog.ui
    projectsettingsdialog.ui
)

set(CODEVIS_DESKTOP_QT_LIBRARIES
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
if (USE_QT_WEBENGINE)
    set(CODEVIS_DESKTOP_QT_LIBRARIES "${CODEVIS_DESKTOP_QT_LIBRARIES}" Qt::WebEngineWidgets)
endif()

# Mac and Windows are currently failing on the CI
if (UNIX AND NOT APPLE)
    set(CodeVisDesktop_DBUS_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/org.codethink.CodeVis.xml
    )

    qt_generate_dbus_interface(
        codevis_dbus_interface.h
        org.codethink.CodeVis.xml

        # Only parse scriptable properties.
        OPTIONS -p -s -m
    )

    # install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.codethink.CodeVis.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

    qt_add_dbus_adaptor(CodeVisDesktop_DBUS_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/org.codethink.CodeVis.xml
        codevis_dbus_interface.h
        CodeVisDBusInterface # Clasname used for the adaptor.
    )

    set(CODEVIS_DESKTOP_QT_LIBRARIES ${CODEVIS_DESKTOP_QT_LIBRARIES} Qt::DBus)
endif()

if (APPLE)
    set(CodeVisIcon_SRCS ${PROJECT_SOURCE_DIR}/iconset.icns)
    set_source_files_properties(${CodeVisIcon_SRCS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    # clang headers
    file(GLOB_RECURSE AppleHeaders_SRCS ${CT_CLANG_HEADERS_DIR}/*.h)
    set_source_files_properties(${AppleHeaders_SRCS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/include")
endif()

# if (WIN32)
#    set(CodeVisIcon_SRCS ${PROJECT_SOURCE_DIR}/packaging/windows/applicationicon.rc)
#    set(WindowsKeyword WIN32)
# endif()

add_library(codevis_lib STATIC
    ${CodeVisDesktop_DBUS_SRCS}
    ${CodeVisIcon_SRCS}
    ${CodeVisDesktop_SRCS}
    ${AppleHeaders_SRCS}
)

target_link_libraries(
    codevis_lib

    KF${KF_MAJOR_VERSION}::WidgetsAddons
    Codethink::lvtmdl
    Codethink::lvtprj
    Codethink::lvtqtw
    Codethink::lvtqtc
    Codethink::lvtplg
    Codethink::lvtcgn_adapter

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

install(TARGETS codevis_desktop ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

file(COPY ${CMAKE_SOURCE_DIR}/python/ DESTINATION ${CMAKE_BINARY_DIR}/desktopapp/python/)

if (COMPILE_TESTS AND NOT IGNORE_BROKEN_TESTS_KDE_CI)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testcrash.lks ${CMAKE_CURRENT_BINARY_DIR}/testcrash.lks COPYONLY)
    qt_wrap_cpp(test_desktopapp_SRCS
        testmainwindow.h
    )

    add_executable(test_desktopapp
        ${test_desktopapp_SRCS}
        mainwindow.t.cpp
        apptesting_fixture.cpp
        testmainwindow.cpp
        apptesting_fixture.h
    )

    target_link_libraries(test_desktopapp
        Codethink::lvttst
        Codethink::lvttst_fixture_qt
        Codethink::lvttst_tmpdir
        codevis_lib
        ${SYSTEM_EXTRA_LIBRARIES}
        Qt::Test
    )

    add_test(NAME test_desktopapp COMMAND test_desktopapp)
    if (WIN32)
        add_custom_command(TARGET test_desktopapp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CONAN_QT_ROOT}/bin/Qt${QT_MAJOR_VERSION}Test.dll"
                $<TARGET_FILE_DIR:test_desktopapp>)
    endif()

    add_executable(test_projectsettingsdialog projectsettingsdialog.t.cpp)
    target_link_libraries(test_projectsettingsdialog
        Codethink::lvttst
        Codethink::lvttst_fixture_qt
        codevis_lib
        ${EXTRA_SYSTEM_LIBRARIES}
        Qt::Test
    )
    add_test(NAME test_projectsettingsdialog COMMAND test_projectsettingsdialog)
endif()
