#include <QApplication>

#include "ui/MainWindow.hpp"


int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::ApplicationAttribute::AA_ShareOpenGLContexts, true);
    QApplication a(argc, argv);
    a.setStyle("Fusion");

    MainWindow w;

    w.show();

    return a.exec();
}
