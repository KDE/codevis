if (COMPILE_TESTS)
    add_library(lvttst
        ct_lvttst_main.t.cpp
        ct_lvttst_testcfgoptions.cpp
        ct_lvttst_testcfgoptions.h
    )
    add_library(Codethink::lvttst ALIAS lvttst)
    target_include_directories(lvttst
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    target_link_libraries(lvttst
        Catch2::Catch2
    )

    if (BUILD_DESKTOP_APP)
        add_library(lvttst_fixture_qt
            ct_lvttst_fixture_qt.cpp
            ct_lvttst_fixture_qt.h
            ct_lvttst_osstream.cpp
            ct_lvttst_osstream.h
        )
        add_library(Codethink::lvttst_fixture_qt ALIAS lvttst_fixture_qt)

        target_include_directories(lvttst_fixture_qt
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )

        target_link_libraries(lvttst_fixture_qt
            Qt5::Widgets
        )
    endif()
    add_library(lvttst_tmpdir
        ct_lvttst_tmpdir.cpp
        ct_lvttst_tmpdir.h
    )
    add_library(Codethink::lvttst_tmpdir ALIAS lvttst_tmpdir)

    target_include_directories(lvttst_tmpdir
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )
endif()
