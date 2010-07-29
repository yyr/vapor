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
	
	savedParamsInstances = 0;
	savedDefaultParams = 0;
	
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
	if (savedParamsInstances){
		for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
			vector<Params*> pInstances = (*savedParamsInstances)[i];
			for (int j = 0; j< pInstances.size(); j++){
				delete pInstances[j];
			}
		}
		delete savedParamsInstances;
	}
	if (savedDefaultParams){
		for (int i = 1; i < savedDefaultParams->size(); i++){
			delete (*savedDefaultParams)[i];
		}
		delete savedDefaultParams;
	}


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
			for (int k = 1; k <= Params::GetNumParamsClasses(); k++){
				Params::GetAllParamsInstances(k,currentActiveViznum).clear();
			}

			vizWinMgr->killViz(currentActiveViznum);
			break;
		case remove:
			{
			//Re-create removed window.  lastActiveViznum was the removed
			//visualizer number.  First setup the params needed.
			//Reset all the applicable params.  The "previous" params
			//are those from the "currentActiveViznum"
			//Using an argument of -2 so that launchVisualizer will
			//recreate params for this visualizer
			newWinNum = vizWinMgr->launchVisualizer(lastActiveViznum, windowName,-2);
			vizWinMgr->getVizWin(newWinNum)->setBackgroundColor(backgroundColor);
			// make them current, in the correct order:
			AnimationParams* ap = (AnimationParams*)(*savedDefaultParams)[Params::GetTypeFromTag(Params::_animationParamsTag)];
			if (ap) {
				vizWinMgr->getAnimationRouter()->makeCurrent(vizWinMgr->getAnimationParams(currentActiveViznum),
					ap, true);
			}
			RegionParams* rp = (RegionParams*)(*savedDefaultParams)[Params::GetTypeFromTag(Params::_regionParamsTag)];
			if(rp) {
				vizWinMgr->getRegionRouter()->makeCurrent(vizWinMgr->getRegionParams(currentActiveViznum),
					rp, true);
			}
			ViewpointParams* vpParams = (ViewpointParams*)(*savedDefaultParams)[Params::GetTypeFromTag(Params::_viewpointParamsTag)];
			if(vpParams) {
				vizWinMgr->getViewpointRouter()->makeCurrent(vizWinMgr->getViewpointParams(currentActiveViznum),
					vpParams, true);
			}
			//There will initially exist one instance of each render params.
			//If there is more than one instance in the saved list, then we need to
			//create additional params for the instances
			
			for (int i = 1; i<=Params::GetNumParamsClasses(); i++){
				vector<Params*> params = (*savedParamsInstances)[i];
				EventRouter* eRouter = vizWinMgr->getEventRouter(i);
				if (!params[0]->isRenderParams()) continue;
				eRouter->makeCurrent(vizWinMgr->getParams(currentActiveViznum,i, 0),params[0],true, 0);
				for (int inst = 1; inst < params.size(); inst++){
					eRouter->makeCurrent(0, params[inst], true, inst);
				}
			}

			break;
			}
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
	savedParamsInstances = Params::cloneAllParamsInstances(viznum);
	savedDefaultParams = Params::cloneAllDefaultParams();

	if(win) backgroundColor = win->getBackgroundColor();
}
