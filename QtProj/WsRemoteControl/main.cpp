#include "wsremotecontrol.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WsRemoteControl w;
    w.show();

    return a.exec();
}
