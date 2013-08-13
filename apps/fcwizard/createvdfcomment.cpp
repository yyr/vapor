//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           createvdfcomment.cpp
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    Implements a dialog for the user to
//                      add a top-level comment to their vdf file
//                      being created.
//

#include "createvdfcomment.h"
#include "ui/Page3cmt.h"
#include "dataholder.h"

CreateVdfComment::CreateVdfComment(DataHolder *DH, QWidget *parent) :
    QDialog(parent), Ui_Page3cmt()
{
    setupUi(this);
    dataHolder = DH;
}

// Write data to dataHolder
void CreateVdfComment::on_buttonBox_accepted() {
    string comment = vdfComment->toPlainText().toStdString();
    dataHolder->setVDFcomment(comment);
}
