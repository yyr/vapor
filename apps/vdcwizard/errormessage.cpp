//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                 *
//     University Corporation for Atmospheric Research                   *
//                   All Rights Reserved                                 *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           errormessage.cpp
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    A dialog for listing MyBase error messages to the user
//
//
//

#include <iostream>
#include "errormessage.h"
#include "ui/ErrMsg.h"
#include "ui/CmdLine.h"

using namespace std;

ErrorMessage::ErrorMessage(QWidget *parent, QWizard *wizard) :
    QDialog(parent), Ui_ErrMsg()
{
    setupUi(this);
}

void ErrorMessage::on_buttonBox_accepted() {
	errorList->clear();
	hide();
}

CommandLine::CommandLine(QWidget *parent, QWizard *wizard) :
	QDialog(parent), Ui_CommandLine()
{
	setupUi(this);
}

void CommandLine::on_buttonBox_accepted() {
	commandLineText->clear();
	hide();
}
