#include "selectfilepage.h"
#include "createvdfpage.h"
#include "createvdfcomment.h"
#include "populatedatapage.h"
#include "ui/Page3.h"
//#include "momvdfcreate.cpp"

CreateVdfPage::CreateVdfPage(SelectFilePage *Page, QWidget *parent) :
    QWizardPage(parent), Ui_Page3()
{
    setupUi(this);

    selectFilePage = Page;
    vdfAdvancedOpts = new CreateVdfAdvanced;
    vdfTLComment = new CreateVdfComment;
}

void CreateVdfPage::checkArguments() {
    qDebug() << "VDF creation args look good so far...";
}

void CreateVdfPage::runVdfCreate() {
    qDebug() << "Running VDF Create.";
}

void CreateVdfPage::on_advanceOptionButton_clicked()
{
    vdfAdvancedOpts->show();
}

void CreateVdfPage::on_pushButton_4_clicked()
{
    vdfTLComment->show();
}

void CreateVdfPage::on_goButton_clicked() {
    QList<QString> list;
    list = selectFilePage->getSelectedFiles();
    qDebug() << list;
}
