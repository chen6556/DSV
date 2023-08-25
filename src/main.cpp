#include <QtGui>
#include <QtQml>
#include <QStyleFactory>
#include <QApplication>
#include "draw/Canvas.hpp"
#include "io/GUISetting.hpp"
#include "io/GlobalSetting.hpp"


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    qmlRegisterType<Canvas>("DSV.Canvas", 1, 0, "DSVCanvas");
    qmlRegisterType<GUISetting>("DSV.Setting", 1, 0, "DSVSetting");

    GlobalSetting::get_instance()->load_setting();
    
    QUrl source(QStringLiteral("../../src/ui/MainWindow.qml"));
    QQmlApplicationEngine engine;
    engine.load(source);
    
    return app.exec();
}
