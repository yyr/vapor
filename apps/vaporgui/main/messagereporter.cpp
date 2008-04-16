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
	


	reset(Session::getInstance()->getLogfileName().c_str());
	//setup defaults:
	maxLogMsg[Fatal] = 1;
	maxPopup[Fatal] = 1;
	maxLogMsg[Error] = 20;
	maxPopup[Error] = 3;
	maxLogMsg[Warning] = 10;
	maxPopup[Warning] = 3;
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
	if (logFile && logFile != stderr ) fclose(logFile);
	//Special case for stderr log file
	if(strcmp(newLogFileName, "stderr") != 0) {
		logFile = fopen(newLogFileName, "w");
		if (!logFile) {
			std::string s("VAPoR session log file cannot be opened:\n");
			s += newLogFileName;
			doPopup(Error,s.c_str());
			return;
		}
	} else {
		logFile = stderr;
	}
	//reset message counts
	messageCount.clear();
	
	Session::getInstance()->setLogfileName(newLogFileName);
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
	bool gotIt = messageMutex->tryLock();
	if (!gotIt) return;
	char* message = convertText(format, args);
	va_end(args);
	postMessage(Warning, message);
	messageMutex->unlock();
	
}
void MessageReporter::infoMsg(const char* format, ...){
	getInstance();
	va_list args;
	va_start(args, format);
	bool gotIt = messageMutex->tryLock();
	if (!gotIt) return;
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
	if (count == maxPopup[t] -1){
		//Users can reset the message count if they don't want to silence it:
		if (doLastPopup(t, message)){
			messageCount[message] = 0;
			return;
		}
	} else if (count < maxPopup[t]){
		
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
//This is the last popup before messages are silenced; if users ask for it,
//they can continue to get messages
bool MessageReporter::
doLastPopup(messagePriority t, const char* message){
	QString longMessage = QString(message)+"\nThis message will not be repeated unless you click Continue."
		+"\nAll messages may be resumed from the Edit Session Parameters dialog";
	int doContinue = 1;
	switch (t){
		case Fatal :
			QMessageBox::critical(MainForm::getInstance(), "VAPoR Fatal Error", message, 
				QMessageBox::Ok, QMessageBox::NoButton );
			break;
		case Error :
			doContinue = QMessageBox::critical(MainForm::getInstance(), "VAPoR Error", longMessage, 
				"OK","Continue displaying message");
			break;
		case Warning :
			doContinue = QMessageBox::warning(MainForm::getInstance(), "VAPoR Warning", longMessage, 
				"OK","Continue displaying message");
			break;
		case Info :
			doContinue = QMessageBox::information(MainForm::getInstance(), "VAPoR Information", longMessage, 
				"OK","Continue displaying message");
			break;
		default:
			assert(0);
	}
	return (doContinue==1);
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

