#include "evoting.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    eVoting w;
    w.show();

    return a.exec();
}
