#include "createvdfcomment.h"
#include "ui/Page3cmt.h"

CreateVdfComment::CreateVdfComment(QWidget *parent) :
    QDialog(parent), Ui_Page3cmt()
{
    setupUi(this);
}

void CreateVdfComment::on_buttonBox_accepted() {
    Comment = vdfComment->toPlainText();
}
