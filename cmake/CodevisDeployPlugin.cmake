macro (codevis_install_plugin PLUGIN_NAME)
    install(TARGETS ${PLUGIN_NAME} DESTINATION "${KDE_INSTALL_DATAROOTDIR}/codevis/lks-plugins/${PLUGIN_NAME}")
    install(FILES README.md metadata.json DESTINATION "${KDE_INSTALL_DATAROOTDIR}/codevis/lks-plugins/${PLUGIN_NAME}")
endmacro()

macro (codevis_install_python_plugin PLUGIN_NAME)
    install(FILES ${PLUGIN_NAME}.py README.md metadata.json DESTINATION "${KDE_INSTALL_DATAROOTDIR}/codevis/lks-plugins/${PLUGIN_NAME}")
endmacro()
