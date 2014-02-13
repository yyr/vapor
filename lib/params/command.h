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
//! \class Command
//!
//!	Provides support for maintaining a queue of recently issued commands,
//! performing UnDo, ReDo, etc.
//! The parent Command class supports a queue of Params changes; each entry has a
//! clone of the previous and next Params instance associated with a change.
//! subclasses could support additional state changes, if needed.
//
class PARAMS_API Command {
public:
	Command(Params*, const char* descr);
	virtual ~Command( ) {
		delete prevRoot;
		if (nextRoot) delete nextRoot;
		if (prevRoot) delete prevRoot;
		tag.clear();
	}
	//! Static method moves the command queue forward, returns the next Params state
	//! \param [out] string ptag the tag associated with the resulting (next) Params instance
	//! \param [out] int inst The instance index of the next Params, if it is a RenderParams
	//! \param [out] int viznum The visualizer index associated with the next Params, or -1 if it's global.
	//! \return Params* pointer to the next Params instance
	//!
	static Params* reDo(string& ptag, int* inst, int* viznum);
	//! Static method moves the command queue back, returns the previous Params state
	//! \param [out] string ptag the tag associated with the resulting (previous) Params instance
	//! \param [out] int inst The instance index of the previous Params, if it is a RenderParams
	//! \param [out] int viznum The visualizer index associated with the previous Params, or -1 if it's global.
	//! \return Params* pointer to the previous Params instance
	//!
	static Params* unDo(string& ptag, int* inst, int* viznum);
	//! Specify the description of a command
	//! \param [in] char* description
	void setDescription(const char* str){
		description = str;
	}
	//! Get the description associated with this command instance
	//! \return string Description text identifying the state change of this command.
	string& getDescription(){return description;}

	//! Static method used to capture the previous Params state (before the state change)
	//! \param [in] prevParams points to previous Params instance
	//! \param [in] desc Textual description of the change
	//! \param [in] typeTag The XML tag associated with the type of Params being changed
	//! \param [in] winnum The visualizer number (or -1 if global)
	//! \param [in] prevInst The instance index, if it's a RenderParams change
	//! \return cmd A new command initialized with prevParams.
	//! \sa captureEnd
	static Command* captureStart(Params* prevParams,  const char* desc){
		Command* cmd = new Command(prevParams, desc);
		return cmd;
	}
	//! Static method used to capture the next Params state (after the state change)
	//! and then to insert the command into the command queue
	//! \param [in] Command A Command instance, previously initialized by captureStart
	//! \param [in] nextParams points to next Params instance
	//! \sa captureStart
	static void captureEnd(Command* pCom, Params *nextParams){
		pCom->nextRoot = nextParams->GetRootNode()->deepCopy();
		AddToHistory(pCom);
	}

	//! Static method used to insert a Command instance into the Command queue
	//! \param [in] Command Command instance
	//! \param [in] ignoreBlocking If true, will insert into the queue even when blocking is enabled.
	//! \return integer zero indicates success.
	//! \sa captureStart, captureEnd
	static int AddToHistory(Command* cmd, bool ignoreBlocking = false);

	//! Static method to go back one position (i.e. undo) in the queue.
	//! Returns the params instance that now is current
	//! \param [out] ptag Tag associated with Params in new current Command
	//! \param [out] inst int* pointer to instance number of Params instance if RenderParams
	//! \param [out] viznum int* pointer to visualizer index (or -1 if this is global)
	//! \return Params instance that is newly current, null if nothing left to backup.
	static Params* BackupQueue(string& ptag, int* inst, int* viznum);

	//! Static method to go forward one position (i.e. redo) in the queue.
	//! Returns the params instance that now is current
	//! \param [out] ptag Tag associated with Params in new current Command
	//! \param [out] inst int* pointer to instance number of Params instance if RenderParams
	//! \param [out] viznum int* pointer to visualizer index (or -1 if this is global)
	//! \return Params instance that is newly current, null if we are at end of queue
	static Params* AdvanceQueue(string& ptag, int* inst, int* viznum);

	//! Static method to put command queue in initial state.
	static void resetCommandQueue();
	
	//Anytime someone wants to stop inserting into the commandQueue, they call blockRecording,
	//then unblock when they are done.  No recording happens until all blockers has stopped
	//blocking it.
	//
	//! static method to stop inserting commands into the queue.
	//! subsequent commands do not get inserted until unblockRecording() is called
	//sa unblockRecording, isRecording
	static void blockRecording() {recordingCount++;}
	//! static method to resume inserting commands into the queue.
	//! Should be called after blockRecording
	//sa blockRecording, isRecording
	static void unblockRecording() {recordingCount--;}
	//! static method to tell if commands are being inserted in the queue
	//sa blockRecording, unblockRecording
	// \return bool indicates whether or not recording is enabled.
	static bool isRecording() {return (recordingCount == 0);}
	//! static method retrieves a command from the queue,
	//! with index relative to the last executed command.
	//! \param [in] offset position relative to last command, positive offsets were executed earlier.
	//! \return Command* pointer to specified command, null if invalid.
	static Command* CurrentCommand(int offset) {
		int posn = currentQueuePos - offset;
		while (posn < 0) posn += MAX_HISTORY;
		return commandQueue[posn%MAX_HISTORY];
	}
protected:
	static Command* CurrentUndoCommand() {return CurrentCommand(0);}
	static Command* CurrentRedoCommand() {return CurrentCommand(-1);}
	
	string description;
	ParamNode* prevRoot;
	ParamNode* nextRoot;
	string tag;
	int instance;
	int winnum;
	//Statics indicate state of queue:
	static Command* commandQueue[MAX_HISTORY];
	static int currentQueuePos;
	static int endQueuePos;
	static int startQueuePos;
	static int recordingCount;
};
	
};
#endif
 
