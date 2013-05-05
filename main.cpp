#include <QtGui/QApplication>

#include "ui/MainWindow.h"

#include <stdio.h>
#include <unistd.h>

#include "util/Logger.h"

int main(int argc, char *argv[])
{
    LOG_DEBUG(Logger::Misc, "HALLO DU");
    LOG_DEBUGF(Logger::Misc, "HALLO DU %s", "da");


    /*
     * Not needed, if user is in group i2c
     * Maybe expand this and check first if the interface can be accessed
    if( getuid() != 0)
    {
        printf("This application needs root privileges, it seems that you are not root. Some functionality might not be available\n");
    }*/

    QApplication a(argc, argv);
    MainWindow w;

    w.show();

    int ret = a.exec();

    return ret;
}
