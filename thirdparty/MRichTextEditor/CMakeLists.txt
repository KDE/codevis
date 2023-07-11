
AddTargetLibrary(
    LIBRARY_NAME
        MRichTextEdit
    SOURCES
        mrichtextedit.cpp
        mtextedit.cpp
    QT_HEADERS
        mrichtextedit.h
        mtextedit.h
    DESIGNER_FORMS
        mrichtextedit.ui
    LIBRARIES
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
)
