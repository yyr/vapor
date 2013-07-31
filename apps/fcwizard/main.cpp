//#include "intropage.h"
#include "fcwizard.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    FCWizard w;
    w.show();

    return a.exec();
}
