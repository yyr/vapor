//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           vdcwizard.cpp
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

#include <QDebug>
#include <QtGui/QMessageBox>

#include "vdcwizard.h"

using namespace VAPoR;

VDCWizard::VDCWizard(QWidget *parent) :
    QWizard(parent)
{
	#ifdef _WIN32
		resize(QSize(525,500).expandedTo(minimumSizeHint()));
	#else
		resize(QSize(500,450).expandedTo(minimumSizeHint()));
		setWizardStyle(QWizard::AeroStyle);
	#endif
	//setWizardStyle(QWizard::ClassicStyle);
    //setWizardStyle(QWizard::ModernStyle);
    //setWizardStyle(QWizard::MacStyle);
    //resize(QSize(500,450).expandedTo(minimumSizeHint()));

    dataHolder = new DataHolder;
    introPage = new IntroPage(dataHolder);
    selectFilePage = new SelectFilePage(dataHolder);
    createVdfPage = new CreateVdfPage(dataHolder);
    populateDataPage = new PopulateDataPage(dataHolder);

    setPage(Intro_Page, introPage);
    setPage(SelectFile_Page, selectFilePage);
    setPage(Create_VdfPage, createVdfPage);
    setPage(Populate_DataPage, populateDataPage);

    setOption(QWizard::NoBackButtonOnStartPage,true);
}
