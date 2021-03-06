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
#include "ui/Page4.h"
#include "dataholder.h"
#include "errormessage.h"
#include "vdfbadfile.h"

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
	CommandLine *commandLine;
	ErrorMessage *errorMessage;
	VdfBadFile *successMessage;
	VdfBadFile *checkOverwrites;

	int Complete;
    bool isComplete() const;
	void findVars();
    vector<string> varList;

private slots:
    void on_showCommandButton_clicked();
	void on_clearAllButton_clicked();
    void on_selectAllButton_clicked();
    void on_advancedOptionsButton_clicked() { popAdvancedOpts->show(); }
    void on_startTimeSpinner_valueChanged(QString value){ dataHolder->setPDstartTime(value.toStdString()); }
    void on_numtsSpinner_valueChanged(QString value){ dataHolder->setPDnumTS(value.toStdString()); }
    void on_cancelButton_clicked();

private:
	void enableWidgets();
    void disableWidgets();
	void cleanupPage();
    void initializePage();
    void setupVars();
    void populateCheckedVars();
    bool validatePage();
	bool checkForOverwrites();
	bool activateCancel;
};

#endif // POPULATEDATAPAGE_H
