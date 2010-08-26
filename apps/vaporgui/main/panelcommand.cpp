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
//	Description:	Implements the PanelCommand class and two of its subclasses
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
PanelCommand::PanelCommand(Params* prevParams, const char* descr, int prevInst){
	//Make a copy of previous panel:
	previousPanel = prevParams->deepCopy();
	nextPanel = 0;
	previousInstance = prevInst;
	description = QString(descr);
}
void PanelCommand::
setNext(Params* next){
	if (next)
		nextPanel = next->deepCopy();
}
void PanelCommand::unDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr::getEventRouter(previousPanel->GetParamsBaseTypeId())->makeCurrent(nextPanel,previousPanel, false, previousInstance);
	Session::getInstance()->unblockRecording();
	
}
void PanelCommand::reDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr::getEventRouter(previousPanel->GetParamsBaseTypeId())->makeCurrent(previousPanel,nextPanel, false, previousInstance);
	Session::getInstance()->unblockRecording();
	
}
PanelCommand::~PanelCommand(){
	if (previousPanel) delete previousPanel;
	if (nextPanel) delete nextPanel;
	
}
PanelCommand* PanelCommand::
captureStart(Params* p,   const char* description, int prevInst){
	if (!Session::getInstance()->isRecording()) return 0;
	//If instance is default, get it from vizwinmgr:
	if ((prevInst < 0) && p->isRenderParams()){
		prevInst = VizWinMgr::getInstance()->getActiveInstanceIndex(p->GetParamsBaseTypeId());
	}
	PanelCommand* cmd = new PanelCommand(p, description, prevInst);
	return cmd;
}
void PanelCommand::
captureEnd(PanelCommand* pCom, Params *p) {
	if (!pCom) return;
	if (!Session::getInstance()->isRecording()) return;
	pCom->setNext(p);
	Session::getInstance()->addToHistory(pCom);
}

//ReenablePanelCommand methods:
ReenablePanelCommand::ReenablePanelCommand(Params* prevParams, const char* descr, int prevInst) :
	PanelCommand(prevParams, descr, prevInst){}

void ReenablePanelCommand::unDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr::getEventRouter(previousPanel->GetParamsBaseTypeId())->makeCurrent(nextPanel,previousPanel, false, previousInstance, true);
	Session::getInstance()->unblockRecording();
	
}
void ReenablePanelCommand::reDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr::getEventRouter(previousPanel->GetParamsBaseTypeId())->makeCurrent(previousPanel,nextPanel, false, previousInstance, true);
	Session::getInstance()->unblockRecording();
}

ReenablePanelCommand* ReenablePanelCommand::
captureStart(Params* p,   const char* description, int prevInst){
	if (!Session::getInstance()->isRecording()) return 0;
	assert(p->isRenderParams());
	//If instance is default, get it from vizwinmgr:
	if ((prevInst < 0) && p->isRenderParams()){
		prevInst = VizWinMgr::getInstance()->getActiveInstanceIndex(p->GetParamsBaseTypeId());
	}
	ReenablePanelCommand* cmd = new ReenablePanelCommand(p, description, prevInst);
	return cmd;
}

void ReenablePanelCommand::
captureEnd(ReenablePanelCommand* pCom, Params *p) {
	PanelCommand::captureEnd(pCom, p);
}

InstancedPanelCommand::InstancedPanelCommand(Params* prev, const char* descr, int prevInst, instanceType myType, int nextIndx) :
	PanelCommand(prev,descr,prevInst)
{
	instancedCommandType = myType;
	nextPanel = 0;
	nextIndex = nextIndx;
}
void InstancedPanelCommand::
capture(Params *p, const char* descr, int prevInst, instanceType myType, int nextInst) {
	if (!Session::getInstance()->isRecording()) return;
	InstancedPanelCommand* pCom = new InstancedPanelCommand(p,descr,prevInst,myType,nextInst);
	Session::getInstance()->addToHistory(pCom);
}
//Undo must both install the previous instance and make it current.
//for delete Instance, the previous instance is inserted and made current
//for change instance, the instance num is reset.
//Otherwise (new, copy, change) the next instance is removed and 
void InstancedPanelCommand::unDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//All instance commands act on current active Viz, except for copy-to
	int winnum = vizMgr->getActiveViz();
	
	Params::ParamsBaseType pType = previousPanel->GetParamsBaseTypeId();
	EventRouter* evRouter = VizWinMgr::getEventRouter(pType);
	int lastInstance;
	switch (instancedCommandType){
		case changeInstance :
			{
				//just change the current instance; no reactivation required:
				vizMgr->setCurrentInstanceIndex(winnum, previousInstance, pType);
				//Don't call makeCurrent, since we are not changing existing params.
				//Just tell the glwindow that the instance has changed:
				RenderParams* activeParams = (RenderParams*)vizMgr->getParams(winnum, pType);
				
				vizMgr->getVizWin(winnum)->getGLWindow()->setActiveParams(activeParams, pType);
			}
			break;
		case newInstance :
		case copyInstance : 
			{
				//delete the last instance.  It should already be disabled.
				//Don't need to change the current instance
				//If nextIndex is not -1, it's the visualizer num we copied to:
				if (nextIndex >= 0) winnum = nextIndex;
				lastInstance = vizMgr->getNumInstances(winnum, pType) -1;
				evRouter->removeRendererInstance(winnum, lastInstance);
				
				//Then set current instance to previousInstance:  unnecessary
			}
			break;
		case deleteInstance :
			{
				//insert a clone at the specified position:
				RenderParams* rpClone = dynamic_cast<RenderParams*>(previousPanel->deepCopy());
				vizMgr->insertInstance(winnum, previousInstance, rpClone);
				//Possibly enable it:
				evRouter->updateRenderer(rpClone, false, false);
				//Then set current instance to previousInstance:
				vizMgr->setCurrentInstanceIndex(winnum, previousInstance, pType);
				RenderParams* activeParams = (RenderParams*)vizMgr->getParams(winnum, pType, previousInstance);
				vizMgr->getVizWin(winnum)->getGLWindow()->setActiveParams(activeParams, pType);
				
			}
			break;
		default:
			assert(0);
			break;
	}
	evRouter->updateTab();
	//Rerender:
	vizMgr->getVizWin(winnum)->updateGL();
	Session::getInstance()->unblockRecording();
	
}
void InstancedPanelCommand::reDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//All instance commands act on current active Viz, except the
	//copy-to, that uses nextIndex to identify the copy target
	int winnum = vizMgr->getActiveViz();

	Params::ParamsBaseType pType = previousPanel->GetParamsBaseTypeId();

	EventRouter* evRouter = VizWinMgr::getEventRouter(pType);
	switch (instancedCommandType){
		case changeInstance :
			//just change it again:
			evRouter->changeRendererInstance(winnum, nextIndex);
			
			break;
	// if it's a delete, need again to remove the previousInstance.
		case deleteInstance :
			evRouter->removeRendererInstance(winnum, previousInstance);
			break;
		case copyInstance :
			if (nextIndex >= 0) winnum = nextIndex;
			evRouter->copyRendererInstance(winnum,(RenderParams*)previousPanel);
			break;
		case newInstance :
			evRouter->newRendererInstance(winnum);
			break;
		default:
			assert(0);
			break;
	}
	//Rerender:
	vizMgr->getVizWin(winnum)->updateGL();
	Session::getInstance()->unblockRecording();
}
