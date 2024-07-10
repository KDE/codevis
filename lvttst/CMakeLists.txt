if (COMPILE_TESTS)
    add_library(lvttst STATIC
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
        add_library(lvttst_fixture_qt STATIC
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
            Qt${QT_MAJOR_VERSION}::Widgets
        )
    endif()
    add_library(lvttst_tmpdir STATIC
        ct_lvttst_tmpdir.cpp
        ct_lvttst_tmpdir.h
    )
    target_link_libraries(lvttst_tmpdir Qt6::Core)

    add_library(Codethink::lvttst_tmpdir ALIAS lvttst_tmpdir)

    target_include_directories(lvttst_tmpdir
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    add_executable(test_lvttst_tmpdir test_lvttst_tmpdir.t.cpp)
    target_link_libraries(test_lvttst_tmpdir Codethink::lvttst Codethink::lvttst_tmpdir)
    add_test(NAME test_lvttst_tmpdir COMMAND test_lvttst_tmpdir)
endif()
