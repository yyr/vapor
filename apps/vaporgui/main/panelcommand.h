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


class PanelCommand : public Command {
public:
	//Constructor is called when a command is executed
	PanelCommand(Params* prev, const char* descr);
	void setNext(Params* next);
	virtual ~PanelCommand();
	virtual void reDo();
	virtual void unDo();
	static PanelCommand* captureStart(Params* p,  const char* description);
	static void captureEnd(PanelCommand* pCom, Params *p);

protected:
	
	//Need sufficient state to be able to undo or redo
	Params* previousPanel;
	Params* nextPanel;
	
	
};
};

#endif //PANELCOMMAND_H
