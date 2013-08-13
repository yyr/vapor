//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           popdataadvanced.cpp
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

#include "popdataadvanced.h"
#include "ui/Page4adv.h"
#include "dataholder.h"

PopDataAdvanced::PopDataAdvanced(DataHolder *DH, QWidget *parent) :
    QDialog(parent), Ui_Page4adv()
{
    dataHolder = DH;
    setupUi(this);
}

void PopDataAdvanced::on_restoreDefaultButton_clicked(){
    refinementLevelSpinner->setValue(0);
    compressionLevelSpinner->setValue(0);
    numThreadsSpinner->setValue(0);
}

void PopDataAdvanced::on_acceptButton_clicked() {
    dataHolder->setPDrefLevel(refinementLevelSpinner->text().toStdString());
    dataHolder->setPDcompLevel(compressionLevelSpinner->text().toStdString());
    dataHolder->setPDnumThreads(numThreadsSpinner->text().toStdString());
    hide();
}
