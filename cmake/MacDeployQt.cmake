
# get absolute path to qmake, then use it to find macdeployqt executable

get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

function(macdeployqt target)

    # POST_BUILD step
    # - after build, we have a bin/lib for analyzing qt dependencies
    # - we run macdeployqt on target and deploy Qt libs

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${MACDEPLOYQT_EXECUTABLE}"
            "$<TARGET_FILE_DIR:${target}>/../.."
            -always-overwrite
        COMMENT "Deploying Qt libraries using macdeployqt for compilation target '${target}' ..."
    )

endfunction()
