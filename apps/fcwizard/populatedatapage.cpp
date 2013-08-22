//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           populatedatapage.cpp
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    QWizardPage reimplementation that steps the user
//                      through selecting parameters for populating VDC
//                      data content.
//

#include "createvdfpage.h"
#include "populatedatapage.h"
#include "ui/Page4.h"
#include "dataholder.h"

using namespace VAPoR;

PopulateDataPage::PopulateDataPage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page4()
{
    setupUi(this);

    QPixmap populateDataPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/2VDFsmall.png");
    populateDataLabel->setPixmap(populateDataPixmap);

    dataHolder = DH;
    popAdvancedOpts = new PopDataAdvanced(dataHolder);

    //NEED TO PUT THIS ALL INTO A DATAHOLDER FUNCTION //

}

void PopulateDataPage::findVars(){
    //VDFIOBase *vdfio = NULL;
    //wcwriter = new WaveCodecIO;
}

void PopulateDataPage::setupVars() {
    varList = dataHolder->getPDDisplayedVars();
    dataHolder->setVDFSelectedVars(varList);
    dataHolder->setVDFDisplayedVars(varList);
    tableWidget->setRowCount(varList.size()/3+1);
    tableWidget->setColumnCount(3);
    tableWidget->horizontalHeader()->setVisible(false);

    for (int i=0;i<varList.size();i++){
        QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(varList.at(i)));
        item->setCheckState(Qt::Checked);
        int row = i/3;
        int col = i%3;
        tableWidget->setItem(row,col,item);
    }
    tableWidget->resizeColumnsToContents();
}

// set some initial values in the page widgets, and set the appropriate
// buttons on the wizard (in this case, we only need back and next)
void PopulateDataPage::initializePage(){
    numtsSpinner->setValue(atoi(dataHolder->getPDnumTS().c_str()));
    numtsSpinner->setMaximum(atoi(dataHolder->getPDnumTS().c_str()));
    dataHolder->findPopDataVars();
    setupVars();

    QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch << QWizard::BackButton << QWizard::FinishButton;
    wizard()->setButtonLayout(layout);
}


// if the user came from the createvdfpage, we need to add the "save/exit" button
// 'save and quit' button to the wizard.
void PopulateDataPage::cleanupPage(){
    if (dataHolder->getOperation() == "2vdf"){
        QList<QWizard::WizardButton> layout;
        layout << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
        wizard()->setButtonLayout(layout);
    }
    else {
        QList<QWizard::WizardButton> layout;
        layout << QWizard::Stretch << QWizard::BackButton << QWizard::CustomButton1 << QWizard::NextButton;
        wizard()->setButtonLayout(layout);
    }
}
