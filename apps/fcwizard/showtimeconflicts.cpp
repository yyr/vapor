

#include "showtimeconflicts.h"
#include "ui/Page4conf.h"
#include "dataholder.h"

using namespace VAPoR;

ShowTimeConflicts::ShowTimeConflicts(DataHolder *DH, QWidget *parent) :
    QDialog(parent), Ui_Page4conf()
{
    setupUi(this);
    dataHolder = DH;
    timeConflictList->addItem("1200-1277");
    timeConflictList->addItem("1453-1502");
    timeConflictList->addItem("1566-1821");
}
