//************************************************************************
//                                                                        *
//             Copyright (C)  2004                                        *
//     University Corporation for Atmospheric Research                    *
//             All Rights Reserved                                        *
//                                                                        *
//************************************************************************/
//
//    File:        main.cpp
//
//    Author:        Alan Norton
//            National Center for Atmospheric Research
//            PO 3000, Boulder, Colorado
//
//    Date:        June 2004
//
//    Description:  Main program for vapor gui

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sstream>
#include <qapplication.h>
#include "mainform.h"
#include <qfont.h>
#include <QMessageBox>
#include "messagereporter.h"
#include "versionchecker.h"
#include "bannergui.h"
#include "vapor/GetAppPath.h"
#ifdef WIN32
#include "Windows.h"
#define PYTHONVERSION "2.7"
#else
#include <unistd.h>
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
        default://ignore QtCriticalMsg and QtSystemMsg
            break;
    }
}


//
// Open a file named by the environment variable, path_var. Exit on failure
//
FILE *OpenLog(string path_var) {
    FILE *fp = NULL;
    const char *cstr = getenv(path_var.c_str());
    if (! cstr) return(NULL);
    string s = cstr;
    if (! s.empty()) {
        fp = fopen(s.c_str(), "w");
        if (! fp) {
            cerr << "Failed to open " << s << " : " << strerror(errno) << endl;
            exit(1);
        }
        MyBase::SetDiagMsgFilePtr(fp);
    }
    return(fp);
}

QApplication* app;
int main( int argc, char ** argv ) {

#ifdef WIN32
	std::stringstream ss;
	string vHome = getenv("VAPOR_HOME");
	ss << "GRIB_DEFINITION_PATH=" << vHome << "share\\grib_api\\definitions";
	string gribDef = ss.str();
	if (putenv(gribDef.c_str())!=0) {
		MyBase::SetErrMsg("putenv failed on GRIB_DEFINITION_PATH");
	}
	//cout << getenv("GRIB_DEFINITION_PATH") << endl;
#endif

    //Install our own message handler.
    //Needed for SGI to avoid dithering:

    FILE *diagfp = OpenLog("VAPOR_DIAG_LOG");
    FILE *errfp = OpenLog("VAPOR_ERR_LOG");

#ifdef Darwin
    if (! getenv("DISPLAY")) setenv("DISPLAY", ":0.0",0);
#endif
#ifdef Q_WS_X11
    if (!getenv("DISPLAY")){
        fprintf(stderr,"Error:  X11 DISPLAY variable is not defined. %s \n",
        "Vapor user interface requires X server to be available.");
        exit(-1);
    }
#endif
//Following needed for running vnc:
#ifdef Linux
    QApplication::setGraphicsSystem("raster");
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

    const char *s = getenv("VAPOR_PYTHONHOME");
    string phome = s ? s : "";
    string pythonversion ("python");
    pythonversion += PYTHONVERSION;
    if (! phome.empty()) {
        string msg("The VAPOR_PYTHONHOME variable is specified as: \n");
        msg += phome;
        msg += "\n";
        msg += "The VAPOR ";
        msg += pythonversion;
        msg += " environment will operate in this path\n";
        msg += " This path must be the location of a Python ";
        msg += pythonversion;
        msg += " installation\n";
        msg += "Unset the VAPOR_PYTHONHOME environment variable to revert to the installed ";
        msg += "VAPOR " + pythonversion + " environment.";
        QMessageBox::warning(0,"VAPOR_PYTHONHOME warning", msg.c_str());
    } 
                               

    app = &a;
    a.setPalette(QPalette(QColor(233,236,216), QColor(233,236,216)));
    
    //Depending on the platform, we may want nondefault fonts!
    
    //The pointsize of 10 works ok on linux and irix, not windows
    //The weight of 55 is slightly heavier than normal.
    //default font is OK 
    QFont myFont = a.font();
    myFont.setPointSize(10);
    a.setFont(myFont);

    QString fileName("");
    if (argc > 1) fileName = argv[1];
    MainForm* mw = new MainForm(fileName,app);
    
    //QString version_file_name = "http://vis.ucar.edu/~milesrl/vapor-version";
    QString version_file_name = "http://www.vapor.ucar.edu/sites/default/files/vapor-version";
    //QString banner_file_name = "testbanner.png";
    VersionChecker* vc = new VersionChecker();
    vc->request(version_file_name);
    
    mw->setWindowTitle("VAPOR User Interface");
    mw->show();
    std::string banner_file_name = "vapor_banner.png";
    BannerGUI* banner;
    banner = new BannerGUI(banner_file_name, 3000);
    a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
    int estatus = a.exec();

    if (diagfp) fclose(diagfp);
    if (errfp) fclose(errfp);
    exit(estatus);
}
