
include(Scout.common)

TEMPLATE = app
TARGET = Scout
QT += xml

INCLUDEPATH += \
    ./GeneratedFiles \
    ./GeneratedFiles/$$CONFIG_SUB_DIR \

MOC_DIR += ./GeneratedFiles/$$CONFIG_SUB_DIR
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles

#Include file(s)
include(Scout.pri)
