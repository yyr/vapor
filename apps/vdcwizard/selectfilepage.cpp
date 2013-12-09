//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           selectfilepage.cpp
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    A QWizardPage that steps the user through selecting
//                      which vdf file to write/include for their processing,
//                      which NetCDF files to reference, and what their
//                      NetCDF data type is (mom, pop, or roms)

#include "vdcwizard.h"
#include "selectfilepage.h"
#include "ui/Page2.h"
#include "intropage.h"
#include "dataholder.h"
#include "vdfbadfile.h"

#include "Images/makeVDFsmall.xpm"
#include "Images/2VDFsmall.xpm"

using namespace std;
using namespace VAPoR;

SelectFilePage::SelectFilePage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page2()
{
    setupUi(this);

	Complete=1;
    momPopOrRoms = "mom";
    dataHolder = DH;
    vdfBadFile = new VdfBadFile;
    vdfBadFile->exitButton->hide();
    vdfBadFile->continueButton->hide();
    
	errorMessage = new ErrorMessage;
	
	QString selectedDirectory;

	vdfCreatePixmap = QPixmap(makeVDFsmall);
	toVdfPixmap = QPixmap(_VDFsmall);
}

void SelectFilePage::on_browseOutputVdfFile_clicked() {
	QString file;
    if (dataHolder->getOperation()=="2vdf") file = QFileDialog::getOpenFileName(this,"Select output metada (.vdf) file.",selectedDirectory);//,"/glade/proj3/DASG/pearse/data");
    else file = QFileDialog::getSaveFileName(this,"Select output metada (.vdf) file.",selectedDirectory);//,selectedFilter=tr("files(*.vdf )"));
	selectedDirectory = QDir(file).absolutePath();
	int size = file.split(".",QString::SkipEmptyParts).size();
	if (file != ""){
		QString base = file.split(".",QString::SkipEmptyParts).at(0);
		QString extension = file.split(".",QString::SkipEmptyParts).at(size-1);
		QStringList list;
		list.append(base);
		if ((size==1)||(extension == NULL)||(extension != "vdf")) {
			list.append(".vdf");
			file = list.join("");
		}
		outputVDFtext->setText(file);
        dataHolder->setVDFfileName(file.toStdString());
        dataHolder->setPDVDFfile(file.toStdString());
    }
}

void SelectFilePage::on_outputVDFtext_textChanged() {
    dataHolder->setVDFfileName(outputVDFtext->toPlainText().toStdString());
	dataHolder->ncdfFilesChanged = true;
	completeChanged();
}


void SelectFilePage::on_addFileButton_clicked() {
	QStringList fileNames = QFileDialog::getOpenFileNames(this,"Select basis files for metadata creation.",selectedDirectory);//"/glade/proj4/DASG/pearse/data");
	if (fileNames.count() >0){
		int count = fileList->count();
		for (int i=0;i<count;i++){
			fileNames.append(fileList->item(i)->text());
		}
	
		fileNames.removeDuplicates();
		fileList->clear();
		fileList->addItems(fileNames);
	
		if (fileList->count() > 0) {
			momRadioButton->setEnabled(true);
			popRadioButton->setEnabled(true);
			romsRadioButton->setEnabled(true);
			wrfRadioButton->setEnabled(true);
		}
	
		selectedDirectory = QDir(fileNames.at(0)).absolutePath();
		// convert fileList into a vector of std::string
		// (not QStrings) to feed into DCReaderMOM
		stdFileList = getSelectedFiles();
		dataHolder->setFiles(stdFileList);
		dataHolder->ncdfFilesChanged = true;
		completeChanged();
	} 
}

void SelectFilePage::on_removeFileButton_clicked() {
    dataHolder->ncdfFilesChanged = true;
	qDeleteAll(fileList->selectedItems());

    /*QStringList fileNames;
    int count = fileList->count();
    for (int i=0;i<count;i++) {
        fileNames.append(fileList->item(i)->text());
    }*/
    stdFileList = getSelectedFiles();
    dataHolder->setFiles(stdFileList);
	completeChanged();
}

void SelectFilePage::on_momRadioButton_clicked() {
    if (momRadioButton->isChecked()) dataHolder->setFileType("mom");
	else dataHolder->setFileType("");
    completeChanged();
}

void SelectFilePage::on_popRadioButton_clicked() {
    if (popRadioButton->isChecked()) dataHolder->setFileType("mom");
    else dataHolder->setFileType("");
	completeChanged();
}

void SelectFilePage::on_romsRadioButton_clicked() {
    if (romsRadioButton->isChecked()) dataHolder->setFileType("roms");
    else dataHolder->setFileType("");
	completeChanged();
}

void SelectFilePage::on_wrfRadioButton_clicked() {
    if (wrfRadioButton->isChecked()) dataHolder->setFileType("wrf");
    else dataHolder->setFileType("");
    completeChanged();
}

vector<string> SelectFilePage::getSelectedFiles() {
    vector <string> stdStrings;
    for (int i=0;i<fileList->count();i++)
        stdStrings.push_back(fileList->item(i)->text().toStdString());
    return stdStrings;
}

void SelectFilePage::cleanupPage() {
	QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch;
    wizard()->setButtonLayout(layout);
}

void SelectFilePage::initializePage() {

	if (dataHolder->getOperation() == "vdfcreate"){
        selectFilePixmap->setPixmap(vdfCreatePixmap);
        vdfLabel->setText("Output VDF File:");
        Title->setText("Files for Create VDF");
    }
    else {
        selectFilePixmap->setPixmap(toVdfPixmap);
        vdfLabel->setText("Reference VDF File:");
        Title->setText("Files for Populate VDC");
    }

    wizard()->setButtonText(QWizard::CustomButton1, tr("Save and Quit"));
    wizard()->setOption(QWizard::HaveCustomButton1, true);

    QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
    wizard()->setButtonLayout(layout);

	resize(sizeHint());
	completeChanged();
}

bool SelectFilePage::isComplete() const {
	if (Complete==1){
	//check for correct inputs and try creating DCReader
		if ((dataHolder->getFileType() == "") &&
    		(dataHolder->getVDFfileName() == "")) return false;
		if ((dataHolder->getFileType() != "") &&
       		(dataHolder->getVDFfileName() != "")){
			return true;
		}
	}
	return false;
}

int SelectFilePage::nextId() const{
	wizard()->button(QWizard::NextButton)->setEnabled(false);
	if (isComplete() == true){
		//if there has been a change to the ncdf files, we will need to generate a new
		//DCReader, and go through our error checking process
		if (dataHolder->createReader()==0) {
			wizard()->button(QWizard::NextButton)->setEnabled(true);
			if (dataHolder->getOperation() == "vdfcreate") return VDCWizard::Create_VdfPage;
	        else return VDCWizard::Populate_DataPage;
		}	
        else {

            for(int i=0;i<dataHolder->getErrors().size();i++){
				errorMessage->errorList->append(dataHolder->getErrors()[i]);
				errorMessage->errorList->append("\n");
			}
			wizard()->button(QWizard::NextButton)->setDisabled(true);
    	    errorMessage->show();
           	dataHolder->clearErrors();
			MyBase::SetErrCode(0);
	    }
	}
	wizard()->button(QWizard::NextButton)->setEnabled(true);
	return VDCWizard::SelectFile_Page;
}
