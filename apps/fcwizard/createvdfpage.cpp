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
#include <QtGui>
#include "createvdfpage.h"
#include "createvdfcomment.h"
//#include "createvdfaddnewvar.h"
#include "ui/Page3.h"
#include "dataholder.h"
//#include <QString>

using namespace VAPoR;

CreateVdfPage::CreateVdfPage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page3()
{
    setupUi(this);

    QPixmap createVDFPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/makeVDFsmall.png");
    createVdfLabel->setPixmap(createVDFPixmap);

    dataHolder = DH;
    vdfAdvancedOpts = new CreateVdfAdvanced(dataHolder);
    vdfTLComment = new CreateVdfComment(dataHolder);
    vdfNewVar = new CreateVdfAddNewVar(dataHolder);

    connect(vdfNewVar->buttonBox, SIGNAL(accepted()), this,
            SLOT(addVar()));
}

// Check all loaded variables
void CreateVdfPage::on_selectAllButton_clicked() {
    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setCheckState(Qt::Checked);
    }
    dataHolder->setVDFSelectedVars(dataHolder->getVDFDisplayedVars());
}

// Uncheck all loaded variables
void CreateVdfPage::on_clearAllButton_clicked() {
    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setCheckState(Qt::Unchecked);
    }

    dataHolder->clearVDFSelectedVars();
    cout << "test" << dataHolder->getVDFSelectedVars().size();
    dataHolder->deleteVDFSelectedVar("dzt");
    cout << dataHolder->getVDFSelectedVars().size();
}

// Call vdfcreate and exit without continuing to the populate data page
void CreateVdfPage::saveAndExit() {
    populateCheckedVars();
    dataHolder->VDFCreate();
    exit(0);
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

    setupVars();

    QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch << QWizard::BackButton << QWizard::CustomButton1 << QWizard::NextButton;
    wizard()->setButtonLayout(layout);
    connect(wizard(), SIGNAL(customButtonClicked(int)), this, SLOT(saveAndExit()));
}

void CreateVdfPage::setupVars() {
    varList = dataHolder->getNcdfVars();
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

void CreateVdfPage::addVar() {
    int size = dataHolder->getVDFDisplayedVars().size();
    QString selection = vdfNewVar->addVar->toPlainText();
    dataHolder->addVDFDisplayedVar(selection.toStdString());
    dataHolder->addVDFSelectedVar(selection.toStdString());

    QTableWidgetItem *newVar = new QTableWidgetItem(selection);
    newVar->setCheckState(Qt::Checked);
    qDebug() << size << tableWidget->rowCount();

    if (tableWidget->rowCount() <= size/3) {
        qDebug() << "oi";
        tableWidget->insertRow(tableWidget->rowCount());
    }

    int row = size/3;
    int col = size%3;
    tableWidget->setItem(row,col,newVar);
    varList = dataHolder->getVDFDisplayedVars();
    //tableWidget->setItem(size/3,size%3,newVar);
}

void CreateVdfPage::populateCheckedVars() {
    vector<string> varsVector;

    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        if (tableWidget->item(row,col)->checkState() > 0){
            varsVector.push_back(tableWidget->item(row,col)->text().toStdString());
        }
    }

    dataHolder->setVDFSelectedVars(varsVector);
}

bool CreateVdfPage::validatePage() {
    populateCheckedVars();
    dataHolder->VDFCreate();
    return true;
}
