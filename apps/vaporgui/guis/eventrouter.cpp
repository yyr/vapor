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
#include "vapor/ControlExecutive.h"
#include <qapplication.h>
#include <qcursor.h>
#include <qmessagebox.h>
#include <qfileinfo.h>
#include <QFileDialog>
#include <vapor/DataMgr.h>
#include <vapor/errorcodes.h>
#include "vapor/GetAppPath.h"
#include <vector>
#include <iostream>
#include <fstream>

using namespace VAPoR;


//Copy the specified renderer to the last instance in specified window:
void EventRouter::copyRendererInstance(int toWindow, RenderParams* rp){
	
	string type = ControlExec::GetTagFromType(myParamsBaseType);
	//Clone this params
	RenderParams* newP = (RenderParams*)rp->deepCopy();
	
	int rc = ControlExec::AddParams(toWindow,type,newP);
	newP->SetVizNum(toWindow);
	newP->SetEnabled(false);
	assert (!rc);
	//update tab is only needed up update the instanceTable when we are copying in the same viz
	updateTab ();
}
void EventRouter::changeRendererInstance(int winnum, int newCurrentInst){
	
	string type = ControlExec::GetTagFromType(myParamsBaseType);
	assert(newCurrentInst >= 0 && newCurrentInst < ControlExec::GetNumParamsInstances(winnum, type));
	int rc = ControlExec::SetCurrentParamsInstance(winnum,type,newCurrentInst);
	assert (!rc);
	updateTab();
}
//Put a default instance of specified renderer as the last instance:
void EventRouter::newRendererInstance(int winnum){
	
	string type = ControlExec::GetTagFromType(myParamsBaseType);
	RenderParams* newP = dynamic_cast<RenderParams*>(ControlExec::GetDefaultParams(type)->deepCopy());
	
	int rc = ControlExec::AddParams(winnum,type,newP);
	newP->SetVizNum(winnum);
	newP->SetEnabled(false);
	assert (!rc);
	updateTab ();
}
void EventRouter::removeRendererInstance(int winnum, int instance){
	
	string type = ControlExec::GetTagFromType(myParamsBaseType);
	assert( ControlExec::GetNumParamsInstances(winnum, type) > 1);
	//make sure it's disabled
	RenderParams* rp = (RenderParams*)(ControlExec::GetParams(winnum, type, instance));
	//disable it first if necessary:
	if (rp->IsEnabled()){
		rp->SetEnabled(false);
		updateRenderer(rp, true, instance, false);
	}
	int rc = ControlExec::RemoveParams(winnum, type, instance);
	assert (!rc);
	updateTab();
}
void EventRouter::performGuiChangeInstance(int newCurrent){
	
	int winnum = ControlExec::GetActiveVizIndex();
	if (winnum < 0) return;
	string type = ControlExec::GetTagFromType(myParamsBaseType);
	int instance = ControlExec::GetCurrentRenderParamsInstance(winnum,type);
	if (instance == newCurrent) return;
	
	changeRendererInstance(winnum, newCurrent);
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	VizWin* vw = vizMgr->getVizWin(winnum);
	vw->updateGL();
}
void EventRouter::performGuiNewInstance(){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	
	newRendererInstance(winnum);
}
void EventRouter::performGuiDeleteInstance(){
	
	int winnum = ControlExec::GetActiveVizIndex();
	if (winnum < 0) return;
	string type = ControlExec::GetTagFromType(myParamsBaseType);
	int instance = ControlExec::GetCurrentRenderParamsInstance(winnum,type);
	removeRendererInstance(winnum, instance);
	
	VizWin* vw = VizWinMgr::getInstance()->getVizWin(winnum);
	vw->updateGL();
}
void EventRouter::performGuiCopyInstance(){
	
	
	int winnum = ControlExec::GetActiveVizIndex();
	if (winnum < 0) return;
	string type = ControlExec::GetTagFromType(myParamsBaseType);
	Params* rParams = ControlExec::GetCurrentParams(winnum,type);
	
	copyRendererInstance(winnum, (RenderParams*)rParams);
}
//Copy current instance to another visualizer
void EventRouter::performGuiCopyInstanceToViz(int towin){
	
	int winnum = ControlExec::GetActiveVizIndex();
	if (winnum < 0) return;
	string type = ControlExec::GetTagFromType(myParamsBaseType);
	Params* rParams = ControlExec::GetCurrentParams(winnum,type);
	
	copyRendererInstance(towin, (RenderParams*)rParams);
	//put the copy-source in the command
	
}
void EventRouter::guiSetLocal(Params* p, bool lg){
	if (textChangedFlag) confirmText(false);
	p->SetLocal(lg);
	ViewpointParams* q = dynamic_cast<ViewpointParams*>(p);
	if (q){
		if (lg) q->SetChanged(true);
		else {//set the global change bit:
			ViewpointParams* glParams = (ViewpointParams*)ControlExec::GetDefaultParams(Params::_viewpointParamsTag);
			glParams->SetChanged(true);
		}
	}
	updateTab();
}

/* Handle the change of status associated with change of enablement 
 
 * If the window is new, (i.e. we are just creating a new window, use 
 * prevEnabled = false
 
 */
void EventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled,int instance, bool newWindow){
	
	
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = rParams->GetVizNum();
	
	if (newWindow) {
		prevEnabled = false;
	}
	
	bool nowEnabled = rParams->IsEnabled();
	if (prevEnabled == nowEnabled) return;

	VizWin* viz = 0;
	if(winnum >= 0){//Find the viz that this applies to:
		viz = vizWinMgr->getVizWin(winnum);
	} 
	
	//cases to consider:
	//1.  unchanged disabled renderer; do nothing.
	//  enabled renderer, just refresh:
	
	if (prevEnabled == nowEnabled) {
		if (!prevEnabled) return;
		vizWinMgr->forceRender(rParams, false);
		return;
	}
	
	if (nowEnabled && !prevEnabled ){//For case 2.:  create a renderer in the active window:
		int rc = ControlExec::ActivateRender(winnum,rParams->GetName(),instance,true);
		
		//force the renderer to refresh 
		if (!rc) vizWinMgr->forceRender(rParams, true);
	
		return;
	}
	
	assert(prevEnabled && !nowEnabled); //case 6, disable 
	int rc = ControlExec::ActivateRender(winnum,rParams->GetName(),instance,false);
	if (!rc) vizWinMgr->forceRender(rParams, true);

	return;
}
