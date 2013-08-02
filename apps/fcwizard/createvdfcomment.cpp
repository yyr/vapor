#include "createvdfcomment.h"
#include "ui/Page3cmt.h"
#include "dataholder.h"

CreateVdfComment::CreateVdfComment(DataHolder *DH, QWidget *parent) :
    QDialog(parent), Ui_Page3cmt()
{
    setupUi(this);
    dataHolder = DH;
}

void CreateVdfComment::on_buttonBox_accepted() {
    string comment = vdfComment->toPlainText().toStdString();
    dataHolder->setVDFcomment(comment);
}
