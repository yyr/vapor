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

#ifndef CREATEVDFCOMMENT_H
#define CREATEVDFCOMMENT_H

#include <QDialog>
#include "ui/Page3cmt.h"
#include "dataholder.h"


namespace Ui {
class CreateVdfComment;
}

class CreateVdfComment : public QDialog, public Ui_Page3cmt
{
    Q_OBJECT
    
public:
    explicit CreateVdfComment(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;

private slots:
    void on_buttonBox_accepted();
};

#endif // CREATEVDFCOMMENT_H
