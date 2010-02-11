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
#include "GL/glew.h"
#include "vizactivatecommand.h"
#include "session.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include <qcolor.h>
#include "dvrparams.h"
#include "ParamsIso.h"
#include "animationeventrouter.h"
#include "viewpointeventrouter.h"
#include "regioneventrouter.h"
#include "floweventrouter.h"
#include "dvreventrouter.h"
#include "isoeventrouter.h"
#include "probeeventrouter.h"
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VAPoR;
VizActivateCommand::VizActivateCommand(VizWin* win, int prevViz, int nextViz, Command::activateType t){
	lastActiveViznum = prevViz;
	currentActiveViznum = nextViz;
	thisType = t;
	regionParams=0;
	vpParams = 0;
	dvrParamsList.clear();
	isoParamsList.clear();
	probeParamsList.clear();
	flowParamsList.clear();
	animationParams = 0;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	backgroundColor = QColor(0,0,0);
	
	//Construct description text, and other data needed:
	switch (thisType){
		case create:
			windowName = new char[vizWinMgr->getVizWinName(nextViz).length()+1];
			strcpy((char*)windowName, (const char*)vizWinMgr->getVizWinName(nextViz).toAscii());
			
			description = (const char*)(QString("create ")+windowName).toAscii();
			
			break;
		case remove:
			windowName = new char[vizWinMgr->getVizWinName(prevViz).length()+1];
			strcpy((char*)windowName, (const char*)vizWinMgr->getVizWinName(prevViz).toAscii());
			
			description = (const char*)(QString("remove ")+windowName).toAscii();
			//clone and save all the applicable (local!) params:
			cloneStateParams(win, prevViz);
			break;
		case activate:
			windowName = new char[vizWinMgr->getVizWinName(nextViz).length()+1];
			strcpy((char*)windowName, vizWinMgr->getVizWinName(nextViz).toAscii());
			description = (const char*)(QString("activate ")+windowName).toAscii();
			break;
		default:
			assert(0);
	}
	
}
VizActivateCommand::~VizActivateCommand(){
	for (unsigned int i = 0; i<dvrParamsList.size(); i++) delete dvrParamsList[i];
	for (unsigned int i = 0; i<flowParamsList.size(); i++) delete flowParamsList[i];
	for (unsigned int i = 0; i<probeParamsList.size(); i++) delete probeParamsList[i];
	for (unsigned int i = 0; i<isoParamsList.size(); i++) delete isoParamsList[i];

	if (regionParams) delete regionParams;
	if (vpParams) delete vpParams;
	
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
			
			vizWinMgr->getAllDvrParams(currentActiveViznum).clear();
			vizWinMgr->getAllIsoParams(currentActiveViznum).clear();
			vizWinMgr->getAllFlowParams(currentActiveViznum).clear();
			vizWinMgr->getAllProbeParams(currentActiveViznum).clear();

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
			//Using an argument of -2 so that launchVisualizer will
			//recreate params for this visualizer
			newWinNum = vizWinMgr->launchVisualizer(lastActiveViznum, windowName,-2);
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
			//There will initially exist one instance of each render params.
			//If there is more than one instance in the saved list, then we need to
			//create additional params for the instances
			//number of instances:
			vizWinMgr->getDvrRouter()->makeCurrent(vizWinMgr->getDvrParams(currentActiveViznum,0),
					dvrParamsList[0], true, 0);
			for (unsigned int inst = 1; inst < dvrParamsList.size(); inst++){
				vizWinMgr->getDvrRouter()->makeCurrent(0, dvrParamsList[inst], true, inst);
			}
			vizWinMgr->getIsoRouter()->makeCurrent(vizWinMgr->getIsoParams(currentActiveViznum,0),
					isoParamsList[0], true, 0);
			for (unsigned int inst = 1; inst < isoParamsList.size(); inst++){
				vizWinMgr->getIsoRouter()->makeCurrent(0, isoParamsList[inst], true, inst);
			}
			vizWinMgr->getProbeRouter()->makeCurrent(vizWinMgr->getProbeParams(currentActiveViznum,0),
					probeParamsList[0], true, 0);
			for (unsigned int inst = 1; inst < probeParamsList.size(); inst++){
				vizWinMgr->getProbeRouter()->makeCurrent(0,probeParamsList[inst], true, inst);
			}
			vizWinMgr->getFlowRouter()->makeCurrent(vizWinMgr->getFlowParams(currentActiveViznum,0),
					flowParamsList[0], true, 0);
			for (unsigned int inst = 1; inst < flowParamsList.size(); inst++){
				vizWinMgr->getFlowRouter()->makeCurrent(0,flowParamsList[inst], true, inst);
			}
			
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
			//Therefore we need to disconnect the params in the state before killViz
			//This maybe wrong--why shouldn't we delete the params???
		
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
	//Clone the whole vector of dvr/probe/flow params
	for (int inst = 0; inst<vizWinMgr->getNumDvrInstances(viznum); inst++){
		dvrParamsList.push_back((DvrParams*)vizWinMgr->getDvrParams(viznum, inst)->deepCopy());
	}
	for (int inst = 0; inst<vizWinMgr->getNumIsoInstances(viznum); inst++){
		isoParamsList.push_back((ParamsIso*)vizWinMgr->getIsoParams(viznum, inst)->deepCopy());
	}
	for (int inst = 0; inst<vizWinMgr->getNumProbeInstances(viznum); inst++){
		probeParamsList.push_back((ProbeParams*)vizWinMgr->getProbeParams(viznum, inst)->deepCopy());
	}
	for (int inst = 0; inst<vizWinMgr->getNumFlowInstances(viznum); inst++){
		flowParamsList.push_back((FlowParams*)vizWinMgr->getFlowParams(viznum, inst)->deepCopy());
	}
	if(win) backgroundColor = win->getBackgroundColor();
}
