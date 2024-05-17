#include "ui/MainWindow.hpp"

#include <QSplashScreen>
#include <QApplication>


int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication a(argc, argv);

    QSplashScreen splash(QPixmap(":/icons/DSV_logo.png"));
    splash.show();

    MainWindow w;

    w.show();
    splash.finish(&w);

    return a.exec();
}
