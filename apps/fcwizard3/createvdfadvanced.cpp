#include "createvdfadvanced.h"
#include "ui/Page3adv.h"

CreateVdfAdvanced::CreateVdfAdvanced(QWidget *parent) :
    QDialog(parent), Ui_Page3adv()
{
    setupUi(this);
}

void CreateVdfAdvanced::on_acceptButton_clicked()
{
    hide();
}
