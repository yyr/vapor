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

#include <QDebug>
#include <cstdio>
#include "createvdfpage.h"
#include "populatedatapage.h"
#include "ui/Page4.h"
#include "dataholder.h"

using namespace VAPoR;

PopulateDataPage::PopulateDataPage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page4()
{
    setupUi(this);

    //QPixmap populateDataPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/2VDFsmall.png");
    QPixmap populateDataPixmap("../../../Images/2VDFsmall.png");
    populateDataLabel->setPixmap(populateDataPixmap);

    dataHolder = DH;
    popAdvancedOpts = new PopDataAdvanced(dataHolder);

	errorMessage = new ErrorMessage;
}

void PopulateDataPage::findVars(){
    //VDFIOBase *vdfio = NULL;
    //wcwriter = new WaveCodecIO;
}

void PopulateDataPage::on_selectAllButton_clicked() {
    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setCheckState(Qt::Checked);
    }
    //dataHolder->setVDFSelectedVars(dataHolder->getVDFDisplayedVars());
	dataHolder->setPDSelectedVars(dataHolder->getPDDisplayedVars());
}

// Uncheck all loaded variables
void PopulateDataPage::on_clearAllButton_clicked() {
    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setCheckState(Qt::Unchecked);
    }

    dataHolder->clearPDSelectedVars();
    cout << dataHolder->getPDSelectedVars().size();
}

void PopulateDataPage::setupVars() {
    varList = dataHolder->getPDDisplayedVars();
    dataHolder->setPDSelectedVars(varList);
    dataHolder->setPDDisplayedVars(varList);
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
    startTimeSpinner->setMaximum(atoi(dataHolder->getPDStartTime().c_str()));
	startTimeSpinner->setValue(atoi(dataHolder->getPDStartTime().c_str()));
    numtsSpinner->setValue(atoi(dataHolder->getPDnumTS().c_str()));
    numtsSpinner->setMaximum(atoi(dataHolder->getPDnumTS().c_str()));
    dataHolder->findPopDataVars();

    //dataHolder->setPDnumTS(strstream);//dataHolder->reader->GetNumTimeSteps());
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

void PopulateDataPage::populateCheckedVars() {
    vector<string> varsVector;

    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        if (tableWidget->item(row,col)->checkState() > 0){
            varsVector.push_back(tableWidget->item(row,col)->text().toStdString());
        }
    }

    dataHolder->setPDSelectedVars(varsVector);
}

bool PopulateDataPage::validatePage() {
    populateCheckedVars();

    cout << "vdc changed:" << dataHolder->vdcSettingsChanged << endl;

    if (isComplete() == true) {
        if (dataHolder->vdcSettingsChanged==true) {
            cout << "creating vdc" << endl;
            if (dataHolder->run2VDF()==0) {   
                dataHolder->vdcSettingsChanged=false;
                return true;
            }   
            else {
                dataHolder->vdcSettingsChanged=false;
                for (int i=0;i<dataHolder->getErrors().size();i++){
                    errorMessage->errorList->append(dataHolder->getErrors()[i]);
                    errorMessage->errorList->append("\n");
                }   
                wizard()->button(QWizard::FinishButton)->setDisabled(true);
                errorMessage->show();
                dataHolder->clearErrors();
                MyBase::SetErrCode(0);
                return false;
            }   
        }   
        else return true;
    }   
    return false;   



dataHolder->run2VDF();
    
    return true;
}
