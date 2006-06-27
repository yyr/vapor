//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		panelcommand.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the PanelCommand class.  
//	 Implementation of PanelCommand class
//	 Supports redo/undo of actions in param classes
//
#include "panelcommand.h"
#include "params.h"
#include "session.h"
#include "vizwinmgr.h"
#include "eventrouter.h"
using namespace VAPoR;
//Constructor:  called when a new command is issued.
//
PanelCommand::PanelCommand(Params* prevParams, const char* descr){
	//Make a copy of previous panel:
	previousPanel = prevParams->deepCopy();
	nextPanel = 0;
	description = *(new QString(descr));
}
void PanelCommand::
setNext(Params* next){
	nextPanel = next->deepCopy();

}
void PanelCommand::unDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr::getEventRouter(previousPanel->getParamType())->makeCurrent(nextPanel,previousPanel, false);
	Session::getInstance()->unblockRecording();
	
}
void PanelCommand::reDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr::getEventRouter(previousPanel->getParamType())->makeCurrent(previousPanel,nextPanel, false);
	Session::getInstance()->unblockRecording();
	
}
PanelCommand::~PanelCommand(){
	if (previousPanel) delete previousPanel;
	if (nextPanel) delete nextPanel;
	delete description;
}
PanelCommand* PanelCommand::
captureStart(Params* p,   char* description){
	if (!Session::getInstance()->isRecording()) return 0;
	PanelCommand* cmd = new PanelCommand(p,   description);
	return cmd;
}
void PanelCommand::
captureEnd(PanelCommand* pCom, Params *p) {
	if (!pCom) return;
	if (!Session::getInstance()->isRecording()) return;
	pCom->setNext(p);
	Session::getInstance()->addToHistory(pCom);
}

