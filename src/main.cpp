#include "ui/mainwindow.hpp"
#include <QtGui>
#include <QtQml>
#include <QStyleFactory>
#include <QApplication>


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    qmlRegisterType<Canvas>("DSV.Canvas", 1, 0, "DSVCanvas");
    
    QUrl source(QStringLiteral("../../src/ui/MainWindow.qml"));
    QQmlApplicationEngine engine;
    engine.load(source);
    
    return app.exec();
}
