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
	Complete=1;
    cancelButton->setEnabled(false);
    activateCancel = 0;
	progressBar->setValue(0);    
	progressBar->setTextVisible(false);

	QPixmap populateDataPixmap(_VDFsmall);
    populateDataLabel->setPixmap(populateDataPixmap);

    dataHolder = DH;
    popAdvancedOpts = new PopDataAdvanced(dataHolder);
	errorMessage = new ErrorMessage;
	commandLine = new CommandLine;
	successMessage = new VdfBadFile(this);
	successMessage->buttonBox->setVisible(false);
    successMessage->label->setText("Success!");
    successMessage->label_2->setText("The specified data has finished the conversion process.  You may choose to exit the wizard, or make further data conversions.");

	checkOverwrites = new VdfBadFile(this);
	checkOverwrites->exitButton->setVisible(false);
	checkOverwrites->continueButton->setVisible(false);
	checkOverwrites->label->setText("Warning");
	checkOverwrites->label_2->setText("Some of the selected variables already have data files in the vdf directory.  These files may be overwritten.  Do you still want to proceed?");
}

void PopulateDataPage::on_showCommandButton_clicked() {
    string command;
    command = dataHolder->getPopDataCmd();
    commandLine->commandLineText->clear();

    QString qcommand = QString::fromUtf8(command.c_str());
    commandLine->commandLineText->insertPlainText(qcommand);
    commandLine->show();
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
}

void PopulateDataPage::on_cancelButton_clicked(){
	activateCancel = 1;
	enableWidgets();
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
	tableWidget->clear();
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

    if (dataHolder->getFileType()!="mom"){
        // If ELEVATION var is already included
        if (std::find(varsVector.begin(), varsVector.end(), "ELEVATION") == varsVector.end()) {
            varsVector.push_back("ELEVATION");
        }               
    }   

    dataHolder->setPDSelectedVars(varsVector);
}

bool PopulateDataPage::isComplete() const{
    if (Complete==0) return false;
    else return true;

//	return dataHolder->vdcSettingsChanged;
}

bool PopulateDataPage::checkForOverwrites() {
	QString qdirString = QString::fromStdString(dataHolder->getVDFfileName());
	QFileInfo qfileinfo(qdirString);

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

void PopulateDataPage::enableWidgets() {
    cancelButton->setEnabled(false);
	showCommandButton->setEnabled(true);
    selectAllButton->setEnabled(true);
    clearAllButton->setEnabled(true);
    startTimeSpinner->setEnabled(true);
    numtsSpinner->setEnabled(true);
    advancedOptionsButton->setEnabled(true);

    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setFlags(tableWidget->item(row,col)->flags() |= Qt::ItemIsEnabled);
    }   
}

void PopulateDataPage::disableWidgets() {
	showCommandButton->setEnabled(false);
	selectAllButton->setEnabled(false);
	clearAllButton->setEnabled(false);
	startTimeSpinner->setEnabled(false);
	numtsSpinner->setEnabled(false);
	advancedOptionsButton->setEnabled(false);

	for (int i=0; i<varList.size(); i++) {
    	int row = i/3;
     	int col = i%3;
		tableWidget->item(row,col)->setFlags(tableWidget->item(row,col)->flags() & ~Qt::ItemIsEnabled);
    }   
	cancelButton->setEnabled(true);
}

bool PopulateDataPage::validatePage() {
	wizard()->button(QWizard::NextButton)->setDisabled(true);
    populateCheckedVars();
	percentCompleteLabel->setStyleSheet("QLabel {color : black}");

	progressBar->reset();
	disableWidgets();	
	Complete=0;
	completeChanged();
	wizard()->button(QWizard::BackButton)->setEnabled(false);
	QApplication::processEvents();	

	if (checkForOverwrites()==false) {
        enableWidgets();
        Complete=1;
        completeChanged();
        wizard()->button(QWizard::BackButton)->setEnabled(true);
        QApplication::processEvents();
		return false;
	}
	else {
		
		int varsSize = dataHolder->getPDSelectedVars().size();
		int tsSize = atoi(dataHolder->getPDnumTS().c_str());
		int dataChunks = varsSize * tsSize;
		progressBar->setRange(0,dataChunks-1);
	
		char percentComplete[20];
    
		//Cycle through variables in each timestep
		for (int timeStep=0;timeStep<tsSize;timeStep++){
			for (int var=0;var<varsSize;var++){
				if (activateCancel==0){
					std::stringstream ss;
					ss.clear();
					ss << tsSize-timeStep-1;
					
					if (dataHolder->run2VDFincremental(ss.str(),dataHolder->getPDSelectedVars().at(var)) != 0){
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
				    
						enableWidgets();
					    Complete=1;
					    completeChanged();
					    wizard()->button(QWizard::BackButton)->setEnabled(true);
					    QApplication::processEvents();
						return false;
					}
					
					// Update progress bar
					sprintf(percentComplete,"%.1f%% Complete",(100*((double)(varsSize*timeStep+var+1)/(double)(dataChunks))));
					percentCompleteLabel->setText(QString::fromUtf8(percentComplete));
					progressBar->setValue((varsSize*timeStep)+var);
					QApplication::processEvents();
				}
			}
		}
		if (activateCancel==1){
			percentCompleteLabel->setStyleSheet("QLabel {color : red}");
	        percentCompleteLabel->setText("Process Aborted");
	        enableWidgets();
			cancelButton->setEnabled(false);
			activateCancel=0;
            Complete=1;
            completeChanged();
            wizard()->button(QWizard::BackButton)->setEnabled(true);
            QApplication::processEvents();
			return false;
		}
	}
	successMessage->show();
	successMessage->raise();
	successMessage->activateWindow();
	enableWidgets();
    Complete=1;
    completeChanged();
    wizard()->button(QWizard::BackButton)->setEnabled(true);
    QApplication::processEvents();	
	//stay on page if successMessage does not exit(0)
	return false; 	
}
