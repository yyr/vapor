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
using namespace VAPoR;
VizActivateCommand::VizActivateCommand(int prevViz, int nextViz, Command::activateType t){
	lastActiveViznum = prevViz;
	currentActiveViznum = nextViz;
	thisType = t;
	regionParams=0;
	vpParams = 0;
	contourParams = 0;
	dvrParams = 0;
	isoParams = 0;
	animationParams = 0;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	
	
	//Construct description text, and other data needed:
	switch (thisType){
		case create:
			windowName = vizWinMgr->getVizWinName(nextViz)->ascii();
			description = (QString("create ")+windowName).ascii();
			
			break;
		case remove:
			windowName = vizWinMgr->getVizWinName(prevViz)->ascii();
			description = (QString("remove ")+windowName).ascii();
			//clone and save all the applicable (local!) params:
			cloneStateParams(prevViz);
			break;
		case activate:
			windowName = vizWinMgr->getVizWinName(nextViz)->ascii();
			description = (QString("activate ")+windowName).ascii();
			break;
		default:
			assert(0);
	}
	
}
VizActivateCommand::~VizActivateCommand(){
	if (dvrParams) delete dvrParams;
	if (contourParams) delete contourParams;
	if (isoParams) delete isoParams;
	if (regionParams) delete regionParams;
	if (vpParams) delete vpParams;
	//!  what about description and viz name???
}

void VizActivateCommand::unDo(){
	Session::getInstance()->blockRecording();
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	switch (thisType){
		case create:
			//delete the specified window.
			//State is already saved.  But to avoid damage to the history
			//when the visualizer is removed, we must
			//first disconnect the visualizer from the current panels 
			vizWinMgr->setContourParams(currentActiveViznum, 0);
			vizWinMgr->setDvrParams(currentActiveViznum, 0);
			vizWinMgr->setIsoParams(currentActiveViznum, 0);
			vizWinMgr->setRegionParams(currentActiveViznum, 0);
			vizWinMgr->setViewpointParams(currentActiveViznum, 0);
			vizWinMgr->setAnimationParams(currentActiveViznum, 0);
			vizWinMgr->killViz(currentActiveViznum);
			break;
		case remove:
			//Re-create removed window.  lastActiveViznum was the removed
			//visualizer number.  First setup the params needed.
			//Reset all the applicable params.  The "previous" params
			//are those from the "currentActiveViznum"
			
			vizWinMgr->launchVisualizer(lastActiveViznum, windowName);
			
			// make them current, in the correct order:
			if (animationParams) animationParams->makeCurrent(vizWinMgr->getAnimationParams(currentActiveViznum), true);
			if(regionParams) regionParams->makeCurrent(vizWinMgr->getRegionParams(currentActiveViznum), true);
			if(vpParams) vpParams->makeCurrent(vizWinMgr->getViewpointParams(currentActiveViznum),true);
			
			contourParams->makeCurrent(vizWinMgr->getContourParams(currentActiveViznum),true);
			dvrParams->makeCurrent(vizWinMgr->getDvrParams(currentActiveViznum),true);
			isoParams->makeCurrent(vizWinMgr->getIsoParams(currentActiveViznum),true);
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
	switch (thisType){
		case create:
			//Need to create default panels (from global params)
			vizWinMgr->createDefaultParams(currentActiveViznum);
			vizWinMgr->launchVisualizer(currentActiveViznum, windowName);
			break;
		case remove:
			//Note that normally killing a viz has the side-effect of deleting its params.
			//Therefore we need to disconnect the params in the state:
			vizWinMgr->setContourParams(lastActiveViznum, 0);
			vizWinMgr->setDvrParams(lastActiveViznum, 0);
			vizWinMgr->setIsoParams(lastActiveViznum, 0);
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

void VizActivateCommand::cloneStateParams(int viznum){
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
	
	isoParams = (IsosurfaceParams*)vizWinMgr->getRealIsoParams(viznum)->deepCopy();
	contourParams = (ContourParams*)vizWinMgr->getRealContourParams(viznum)->deepCopy();
	dvrParams = (DvrParams*)vizWinMgr->getRealDvrParams(viznum)->deepCopy();
}
