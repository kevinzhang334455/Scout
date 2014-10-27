
HEADERS += ./src/App/GuiApplication_APP.h \
    ./src/App/Preferences_APP.h \
    ./src/App/ProjectLoader_APP.h \
    ./src/Gui_QT/CPPHighlighter_GUI.h \
    ./src/Gui_QT/FindDialog_GUI.h \
    ./src/Gui_QT/GoToLine_GUI.h \
    ./src/Gui_QT/LogWidget_GUI.h \
    ./src/Gui_QT/MainWindow_GUI.h \
    ./src/Gui_QT/SetupDialog_GUI.h \
    ./src/Gui_QT/SetupExtProcessorDialog_GUI.h
SOURCES += ./src/App/GuiApplication_APP.cpp \
    ./src/App/Preferences_APP.cpp \
    ./src/App/ProjectLoader_APP.cpp \
    ./src/Gui_QT/CPPHighlighter_GUI.cpp \
    ./src/Gui_QT/FindDialog_GUI.cpp \
    ./src/Gui_QT/GoToLine_GUI.cpp \
    ./src/Gui_QT/LogWidget_GUI.cpp \
    ./src/Gui_QT/main_GUI.cpp \
    ./src/Gui_QT/MainWindow_GUI.cpp \
    ./src/Gui_QT/SetupDialog_GUI.cpp \
    ./src/Gui_QT/SetupExtProcessorDialog_GUI.cpp
FORMS += ./src/Gui_QT/Forms/AboutDialog.ui \
    ./src/Gui_QT/Forms/FindDialog.ui \
    ./src/Gui_QT/Forms/GoToLine.ui \
    ./src/Gui_QT/Forms/MainWindow.ui \
    ./src/Gui_QT/Forms/SetupDialog.ui \
    ./src/Gui_QT/Forms/SetupExtProcessorDialog.ui
RESOURCES += src/Gui_QT/Resources/MainWindow.qrc
