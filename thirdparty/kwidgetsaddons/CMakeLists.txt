
AddTargetLibrary(
    LIBRARY_NAME
        InternalKWidgetsAddons
    SOURCES
        kfontchooser.cpp
        kfontchooserdialog.cpp
        kfontrequester.cpp
        kcolorbutton.cpp
        kmessagewidget.cpp
    HEADERS
        fonthelpers_p.h
    QT_HEADERS
        kfontchooserdialog.h
        kfontrequester.h
        kfontchooser.h
        kcolorbutton.h
        kmessagewidget.h
    DESIGNER_FORMS
        kfontchooserwidget.ui
    LIBRARIES
        Qt5::Core Qt5::Gui Qt5::Widgets
)
