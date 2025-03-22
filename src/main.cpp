#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::ApplicationAttribute::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    a.setStyle("Fusion");

    MainWindow w;
    w.show();
    return a.exec();
}
