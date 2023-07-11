# - Returns a version string from Git tags
#
# This function inspects the annotated git tags for the project and returns a string
# into a CMake variable
#
#  get_git_version(<var>)
#
# - Example
#
# include(GetGitVersion)
# get_git_version(GIT_VERSION)

find_package(Git)
if(__get_git_version)
  return()
endif()
set(__get_git_version INCLUDED)

function(get_git_version var)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --match "v[0-9]*.[0-9]*.[0-9]*" --abbrev=8
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE status
        OUTPUT_VARIABLE GIT_VERSION
        ERROR_QUIET)

    if(${status})
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE status
            OUTPUT_VARIABLE GIT_VERSION
            ERROR_QUIET)
    endif()

    string(COMPARE EQUAL "${GIT_VERSION}" "" status)
    if (NOT ${status})
        string(STRIP ${GIT_VERSION} GIT_VERSION)
    else()
        set(GIT_VERSION "Unknown")
    endif()

    message("-- git Version: ${GIT_VERSION}")
    set(${var} ${GIT_VERSION} PARENT_SCOPE)
endfunction()

function(get_git_authors var)
    execute_process(COMMAND ${GIT_EXECUTABLE} --no-pager shortlog -sc --all
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE AUTHORS
        ERROR_QUIET)

    string(REGEX REPLACE "\n" ";" AUTHORS "${AUTHORS}")
    set(OUTPUT_AUTHORS "")
    foreach(AUTHOR ${AUTHORS})
        string(STRIP ${AUTHOR} AUTHOR)
        string(REGEX REPLACE "[0-9]+" "" AUTHOR ${AUTHOR})
        string(STRIP ${AUTHOR} AUTHOR)
        SET(OUTPUT_AUTHORS "${OUTPUT_AUTHORS} \n ${AUTHOR}")
    endforeach()
    set(${var} ${OUTPUT_AUTHORS} PARENT_SCOPE)
    message("Found authors: " ${OUTPUT_AUTHORS})
endfunction()
