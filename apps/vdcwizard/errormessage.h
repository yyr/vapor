//************************************************************************
//                                                                                        
//                   Copyright (C)  2013                                 *
//     University Corporation for Atmospheric Research                   *
//                   All Rights Reserved                                 *
//                                                                                        
//************************************************************************/
//
//      File:           errormessage.h
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    An error message dialog for relaying MyBase errors
//                      to the user.
//

#ifndef ERRORMESSAGE_H
#define ERRORMESSAGE_H

#include <QWizard>
#include <QDialog>
#include "ui/ErrMsg.h"
#include "ui/CmdLine.h"

namespace Ui {
class ErrorMessage;
class CommandLine;
}

class ErrorMessage : public QDialog, public Ui_ErrMsg
{
    Q_OBJECT

public:
    ErrorMessage(QWidget *parent=0, QWizard *wiz=0);

private slots:
    void on_buttonBox_accepted();
};

class CommandLine : public QDialog, public Ui_CommandLine
{
    Q_OBJECT

public:
    CommandLine(QWidget *parent=0, QWizard *wiz=0);

private slots:
    void on_buttonBox_accepted();
};

#endif // ERRORMESSAGE_H
