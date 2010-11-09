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

#define ALLOC_SIZE 4096
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
#include <vapor/DataMgr.h>
#include "mainform.h"
#include "tabmanager.h"
#include "session.h"
#include "glwindow.h"
#include <qstring.h>
#include <qmessagebox.h>
#include <qmutex.h>
#include <qapplication.h>

#include <QEvent>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif

using namespace VAPoR;

char* MessageReporter::messageString = 0;
int MessageReporter::messageSize = 0;
MessageReporter* MessageReporter::theReporter = 0;
std::vector<std::string> MessageReporter::savedErrMsgs;
std::vector<int> MessageReporter::savedErrCodes;
QMutex MessageReporter::messageListMutex;

MessageReporter::MessageReporter() {
	MyBase::SetErrMsgCB(addErrorMessageCBFcn);
	logFile = 0;
	if (!Session::isInitialized()) return;
	reset(Session::getInstance()->getLogfileName().c_str());
	setDefaultPrefs();
	
}
void MessageReporter::setDefaultPrefs(){
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
	
	if (messageString) delete messageString;
	//Destructor for messageCount should delete its contents?
	//messageCount.clear();
}
void MessageReporter::
reset(const char* newLogFileName){
	//Save name, reopen the log file
	if (logFile && logFile != stderr ) fclose(logFile);
	//Special case for stderr log file 
	bool debug_set = false;
#ifndef WIN32 //On windows, continue to write to logfile, not console with VAPOR_DEBUG
	debug_set = getenv("VAPOR_DEBUG");
#endif
	if((strlen(newLogFileName) > 0) && (strcmp(newLogFileName, "stderr") != 0) &&
			!debug_set ){
		logFile = fopen(newLogFileName, "w");
		int retcode = 0;
		//turn off buffering
		if (logFile) retcode = setvbuf(logFile, 0, _IONBF , 2);
		if (!logFile || retcode) {
			std::string s("VAPoR session log file cannot be opened:\n");
			s += newLogFileName;
			s += "\n Choose another log file from user preferences";
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
	
	char* message = convertText(format, args);
	va_end(args);
	postMessage(Fatal, message);
	
	
}
void MessageReporter::errorMsg(const char* format, ...){
	getInstance();
	va_list args;
	va_start(args, format);
	
	char* message = convertText(format, args);
	va_end(args);
	postMessage(Error, message);
	
	
}
void MessageReporter::warningMsg(const char* format, ...){
	getInstance();
	va_list args;
	va_start(args, format);
	char* message = convertText(format, args);
	va_end(args);
	postMessage(Warning, message);
	
}
void MessageReporter::infoMsg(const char* format, ...){
	if (!Session::isInitialized()) return;
	getInstance();
	va_list args;
	va_start(args, format);
	
	char* message = convertText(format, args);
	va_end(args);
	postMessage(Info, message);
	
}
	
void MessageReporter::
postMsg(messagePriority t, const char* message){

	assert(t>=0 && t <=3);
	
	int count = messageCount[message];
	if ((count < maxLogMsg[t]) || getenv("VAPOR_DEBUG")){
		writeLog(t, message);
	}
	if (t == Fatal || count < maxPopup[t]-1){
		
		messageCount[message] = count + 1;
		doPopup(t, message);	
	}
	else if (count == maxPopup[t]-1){
		//Duplicate the message, because another invocation of this method, 
		//triggered by doLastPopup()
		//will change the value of message during the call.
		char* msg = strdup(message);
		messageCount[message] = count + 1;
		//Users can reset the message count if they don't want to silence it:
		if (doLastPopup(t, message)){
			messageCount[msg] = 0;
		}
	} 
	
	
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
	QString title;
	QMessageBox::Icon msgIcon = QMessageBox::Information;
	static int count = 0;  //Use to slightly jitter the popups
	
	switch (t) {
		case Fatal :
			title = "VAPOR Fatal Error";
			msgIcon = QMessageBox::Critical;
			break;
		case Error :
			title = "VAPOR Error";
			msgIcon = QMessageBox::Critical;
			break;
		case Warning :
			title = "VAPOR Warning";
			msgIcon = QMessageBox::Warning;
			break;
		case Info : 
			title = "VAPOR Information";
			msgIcon = QMessageBox::Information;
			break;
		default: 
			assert(0);
	}
	QMessageBox* msgBox = new QMessageBox(title,message, msgIcon,
		QMessageBox::Ok,Qt::NoButton,Qt::NoButton,
		MainForm::getInstance()->getTabManager());
	
	msgBox->adjustSize();
	QPoint tabPsn = MainForm::getInstance()->getTabManager()->mapToGlobal(QPoint(0,0));
	tabPsn.setY(tabPsn.y() + (5*count++)%20);
	MainForm::getInstance()->getTabManager()->scrollFrontToTop();
	//pnt is the absolute position of the tab manager?
	msgBox->move(tabPsn);
	msgBox->exec();
	delete msgBox;
	return;
}
//This is the last popup before messages are silenced; if users ask for it,
//they can continue to get messages
bool MessageReporter::
doLastPopup(messagePriority t, const char* message){
	QString longMessage = QString(message)+"\nThis message will not be repeated \n unless you click Continue."
		+"\nAll messages may be resumed from\nthe Edit User Preferences dialog";
	int doContinue = 0;
	// Now do as in doPopup, but include an OK and Continue button
	QString title;
	QMessageBox::Icon msgIcon = QMessageBox::Information;
	
	switch (t) {
		case Fatal :
			title = "VAPOR Fatal Error";
			msgIcon = QMessageBox::Critical;
			break;
		case Error :
			title = "VAPOR Error";
			msgIcon = QMessageBox::Critical;
			break;
		case Warning :
			title = "VAPOR Warning";
			msgIcon = QMessageBox::Warning;
			break;
		case Info : 
			title = "VAPOR Information";
			msgIcon = QMessageBox::Information;
			break;
		default: 
			assert(0);
	}
	QMessageBox* msgBox = new QMessageBox(title,longMessage, msgIcon,
		QMessageBox::Ok,QMessageBox::Cancel,Qt::NoButton,
		MainForm::getInstance()->getTabManager());
	msgBox->setButtonText(2,"Continue");
	
	msgBox->adjustSize();
	QPoint tabPsn = MainForm::getInstance()->getTabManager()->mapToGlobal(QPoint(0,0));
	
	MainForm::getInstance()->getTabManager()->scrollFrontToTop();
	//pnt is the absolute position of the tab manager
	msgBox->move(tabPsn);
	msgBox->exec();
	if (msgBox->result() != QMessageBox::Ok) doContinue = 1;
	delete msgBox;
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

//This is 
//called by customEvent as it unloads the saved messages
void MessageReporter::postMessages(const char* msg, int err_code){
	QString strng("Code: ");
	strng += QString::number(err_code,16);
	strng += "\n Message: ";
	strng += msg;
	
	if (err_code & 0x2000){
		MessageReporter::warningMsg((const char*)strng.toAscii());
	} else if (err_code & 0x4000){
		MessageReporter::errorMsg((const char*)strng.toAscii());
	} else if (err_code & 0x8000){
		MessageReporter::fatalMsg((const char*)strng.toAscii());
	} else {
		MessageReporter::errorMsg((const char*)(QString("Unclassified error: ")+strng).toAscii());
	}
	MainForm::getInstance()->getTabManager()->getFrontEventRouter()->updateUrgentTabState();
}

void MessageReporter::addErrorMessageCBFcn(const char* message,int errcode){
	//Turn off error code just on non-VDF generated errors:
	if(MyBase::GetErrCode() > 0xfff) MyBase::SetErrCode(0);
	if (!getMessageLock()) assert(0);
	savedErrMsgs.push_back(std::string(message));
	savedErrCodes.push_back(errcode);
	if (savedErrMsgs.size() == 1){ 
		QEvent* postMessageEvent = new QEvent((QEvent::Type)65432);
		QApplication::postEvent(MessageReporter::getInstance(), postMessageEvent);
	}
	releaseMessageLock();
	MainForm::getInstance()->getTabManager()->getFrontEventRouter()->updateUrgentTabState();
	
}

void MessageReporter::customEvent(QEvent* e)
{
	if (e->type() != 65432) {assert(0); return;}
	if(!getMessageLock()) assert(0);
    if(savedErrMsgs.size() == 0) {
		releaseMessageLock();
		return;
	}

	//Copy all the saved messages.  We can't post them
	//until the lock is released, to avoid deadlock.
	std::vector<std::string> tempMsgs(savedErrMsgs);
	std::vector<int> tempCodes(savedErrCodes);
	
	savedErrMsgs.clear();
	savedErrCodes.clear();

	releaseMessageLock();
	for (int i= 0; i< tempMsgs.size(); i++){
		postMessages(tempMsgs[i].c_str(),tempCodes[i]);
	}
	
}
bool MessageReporter::getMessageLock(){//Lock on adding to message list
	
	for (int i = 0; i< 10; i++){
		if(messageListMutex.tryLock()) return true;
#ifdef WIN32
		Sleep(100);
#else
		sleep(1);
#endif
	}
	
	return false;
}
