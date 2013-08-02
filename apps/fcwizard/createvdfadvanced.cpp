#include "createvdfadvanced.h"
#include "ui/Page3adv.h"
#include "dataholder.h"
#include <QString>

CreateVdfAdvanced::CreateVdfAdvanced(DataHolder *DH, QWidget *parent) :
    QDialog(parent), Ui_Page3adv()
{
    setupUi(this);

    dataHolder = DH;
}

void CreateVdfAdvanced::on_acceptButton_clicked()
{
    dataHolder->setVDFcrList(compressionRatioBox->toPlainText().toStdString());
    //CRList = compressionRatioBox->toPlainText();
    QString SBFx = bsxSpinner->text();
    QString SBFy = bsySpinner->text();
    QString SBFz = bszSpinner->text();
    QString SBFactor = SBFx + "x" + SBFy + "x" + SBFz;
    dataHolder->setVDFSBFactor(SBFactor.toStdString());

    QString px = periodicxButton->isChecked() ? "1:" : "0:";
    QString py = periodicyButton->isChecked() ? "1:" : "0:";
    QString pz = periodiczButton->isChecked() ? "1" : "0";
    QString Periodicity = px + py + pz;
    dataHolder->setVDFPeriodicity(Periodicity.toStdString());

    hide();
}

void CreateVdfAdvanced::on_cancelButton_clicked() {
    hide();
}
