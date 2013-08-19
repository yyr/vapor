#include "createvdfaddnewvar.h"
#include "ui/Page3addnewvar.h"
#include "dataholder.h"

CreateVdfAddNewVar::CreateVdfAddNewVar(DataHolder *DH, QWidget *parent) :
    QDialog(parent), Ui_Page3addnewvar()
{
    setupUi(this);
    dataHolder = DH;
}

void CreateVdfAddNewVar::on_buttonBox_accepted() {
    dataHolder->addVDFSelectedVar(addVar->toPlainText().toStdString());
    //dataHolder->fileVars.push_back(var);
}
