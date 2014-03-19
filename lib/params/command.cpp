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
#include "arrowparams.h"

using namespace VAPoR;

//Statics
Command* Command::commandQueue[MAX_HISTORY] = {MAX_HISTORY*0};
int Command::startQueuePos = 0;
int Command::endQueuePos = 0;
int Command::currentQueuePos = 0;
int Command::recordingCount = 1;  //Start with command queueing blocked.

Command::Command(Params* prevParams, const char* descr){
	prevRoot = prevParams->GetRootNode()->deepCopy();
	description = string(descr);
	tag = prevParams->GetName();
	instance = prevParams->GetInstanceIndex();
	winnum = prevParams->GetVizNum();
	nextRoot = 0;
}

Params* Command::unDo(){
	//Find the Params instance, substitute the previous root param node
	Command* cmd = CurrentUndoCommand();
	if (!cmd) return 0;
	Params* p = Params::GetParamsInstance(cmd->tag, cmd->winnum, cmd->instance);
	ArrowParams* ap = dynamic_cast<ArrowParams*>(p);
	ParamNode* r = p->GetRootNode();
	if (r){
		//detach from its parent Params...
		r->SetParamsBase(0);
		delete r;
	}
	p->SetRootParamNode(cmd->prevRoot->deepCopy());
	return p;
}
Params* Command::reDo(){
	//Find the Params instance, substitute the next root param node
	Command* cmd = CurrentRedoCommand();
	if (!cmd) return 0;
	Params* p = Params::GetParamsInstance(cmd->tag, cmd->winnum, cmd->instance);
	ParamNode* r = p->GetRootNode();
	if (r){
		//detach from its parent Params...
		r->SetParamsBase(0);
		delete r;
	}
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
Params* Command::BackupQueue(){
	//Make sure we can do it!
	//
	if(currentQueuePos <= startQueuePos) return 0;
	Command* cmd = CurrentUndoCommand();
	if (!cmd) return 0;
	Params* p = cmd->unDo();
	currentQueuePos--;
	return p;
}
Params* Command::AdvanceQueue(){
	//Make sure we can do it:
	//
	if(currentQueuePos >= endQueuePos) return 0;
	//perform the next command
	//
	Command* cmd = CurrentRedoCommand();
	if (!cmd) return 0;
	Params* p = cmd->reDo();
	currentQueuePos++;
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
	recordingCount = 0;  //start recording.
}

