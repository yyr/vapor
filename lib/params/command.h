//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		command.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//
#ifndef COMMAND_H
#define COMMAND_H

#define MAX_HISTORY 1000

#include "params.h"
#include "vapor/ParamNode.h"

namespace VAPoR{
class Params;

class PARAMS_API Command {
public:
	Command(Params*, const char* descr, const string& tg, int, int);
	virtual ~Command( ) {
		delete prevRoot;
		if (nextRoot) delete nextRoot;
		if (prevRoot) delete prevRoot;
		tag.clear();
	}
	virtual Params* reDo(string& ptag, int* inst, int* viznum);
	virtual Params* unDo(string& ptag, int* inst, int* viznum);
	void setDescription(const char* str){
		description = str;
	}
	string& getDescription(){return description;}
	static Command* captureStart(Params* prevParams,  const char* desc, string typeTag, int prevInst = -1, int winnum = -1){
		Command* cmd = new Command(prevParams, desc, typeTag, prevInst, winnum);
		return cmd;
	}
	static void captureEnd(Command* pCom, Params *nextParams){
		pCom->nextRoot = nextParams->GetRootNode()->deepCopy();
	}
	static int AddToHistory(Command* cmd);
	static Params* BackupQueue(string& ptag, int* inst, int* viznum);
	static Params* AdvanceQueue(string& ptag, int* inst, int* viznum);
	static void resetCommandQueue();
	static Command* CurrentUndoCommand() {return commandQueue[(currentQueuePos)%MAX_HISTORY];}
	static Command* CurrentRedoCommand() {return commandQueue[(currentQueuePos+1)%MAX_HISTORY];}
	static Command* CurrentCommand(int offset) {
		int posn = currentQueuePos - offset;
		while (posn < 0) posn += MAX_HISTORY;
		return commandQueue[posn%MAX_HISTORY];
	}
	//Anytime someone wants to block recording, they call blockRecording,
	//then unblock when they are done.  No recording happens until everyone has stopped
	//blocking it.
	//
	
	static void blockRecording() {recordingCount++;}
	static void unblockRecording() {recordingCount--;}
	static bool isRecording() {return (recordingCount == 0);}
protected:
	string description;
	ParamNode* prevRoot;
	ParamNode* nextRoot;
	string tag;
	int instance;
	int winnum;

	static Command* commandQueue[MAX_HISTORY];
	static int currentQueuePos;
	static int endQueuePos;
	static int startQueuePos;
	static int recordingCount;
};
	
};
#endif
 
