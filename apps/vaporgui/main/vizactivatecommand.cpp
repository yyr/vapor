//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizactivatecommand.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Implements the VizActivateCommand class
//		This is a Command class that supports undo/redo of activate
//		window events from the visualizer window.
//
#include "vizactivatecommand.h"
#include "session.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include <qcolor.h>
#include "dvrparams.h"
#include "animationeventrouter.h"
#include "viewpointeventrouter.h"
#include "regioneventrouter.h"
#include "floweventrouter.h"
#include "dvreventrouter.h"
#include "probeeventrouter.h"
using namespace VAPoR;
VizActivateCommand::VizActivateCommand(VizWin* win, int prevViz, int nextViz, Command::activateType t){
	lastActiveViznum = prevViz;
	currentActiveViznum = nextViz;
	thisType = t;
	regionParams=0;
	vpParams = 0;
	dvrParams = 0;
	probeParams = 0;
	animationParams = 0;
	flowParams = 0;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	backgroundColor = QColor(0,0,0);
	
	//Construct description text, and other data needed:
	switch (thisType){
		case create:
			windowName = vizWinMgr->getVizWinName(nextViz).ascii();
			description = (QString("create ")+windowName).ascii();
			
			break;
		case remove:
			windowName = vizWinMgr->getVizWinName(prevViz).ascii();
			description = (QString("remove ")+windowName).ascii();
			//clone and save all the applicable (local!) params:
			cloneStateParams(win, prevViz);
			break;
		case activate:
			windowName = vizWinMgr->getVizWinName(nextViz).ascii();
			description = (QString("activate ")+windowName).ascii();
			break;
		default:
			assert(0);
	}
	
}
VizActivateCommand::~VizActivateCommand(){
	if (dvrParams) delete dvrParams;
	if (probeParams) delete probeParams;

	if (regionParams) delete regionParams;
	if (vpParams) delete vpParams;
	if (flowParams) delete flowParams;
	//!  what about description and viz name???
}

void VizActivateCommand::unDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int newWinNum;
	switch (thisType){
		case create:
			//delete the specified window.
			//State is already saved.  But to avoid damage to the history
			//when the visualizer is removed, we must
			//first disconnect the visualizer from the current panels 
			
			vizWinMgr->setDvrParams(currentActiveViznum, 0);
			vizWinMgr->setProbeParams(currentActiveViznum, 0);
			vizWinMgr->setRegionParams(currentActiveViznum, 0);
			vizWinMgr->setViewpointParams(currentActiveViznum, 0);
			vizWinMgr->setAnimationParams(currentActiveViznum, 0);
			vizWinMgr->setFlowParams(currentActiveViznum, 0);
			vizWinMgr->killViz(currentActiveViznum);
			break;
		case remove:
			//Re-create removed window.  lastActiveViznum was the removed
			//visualizer number.  First setup the params needed.
			//Reset all the applicable params.  The "previous" params
			//are those from the "currentActiveViznum"
			
			newWinNum = vizWinMgr->launchVisualizer(lastActiveViznum, windowName);
			vizWinMgr->getVizWin(newWinNum)->setBackgroundColor(backgroundColor);
			// make them current, in the correct order:
			if (animationParams) {
				vizWinMgr->getAnimationRouter()->makeCurrent(vizWinMgr->getAnimationParams(currentActiveViznum),
					animationParams, true);
			}
			if(regionParams) {
				vizWinMgr->getRegionRouter()->makeCurrent(vizWinMgr->getRegionParams(currentActiveViznum),
					regionParams, true);
			}
			if(vpParams) {
				vizWinMgr->getViewpointRouter()->makeCurrent(vizWinMgr->getViewpointParams(currentActiveViznum),
					vpParams, true);
			}
			
			vizWinMgr->getDvrRouter()->makeCurrent(vizWinMgr->getDvrParams(currentActiveViznum),
					dvrParams, true);
			vizWinMgr->getProbeRouter()->makeCurrent(vizWinMgr->getProbeParams(currentActiveViznum),
					probeParams, true);
			vizWinMgr->getFlowRouter()->makeCurrent(vizWinMgr->getFlowParams(currentActiveViznum),
					flowParams, true);
			
			break;
		case activate:
			vizWinMgr->winActivated(lastActiveViznum);
			break;
		default:
			assert(0);
	}
	Session::getInstance()->unblockRecording();
}
void VizActivateCommand::reDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int newWinNum;
	switch (thisType){
		case create:
			//Need to create default panels (from global params)
			vizWinMgr->createDefaultParams(currentActiveViznum);
			newWinNum = vizWinMgr->launchVisualizer(currentActiveViznum, windowName);
			vizWinMgr->getVizWin(newWinNum)->setBackgroundColor(backgroundColor);
			break;
		case remove:
			//Note that normally killing a viz has the side-effect of deleting its params.
			//Therefore we need to disconnect the params in the state:
		
			vizWinMgr->setDvrParams(lastActiveViznum, 0);
			vizWinMgr->setFlowParams(lastActiveViznum, 0);
			vizWinMgr->setProbeParams(lastActiveViznum, 0);
			vizWinMgr->setRegionParams(lastActiveViznum, 0);
			vizWinMgr->setViewpointParams(lastActiveViznum, 0);
			vizWinMgr->setAnimationParams(lastActiveViznum, 0);
			vizWinMgr->killViz(lastActiveViznum);
			break;
		case activate:
			vizWinMgr->winActivated(currentActiveViznum);
			break;
		default:  assert(0);
	}
	Session::getInstance()->unblockRecording();
}

void VizActivateCommand::cloneStateParams(VizWin* win, int viznum){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	if (vizWinMgr->getRealVPParams(viznum))
			vpParams = (ViewpointParams*)vizWinMgr->getRealVPParams(viznum)->deepCopy();
	else vpParams = 0;
	if (vizWinMgr->getRealRegionParams(viznum))
		regionParams = (RegionParams*)vizWinMgr->getRealRegionParams(viznum)->deepCopy();
	else {regionParams = 0;}
	if (vizWinMgr->getRealAnimationParams(viznum))
		animationParams = (AnimationParams*)vizWinMgr->getRealAnimationParams(viznum)->deepCopy();
	else {animationParams = 0;}
	
	dvrParams = (DvrParams*)vizWinMgr->getRealDvrParams(viznum)->deepCopy();
	probeParams = (ProbeParams*)vizWinMgr->getRealProbeParams(viznum)->deepCopy();
	flowParams = (FlowParams*)vizWinMgr->getRealFlowParams(viznum)->deepCopy();
	if(win) backgroundColor = win->getBackgroundColor();
}
