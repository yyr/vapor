#include "popdataadvanced.h"
#include "ui/Page4adv.h"

PopDataAdvanced::PopDataAdvanced(QWidget *parent) :
    QDialog(parent), Ui_Page4adv()
{
    setupUi(this);
}

void PopDataAdvanced::on_acceptButton_clicked()
{
    hide();
}
