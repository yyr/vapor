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

#define ALLOC_SIZE 256
#ifdef WIN32
#define VSNPRINTF _vsnprintf
#include "windows.h"
#include "Winnetwk.h"
#else
#define VSNPRINTF vsnprintf
#include <unistd.h>
#endif
#include <sys/types.h>
#include "messagereporter.h"
#include "vapor/DataMgr.h"
#include "mainform.h"
#include "session.h"
#include <qstring.h>
#include <qmessagebox.h>
#include <qmutex.h>


using namespace VAPoR;

char* MessageReporter::messageString = 0;
int MessageReporter::messageSize = 0;
QMutex* MessageReporter::messageMutex = 0;
MessageReporter* MessageReporter::theReporter = 0;

MessageReporter::MessageReporter() {
	logFile = 0;
	
	messageMutex = new QMutex();
	char buf[50];
#ifdef WIN32
	//Use the user name in the log file name
	char buf2[50];
	//WCHAR buf2[50];
	DWORD size = 50;
	//Don't Use QT to convert from unicode back to ascii
	//WNetGetUserA(0,(LPWSTR)buf2,&size);
	WNetGetUserA(0,(LPSTR)buf2,&size);
	sprintf(buf, "C:/TEMP/vaporlog.%s.txt", buf2);
	//QString qstr((QChar*)buf2, size);
	//sprintf (buf, "C:/TEMP/vaporlog.%s.txt", qstr.latin1());
#else
	uid_t	uid = getuid();
	sprintf (buf, "/tmp/vaporlog.%6.6d.txt", uid);
#endif
	reset(buf);
	//setup defaults:
	maxLogMsg[Fatal] = 1;
	maxPopup[Fatal] = 1;
	maxLogMsg[Error] = 10;
	maxPopup[Error] = 10;
	maxLogMsg[Warning] = 10;
	maxPopup[Warning] = 5;
	maxLogMsg[Info] = 0;
	maxPopup[Info] = 0;
	
}
MessageReporter::~MessageReporter(){
	if (logFile) fclose(logFile);
	
	delete messageMutex;
	if (messageString) delete messageString;
	//Destructor for messageCount should delete its contents?
	//messageCount.clear();
}
void MessageReporter::
reset(const char* newLogFileName){
	//Save name, reopen the log file
	if (logFile) fclose(logFile);
	
	logFile = fopen(newLogFileName, "w");
	//reset message counts
	messageCount.clear();
	if (!logFile) doPopup(Error,"VAPoR session log file cannot be opened");
	else Session::getInstance()->setLogfileName(newLogFileName);
}

void MessageReporter::fatalMsg(const char* format, ...){
	getInstance();
	va_list args;
	va_start(args, format);
	messageMutex->lock();
	char* message = convertText(format, args);
	va_end(args);
	postMessage(Fatal, message);
	messageMutex->unlock();
	
}
void MessageReporter::errorMsg(const char* format, ...){
	getInstance();
	va_list args;
	va_start(args, format);
	messageMutex->lock();
	char* message = convertText(format, args);
	va_end(args);
	postMessage(Error, message);
	messageMutex->unlock();
	
}
void MessageReporter::warningMsg(const char* format, ...){
	getInstance();
	va_list args;
	va_start(args, format);
	messageMutex->lock();
	char* message = convertText(format, args);
	va_end(args);
	postMessage(Warning, message);
	messageMutex->unlock();
	
}
void MessageReporter::infoMsg(const char* format, ...){
	getInstance();
	va_list args;
	va_start(args, format);
	messageMutex->lock();
	char* message = convertText(format, args);
	va_end(args);
	postMessage(Info, message);
	messageMutex->unlock();
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

char* MessageReporter::
convertText(const char* format, va_list args){
	bool done = false;
	if (!messageString) {
		messageString = new char[ALLOC_SIZE];
		messageSize = ALLOC_SIZE;
	}
	while (! done) {
		int rc = VSNPRINTF(messageString, messageSize, format, args);
		if (rc < (messageSize-1)) {
			done = true;
		} else {
			if (messageString) delete [] messageString;
			messageString = new char[messageSize + ALLOC_SIZE];
			assert(messageString != NULL);
			messageSize += ALLOC_SIZE;
		}
	}
	return messageString;
}

