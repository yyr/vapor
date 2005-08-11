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
#include "glutil.h"
#include "messagereporter.h"
using namespace VAPoR;
void myMessageOutput( QtMsgType type, const char *msg )
{
    switch ( type ) {
        case QtDebugMsg:
			MessageReporter::infoMsg("qDebug: %s\n", msg );
            break;
        case QtWarningMsg:
            MessageReporter::infoMsg("qWarning: %s\n", msg );
            break;
        case QtFatalMsg:
            MessageReporter::fatalMsg("qFatal %s\n", msg ); 
			break;
    }
}
QApplication* app;
int main( int argc, char ** argv ) {
	//Install our own message handler.
	//Comment out the next line to see qWarnings in console:
	//qInstallMsgHandler( myMessageOutput );
	//Needed for SGI to avoid dithering:
	QApplication::setColorSpec( QApplication::ManyColor );
    QApplication a( argc, argv );
	app = &a;
	a.setStyle("windows");
	a.setPalette(QPalette(QColor(233,236,216), QColor(233,236,216)));
	//Depending on the platform, we may want nondefault fonts!
    //QFont myFont(QString("Helvetica"), 11, 55);
    //a.setFont(myFont);
    MainForm* mw = new MainForm();
	
	
    mw->setCaption( "VAPoR User Interface" );
    mw->show();
    a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
    return a.exec();
}
void VAPoR::BailOut(const char *errstr, char *fname, int lineno)
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
	MessageReporter::fatalMsg(errorMessage);
    app->quit();
}
