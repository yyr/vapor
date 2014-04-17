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
	ControlExecutive* ce = ControlExecutive::getInstance();
	string type = ce->GetTagFromType(myParamsBaseType);
	//Clone this params
	RenderParams* newP = (RenderParams*)rp->deepCopy();
	newP->SetVizNum(toWindow);
	newP->SetEnabled(false);
	ce->AddParams(toWindow,type,newP);
	//update tab is only needed up update the instanceTable when we are copying in the same viz
	updateTab ();
}
void EventRouter::changeRendererInstance(int winnum, int newCurrentInst){
	ControlExecutive* ce = ControlExecutive::getInstance();
	string type = ce->GetTagFromType(myParamsBaseType);
	assert(newCurrentInst >= 0 && newCurrentInst < ce->GetNumParamsInstances(winnum, type));
	ce->SetCurrentRenderParamsInstance(winnum,type,newCurrentInst);
	updateTab();
}
//Put a default instance of specified renderer as the last instance:
void EventRouter::newRendererInstance(int winnum){
	ControlExecutive* ce = ControlExecutive::getInstance();
	string type = ce->GetTagFromType(myParamsBaseType);
	RenderParams* newP = dynamic_cast<RenderParams*>(ce->GetDefaultParams(type)->deepCopy());
	newP->SetVizNum(winnum);
	newP->SetEnabled(false);
	ce->AddParams(winnum,type,newP);
	updateTab ();
}
void EventRouter::removeRendererInstance(int winnum, int instance){
	ControlExecutive* ce = ControlExecutive::getInstance();
	string type = ce->GetTagFromType(myParamsBaseType);
	assert( ce->GetNumParamsInstances(winnum, type) > 1);
	//make sure it's disabled
	RenderParams* rp = (RenderParams*)(ce->GetParams(winnum, type, instance));
	//disable it first if necessary:
	if (rp->IsEnabled()){
		rp->SetEnabled(false);
		updateRenderer(rp, true, instance, false);
	}
	ce->RemoveParams(winnum, type, instance);
	updateTab();
}
void EventRouter::performGuiChangeInstance(int newCurrent, bool undoredo){
	ControlExecutive* ce = ControlExecutive::getInstance();
	
	int winnum = ce->GetActiveVizIndex();
	if (winnum < 0) return;
	string type = ce->GetTagFromType(myParamsBaseType);
	int instance = ce->GetCurrentRenderParamsInstance(winnum,type);
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
	ControlExecutive* ce = ControlExecutive::getInstance();
	int winnum = ce->GetActiveVizIndex();
	if (winnum < 0) return;
	string type = ce->GetTagFromType(myParamsBaseType);
	int instance = ce->GetCurrentRenderParamsInstance(winnum,type);
	removeRendererInstance(winnum, instance);
	
	VizWin* vw = VizWinMgr::getInstance()->getVizWin(winnum);
	vw->updateGL();
}
void EventRouter::performGuiCopyInstance(){
	
	ControlExecutive* ce = ControlExecutive::getInstance();
	int winnum = ce->GetActiveVizIndex();
	if (winnum < 0) return;
	string type = ce->GetTagFromType(myParamsBaseType);
	Params* rParams = ce->GetCurrentParams(winnum,type);
	
	copyRendererInstance(winnum, (RenderParams*)rParams);
}
//Copy current instance to another visualizer
void EventRouter::performGuiCopyInstanceToViz(int towin){
	
	ControlExecutive* ce = ControlExecutive::getInstance();
	int winnum = ce->GetActiveVizIndex();
	if (winnum < 0) return;
	string type = ce->GetTagFromType(myParamsBaseType);
	Params* rParams = ce->GetCurrentParams(winnum,type);
	
	copyRendererInstance(towin, (RenderParams*)rParams);
	//put the copy-source in the command
	
}
void EventRouter::guiSetLocal(Params* p, bool lg){
	if (textChangedFlag) confirmText(false);
	p->SetLocal(lg);
	updateTab();
}

