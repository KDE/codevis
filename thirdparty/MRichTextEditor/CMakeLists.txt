
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
        Qt::Core
        Qt::Gui
        Qt::Widgets
)
set_property(TARGET MRichTextEdit PROPERTY CXX_STANDARD 17)
