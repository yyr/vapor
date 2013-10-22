//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           vdfbadfile.h
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

#ifndef VDFBADFILE_H
#define VDFBADFILE_H

#include <QDialog>
#include "ui/Page2badfile.h"

namespace Ui {
class VdfBadFile;
}

class VdfBadFile : public QDialog, public Ui_Page2badfile
{
    Q_OBJECT

public:

    VdfBadFile(QWidget *parent=0);
    void on_buttonBox_accepted();// { hide(); }
	void on_continueButton_clicked();
	void on_exitButton_clicked();

	bool Continue;
};

#endif // VDFBADFILE_H
