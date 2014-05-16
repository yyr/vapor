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
class UndoRedoHelper;
//! \class Command
//!
//! \brief Provides support for maintaining a queue of recently issued commands,
//! performing UnDo, ReDo, etc.
//! The parent Command class supports a queue of Params changes; each entry has a
//! clone of the previous and next Params instance associated with a change.
//! \author Alan Norton
//! \version 3.0
//! \date    February, 2014
//!
//! The Command class automatically provides the functionality needed for Undo and Redo.
//! Each Command instance contains two ParamNode roots, corresponding to the state of the Params before
//! and after a state change.  Command instances are inserted into the Undo/Redo queue by invoking
//! Command::CaptureStart and then Command::CaptureEnd.  The CaptureStart and CaptureEnd methods are
//! generated by the methods Params::SetValueLong(), Params::SetValueDouble(), Params::SetValueString(),
//! so is it only necessary to directly use Command::CaptureStart() and CaptureEnd() when more than one state change occurs
//! in a single entry of the Undo/Redo queue.
//!

class PARAMS_API Command {
public:
	Command(Params*, const char* descr);
	virtual ~Command( ) {
		if (nextRoot) delete nextRoot;
		if (prevRoot) delete prevRoot;
		tag.clear();
	}
	
	//! Specify the description of a command
	//! \param [in] char* description
	void setDescription(const char* str){
		description = str;
	}
	//! Get the description associated with this command instance
	//! \return string Description text identifying the state change of this command.
	string getDescription(){return description;}

	//! Static method used to capture the previous Params state (before the state change)
	//! \param [in] prevParams points to previous Params instance
	//! \param [in] desc Textual description of the change
	//! \param [in] typeTag The XML tag associated with the type of Params being changed
	//! \param [in] winnum The visualizer number (or -1 if global)
	//! \param [in] prevInst The instance index, if it's a RenderParams change
	//! \return cmd A new command initialized with prevParams.
	//! \sa CaptureEnd
	static Command* CaptureStart(Params* prevParams,  const char* desc){
		if (!isRecording()) return 0;
		Command* cmd = new Command(prevParams, desc);
		blockCapture();
		return cmd;
	}
	//! Static method used to capture the next Params state (after the state change)
	//! and then to insert the command into the command queue
	//! \param [in] Command A Command instance, previously initialized by CaptureStart
	//! \param [in] nextParams points to next Params instance
	//! \sa CaptureStart
	static void CaptureEnd(Command* pCom, Params *nextParams){
		if (!pCom) return;
		if (nextParams){
			pCom->nextRoot = nextParams->GetRootNode()->deepCopy();
			if (!pCom->prevRoot){ //If the first Params was null, get the tag, etc. from the next params:
				pCom->tag = nextParams->GetName();
				pCom->instance = nextParams->GetInstanceIndex();
				pCom->winnum = nextParams->GetVizNum();
			}
		}
		unblockCapture();
		AddToHistory(pCom);
	}

	

	//! Static method to go back one position (i.e. undo) in the queue.
	//! Returns the params instance that now is current
	//! \return Params instance that is newly current, null if nothing left to backup.
	static Params* BackupQueue();

	//! Static method to go forward one position (i.e. redo) in the queue.
	//! Returns the params instance that now is current
	//! \return Params instance that is newly current, null if we are at end of queue
	static Params* AdvanceQueue();

	//! Static method to put command queue in initial state.
	static void resetCommandQueue();
	
	
	//! static method to tell if commands are being inserted in the queue
	//sa blockCapture, unblockCapture
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
	//! Obtain a copy of the next params in the queue
	//! Useful if additional processing is needed during Undo or Redo
	//! Note that the returned Params copy should be deleted after it is used
	//! \retval Params* that was "next" in the command
	Params* CopyNextParams();
	//! Obtain a copy of the previous params in the queue
	//! Useful if additional processing is needed during Undo or Redo
	//! Note that the returned Params copy should be deleted after it is used
	//! \retval Params* that was "previous" in the command
	Params* CopyPreviousParams();
#ifndef DOXYGEN_SKIP_THIS
	//Anytime it is necessary to stop inserting into the commandQueue, call blockCapture,
	//then unblock when they are done.  No recording happens until all blockers has stopped
	//blocking it.   This is not currently regarded as part of the public API.
	//
	//! static method to stop inserting commands into the queue.
	//! subsequent commands do not get inserted until unblockCapture() is called
	//sa unblockCapture, isRecording
	static void blockCapture() {recordingCount++;}
	//! static method to resume inserting commands into the queue.
	//! Should be called after blockCapture
	//sa blockCapture, isRecording
	static void unblockCapture() {recordingCount--; assert(recordingCount>=0);}
protected:
	void applyHelpers(bool isUndo, Params* prev, Params* next);
	//! Static method used to insert a Command instance into the Command queue
	//! \param [in] Command Command instance
	//! \param [in] ignoreBlocking If true, will insert into the queue even when blocking is enabled.
	//! \return integer zero indicates success.
	//! \sa CaptureStart, CaptureEnd
	static int AddToHistory(Command* cmd, bool ignoreBlocking = false);
	static Command* CurrentUndoCommand() {return CurrentCommand(0);}
	static Command* CurrentRedoCommand() {return CurrentCommand(-1);}
	//! Static method moves the command queue forward, returns the next Params state
	//! \return Params* pointer to the next Params instance
	//!
	static Params* reDo();
	//! Static method moves the command queue back, returns the previous Params state
	//! \return Params* pointer to the previous Params instance
	//!
	static Params* unDo();
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
#endif //DOXYGEN_SKIP_THIS
};
	
};
#endif
 
