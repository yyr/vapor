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
#include <qapplication.h>
#include <qcursor.h>
#include "vapor/DataMgr.h"
#include "vapor/errorcodes.h"
#include "session.h"
#include "messagereporter.h"
#include <vector>

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
	assert(newCurrentInst >= 0 && newCurrentInst < vizMgr->getNumInstances(winnum, myParamsType));
	vizMgr->setCurrentInstanceIndex(winnum, newCurrentInst, myParamsType);
	updateTab();
	vizMgr->getVizWin(winnum)->getGLWindow()->setActiveParams(vizMgr->getParams(winnum, myParamsType),myParamsType);
}
//Put a default instance of specified renderer as the last instance:
void EventRouter::newRendererInstance(int winnum){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//Clone default params
	RenderParams* newP = ((RenderParams*)vizMgr->getGlobalParams(myParamsType))->deepRCopy();
	newP->setVizNum(winnum);
	newP->setEnabled(false);
	vizMgr->appendInstance(winnum, newP);
	updateTab ();
}
void EventRouter::removeRendererInstance(int winnum, int instance){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	assert( vizMgr->getNumInstances(winnum, myParamsType) > 1);
	//make sure it's disabled
	RenderParams* rp = (RenderParams*)(vizMgr->getParams(winnum, myParamsType, instance));
	//disable it first if necessary:
	if (rp->isEnabled()){
		rp->setEnabled(false);
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
}
void EventRouter::performGuiDeleteInstance(){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int instance = vizMgr->getCurrentInstanceIndex(winnum, myParamsType);
	Params* rParams = vizMgr->getParams(winnum,myParamsType, instance);
	InstancedPanelCommand::capture(rParams, "remove renderer instance", instance, VAPoR::deleteInstance);
	removeRendererInstance(winnum, instance);
	Params* p = vizMgr->getParams(winnum, myParamsType);
	VizWin* vw = vizMgr->getVizWin(winnum);
	vw->getGLWindow()->setActiveParams(p,myParamsType);
	vw->updateGL();
}
void EventRouter::performGuiCopyInstance(){
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	Params* rParams = vizMgr->getParams(winnum,myParamsType);
	int currentInstance = vizMgr->getCurrentInstanceIndex(winnum, myParamsType);
	InstancedPanelCommand::capture(rParams, "copy renderer instance", currentInstance, VAPoR::copyInstance);
	copyRendererInstance(winnum, (RenderParams*)rParams);
}
//Copy current instance to another visualizer
void EventRouter::performGuiCopyInstanceToViz(int towin){
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	Params* rParams = vizMgr->getParams(winnum,myParamsType);
	int currentInstance = vizMgr->getCurrentInstanceIndex(winnum, myParamsType);
	
	copyRendererInstance(towin, (RenderParams*)rParams);
	//put the copy-source in the command
	
	InstancedPanelCommand::capture(rParams, "copy renderer instance to viz", currentInstance, VAPoR::copyInstance, towin);
}

void EventRouter::refreshHistogram(RenderParams* renParams){
	size_t min_dim[3],max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	DataMgr* dataMgr = Session::getInstance()->getDataMgr();
	if(!dataMgr) return;
	RegionParams* rParams;
	int vizNum = renParams->getVizNum();
	if (vizNum<0) {
		//It's possible that multiple different region params apply to
		//different dvr panels if they are shared.  But the current active
		//region params will apply here.
		rParams = vizWinMgr->getActiveRegionParams();
	} else {
		rParams = vizWinMgr->getRegionParams(vizNum);
	}
	
	size_t fullHeight = rParams->getFullGridHeight();
	int varNum = renParams->getSessionVarNum();
	
	if (!DataStatus::getInstance()->getDataMgr()) return;
	int numVariables = DataStatus::getInstance()->getNumSessionVariables();
	if (!histogramList){
		histogramList = new Histo*[numVariables];
		numHistograms = numVariables;
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
	}
	if (histogramList[varNum]){
		delete histogramList[varNum];
		histogramList[varNum] = 0;
	}
	int numTrans = renParams->getNumRefinements();
	int timeStep = vizWinMgr->getAnimationParams(vizNum)->getCurrentFrameNumber();
	const float * bnds = renParams->getCurrentDatarange();
	
	bool dataValid = rParams->getAvailableVoxelCoords(numTrans, min_dim, max_dim, min_bdim, max_bdim, timeStep, &varNum, 1);
	if(!dataValid){
		MessageReporter::warningMsg("Histogram data unavailable for refinement %d at timestep %d", numTrans, timeStep);
		return;
	}
	
	//Check if the region/resolution is too big:
	  int numMBs = RegionParams::getMBStorageNeeded(rParams->getRegionMin(), rParams->getRegionMax(), numTrans);
	  int cacheSize = DataStatus::getInstance()->getCacheMB();
	  if (numMBs > (int)(0.75*cacheSize)){
		  MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small for current region and resolution.\n%s \n%s",
			  "Lower the refinement level, reduce the region size, or increase the cache size.",
			  "Rendering has been disabled.");
		  renParams->setEnabled(false);
		  updateTab();
		  return;
	  }
	
	//Now get the data:
	
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	unsigned char* data = (unsigned char*) dataMgr->GetRegionUInt8(
					timeStep, 
					(const char*)DataStatus::getInstance()->getVariableName(varNum).c_str(),
					numTrans,
					min_bdim, max_bdim,
					fullHeight,
					renParams->getCurrentDatarange(),
					0 //Don't lock!
		);
	QApplication::restoreOverrideCursor();
	//Make sure we can build a histogram
	if (!data) {
		MessageReporter::errorMsg("Invalid/nonexistent data cannot be histogrammed");
		return;
	}

	histogramList[varNum] = new Histo(data,
			min_dim, max_dim, min_bdim, max_bdim, bnds[0], bnds[1]);
	
}
//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
Histo* EventRouter::getHistogram(RenderParams* renParams, bool mustGet){
	
	int numVariables = DataStatus::getInstance()->getNumSessionVariables();
	int varNum = renParams->getSessionVarNum();
	if (varNum >= numHistograms || !histogramList){
		if (!mustGet) return 0;
		histogramList = new Histo*[numVariables];
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
		numHistograms = numVariables;
	}
	
	const float* currentDatarange = renParams->getCurrentDatarange();
	if (histogramList[varNum]) return histogramList[varNum];
	
	if (!mustGet) return 0;
	histogramList[varNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	refreshHistogram(renParams);
	return histogramList[varNum];
	
}


	
