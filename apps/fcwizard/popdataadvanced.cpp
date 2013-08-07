#include "popdataadvanced.h"
#include "ui/Page4adv.h"
#include "dataholder.h"

PopDataAdvanced::PopDataAdvanced(DataHolder *DH, QWidget *parent) :
    QDialog(parent), Ui_Page4adv()
{
    dataHolder = DH;
    setupUi(this);
}

void PopDataAdvanced::on_restoreDefaultButton_clicked(){
    refinementLevelSpinner->setValue(0);
    compressionLevelSpinner->setValue(0);
    numThreadsSpinner->setValue(0);
}

void PopDataAdvanced::on_acceptButton_clicked() {
    dataHolder->setPDrefLevel(refinementLevelSpinner->text().toStdString());
    dataHolder->setPDcompLevel(compressionLevelSpinner->text().toStdString());
    dataHolder->setPDnumThreads(numThreadsSpinner->text().toStdString());
    hide();
}
