#=============================================================================
# SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
#
# SPDX-License-Identifier: MIT
#=============================================================================

function(compile_configuration target_or_source_var)
    set(options)
    set(oneValueArgs EXPORT_HEADER GENERATOR)
    set(multiValueArgs OPTIONS DEPENDS)

    cmake_parse_arguments(_WRAP_CPP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(configuration_files ${_WRAP_CPP_UNPARSED_ARGUMENTS})


    if (NOT DEFINED _WRAP_CPP_GENERATOR)
        set(GENERATOR "--qsettings")
        message("No generator set, defaulting to --qsettings. Options are qsettings, kconfig or qobject")
    else()
        set(GENERATOR "--${_WRAP_CPP_GENERATOR}")
    endif()

    if (NOT DEFINED _WRAP_CPP_EXPORT_HEADER)
        set(EXPORT_HEADER_USAGE)
        message("THERES NO EXPORT HEADER")
    else()
        if (_WRAP_CPP_EXPORT_HEADER STREQUAL "")
            set(EXPORT_HEADER_USAGE)
            message("Export header is empty")
        else()
            message("EXPORT HEADER SET AS ${_WRAP_CPP_EXPORT_HEADER}")
            set(EXPORT_HEADER_USAGE "--with-export-header=${_WRAP_CPP_EXPORT_HEADER}")
        endif()
    endif()

    foreach(it ${configuration_files})
        get_filename_component(it ${it} ABSOLUTE)
        get_filename_component(_FILENAME_ ${it} NAME_WE)
        message("File to process ${it}")

        add_custom_command(
            OUTPUT "${_FILENAME_}.cpp" "${_FILENAME_}.h"
            COMMAND ConfigurationParser::confgen ${GENERATOR} ${EXPORT_HEADER_USAGE} ${it}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            MAIN_DEPENDENCY ${it}
        )

        qt_wrap_cpp(_wrapped_moc "${CMAKE_CURRENT_BINARY_DIR}/${_FILENAME_}.h")
        if (TARGET ${target_or_source_var})
            target_sources(${target_or_source_var} PRIVATE "${_FILENAME_}.cpp")
            target_sources(${target_or_source_var} PRIVATE ${_wrapped_moc})
        else()
            set(${target_or_source_var} ${${target_or_source_var}} ${_wrapped_moc} "${_FILENAME_}.cpp" PARENT_SCOPE)
        endif()
    endforeach()

    message("GENERATED ${outfile}")
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()
