kdoctools_create_handbook(
  index.docbook 
  INSTALL_DESTINATION ${KDE_INSTALL_DOCBUNDLEDIR}/en 
  SUBDIR codevis
)

kdoctools_create_manpage(
  man-codevis_desktop.1.docbook 1 
  INSTALL_DESTINATION ${KDE_INSTALL_MANDIR}
)
