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
#include "selectfilepage.h"
#include "populatedatapage.h"
#include "ui/Page4.h"
#include "dataholder.h"

#include "Images/2VDFsmall.xpm"

using namespace VAPoR;

PopulateDataPage::PopulateDataPage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page4()
{
    setupUi(this);

	progressBar->setValue(0);    
	progressBar->setTextVisible(true);

	QPixmap populateDataPixmap(_VDFsmall);
    populateDataLabel->setPixmap(populateDataPixmap);

    dataHolder = DH;
    popAdvancedOpts = new PopDataAdvanced(dataHolder);
	errorMessage = new ErrorMessage;
	successMessage = new VdfBadFile;
	successMessage->buttonBox->setVisible(false);
    successMessage->label->setText("Success!");
    successMessage->label_2->setText("The specified data was successfully translated into the VDC format.  Click the 'Continue' button if you'd like to perform further data conversions.");
}

void PopulateDataPage::on_selectAllButton_clicked() {
    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setCheckState(Qt::Checked);
    }
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
    tableWidget->setColumnWidth(0,tableWidget->width()/3);
    tableWidget->setColumnWidth(1,tableWidget->width()/3);
    tableWidget->setColumnWidth(2,tableWidget->width()/3);
}

// set some initial values in the page widgets, and set the appropriate
// buttons on the wizard (in this case, we only need back and next)
void PopulateDataPage::initializePage(){
    startTimeSpinner->setMaximum(atoi(dataHolder->getPDStartTime().c_str()));
	startTimeSpinner->setValue(atoi(dataHolder->getPDStartTime().c_str()));
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

bool PopulateDataPage::isComplete() {
	return dataHolder->vdcSettingsChanged;
}

bool PopulateDataPage::validatePage() {
    int varsSize = dataHolder->getPDSelectedVars().size();
	int tsSize = atoi(dataHolder->getPDnumTS().c_str());
	int dataChunks = varsSize * tsSize;
	progressBar->setRange(0,dataChunks);

	stringstream ss;
	char percentComplete[40];

	populateCheckedVars();
    cout << dataHolder->getErrors().size() << endl;
    cout << "creating vdc" << endl;
    
	//Cycle through variables in each timestep
	for (int timeStep=0;timeStep<tsSize;timeStep++){
		for (int var=0;var<varsSize;var++){
			stringstream ss;
			ss << timeStep;
			if (dataHolder->run2VDFincremental(ss.str(),dataHolder->getPDSelectedVars().at(var)) != 0) {
				cout << "error time" << dataHolder->getErrors().size() << endl;
        		dataHolder->vdcSettingsChanged=false;
        		for (int i=0;i<dataHolder->getErrors().size();i++){
            		errorMessage->errorList->append(dataHolder->getErrors()[i]);
            		errorMessage->errorList->append("\n");
        		}   
        		errorMessage->show();
        		errorMessage->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        		errorMessage->raise();
        		errorMessage->activateWindow();
        		dataHolder->clearErrors();
		        MyBase::SetErrCode(0);
				progressBar->reset();
				return false;
			}
			cout << timeStep << " " << var << " " << (timeStep*varsSize)+var << "/" << dataChunks << endl;
		    sprintf(percentComplete,"%.0f%% Complete",(100*((double)(varsSize*timeStep+var)/(double)dataChunks)));
			cout << percentComplete << endl;	
			//ss << 100*((double)(tsSize*var)/(double)dataChunks);
			//ss << "%";
			percentCompleteLabel->setText(QString::fromUtf8(percentComplete));//ss.str()));
			progressBar->setValue((varsSize*timeStep)+var);
			QApplication::processEvents();
		}
	}	
	
    successMessage->show();
	return false;   
}
