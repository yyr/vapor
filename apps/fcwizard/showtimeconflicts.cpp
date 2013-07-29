#include "showtimeconflicts.h"
#include "ui/Page4conf.h"

ShowTimeConflicts::ShowTimeConflicts(QWidget *parent) :
    QDialog(parent), Ui_Page4conf()
{
    setupUi(this);
    timeConflictList->addItem("1200-1277");
    timeConflictList->addItem("1453-1502");
    timeConflictList->addItem("1566-1821");
}
