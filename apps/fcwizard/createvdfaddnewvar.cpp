#include "createvdfaddnewvar.h"
#include "ui/Page3addnewvar.h"
#include "dataholder.h"

CreateVdfAddNewVar::CreateVdfAddNewVar(DataHolder *DH, CreateVdfPage *parent);//QWidget *parent) :
    QDialog(parent), Ui_Page3addnewvar()
{
    setupUi(this);
    dataHolder = DH;
    parentPage = parent;
}

void CreateVdfAddNewVar::on_buttonBox_accepted() {
    string var = addVar->toPlainText().toStdString();
    parent->tableWidget->setItem(6,1,newQTableWidgetItem("test"));
    //dataHolder->fileVars.push_back(var);
}
