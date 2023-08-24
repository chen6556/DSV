#include "ui/mainwindow.hpp"
#include <QtGui>
#include <QtQml>
#include <QStyleFactory>
#include <QApplication>


int main(int argc, char *argv[])
{
    /* QApplication a(argc, argv);

    MainWindow w;

    w.show(); */

    QGuiApplication app(argc, argv);
    qmlRegisterType<MainWindow>("DSV.MainWindow", 1, 0, "MainWindow");
    qmlRegisterType<Canvas>("DSV.Canvas", 1, 0, "DSVCanvas");
    
    QUrl source(QStringLiteral("../../src/ui/MainWindow.qml"));
    QQmlApplicationEngine engine;
    engine.load(source);
    
    return app.exec();
}
