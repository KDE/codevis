# =========================
# LVT Codegen model library
# =========================
find_package(Python COMPONENTS Development REQUIRED)

AddTargetLibrary(
    LIBRARY_NAME
        lvtcgn_mdl
    SOURCES
        ct_lvtcgn_generatecode.cpp
    HEADERS
        ct_lvtcgn_generatecode.h
    LIBRARIES
        ${SYSTEM_EXTRA_LIBRARIES}
        Python::Python
)

# TODO [#438]: Move thirdparty dependencies to Conan (Using 'cpp:result' and 'pybind11' here)
target_include_directories(lvtcgn_mdl
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../thirdparty/
)

if (COMPILE_TESTS)
    add_executable(test_lvtcgn_codegen ct_lvtcgn_generatecode.t.cpp ct_lvtcgn_testutils.h)
    target_link_libraries(test_lvtcgn_codegen
        Codethink::lvtcgn_mdl
        Codethink::lvttst
        Codethink::lvttst_tmpdir
        ${SYSTEM_EXTRA_LIBRARIES}
    )
    add_test(NAME test_lvtcgn_codegen COMMAND test_lvtcgn_codegen)
endif()

if (BUILD_DESKTOP_APP)
    # =======================
    # LVT Codegen GUI library
    # =======================
    AddTargetLibrary(
        LIBRARY_NAME
            lvtcgn_gui
        SOURCES
            ct_lvtcgn_codegendialog.cpp
            ct_lvtcgn_cogedentreemodel.cpp
        HEADERS
            ct_lvtcgn_codegendialog.h
            ct_lvtcgn_cogedentreemodel.h
        DESIGNER_FORMS
            ct_lvtcgn_codegendialog.ui
        LIBRARIES
            ${SYSTEM_EXTRA_LIBRARIES}
            Qt::Core
            Qt::Gui
            Qt::Widgets
            KF${KF_MAJOR_VERSION}::WidgetsAddons
            Codethink::lvtqtw
            Codethink::lvtcgn_mdl
    )

    if (COMPILE_TESTS)
        add_executable(test_lvtcgn_codegendialog ct_lvtcgn_codegendialog.t.cpp ct_lvtcgn_testutils.h)
        target_link_libraries(test_lvtcgn_codegendialog
            Codethink::lvtcgn_gui
            Codethink::lvtcgn_mdl
            Codethink::lvttst
            Codethink::lvttst_fixture_qt
            Codethink::lvttst_tmpdir
            Codethink::lakospreferences
            ${SYSTEM_EXTRA_LIBRARIES}
        )
        add_test(NAME test_lvtcgn_codegendialog COMMAND test_lvtcgn_codegendialog)
    endif()

    # ===============================
    # LVT Codegen App Adapter library
    # ===============================
    AddTargetLibrary(
        LIBRARY_NAME
            lvtcgn_adapter
        SOURCES
            ct_lvtcgn_app_adapter.cpp
        HEADERS
            ct_lvtcgn_app_adapter.h
        LIBRARIES
            ${SYSTEM_EXTRA_LIBRARIES}
            Codethink::lvtcgn_mdl
            Codethink::lvtcgn_gui
            Codethink::lvtldr
            Codethink::lvtshr
            lakospreferences
    )

    if (COMPILE_TESTS)
        add_executable(test_lvtcgn_app_adapter ct_lvtcgn_app_adapter.t.cpp)
        target_link_libraries(test_lvtcgn_app_adapter
                Codethink::lvtcgn_adapter
                Codethink::lvttst
                Codethink::lvttst_tmpdir
                ${SYSTEM_EXTRA_LIBRARIES}
                )
        add_test(NAME test_lvtcgn_app_adapter COMMAND test_lvtcgn_app_adapter)
    endif()
endif()
