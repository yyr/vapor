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
    successMessage->label_2->setText("At least one of your variables already has data in the targeted output folder.  Some of this data may be overwritten upon proceeding.  Do you wish to continue?");

	checkOverwrites = new VdfBadFile;
	checkOverwrites->exitButton->setVisible(false);
	checkOverwrites->continueButton->setVisible(false);
	checkOverwrites->label->setText("Warning");
	checkOverwrites->label_2->setText("Some of the selected variables already have data files in the vdf directory.  These files may be overwritten.  Do you still want to proceed?");
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
    //cout << dataHolder->getPDSelectedVars().size();
}

void PopulateDataPage::setupVars() {
    if (dataHolder->getOperation()=="2vdf") varList = dataHolder->getPDDisplayedVars();
	else varList = dataHolder->getVDFSelectedVars();
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

bool PopulateDataPage::checkForOverwrites() {
	/*QString qbaseDir = QString::fromStdString(dataHolder->getVDFfileName());
	string baseDir = QFileInfo(qbaseDir).absolutePath().toStdString();
	for (int var=0; var<dataHolder->getPDSelectedVars().size(); var++){
		char fileLocation[100];
		strcpy(fileLocation, baseDir.c_str());
		strcat(fileLocation,"/");
		strcat(fileLocation, dataHolder->getPDSelectedVars().at(var).c_str());
		QDir dir(QString::fromStdString(fileLocation));
	*/
	QString qdirString = QString::fromStdString(dataHolder->getVDFfileName());
	QFileInfo qfileinfo(qdirString);
	//QFileInfo dataDirInfo(qfileinfo.baseName());

	char fileLocation[100];
	strcpy(fileLocation,qfileinfo.path().toStdString().c_str());
	strcat(fileLocation,"/");
	strcat(fileLocation,qfileinfo.baseName().toStdString().c_str());
	strcat(fileLocation,"_data");
	QString tempString = QString::fromStdString(fileLocation);
	QFileInfo dataDirInfo(tempString);	
	if (dataDirInfo.exists()){
			if (checkOverwrites->exec()==QDialog::Accepted) return true;
			else return false;
		}
	return true;
}

bool PopulateDataPage::validatePage() {
    populateCheckedVars();
	
	if (checkForOverwrites()==false) return false;
	else {
		
		int varsSize = dataHolder->getPDSelectedVars().size();
		int tsSize = atoi(dataHolder->getPDnumTS().c_str());
		int dataChunks = varsSize * tsSize;
		progressBar->setRange(0,dataChunks-1);
	
		stringstream ss;
		char percentComplete[20];
	
   		//cout << dataHolder->getErrors().size() << endl;
    
		//Cycle through variables in each timestep
		for (int timeStep=0;timeStep<tsSize;timeStep++){
			for (int var=0;var<varsSize;var++){
				stringstream ss;
				ss << timeStep;
				if (dataHolder->run2VDFincremental(ss.str(),dataHolder->getPDSelectedVars().at(var)) != 0) {
					//cout << "error time" << dataHolder->getErrors().size() << endl;
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
				//cout << timeStep << " " << var << " " << (timeStep*varsSize)+var << "/" << dataChunks << endl;
			    sprintf(percentComplete,"%.1f%% Complete",(100*((double)(varsSize*timeStep+var)/(double)(dataChunks-1))));
				//cout << percentComplete << endl;	
				percentCompleteLabel->setText(QString::fromUtf8(percentComplete));
				progressBar->setValue((varsSize*timeStep)+var);
				QApplication::processEvents();
			}
		}	
		//progressBar->reset();	
   		successMessage->show();
	
		//stay on page if successMessage does not exit(0)
		return false;   
	}
}
