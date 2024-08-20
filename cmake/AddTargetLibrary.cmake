# AddTargetLibrary(
#    LIBRARY_NAME lvtxxx
#    SOURCES source1.cpp source2.cpp source3.cpp
#    HEADERS source1.h source2.h source3.h
#    LIBRARIES -lmath -lthread
#    DESIGNER_FORMS widget.ui
#)

macro(AddTargetLibrary)
    set(booleanValueArgs "")
    set(oneValueArgs LIBRARY_NAME)
    set(multiValueArgs SOURCES HEADERS LIBRARIES DESIGNER_FORMS)
    cmake_parse_arguments(DTARGS "${booleanValueArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    include_directories(${PROJECT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    if (BUILD_DESKTOP_APP)
        cmake_language(CALL
            qt${QT_MAJOR_VERSION}_wrap_ui
            DTARGS_SOURCES ${DTARGS_DESIGNER_FORMS}
        )
    endif()

    add_library(${DTARGS_LIBRARY_NAME} SHARED ${DTARGS_SOURCES} ${DTARGS_HEADERS})
    add_library(Codethink::${DTARGS_LIBRARY_NAME} ALIAS ${DTARGS_LIBRARY_NAME})

    target_link_libraries(
        ${DTARGS_LIBRARY_NAME}
        ${DTARGS_LIBRARIES}
    )
    generate_export_header(${DTARGS_LIBRARY_NAME}
    BASE_NAME ${DTARGS_LIBRARY_NAME}
    )

    target_include_directories(${DTARGS_LIBRARY_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/lakos/${DTARGS_LIBRARY_NAME}>
    )

    install(TARGETS
        ${DTARGS_LIBRARY_NAME}
        DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
    )
endmacro()
