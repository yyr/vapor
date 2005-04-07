//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		messagereporting.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2004
//
//	Description:	Defines the MessageReporting class.  This is used for
//	posting various messages that can occur during the operation of the Vapor GUI
//
#ifndef MESSAGEREPORTER_H
#define MESSAGEREPORTER_H


#include <string>
#include <map>

namespace VAPoR{

class MessageReporter {

public:
	MessageReporter();
	~MessageReporter();
	
	static MessageReporter* getInstance(){
		if (!theReporter){
			theReporter = new MessageReporter();
		}
		return theReporter;
	}
	
	//Enum describes various message priorities
	enum messagePriority {
		Fatal = 3,
		Error = 2,
		Warning = 1,
		Info = 0
	};
	//Following is usual way in which classes can post messages.
	//If the current message has not been posted more than the max times,
	//based on its type,
	//it is posted again (to log and/or popup)
	static void postMessage(messagePriority t, const char* message){
		getInstance()->postMsg(t,message);
	}
	static void fatalMsg(const char* message) { postMessage(Fatal, message);}
	static void errorMsg(const char* message) { postMessage(Error, message);}
	static void warningMsg(const char* message) { postMessage(Warning, message);}
	static void infoMsg(const char* message) { postMessage(Info, message);}

	void setMaxLog(messagePriority mP, int num)
		{maxLogMsg[mP] = num;}
	void setMaxPopup(messagePriority mP, int num)
		{maxPopup[mP] = num;}
	int getMaxLog(messagePriority mP) {return maxLogMsg[mP];}
	int getMaxPopup(messagePriority mP) {return maxPopup[mP];}
	const char* getLogFileName() { return logFileName;}
	//Whenever there's a change in settings, reset all error counts to 0
	//Reopen the Log file (it is always used for fatal messages)
	void reset(const char* newLogFileName);

protected:
	static MessageReporter* theReporter;
	void postMsg(messagePriority t, const char* message);
	void writeLog(messagePriority t, const char* message);
	void doPopup(messagePriority t, const char* message);
	std::map<std::string, int> messageCount;
	//Keep track of current settings
	char* logFileName;
	FILE* logFile;
	//Integers to keep result of posting various priority messages:
	//FATAL cannot be changed, always terminate
	//Other priorities post the first "N" to either log or popup
	//If N is 0, no posting
	int maxLogMsg[4], maxPopup[4];

};
};
#endif  //MESSAGEREPORTING_H
