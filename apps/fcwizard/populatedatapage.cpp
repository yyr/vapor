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

PopulateDataPage::PopulateDataPage(DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page4()
{
    setupUi(this);

    dataHolder = DH;
    popAdvancedOpts = new PopDataAdvanced(dataHolder);
    //timeConflicts = new ShowTimeConflicts(dataHolder);
}

void PopulateDataPage::printSomething() {
    qDebug() << "Populate Data Page PRinting";
}

void PopulateDataPage::on_goButton_clicked() {
    checkArguments();
    run2vdf();
}

void PopulateDataPage::on_advancedOptionsButton_clicked()
{
    popAdvancedOpts->show();
}

// This is temporary code that was used as a 'smoke and mirrors'
// example of what the data population might look like.  This will
// be eliminated in the next phase of development.
void PopulateDataPage::on_scanVDF_clicked()
{
    qDebug() << "scan vdf";

    variableList->setRowCount(7);
    variableList->setColumnCount(4);
    variableList->verticalHeader()->setVisible(false);
    variableList->setHorizontalHeaderItem(0,new QTableWidgetItem("Variable"));
    variableList->setHorizontalHeaderItem(1,new QTableWidgetItem("Time Conflict"));
    variableList->setHorizontalHeaderItem(2,new QTableWidgetItem("First Overlap"));
    variableList->setHorizontalHeaderItem(3,new QTableWidgetItem("Details"));


    QTableWidgetItem *u = new QTableWidgetItem("U");
    QTableWidgetItem *v = new QTableWidgetItem("V");
    QTableWidgetItem *w = new QTableWidgetItem("W");
    QTableWidgetItem *precip = new QTableWidgetItem("Precip");
    QTableWidgetItem *wind = new QTableWidgetItem("Wind Velocity");
    QTableWidgetItem *dBZ = new QTableWidgetItem("dBZ");

    u->setCheckState(Qt::Unchecked);
    v->setCheckState(Qt::Unchecked);
    w->setCheckState(Qt::Unchecked);
    precip->setCheckState(Qt::Unchecked);
    wind->setCheckState(Qt::Unchecked);
    dBZ->setCheckState(Qt::Unchecked);

    variableList->setItem(0,0,u);
    variableList->setItem(1,0,v);
    variableList->setItem(2,0,w);
    variableList->setItem(3,0,precip);
    variableList->setItem(4,0,wind);
    variableList->setItem(5,0,dBZ);

    QTableWidgetItem *warning0 = new QTableWidgetItem("ok");
    warning0->setTextColor(Qt::green);
    warning0->setTextAlignment(Qt::AlignCenter);
    variableList->setItem(0,1,warning0);

    QTableWidgetItem *warning1 = new QTableWidgetItem("!");
    warning1->setTextColor(Qt::red);
    warning1->setTextAlignment(Qt::AlignCenter);
    variableList->setItem(1,1,warning1);

    QTableWidgetItem *warning2 = new QTableWidgetItem("!");
    warning2->setTextColor(Qt::red);
    warning2->setTextAlignment(Qt::AlignCenter);
    variableList->setItem(2,1,warning2);

    QTableWidgetItem *warning3 = new QTableWidgetItem("ok");
    warning3->setTextColor(Qt::green);
    warning3->setTextAlignment(Qt::AlignCenter);
    variableList->setItem(3,1,warning3);

    QTableWidgetItem *warning4 = new QTableWidgetItem("ok");
    warning4->setTextColor(Qt::green);
    warning4->setTextAlignment(Qt::AlignCenter);
    variableList->setItem(4,1,warning4);

    QTableWidgetItem *warning5 = new QTableWidgetItem("ok");
    warning5->setTextColor(Qt::green);
    warning5->setTextAlignment(Qt::AlignCenter);
    variableList->setItem(5,1,warning5);

    QTableWidgetItem *firstTimes0 = new QTableWidgetItem("1365-1566");
    //warning5->setTextColor(Qt::green);
    firstTimes0->setTextAlignment(Qt::AlignCenter);
    variableList->setItem(1,2,firstTimes0);

    QTableWidgetItem *firstTimes1 = new QTableWidgetItem("1200-1277");
    //warning5->setTextColor(Qt::green);
    firstTimes1->setTextAlignment(Qt::AlignCenter);
    variableList->setItem(2,2,firstTimes1);

    QPushButton *warnButton0 = new QPushButton;
    warnButton0->setText("more...");
    variableList->setIndexWidget(variableList->model()->index(1,3),warnButton0);
    connect(warnButton0,SIGNAL(clicked()),this,SLOT(warnButton_clicked()));

    QPushButton *warnButton1 = new QPushButton;
    warnButton1->setText("more...");
    variableList->setIndexWidget(variableList->model()->index(2,3),warnButton1);
    connect(warnButton1,SIGNAL(clicked()),this,SLOT(warnButton_clicked()));

    numtsSpinner->setValue(1566);
}

void PopulateDataPage::on_variableList_activated(const QModelIndex &index)
{
    qDebug() << index;
}

void PopulateDataPage::warnButton_clicked(){
    timeConflicts->show();
}

void PopulateDataPage::checkArguments() {
    qDebug() << "data conversion args look good so far...";
}

void PopulateDataPage::run2vdf() {
    qDebug() << "Running 2 VDF.";
}

// in the case that the user hits the 'Back' button, we need to add the
// 'save and quit' button to the wizard.
void PopulateDataPage::cleanupPage() {
    QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch << QWizard::BackButton << QWizard::CustomButton1 << QWizard::NextButton;
    wizard()->setButtonLayout(layout);
}

// set some initial values in the page widgets, and set the appropriate
// buttons on the wizard (in this case, we only need back and next)
void PopulateDataPage::initializePage(){
    numtsSpinner->setValue(atoi(dataHolder->getPDnumTS().c_str()));

    QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch << QWizard::BackButton << QWizard::FinishButton;
    wizard()->setButtonLayout(layout);
}
