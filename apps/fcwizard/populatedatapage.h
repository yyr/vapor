//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           populatedatapage.h
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

#ifndef POPULATEDATAPAGE_H
#define POPULATEDATAPAGE_H

#include <QWizardPage>
#include <QtGui/QtGui>
#include "popdataadvanced.h"
#include "showtimeconflicts.h"
#include "ui/Page4.h"
#include "dataholder.h"

namespace Ui {
class PopulateDataPage;
}

class PopulateDataPage : public QWizardPage, public Ui_Page4
{
    Q_OBJECT
    
public:
    PopulateDataPage(DataHolder *DH, QWidget *parent = 0);
    PopDataAdvanced *popAdvancedOpts;
    ShowTimeConflicts *timeConflicts;
    DataHolder *dataHolder;

    void printSomething();

private slots:
    void on_goButton_clicked();
    void on_advancedOptionsButton_clicked();
    void on_scanVDF_clicked();
    void on_variableList_activated(const QModelIndex &index);
    void warnButton_clicked();

private:
    void cleanupPage();
    void initializePage();
    void checkArguments();
    void run2vdf();
};

#endif // POPULATEDATAPAGE_H
