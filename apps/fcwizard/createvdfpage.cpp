#include <stdio.h>
#include <stdlib.h>
#include "createvdfpage.h"
#include "createvdfcomment.h"
//#include "populatedatapage.h"
#include "ui/Page3.h"
#include "dataholder.h"
#include <QString>

CreateVdfPage::CreateVdfPage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page3()
{
    setupUi(this);

    QPixmap createVDFPixmap("/Users/pearse/Documents/FileConverterWizard/Icons/makeVDFsmall.png");
    //createVDFPixmap.scaled(55,50);
    createVdfLabel->setPixmap(createVDFPixmap.scaled(55,50,Qt::KeepAspectRatio));

    dataHolder = DH;
    vdfAdvancedOpts = new CreateVdfAdvanced(dataHolder);
    vdfTLComment = new CreateVdfComment(dataHolder);
}

void CreateVdfPage::checkArguments() {
    qDebug() << "VDF creation args look good so far...";
}

void CreateVdfPage::on_advanceOptionButton_clicked() {
    vdfAdvancedOpts->show();
}

void CreateVdfPage::on_vdfCommentButton_clicked() {
    vdfTLComment->show();
}

//void CreateVdfPage::on_addNewButton_clicked() {}

void CreateVdfPage::on_selectAllButton_clicked() {
    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setCheckState(Qt::Checked);
    }
}

void CreateVdfPage::on_clearAllButton_clicked() {
    for (int i=0; i<varList.size(); i++) {
        int row = i/3;
        int col = i%3;
        tableWidget->item(row,col)->setCheckState(Qt::Unchecked);
    }
}

/*void CreateVdfPage::on_browseOutputVdfFile_clicked(){
    QString file = QFileDialog::getOpenFileName(this,"Select output metada (.vdf) file.");
    QFileInfo fi(file);
    outputVDFtext->setText(fi.fileName());
    dataHolder->setVDFfileName(fi.absoluteFilePath().toStdString());
}*/

/*void CreateVdfPage::on_outputVDFtext_textChanged() {
    dataHolder->setVDFfileName(outputVDFtext->toPlainText().toStdString());
}*/

void CreateVdfPage::on_goButton_clicked() {
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

void CreateVdfPage::runMomVdfCreate() {
    //qDebug() << Comment + CRList + SBFactor + Periodicity;
}

void CreateVdfPage::initializePage(){
    dataHolder->createReader();

    //startTimeSpinner->setValue(atoi(dataHolder->getVDFStartTime().c_str()));
    //numtsSpinner->setValue(atoi(dataHolder->getVDFnumTS().c_str()));

    startTimeSpinner->setValue(atoi(dataHolder->getVDFStartTime().c_str()));
    numtsSpinner->setValue(atoi(dataHolder->getVDFnumTS().c_str()));

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
}
