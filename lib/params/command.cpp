//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		command.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements Command class
//
#include "command.h"
#include "params.h"

using namespace VAPoR;

//Statics
Command* Command::commandQueue[MAX_HISTORY] = {MAX_HISTORY*0};
int Command::startQueuePos = 0;
int Command::endQueuePos = 0;
int Command::currentQueuePos = 0;
int Command::recordingCount = 0;

Command::Command(Params* prevParams, const char* descr, const string& pTag, int inst, int wnum){
	prevRoot = prevParams->GetRootNode()->deepCopy();
	description = string(descr);
	tag = pTag;
	instance = inst;
	winnum = wnum;
	nextRoot = 0;
}

Params* Command::unDo(string& ptag, int* inst, int* viznum){
	//Find the Params instance, substitute the previous root param node
	Command* cmd = CurrentUndoCommand();
	if (!cmd) return 0;
	Params* p = Params::GetParamsInstance(cmd->tag, cmd->winnum, cmd->instance);
	ptag = cmd->tag;
	if(inst) *inst = cmd->instance;
	if(viznum) *viznum = cmd->winnum;
	p->SetRootParamNode(cmd->prevRoot->deepCopy());
	return p;
}
Params* Command::reDo(string& ptag, int* inst, int* viznum){
	//Find the Params instance, substitute the next root param node
	Command* cmd = CurrentRedoCommand();
	if (!cmd) return 0;

	Params* p = Params::GetParamsInstance(cmd->tag, cmd->winnum, cmd->instance);
	ptag = cmd->tag;
	if(inst) *inst = cmd->instance;
	if(viznum) *viznum = cmd->winnum;
	p->SetRootParamNode(cmd->nextRoot->deepCopy());
	return p;
	
}

int Command::AddToHistory(Command* cmd, bool ignoreBlocking){
		if (!ignoreBlocking && recordingCount>0) {
		delete cmd;
		return -1;
	}
	assert(recordingCount >= 0);  //better not be negative!
	//Check if we must invalidate the remainder of queue:
	//
	if (currentQueuePos > endQueuePos){
		for (int i = currentQueuePos+1; i<= endQueuePos; i++){
			Command* c =  commandQueue[i%MAX_HISTORY];
			if (c != 0) delete c;
			commandQueue[i%MAX_HISTORY] = 0;
		}
		endQueuePos = currentQueuePos;
	}

	//Check if we are at queue end:
	//
	if ((currentQueuePos - startQueuePos) == MAX_HISTORY +1 ){
		//release entry we will be overwriting:
		//
		delete commandQueue[startQueuePos%MAX_HISTORY];
		commandQueue[startQueuePos%MAX_HISTORY] = 0;
		startQueuePos++;
	}
	//Insert the command at the appropriate place:
	
	currentQueuePos++;
	endQueuePos = currentQueuePos;
	commandQueue[endQueuePos%MAX_HISTORY] = cmd;
	return 0;
}
Params* Command::BackupQueue(string& ptag, int* inst, int* viznum){
	//Make sure we can do it!
	//
	if(currentQueuePos <= startQueuePos) return 0;
	Params* p = commandQueue[currentQueuePos%MAX_HISTORY]->unDo(ptag, inst, viznum);
	currentQueuePos--;
	return p;
}
Params* Command::AdvanceQueue(string& ptag, int* inst, int* viznum){
	//Make sure we can do it:
	//
	if(currentQueuePos >= endQueuePos) return 0;
	//perform the next command
	//
	Params* p = commandQueue[(++currentQueuePos)%MAX_HISTORY]->reDo(ptag, inst, viznum);
	return p;
}
void Command::resetCommandQueue(){
	for (int i = 0; i<MAX_HISTORY; i++){
		if (commandQueue[i]) {
			delete commandQueue[i];
			commandQueue[i] = 0;
		}
	}
	currentQueuePos = 0;
	startQueuePos = 0;
	endQueuePos = 0;
}

