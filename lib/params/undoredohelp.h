//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		undoredohelp.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2014
//
//

#ifndef UNDOREDOHELP_H
#define UNDOREDOHELP_H


#include "params.h"

namespace VAPoR{
class Params;
//! \class UndoRedoHelp
//!
//! \brief Pure virtual class provides additional support for performing Undo/Redo
//! when merely changing the XML is not sufficient
//! Whenever the undo or redo requires an extra operation to be completed, a child
//! class of UndoRedoHelp must be registered in the UndoRedoHelpQueue.
//! All entries in the queue are queried to see if one of them applies to a
//! particular undo/redo operation, based on the Params instances that are being redone/undone
//! \author Alan Norton
//! \version 3.0
//! \date    May, 2014
//!

class PARAMS_API UndoRedoHelp {
public:
	UndoRedoHelp() {}
	virtual ~UndoRedoHelp( ) {
	}
	
	//! All subclasses must implement the UndoRedo method
	//! This checks whether the subclass performs Undo or Redo
	//! for the specified change of Params instances.  If
	//! the subclass does not perform the specified Undo/Redo,
	//! then it returns false.  Otherwise it performs any necessary
	//! help functions.
	virtual bool UndoRedo(Params* before, Params* after) = 0;

	//! Static method to put undoredohelp queue in initial and final (empty) state.
	//! Possibly unnecessary?
	static void ResetUndoRedoHelpQueue(){
		for (int i = 0; i< undoRedoHelpQueue.size(); i++){
			delete undoRedoHelpQueue[i];
		}
		undoRedoHelpQueue.clear();
	}

	//! Static method to add a new UndoRedoHelp subclass instance to 
	//! the UndoRedoHelpQueue
	static void AddUndoRedoHelp(UndoRedoHelp* helper){
		undoRedoHelpQueue.push_back(helper);
	}
	
	static vector<UndoRedoHelp*> GetUndoRedoHelpQueue(){
		return undoRedoHelpQueue;
	}
	
	
#ifndef DOXYGEN_SKIP_THIS
protected:
	
	static vector <UndoRedoHelp*> undoRedoHelpQueue;
	
#endif //DOXYGEN_SKIP_THIS
};
//! \class UndoRedoInstanceHelp
//! \brief Subclass of UndoRedoHelp that handles Undo and Redo of
//! Params instance creation and deletion.  Instance creation and deletion events
//! are identified by (1) Either the before or after instance is NULL. and
//! (2) the nonnull Params instance must be RenderParams.
//!
class PARAMS_API UndoRedoInstanceHelp: public UndoRedoHelp {
public:
	UndoRedoInstanceHelp() {}

	bool UndoRedo(Params* before, Params* after);
};
	
};
#endif
 
