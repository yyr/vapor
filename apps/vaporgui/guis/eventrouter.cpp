//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		eventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2006
//
//	Description:	Implements the (pure virtual) EventRouter class.
//		This class supports routing messages from the gui to the params
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include "eventrouter.h"
#include "params.h"
#include "vizwinmgr.h"
#include <vector>

using namespace VAPoR;


//Copy the specified renderer to the last instance, make it current:
void EventRouter::copyRendererInstance(int toWindow, RenderParams* rp){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//Clone this params
	RenderParams* newP = (RenderParams*)rp->deepCopy();
	newP->setVizNum(toWindow);
	newP->setEnabled(false);
	vizMgr->appendInstance(toWindow, newP);
	vizMgr->setCurrentInstanceIndex(toWindow, vizMgr->getNumInstances(toWindow, myParamsType)-1 , myParamsType);
	vizMgr->getVizWin(toWindow)->getGLWindow()->setActiveParams(vizMgr->getParams(toWindow,myParamsType),myParamsType);
	updateTab ();
}
void EventRouter::changeRendererInstance(int winnum, int newCurrentInst){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	assert(newCurrentInst >= 0 && newCurrentInst < vizMgr->getNumInstances(winnum, myParamsType));
	vizMgr->setCurrentInstanceIndex(winnum, newCurrentInst, myParamsType);
	updateTab();
	vizMgr->getVizWin(winnum)->getGLWindow()->setActiveParams(vizMgr->getParams(winnum, myParamsType),myParamsType);
}
//Put a default instance of specified renderer as the last instance, make it current:
void EventRouter::newRendererInstance(int winnum){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//Clone default params
	RenderParams* newP = ((RenderParams*)vizMgr->getGlobalParams(myParamsType))->deepRCopy();
	newP->setVizNum(winnum);
	newP->setEnabled(false);
	vizMgr->appendInstance(winnum, newP);
	vizMgr->setCurrentInstanceIndex(winnum, vizMgr->getNumInstances(winnum, myParamsType)-1, myParamsType);
	vizMgr->getVizWin(winnum)->getGLWindow()->setActiveParams(vizMgr->getParams(winnum,myParamsType),myParamsType);
	updateTab ();
}
void EventRouter::removeRendererInstance(int winnum, int instance){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	assert( vizMgr->getNumInstances(winnum, myParamsType) > 1);
	//make sure it's disabled
	RenderParams* rp = (RenderParams*)(vizMgr->getParams(winnum, myParamsType, instance));
	//disable it first if necessary:
	if (rp->isEnabled()){
		updateRenderer(rp, true, false);
	}
	vizMgr->removeInstance(winnum, instance, myParamsType);
	updateTab();
	vizMgr->getVizWin(winnum)->getGLWindow()->setActiveParams(vizMgr->getParams(winnum,myParamsType),myParamsType);
}
void EventRouter::performGuiChangeInstance(int newCurrent){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int instance = vizMgr->getCurrentInstanceIndex(winnum,myParamsType);
	if (instance == newCurrent) return;
	Params* rParams = vizMgr->getParams(winnum,myParamsType);
	InstancedPanelCommand::capture(rParams, "change current render instance", instance, VAPoR::changeInstance, newCurrent);
	changeRendererInstance(winnum, newCurrent);
	Params* p = vizMgr->getParams(winnum, myParamsType);
	VizWin* vw = vizMgr->getVizWin(winnum);
	vw->getGLWindow()->setActiveParams(p,myParamsType);
	vw->updateGL();
}
void EventRouter::performGuiNewInstance(){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int instance = vizMgr->getCurrentInstanceIndex(winnum,myParamsType);
	Params* fParams = vizMgr->getParams(winnum,myParamsType);
	InstancedPanelCommand::capture(fParams, "create new renderer instance", instance, VAPoR::newInstance);
	newRendererInstance(winnum);
	Params* p = vizMgr->getParams(winnum, myParamsType);
	VizWin* vw = vizMgr->getVizWin(winnum);
	vw->getGLWindow()->setActiveParams(p,myParamsType);
	vw->updateGL();
}
void EventRouter::performGuiDeleteInstance(){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int instance = vizMgr->getCurrentFlowInstIndex(winnum);
	Params* rParams = vizMgr->getParams(winnum,myParamsType);
	InstancedPanelCommand::capture(rParams, "remove renderer instance", instance, VAPoR::deleteInstance);
	removeRendererInstance(winnum, instance);
	Params* p = vizMgr->getParams(winnum, myParamsType);
	VizWin* vw = vizMgr->getVizWin(winnum);
	vw->getGLWindow()->setActiveParams(p,myParamsType);
	vw->updateGL();
}
void EventRouter::performGuiCopyInstance(){
	//TODO: If there is more than one visualizer, provide option of specifying another viz.
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	Params* rParams = vizMgr->getParams(winnum,myParamsType);
	int currentInstance = vizMgr->getCurrentFlowInstIndex(winnum);
	InstancedPanelCommand::capture(rParams, "copy renderer instance", currentInstance, VAPoR::copyInstance);
	copyRendererInstance(winnum, (RenderParams*)rParams);
	Params* p = vizMgr->getParams(winnum, myParamsType);
	VizWin* vw = vizMgr->getVizWin(winnum);
	vw->getGLWindow()->setActiveParams(p,myParamsType);
	vw->updateGL();
}





	
