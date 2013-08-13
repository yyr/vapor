//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           intropage.h
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    An introductory page where the user selects whether
//                      he/she wishes to create a new VDF metadata file, or
//                      to populate VDC data content.
//

#ifndef INTROPAGE_H
#define INTROPAGE_H

#include <QWizardPage>
#include <QtCore>
#include <QtGui>
#include "ui/Page1.h"
#include "dataholder.h"

namespace Ui {
class IntroPage;
}

class IntroPage : public QWizardPage, public Ui_Page1
{
    Q_OBJECT
    
public:
    IntroPage(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;

private:
    void initializePage();

private slots:
    void on_createVDFButton_clicked();
    void on_populateDataButton_clicked();
};

#endif // INTROPAGE_H
