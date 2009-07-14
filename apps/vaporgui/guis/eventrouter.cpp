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
#include <qmessagebox.h>
#include <qfileinfo.h>
#include <qfiledialog.h>
#include "vapor/DataMgr.h"
#include "vapor/errorcodes.h"
#include "GetAppPath.h"
#include "session.h"
#include "messagereporter.h"
#include "transferfunction.h"
#include "loadtfdialog.h"
#include "savetfdialog.h"
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
	if (winnum < 0) return;
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

void EventRouter::refreshHistogram(RenderParams* renParams, int varNum, const float dRange[2]){
	size_t min_dim[3],max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	DataStatus* ds = DataStatus::getInstance();
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
	
	if (!ds->getDataMgr()) return;
	int timeStep = vizWinMgr->getAnimationParams(vizNum)->getCurrentFrameNumber();
	//Don't refresh histo if bypassFlag is set:
	if (renParams->doBypass(timeStep)) return;
	int numVariables = ds->getNumSessionVariables();
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
	const char* varname = ds->getVariableName(varNum).c_str();
	
	int availRefLevel = rParams->getAvailableVoxelCoords(numTrans, min_dim, max_dim, min_bdim, max_bdim, timeStep, &varNum, 1);
	if(availRefLevel < 0) {
		renParams->setBypass(timeStep);
		if (ds->warnIfDataMissing()){
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, 
				"Data unavailable for variable %s\nat refinement level %d",varname, numTrans);
		}
		return;
	}
		
	//Check if the region/resolution is too big:
	  int numMBs = RegionParams::getMBStorageNeeded(rParams->getRegionMin(timeStep), rParams->getRegionMax(timeStep), availRefLevel);
	  int cacheSize = DataStatus::getInstance()->getCacheMB();
	  if (numMBs > (int)(0.75*cacheSize)){
		  renParams->setAllBypass(true);
		  MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small\nfor current region and resolution.\n%s \n%s",
			  "Lower the refinement level,\nreduce the region size,\nor increase the cache size.",
			  "Rendering has been disabled.");
		  renParams->setEnabled(false);
		  updateTab();
		  return;
	  }
	
	//Now get the data:
	
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	unsigned char* data = (unsigned char*) dataMgr->GetRegionUInt8(
					timeStep, 
					varname,
					availRefLevel,
					min_bdim, max_bdim,
					dRange,
					0 //Don't lock!
		);
	QApplication::restoreOverrideCursor();
	//Make sure we can build a histogram
	if (!data) {
		if (ds->warnIfDataMissing())
			renParams->setBypass(timeStep);
			MessageReporter::errorMsg("Invalid/nonexistent data;\ncannot be histogrammed");
		ds->setDataMissing(timeStep, availRefLevel, varNum);
		return;
	}

	histogramList[varNum] = new Histo(data,
			min_dim, max_dim, min_bdim, max_bdim, dRange[0],dRange[1]);
	
}
//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
//Boolean flag is only used by isoeventrouter version
Histo* EventRouter::getHistogram(RenderParams* renParams, bool mustGet, bool ){
	
	int numVariables = DataStatus::getInstance()->getNumSessionVariables();
	int varNum = renParams->getSessionVarNum();
	if (varNum >= numVariables || varNum < 0) return 0;
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
	refreshHistogram(renParams, varNum,currentDatarange);
	return histogramList[varNum];
	
}

//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the DVR params
void EventRouter::
saveTF(RenderParams* rParams){
	if (!DataStatus::getInstance()->getDataMgr()) return;
	SaveTFDialog* saveTFDialog = new SaveTFDialog(rParams,0,
		"Save TF Dialog", true);
	int rc = saveTFDialog->exec();
	if (rc == 1) fileSaveTF(rParams);
}
void EventRouter::
fileSaveTF(RenderParams* rParams){
	//Launch a file save dialog, open resulting file
    QString s = QFileDialog::getSaveFileName(
					Session::getInstance()->getTFFilePath().c_str(),
                    "Vapor Transfer Functions (*.vtf)",
                    0,
                    "save TF dialog",
                    "Choose a filename to save the transfer function" );
	//Did the user cancel?
	if (s.length()== 0) return;
	//Force the name to end with .vtf
	if (!s.endsWith(".vtf")){
		s += ".vtf";
	}
	QFileInfo finfo(s);
	if (finfo.exists()){
		int rc = QMessageBox::warning(0, "Transfer Function File Exists", QString("OK to replace transfer function file \n%1 ?").arg(s), QMessageBox::Ok, 
			QMessageBox::No);
		if (rc != QMessageBox::Ok) return;
	}
	ofstream fileout;
	fileout.open(s.ascii());
	if (! fileout) {
		QString str("Unable to save to file: \n");
		str += s;
		MessageReporter::errorMsg( str.ascii());
		return;
	}
	
	
	if (!((TransferFunction*)(rParams->getMapperFunc()))->saveToFile(fileout)){//Report error if can't save to file
		QString str("Failed to write output file: \n");
		str += s;
		MessageReporter::errorMsg(str.ascii());
		fileout.close();
		return;
	}
	fileout.close();
	Session::getInstance()->updateTFFilePath(&s);
}
void EventRouter::
loadInstalledTF(RenderParams* rParams, int varnum){
	//Get the path from the environment:
	string share = GetAppPath("vapor", "share");
#ifdef WIN32
	const char *slash = "\\";
#else
	const char* slash = "/";
#endif

	QString installPath = (share + slash + "palettes").c_str();
	fileLoadTF(rParams,varnum, installPath.ascii(),false);
}
void EventRouter::
loadTF(RenderParams* rParams, int varnum){
	//If there are no TF's currently in Session, just launch file load dialog.
	
	if (Session::getInstance()->getNumTFs() > 0){
		LoadTFDialog* loadTFDialog = new LoadTFDialog(this, 0,
			"Load TF Dialog", true);
		int rc = loadTFDialog->exec();
		if (rc == 0) return;
		if (rc == 1) {
			fileLoadTF(rParams, varnum, Session::getInstance()->getTFFilePath().c_str(),true);
			
		}
		//if rc == 2, we already (probably) loaded a tf from the session
	} else {
		fileLoadTF(rParams, varnum, Session::getInstance()->getTFFilePath().c_str(),true);
	}
	setEditorDirty(rParams);
}

void EventRouter::
fileLoadTF(RenderParams* rParams, int varnum, const char* startPath, bool savePath){
	if (DataStatus::getInstance()->getNumSessionVariables()<=0) return;
	
	//Open a file load dialog
	
    QString s = QFileDialog::getOpenFileName(
                    startPath,
                    "Vapor Transfer Functions (*.vtf)",
                    0,
                    "load TF dialog",
                    "Choose a transfer function file to open" );
	//Null string indicates nothing selected.
	if (s.length() == 0) return;
	//Force the name to end with .vtf
	if (!s.endsWith(".vtf")){
		s += ".vtf";
	}
	
	ifstream is;
	
	is.open(s.ascii());

	if (!is){//Report error if you can't open the file
		QString str("Unable to open file: \n");
		str+= s;
		MessageReporter::errorMsg(str.ascii());
		return;
	}
	//Start the history save:
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "Load Transfer Function from file");
	
	TransferFunction* t = TransferFunction::loadFromFile(is, rParams);
	if (!t){//Report error if can't load
		QString str("Error loading transfer function. /nFailed to convert input file: \n ");
		str += s;
		MessageReporter::errorMsg(str.ascii());
		//Don't put this into history!
		delete cmd;
		return;
	}

	rParams->hookupTF(t, varnum);
	//Remember the path to the file:
	if(savePath) Session::getInstance()->updateTFFilePath(&s);
	PanelCommand::captureEnd(cmd, rParams);
	
	setEditorDirty(rParams);
	setDatarangeDirty(rParams);
	VizWinMgr::getInstance()->setClutDirty(rParams);
}

	
