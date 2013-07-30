#include "createvdfadvanced.h"
#include "ui/Page3adv.h"

CreateVdfAdvanced::CreateVdfAdvanced(QWidget *parent) :
    QDialog(parent), Ui_Page3adv()
{
    setupUi(this);
}

void CreateVdfAdvanced::on_acceptButton_clicked()
{
    CRList = compressionRatioBox->toPlainText();
    QString SBFx = bsxSpinner->text();
    QString SBFy = bsySpinner->text();
    QString SBFz = bszSpinner->text();
    SBFactor = SBFx + "x" + SBFy + "x" + SBFz;

    px = periodicxButton->isChecked() ? "1:" : "0:";
    py = periodicyButton->isChecked() ? "1:" : "0:";
    pz = periodiczButton->isChecked() ? "1" : "0";
    Periodicity = px + py + pz;

    hide();
}

void CreateVdfAdvanced::on_cancelButton_clicked() {
    hide();
}
