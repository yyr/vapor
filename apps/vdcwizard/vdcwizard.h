//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           vdcwizard.h
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    A QWizard derived class that contains four wizard
//                      pages as well as a data container class that is shared
//                      across all four pages.
//

#ifndef VDCWIZARD_H
#define VDCWIZARD_H

#include <QWizard>
#include <QDialog>
#include <QtCore>
#include <QtGui>
#include "dataholder.h"
#include "intropage.h"
#include "selectfilepage.h"
#include "createvdfpage.h"
#include "populatedatapage.h"

class CreateVdfPage;

class VDCWizard : public QWizard
{
    Q_OBJECT
    
public:
    VDCWizard(QWidget *parent = 0);

    enum { Intro_Page, SelectFile_Page, Create_VdfPage, Populate_DataPage };

    QString operation;
    DataHolder *dataHolder;
    IntroPage *introPage;
    SelectFilePage *selectFilePage;
    CreateVdfPage *createVdfPage;
    PopulateDataPage *populateDataPage;
};

#endif // VDCWIZARDDIALOG_H
