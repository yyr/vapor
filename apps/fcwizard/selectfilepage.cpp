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

#include "fcwizard.h"
#include "selectfilepage.h"
#include "ui/Page2.h"
#include "intropage.h"
#include "dataholder.h"

using namespace std;
using namespace VAPoR;

SelectFilePage::SelectFilePage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page2()
{
    setupUi(this);

    momPopOrRoms = "mom";
    dataHolder = DH;
    vdfCreatePixmap = QPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/makeVDFsmall.png");
    toVdfPixmap = QPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/2vdfsmall.png");
}

void SelectFilePage::on_browseOutputVdfFile_clicked() {
    QString file = QFileDialog::getOpenFileName(this,"Select output metada (.vdf) file.");
    if (file.split(".",QString::SkipEmptyParts).at(-1) != "vdf") vdfBadFile->show();
    outputVDFtext->setText(file);
    dataHolder->setVDFfileName(file.toStdString());
    dataHolder->setPDVDFfile(file.toStdString());
}

void SelectFilePage::on_addFileButton_clicked() {
    QStringList fileNames = QFileDialog::getOpenFileNames(this,"Select basis files for metadata creation.");
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
    }

    // convert fileList into a vector of std::string
    // (not QStrings) to feed into DCReaderMOM
    stdFileList = getSelectedFiles();
    dataHolder->setFiles(stdFileList);
}

void SelectFilePage::on_removeFileButton_clicked() {
    qDeleteAll(fileList->selectedItems());

    QStringList fileNames;
    int count = fileList->count();
    for (int i=0;i<count;i++) {
        fileNames.append(fileList->item(i)->text());
    }
    stdFileList = getSelectedFiles();
    dataHolder->setFiles(stdFileList);
}

void SelectFilePage::on_momRadioButton_clicked() {
    dataHolder->setFileType("mom");
    completeChanged();
}

void SelectFilePage::on_popRadioButton_clicked() {
    dataHolder->setFileType("mom");
    completeChanged();
}

void SelectFilePage::on_romsRadioButton_clicked() {
    dataHolder->setFileType("roms");
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
        selectFilePixmap->setPixmap(vdfCreatePixmap);//.scaled(55,50,Qt::KeepAspectRatio));
        vdfLabel->setText("Output VDF File:");
        Title->setText("Files for Create VDF");
    }
    else {
        selectFilePixmap->setPixmap(toVdfPixmap);//,50,Qt::KeepAspectRatio));
        vdfLabel->setText("Reference VDF File:");
        Title->setText("Files for Populate VDC");
    }

    wizard()->setButtonText(QWizard::CustomButton1, tr("Save and Quit"));
    wizard()->setOption(QWizard::HaveCustomButton1, true);

    QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
    wizard()->setButtonLayout(layout);
}

bool SelectFilePage::isComplete() const {
    if ((dataHolder->getFileType() != "") &&
        (dataHolder->getVDFfileName() != ""))
        return true;
    else return false;
}

int SelectFilePage::nextId() const
{
    if (dataHolder->getOperation() == "vdfcreate") return FCWizard::Create_VdfPage;
    else return FCWizard::Populate_DataPage;
}
