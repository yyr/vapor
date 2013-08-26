//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           createvdfadvanced.h
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    Implements the advanced options user interface
//                      for creating VDF metadata files
//
//

#ifndef CREATEVDFADVANCED_H
#define CREATEVDFADVANCED_H

#include <QtGui/QtGui>
#include <QDialog>
#include "ui/Page3adv.h"
#include "dataholder.h"

namespace Ui {
class CreateVdfAdvanced;
}

class CreateVdfAdvanced : public QDialog, public Ui_Page3adv
{
    Q_OBJECT

public:
    explicit CreateVdfAdvanced(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;
    QString CRList;
    QString SBFactor;
    QString Periodicity;
    
private slots:
    void on_acceptButton_clicked();
    void on_cancelButton_clicked() { hide(); }
};

#endif // CREATEVDFADVANCED_H
