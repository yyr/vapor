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
using namespace VAPoR;
//Constructor:  called when a new command is issued.
//
PanelCommand::PanelCommand(Params* prevParams, Session* ses, const char* descr){
	//Make a copy of previous panel:
	previousPanel = prevParams->deepCopy();
	nextPanel = 0;
	currentSession = ses;
	description = *(new QString(descr));
}
void PanelCommand::
setNext(Params* next){
	nextPanel = next->deepCopy();

}
void PanelCommand::unDo(){
	currentSession->blockRecording();
	previousPanel->makeCurrent(nextPanel, false);
	currentSession->unblockRecording();
	
}
void PanelCommand::reDo(){
	currentSession->blockRecording();
	nextPanel->makeCurrent(previousPanel, false);
	currentSession->unblockRecording();
	
}
PanelCommand::~PanelCommand(){
	if (previousPanel) delete previousPanel;
	if (nextPanel) delete nextPanel;
	delete description;
}
PanelCommand* PanelCommand::
captureStart(Params* p, Session* ses,  char* description){
	if (!ses->isRecording()) return 0;
	PanelCommand* cmd = new PanelCommand(p, ses, description);
	return cmd;
}
void PanelCommand::
captureEnd(PanelCommand* pCom, Params *p) {
	if (!pCom) return;
	if (!pCom->currentSession->isRecording()) return;
	pCom->setNext(p);
	pCom->currentSession->addToHistory(pCom);
}

