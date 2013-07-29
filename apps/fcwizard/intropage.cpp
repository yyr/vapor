#include "fcwizard.h"
#include "intropage.h"
#include "ui/Page1.h"
#include <QDebug>

IntroPage::IntroPage(QWidget *parent) :
    QWizardPage(parent),Ui_Page1()
{
    setupUi(this);

    QPixmap createVDFPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/makeVDFsmall.png");
    QIcon createVDFButtonIcon(createVDFPixmap);
    createVDFButton->setIcon(createVDFButtonIcon);
    createVDFButton->setIconSize(createVDFPixmap.rect().size());
    createVDFButton->setCheckable(1);

    QPixmap convertDataPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/2VDF.png");
    QIcon convertDataButtonIcon(convertDataPixmap);
    populateDataButton->setIcon(convertDataButtonIcon);
    populateDataButton->setIconSize(convertDataPixmap.rect().size());
    populateDataButton->setCheckable(1);

    registerField("vdfcreate",createVDFButton);
    registerField("2vdf",populateDataButton);

}

void IntroPage::on_createVDFButton_clicked()
{
    qDebug() << field("vdfcreate").toBool();
    if (populateDataButton->isChecked()==1) populateDataButton->setChecked(false);
    operation = "vdfcreate";
    completeChanged();          //call isComplete() to enable or disable 'next' button
}

void IntroPage::on_populateDataButton_clicked()
{
    qDebug() << field("2vdf").toBool();
    if (createVDFButton->isChecked()==1) createVDFButton->setChecked(false);
    operation = "2vdf";
    completeChanged();          //call isComplete() to enable or disable 'next' button
}

bool IntroPage::isComplete() const
{
    if((field("vdfcreate").toBool() || field("2vdf").toBool())) return true;
    else return false;
}
