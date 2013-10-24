//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           vdfbadfile.cpp
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    Warning dialog when user selects file without .vdf extension
//
//
//

#include <iostream>
#include "vdfbadfile.h"
#include "ui/Page2badfile.h"

using namespace std;

VdfBadFile::VdfBadFile(QWidget *parent) :
    QDialog(parent), Ui_Page2badfile()
{
    setupUi(this);
}

void VdfBadFile::on_continueButton_clicked() {
	hide();
}

void VdfBadFile::on_exitButton_clicked() {
	exit(0);
}

void VdfBadFile::on_buttonBox_accepted() { hide(); }
