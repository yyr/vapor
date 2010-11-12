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
#include <QMessageBox>
#include "messagereporter.h"
#include "GetAppPath.h"
#ifdef WIN32
#include "Windows.h"
#define PYTHONVERSION "2.6"
#endif
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
	//Needed for SGI to avoid dithering:

#ifdef	Darwin
	if (! getenv("DISPLAY")) setenv("DISPLAY", ":0.0",0);
#endif
#ifdef Q_WS_X11
	if (!getenv("DISPLAY")){
		fprintf(stderr,"Error:  X11 DISPLAY variable is not defined. %s \n",
		"Vapor user interface requires X server to be available.");
		exit(-1);
	}
#endif

#ifdef IRIX    
	    QApplication::setColorSpec( QApplication::ManyColor );
#endif
    	QApplication a( argc, argv,true );
	
	//see qWarnings in console only in debug mode:
#ifdef NDEBUG
	qInstallMsgHandler( myMessageOutput );
#endif

	// Set path for Qt to look for its plugins. 
	//
    vector <string> paths;
    QString filePath =  GetAppPath("VAPOR", "plugins", paths).c_str();
    QStringList filePaths(filePath);
    QCoreApplication::setLibraryPaths(filePaths);
//For Mac and Linux, set the PYTHONHOME in this app
#ifndef WIN32

	const char *s = getenv("PYTHONHOME");
	string phome = s ? s : "";
	string pythonversion ("python");
	pythonversion += PYTHONVERSION;
	if (! phome.empty()) {
		string msg("The PYTHONHOME variable is already specified as: \n");
		msg += phome;
		msg += "\n";
		msg += "The VAPOR ";
		msg += pythonversion;
		msg += " environment will operate in this path\n";
		msg += "Unset the PYTHONHOME environment to revert to the installed ";
		msg += "VAPOR " + pythonversion + " environment.";
		QMessageBox::warning(0,"PYTHONHOME warning", msg.c_str());
	} else {
		vector <string> ppaths;
		ppaths.push_back("lib");
		ppaths.push_back(pythonversion);
		phome =  GetAppPath("VAPOR", "", ppaths).c_str();
		if (! phome.empty()) {	
			const string separator = "/";
			string s = "lib";
			s.append(separator);
			s.append(pythonversion);
			phome.erase(phome.rfind(s)); // remove trailing "lib/pythonX.XX"
		}
		else {
			phome = PYTHONDIR;
		}

		setenv("PYTHONHOME",phome.c_str(),1);
	}
	MyBase::SetDiagMsg("PYTHONMOME = %s", phome.c_str());
							   
#endif

	app = &a;
	a.setPalette(QPalette(QColor(233,236,216), QColor(233,236,216)));
#ifdef Darwin
	a.setStyle("macintosh");
#else
	a.setStyle("windows");
#endif
	
	//Depending on the platform, we may want nondefault fonts!
	
	//The pointsize of 10 works ok on linux and irix, not windows
	//The weight of 55 is slightly heavier than normal.
	//default font is OK 
	QFont myFont = a.font();
	myFont.setPointSize(10);
	myFont.setWeight(60);
    	a.setFont(myFont);


/*   Helvetica sometimes looks better on some platforms...
	QFont myFont(QString("Helvetica"), 10);
	if (myFont.exactMatch()){
		//qWarning("Using Helvetica font");
		a.setFont(myFont);
	}
	else {//helvetica not available...
		
		myFont = a.font();
		myFont.setPointSize(10);
		myFont.setWeight(55);
		//qWarning("Using default font family: %s", myFont.family().toAscii());
		a.setFont(myFont);
	}

*/
//#endif

	QString fileName("");
    if (argc > 1) fileName = argv[1];
    MainForm* mw = new MainForm(fileName,app);
	
    mw->setWindowTitle( "VAPOR User Interface" );
    mw->show();
    a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
    return a.exec();
}
