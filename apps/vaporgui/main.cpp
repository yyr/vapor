//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		main.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:  Main program for vapor gui

#include <qapplication.h>
#include "mainform.h"
#include <qfont.h>
#include <qmessagebox.h>
#include "glutil.h"
using namespace VAPoR;

QApplication* app;
int main( int argc, char ** argv ) {
	//Needed for SGI to avoid dithering:
	QApplication::setColorSpec( QApplication::ManyColor );
    QApplication a( argc, argv );
	app = &a;
	//Depending on the platform, we may want nondefault fonts!
    //QFont myFont(QString("Helvetica"), 11, 55);
    //a.setFont(myFont);
    MainForm* mw = new MainForm();
    mw->setCaption( "VAPoR GUI prototype" );
    mw->show();
    a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
    return a.exec();
}
void VAPoR::BailOut(char *errstr, char *fname, int lineno)
{
    /* Terminate program after printing an error message.
     * Use via the macros Verify and MemCheck.
     */
    //Error("Error: %s, at %s:%d\n", errstr, fname, lineno);
    //if (coreDumpOnError)
	//abort();
	QString errorMessage(errstr);
	errorMessage += "\n in file: ";
	errorMessage += fname;
	errorMessage += " at line ";
	errorMessage += QString::number(lineno);
	QMessageBox::critical(0, "Unrecoverable error", errorMessage, QMessageBox::Ok, QMessageBox::No);
    app->quit();
}
