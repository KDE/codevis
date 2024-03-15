# add_lvtclp_exec(
#     NAME exec_name
#     NOINSTALL
#     SOURCES foo.cpp bar.cpp
#     LIBRARIES lib1 lib2
# )
function(add_codevis_exec)
    set(options NOINSTALL)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES LIBRARIES)
    cmake_parse_arguments(clp "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${clp_NAME} ${clp_SOURCES})

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set_target_properties(${clp_NAME}
            PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} ${SKIP_CLANG_WARNINGS}")
    endif()

    target_link_libraries(${clp_NAME}
        ${clp_LIBRARIES}
    )

    if (NOT NOINSTAL)
        install(TARGETS ${clp_NAME} ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
    endif()
endfunction()
