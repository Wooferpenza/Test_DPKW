#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("Samopal");
    QApplication::setApplicationName("Test DPKW");
    MainWindow w;
    w.show();

    return a.exec();
}
