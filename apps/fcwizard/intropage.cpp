//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           intropage.cpp
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    An introductory page where the user selects whether
//                      he/she wishes to create a new VDF metadata file, or
//                      to populate VDC data content.
//

#include "intropage.h"
#include "ui/Page1.h"
#include "dataholder.h"
#include <QDebug>

using namespace VAPoR;

IntroPage::IntroPage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent),Ui_Page1()
{
    setupUi(this);

    dataHolder = DH;
    //QPixmap createVDFPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/makeVDF.png");
	QPixmap createVDFPixmap("../../../Images/makeVDF.png");
    QIcon createVDFButtonIcon(createVDFPixmap);
    createVDFButton->setIcon(createVDFButtonIcon);
    createVDFButton->setIconSize(createVDFPixmap.rect().size());
    createVDFButton->setCheckable(1);

    //QPixmap convertDataPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/2VDF.png");
	QPixmap convertDataPixmap("../../../Images/2VDF.png");
    QIcon convertDataButtonIcon(convertDataPixmap);
    populateDataButton->setIcon(convertDataButtonIcon);
    populateDataButton->setIconSize(convertDataPixmap.rect().size());
    populateDataButton->setCheckable(1);

    registerField("vdfcreate",createVDFButton);
    registerField("2vdf",populateDataButton);
}

void IntroPage::on_createVDFButton_clicked()
{
    if (populateDataButton->isChecked()==1) populateDataButton->setChecked(false);
    dataHolder->setOperation("vdfcreate");
    wizard()->next();
}

void IntroPage::on_populateDataButton_clicked()
{
    if (createVDFButton->isChecked()==1) createVDFButton->setChecked(false);
    dataHolder->setOperation("2vdf");
    wizard()->next();
}

void IntroPage::initializePage() {
    cout << "intropage initialized" << endl;
	QList<QWizard::WizardButton> layout;
    wizard()->setButtonLayout(layout);
}
