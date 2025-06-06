#include <QApplication>

#include "ui/MainWindow.hpp"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");
    a.setAttribute(Qt::ApplicationAttribute::AA_EnableHighDpiScaling);

    MainWindow w;

    w.show();

    return a.exec();
}
