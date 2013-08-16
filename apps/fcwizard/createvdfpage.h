//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           createvdfpage.h
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    Implements the vdf creation page for the
//                      File Converter Wizard (FCWizard)
//
//

#ifndef CREATEVDFPAGE_H
#define CREATEVDFPAGE_H

#include <QWizardPage>
#include <QtGui/QtGui>
#include <QTableWidgetItem>
#include "dataholder.h"
#include "createvdfadvanced.h"
#include "createvdfcomment.h"
#include "createvdfaddnewvar.h"
#include "ui/Page3.h"

namespace Ui {
class CreateVdfPage;
}

class CreateVdfPage : public QWizardPage, public Ui_Page3
{
    Q_OBJECT
    
public:
    CreateVdfPage(DataHolder *DH, QWidget *parent = 0);
    CreateVdfAdvanced *vdfAdvancedOpts;
    CreateVdfComment *vdfTLComment;
    DataHolder *dataHolder;
    CreateVdfAddNewVar *vdfNewVar;

    vector<string> varList;
    vector<string> varSelectionList;
    //vector<QTableWidgetItem> varWidgetItems;

public slots:
    void saveAndExit();

private slots:
    void on_advanceOptionButton_clicked() { vdfAdvancedOpts->show(); }
    void on_vdfCommentButton_clicked() { vdfTLComment->show(); }
    void on_newVarButton_clicked() { vdfNewVar->show(); }
    void on_clearAllButton_clicked();
    void on_selectAllButton_clicked();

private:
    bool validatePage();
    void cleanupPage();
    void setupVars();
    void initializePage();
    void checkArguments();
    void runMomVdfCreate();
    QString Comment;
    QString CRList;
    QString SBFactor;
    QString Periodicity;
};

#endif // CREATEVDFPAGE_H
