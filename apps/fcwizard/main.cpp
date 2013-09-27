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
//      Description:    main function for FCWizard, which intializes the
//                      FCWizard class and executes it.
//
//

#include "fcwizard.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	QFont myFont = a.font();
	myFont.setPointSize(13);
	a.setFont(myFont);

    FCWizard w;
    w.show();

    return a.exec();
}
