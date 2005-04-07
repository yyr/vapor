//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		messagereporter.cpp
//
//	Author:		Alan Norton (alan@ucar.edu)
//			National Center for Atmospheric Research
//			1850 Table Mesa Drive
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2005
//
//	Description:	Implements the MessageReporter class 
//
#include "messagereporter.h"
#include "vapor/DataMgr.h"
#include "mainform.h"
#include <qstring.h>
#include <qmessagebox.h>
using namespace VAPoR;

MessageReporter* MessageReporter::theReporter = 0;

MessageReporter::MessageReporter() {
	logFile = 0;
	logFileName = 0;
#ifdef WIN32
	reset("C:\\temp\\vaporlog.txt");
#else
	reset("/tmp/vaporlog.txt");
#endif
	//setup defaults:
	maxLogMsg[Fatal] = 1;
	maxPopup[Fatal] = 1;
	maxLogMsg[Error] = 10;
	maxPopup[Error] = 10;
	maxLogMsg[Warning] = 10;
	maxPopup[Warning] = 10;
	maxLogMsg[Info] = 0;
	maxPopup[Info] = 0;
	
}
MessageReporter::~MessageReporter(){
	if (logFile) fclose(logFile);
	if (logFileName) delete logFileName;
	//Destructor for messageCount should delete its contents?
	//messageCount.clear();
}
void MessageReporter::
reset(const char* newLogFileName){
	//Save name, reopen the log file
	if (logFile) fclose(logFile);
	if (logFileName) delete logFileName;
	logFileName = new char[strlen(newLogFileName)+1];
	strcpy(logFileName,newLogFileName);
	logFile = fopen(logFileName, "w");
	//reset message counts
	messageCount.clear();
	if (!logFile) doPopup(Error, "VAPoR session log file cannot be opened");
}


void MessageReporter::
postMsg(messagePriority t, const char* message){
	assert(t>=0 && t <=3);
	int count = messageCount[message];
	if (count < maxLogMsg[t]){
		writeLog(t, message);
	}
	if (count < maxPopup[t]){
		doPopup(t, message);
	}
	messageCount[message] = count+1;

}
void MessageReporter::
writeLog(messagePriority t, const char* message){
	if (!logFile) return;
	switch (t){
		case Fatal :
			fprintf(logFile, "%s : %s \n","Fatal", message);
			break;
		case Error :
			fprintf(logFile, "%s : %s \n","Error", message);
			break;
		case Warning :
			fprintf(logFile, "%s : %s \n","Warning", message);
			break;
		case Info :
			fprintf(logFile, "%s : %s \n","Info", message);
			break;
		default:
			assert(0);
	}
	fflush(logFile);
	return;
}
void MessageReporter::
doPopup(messagePriority t, const char* message){
	switch (t){
		case Fatal :
			QMessageBox::critical(MainForm::getInstance(), "VAPoR Fatal Error", message, 
				QMessageBox::Ok, QMessageBox::NoButton );
			break;
		case Error :
			QMessageBox::critical(MainForm::getInstance(), "VAPoR Error", message, 
				QMessageBox::Ok, QMessageBox::NoButton );
			break;
		case Warning :
			QMessageBox::warning(MainForm::getInstance(), "VAPoR Warning", message, 
				QMessageBox::Ok, QMessageBox::NoButton );
			break;
		case Info :
			QMessageBox::information(MainForm::getInstance(), "VAPoR Information", message, 
				QMessageBox::Ok, QMessageBox::NoButton );
			break;
		default:
			assert(0);
	}
	return;
}

