//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		panelcommand.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Defines the PanelCommand class.  
//	 Implementation of PanelCommand class
//	 Supports redo/undo of actions in param classes.
//	 Each supported change in a panel results in a cloning of that panel.
//   Minor changes (e.g. intermediate mouse moves) are aggregated until
//	 event such as releasing the mouse occurs.
//
#ifndef PANELCOMMAND_H
#define PANELCOMMAND_H
#include "command.h"

#include "assert.h"
namespace VAPoR {
class Params;
class Session;
enum instanceType {
	changeInstance,
	newInstance,
	copyInstance,
	deleteInstance
}; 

class PanelCommand : public Command {
public:
	//Constructor is called when a command is executed
	PanelCommand(Params* prev, const char* descr, int prevInst = -1);
	void setNext(Params* next);
	virtual ~PanelCommand();
	virtual void reDo();
	virtual void unDo();
	//Default instance parameter is for render params to specify an
	//instance other than the current one
	static PanelCommand* captureStart(Params* p,  const char* description, int prevInst = -1);
	static void captureEnd(PanelCommand* pCom, Params *p);

protected:
	
	//Need sufficient state to be able to undo or redo
	Params* previousPanel;
	Params* nextPanel;
	int previousInstance;
	
	
};
//Instanced panel commands support delete, new, copy, and change instance.
//  Copy needs:  previousPanel = panel being copied
//				previousInstance = instance that was current prior to copy.
//  New needs:  previousInstance = instance that was current prior to copy.
//				previousPanel is the params that were previously current
//				(only used to identify the params type in this command)
//  Delete needs:  previousInstance = instance being deleted,
//					previousPanel is panel that is deleted.
//  Change needs:  previousInstance, and newInstance (ints)
//				previousPanel is the one that was previousInstance.
//   These are atomic, don't need to construct in two methods.
//  to undo "copy",  
//     must delete the last (i.e. the copy) instance (in the target visualizer).
//  same to undo "new"
//  to undo "delete", need to know previousCurrentIndex, and the deleted params.
//	   Must reinsert the params in the previously current position,
//	   Must enable the params if they were enabled, and set up rendering.
//  to redo "copy", need to have params that are being copied, since they
//		can be from another visualizer.  The enablement is always
//		off.
//  to redo "new", don't need anything.
//	to redo "delete", don't need anything, except the instance no. that was deleted.
//Note that copy and new don't enable, don't change current instance
//Delete makes the instance before the deleted one current, or the first one (if the deletion was first)

class InstancedPanelCommand : public PanelCommand {
public:
	InstancedPanelCommand(Params* prev, const char* descr, int prevInst, instanceType myType, int nextIndex = -1);
	virtual void reDo();
	virtual void unDo();
	static void capture(Params* prev, const char* descr, int prevInst, instanceType myType, int nextInst = -1);
protected:
	int nextIndex; //Change uses this for next instance; copy-to uses for next viznum.
	instanceType instancedCommandType;

};
};
#endif //PANELCOMMAND_H
