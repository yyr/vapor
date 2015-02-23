//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           main.cpp
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    main function for VDCWizard, which intializes the
//                      VDCWizard class and executes it.
//
//

#include "vdcwizard.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	QFont myFont = a.font();
	myFont.setPointSize(13);
	a.setFont(myFont);

    VDCWizard w;
    w.show();
    //w.setWindowState(~Qt::WindowMinimized) | Qt::WindowActive);
    w.raise();
    w.activateWindow();

    return a.exec();
}
