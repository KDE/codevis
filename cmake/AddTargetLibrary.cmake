# AddTargetLibrary(
#    LIBRARY_NAME lvtxxx
#    SOURCES source1.cpp source2.cpp source3.cpp
#    HEADERS source1.h source2.h source3.h
#    QT_HEADERS [list of headers that should be preprocessed with moc]
#    LIBRARIES -lmath -lthread
#    DESIGNER_FORMS widget.ui
#)

macro(AddTargetLibrary)
    set(booleanValueArgs "")
    set(oneValueArgs LIBRARY_NAME)
    set(multiValueArgs SOURCES HEADERS QT_HEADERS LIBRARIES DESIGNER_FORMS)
    cmake_parse_arguments(DTARGS "${booleanValueArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    include_directories(${PROJECT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    # Build the Meta Object Compiler targets
    qt5_wrap_cpp(DTARGS_SOURCES ${DTARGS_QT_HEADERS} OPTIONS "--no-warnings" "--no-notes")

    if (BUILD_DESKTOP_APP)
        qt5_wrap_ui(DTARGS_SOURCES ${DTARGS_DESIGNER_FORMS})
    endif()

    add_library(${DTARGS_LIBRARY_NAME} SHARED ${DTARGS_SOURCES} ${DTARGS_HEADERS} ${DTARGS_QT_HEADERS})
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
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endmacro()
