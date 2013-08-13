//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           createvdfpage.cpp
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    Implements the vdf creation page for the
//                      File Converter Wizard (FCWizard)
//
//

#include <stdio.h>
#include <stdlib.h>
#include "createvdfpage.h"
#include "createvdfcomment.h"
#include "ui/Page3.h"
#include "dataholder.h"
#include <QString>

CreateVdfPage::CreateVdfPage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page3()
{
    setupUi(this);

    QPixmap createVDFPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/makeVDFsmall.png");
    createVdfLabel->setPixmap(createVDFPixmap);

    dataHolder = DH;
    vdfAdvancedOpts = new CreateVdfAdvanced(dataHolder);
    vdfTLComment = new CreateVdfComment(dataHolder);
}

void CreateVdfPage::checkArguments() {
    qDebug() << "VDF creation args look good so far...";
}

// Check all loaded variables
void CreateVdfPage::on_selectAllButton_clicked() {
    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setCheckState(Qt::Checked);
    }
}

// Uncheck all loaded variables
void CreateVdfPage::on_clearAllButton_clicked() {
    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setCheckState(Qt::Unchecked);
    }
}

// Call vdfcreate and exit without continuing to the populate data page
void CreateVdfPage::saveAndExit() {
    const char* delim = ":";
    std::stringstream selectionVars;
    vector<string> varsVector;

    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        if (tableWidget->item(row,col)->checkState() > 0){
            varsVector.push_back(tableWidget->item(row,col)->text().toStdString());
        }
    }

    std::copy(varsVector.begin(), varsVector.end(),
              std::ostream_iterator<std::string> (selectionVars,delim));
    dataHolder->setVDFSelectionVars(selectionVars.str());
    dataHolder->runMomVDFCreate();
}

// When the "Back" button is hit, this will get called to remove the "save
// and quit" button, which is not needed for the 'select file page'
void CreateVdfPage::cleanupPage() {
    QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
    wizard()->setButtonLayout(layout);
}

// Create DC reader (via dataHolder and set appropriate widgets with its values.
// Also set the QWizard's buttons to include the custom "save and
// quit" button, in addition to "next" and "back".
void CreateVdfPage::initializePage(){
    dataHolder->createReader();
    startTimeSpinner->setValue(atoi(dataHolder->getVDFStartTime().c_str()));
    numtsSpinner->setValue(atoi(dataHolder->getVDFnumTS().c_str()));
    numtsSpinner->setMaximum(atoi(dataHolder->getVDFnumTS().c_str()));

    varList = dataHolder->getFileVars();
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

    QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch << QWizard::BackButton << QWizard::CustomButton1 << QWizard::NextButton;
    wizard()->setButtonLayout(layout);
    connect(wizard(), SIGNAL(customButtonClicked(int)), this, SLOT(saveAndExit()));
}
