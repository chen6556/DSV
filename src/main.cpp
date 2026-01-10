#include <QApplication>
#include <QOpenGLContext>
#include <QSurfaceFormat>

#include "ui/MainWindow.hpp"


int main(int argc, char *argv[])
{

#ifdef __APPLE__
    QSurfaceFormat default_fmt{QSurfaceFormat::defaultFormat()};
    default_fmt.setVersion(4, 1);
    default_fmt.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    QSurfaceFormat::setDefaultFormat(default_fmt);
#endif

    QApplication a(argc, argv);
    a.setStyle("Fusion");

    MainWindow w;

    w.show();

    return a.exec();
}
