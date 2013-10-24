//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           popdataadvanced.h
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    Advanced options for VDC data population,
//                      including refinement level, compression level,
//                      and number of threads
//

#ifndef POPDATAADVANCED_H
#define POPDATAADVANCED_H

#include <QDialog>
#include "ui/Page4adv.h"
#include "dataholder.h"

namespace Ui {
class PopDataAdvanced;
}

class PopDataAdvanced : public QDialog, public Ui_Page4adv
{
    Q_OBJECT
    
public:
    explicit PopDataAdvanced(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;
    vector<string> varList;
    
private slots:
    void on_cancelButton_clicked() { hide(); }
    void on_acceptButton_clicked();
    void on_restoreDefaultButton_clicked();
};

#endif // POPDATAADVANCED_H
