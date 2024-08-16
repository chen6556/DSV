QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    CMDWidget.cpp \
    DataPanel.cpp \
    LayersManager.cpp \
    Setting.cpp \
    base/Algorithm.cpp \
    base/Editer.cpp \
    base/Geometry.cpp \
    draw/Canvas.cpp \
    draw/Container.cpp \
    draw/Graph.cpp \
    io/DSVParser.cpp \
    io/File.cpp \
    io/GlobalSetting.cpp \
    io/PDFParser.cpp \
    io/PLTParser.cpp \
    io/RS274DParser.cpp \
    io/TextEncoding.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    CMDWidget.h \
    DataPanel.h \
    LayersManager.h \
    MainWindow.h \
    Setting.h \
    base/Algorithm.h \
    base/EarCut.h \
    base/Editer.h \
    base/Geometry.h \
    draw/Canvas.h \
    draw/Container.h \
    draw/GLSL.h \
    draw/Graph.h \
    io/DSVParser.h \
    io/File.h \
    io/GlobalSetting.h \
    io/PDFParser.h \
    io/PLTParser.h \
    io/Parser/Action.h \
    io/Parser/BaseParser.h \
    io/Parser/ParserGen2.h \
    io/RS274DParser.h \
    io/TextEncoding.h

FORMS += \
    CMDWidget.ui \
    DataPanel.ui \
    LayersManager.ui \
    MainWindow.ui \
    Setting.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource/Icons.qrc
