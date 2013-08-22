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
#include <vapor/WaveCodecIO.h>
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
    DataHolder *dataHolder;
    //MetadataVDC metadata;
    //WaveCodecIO wcwriter;

    void findVars();
    vector<string> varList;


private slots:
    void on_advancedOptionsButton_clicked() { popAdvancedOpts->show(); }
    void on_startTimeSpinner_valueChanged(QString value){ dataHolder->setVDFstartTime(value.toStdString()); }
    void on_numtsSpinner_valueChanged(QString value){ dataHolder->setVDFnumTS(value.toStdString()); }

private:
    void cleanupPage();
    void initializePage();
    void setupVars();
};

#endif // POPULATEDATAPAGE_H
