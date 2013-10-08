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
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//Clone this params
	RenderParams* newP = (RenderParams*)rp->deepCopy();
	newP->setVizNum(toWindow);
	newP->setEnabled(false);
	vizMgr->appendInstance(toWindow, newP);
	//update tab is only needed up update the instanceTable when we are copying in the same viz
	updateTab ();
}
void EventRouter::changeRendererInstance(int winnum, int newCurrentInst){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	assert(newCurrentInst >= 0 && newCurrentInst < vizMgr->getNumInstances(winnum, myParamsBaseType));
	vizMgr->setCurrentInstanceIndex(winnum, newCurrentInst, myParamsBaseType);
	updateTab();
}
//Put a default instance of specified renderer as the last instance:
void EventRouter::newRendererInstance(int winnum){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//Clone default params
	RenderParams* newP = dynamic_cast<RenderParams*>(vizMgr->getGlobalParams(myParamsBaseType)->deepCopy());
	newP->setVizNum(winnum);
	newP->setEnabled(false);
	vizMgr->appendInstance(winnum, newP);
	updateTab ();
}
void EventRouter::removeRendererInstance(int winnum, int instance){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	assert( vizMgr->getNumInstances(winnum, myParamsBaseType) > 1);
	//make sure it's disabled
	RenderParams* rp = (RenderParams*)(vizMgr->getParams(winnum, myParamsBaseType, instance));
	//disable it first if necessary:
	if (rp->isEnabled()){
		rp->setEnabled(false);
		updateRenderer(rp, true, false);
	}
	vizMgr->removeInstance(winnum, instance, myParamsBaseType);
	updateTab();
}
void EventRouter::performGuiChangeInstance(int newCurrent, bool undoredo){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	if (winnum < 0) return;
	int instance = vizMgr->getCurrentInstanceIndex(winnum,myParamsBaseType);
	if (instance == newCurrent) return;
	
	changeRendererInstance(winnum, newCurrent);
	
	VizWin* vw = vizMgr->getVizWin(winnum);
	
	vw->updateGL();
}
void EventRouter::performGuiNewInstance(){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	
	newRendererInstance(winnum);
}
void EventRouter::performGuiDeleteInstance(){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int instance = vizMgr->getCurrentInstanceIndex(winnum, myParamsBaseType);
	Params* rParams = vizMgr->getParams(winnum,myParamsBaseType, instance);
	
	removeRendererInstance(winnum, instance);
	
	VizWin* vw = vizMgr->getVizWin(winnum);
	vw->updateGL();
}
void EventRouter::performGuiCopyInstance(){
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	Params* rParams = vizMgr->getParams(winnum,myParamsBaseType);

	
	copyRendererInstance(winnum, (RenderParams*)rParams);
}
//Copy current instance to another visualizer
void EventRouter::performGuiCopyInstanceToViz(int towin){
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	Params* rParams = vizMgr->getParams(winnum,myParamsBaseType);
	
	copyRendererInstance(towin, (RenderParams*)rParams);
	//put the copy-source in the command
	
}


