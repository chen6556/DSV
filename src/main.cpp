#include "ui/mainwindow.hpp"

#include <QStyleFactory>
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;

    w.show();
    
    return a.exec();
}
