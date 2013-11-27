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
#include <qurl.h>
#include <qmessagebox.h>
#include <qfileinfo.h>
#include <QFileDialog>
#include <qgroupbox.h>
#include <vapor/DataMgr.h>
#include <vapor/errorcodes.h>
#include "vapor/GetAppPath.h"
#include "mainform.h"
#include "session.h"
#include "messagereporter.h"
#include "transferfunction.h"
#include "loadtfdialog.h"
#include "savetfdialog.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <qcombobox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <QHBoxLayout>

#ifdef _WINDOWS 
#define _USE_MATH_DEFINES
#include <math.h>
#pragma warning(disable : 4996)
#endif

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
	vizMgr->getVizWin(winnum)->getGLWindow()->setActiveParams(vizMgr->getParams(winnum, myParamsBaseType),myParamsBaseType);
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
	vizMgr->getVizWin(winnum)->getGLWindow()->setActiveParams(vizMgr->getParams(winnum,myParamsBaseType),myParamsBaseType);
}
void EventRouter::performGuiChangeInstance(int newCurrent, bool undoredo){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	if (winnum < 0) return;
	int instance = vizMgr->getCurrentInstanceIndex(winnum,myParamsBaseType);
	if (instance == newCurrent) return;
	if (undoredo){
		Params* rParams = vizMgr->getParams(winnum,myParamsBaseType);
		InstancedPanelCommand::capture(rParams, "change current render instance", instance, VAPoR::changeInstance, newCurrent);
	}
	changeRendererInstance(winnum, newCurrent);
	Params* p = vizMgr->getParams(winnum, myParamsBaseType);
	VizWin* vw = vizMgr->getVizWin(winnum);
	vw->getGLWindow()->setActiveParams(p,myParamsBaseType);
	vw->setColorbarDirty(true);
	vw->updateGL();
}
void EventRouter::performGuiNewInstance(){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int instance = vizMgr->getCurrentInstanceIndex(winnum,myParamsBaseType);
	Params* fParams = vizMgr->getParams(winnum,myParamsBaseType);
	InstancedPanelCommand::capture(fParams, "create new renderer instance", instance, VAPoR::newInstance);
	newRendererInstance(winnum);
}
void EventRouter::performGuiDeleteInstance(){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int instance = vizMgr->getCurrentInstanceIndex(winnum, myParamsBaseType);
	Params* rParams = vizMgr->getParams(winnum,myParamsBaseType, instance);
	InstancedPanelCommand::capture(rParams, "remove renderer instance", instance, VAPoR::deleteInstance);
	removeRendererInstance(winnum, instance);
	Params* p = vizMgr->getParams(winnum, myParamsBaseType);
	VizWin* vw = vizMgr->getVizWin(winnum);
	vw->getGLWindow()->setActiveParams(p,myParamsBaseType);
	vw->updateGL();
}
void EventRouter::performGuiCopyInstance(){
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	Params* rParams = vizMgr->getParams(winnum,myParamsBaseType);
	int currentInstance = vizMgr->getCurrentInstanceIndex(winnum, myParamsBaseType);
	InstancedPanelCommand::capture(rParams, "copy renderer instance", currentInstance, VAPoR::copyInstance);
	copyRendererInstance(winnum, (RenderParams*)rParams);
}
//Copy current instance to another visualizer
void EventRouter::performGuiCopyInstanceToViz(int towin){
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	Params* rParams = vizMgr->getParams(winnum,myParamsBaseType);
	int currentInstance = vizMgr->getCurrentInstanceIndex(winnum, myParamsBaseType);
	
	copyRendererInstance(towin, (RenderParams*)rParams);
	//put the copy-source in the command
	
	InstancedPanelCommand::capture(rParams, "copy renderer instance to viz", currentInstance, VAPoR::copyInstance, towin);
}

//This version of refreshHistogram is for a 3D variable, using the current, full region box.
void EventRouter::refreshHistogram(RenderParams* renParams, int varNum, const float dRange[2]){
	size_t min_dim[3],max_dim[3];
	
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
	int timeStep = vizWinMgr->getAnimationParams(vizNum)->getCurrentTimestep();
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
	int numTrans = renParams->GetRefinementLevel();
	int lod = renParams->GetCompressionLevel();
	const char* varname;
	
	varname = ds->getVariableName3D(varNum).c_str();
	
	int availRefLevel = rParams->getAvailableVoxelCoords(numTrans, lod, min_dim, max_dim, timeStep, &varNum, 1);
	if(availRefLevel < 0) {
		renParams->setBypass(timeStep);
		if (ds->warnIfDataMissing()){
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, 
				"Data unavailable for variable %s\nat refinement level %d",varname, numTrans);
		}
		return;
	}
	if (ds->useLowerAccuracy()){
		
		lod = Min(lod, ds->maxLODPresent3D(varNum, timeStep));
	}
		
	//Check if the region/resolution is too big:
	  double exts[6];
	  rParams->GetBox()->GetLocalExtents(exts, timeStep);
	  int numMBs = RegionParams::getMBStorageNeeded(exts, availRefLevel);
	  int cacheSize = DataStatus::getInstance()->getCacheMB();
	  if (numMBs > (int)(0.75*cacheSize)){
		  renParams->setAllBypass(true);
		  MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small\nfor current region and resolution.\n%s \n%s",
			  "Lower the refinement level,\nreduce the region size,\nor increase the cache size.",
			  "Rendering has been disabled.");
		  int instance = Params::GetCurrentParamsInstanceIndex(renParams->GetParamsBaseTypeId(),renParams->getVizNum());
		  assert(instance >= 0);
		  guiSetEnabled(false, instance);
		  updateTab();
		  return;
	  }
	
	//Now get the data:
	
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	size_t ts = (size_t)timeStep;
	RegularGrid* rg = dataMgr->GetGrid(
		ts, varname, availRefLevel, lod, min_dim, max_dim, 1
	);

	QApplication::restoreOverrideCursor();
	//Make sure we can build a histogram
	if (!rg) {
		dataMgr->SetErrCode(0);
		if (ds->warnIfDataMissing())
			MessageReporter::errorMsg("Invalid/nonexistent data;\ncannot be histogrammed");
		renParams->setBypass(timeStep);
		
		ds->setDataMissing3D(timeStep, availRefLevel, lod, varNum);
		return;
	}
	//Convert local extents to user extents
	const vector<double>& userExts = dataMgr->GetExtents(ts);
	for (int i = 0; i<3; i++) {
		exts[i] += userExts[i];
		exts[i+3] += userExts[i];
	}
	histogramList[varNum] = new Histo(rg, exts, dRange);
	dataMgr->UnlockGrid(rg);
	
}

//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
//Boolean flag is only used by isoeventrouter version
Histo* EventRouter::getHistogram(RenderParams* renParams, bool mustGet, bool){
	
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
void EventRouter::calcLODRefDefault(int dim, float regMBs, int* lodLevel, int* refLevel){
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	
	const vector<size_t> cratios = dataMgr->GetCRatios();
	int maxRefLevel = dataMgr->GetNumTransforms();

	
	Session* ses = Session::getInstance();
	float defaultRefLevel, defaultLOD;
	if (dim == 3){//adjust defaults based on region size and dimension
		defaultRefLevel = ses->getDefaultRefinementFidelity3D()-log(regMBs)/log(2.);
		defaultLOD = ses->getDefaultLODFidelity3D()*regMBs;
	} else {
		defaultRefLevel = ses->getDefaultRefinementFidelity2D()-log(regMBs)/log(2.);
		defaultLOD = ses->getDefaultLODFidelity2D()*regMBs;
	}
	if (defaultRefLevel < 0) defaultRefLevel = 0;
	if (defaultRefLevel > maxRefLevel) defaultRefLevel = maxRefLevel;
	//log(defaultLOD) is the default compression ratio
	if (defaultLOD < 1.e-20f) defaultLOD = 1.e-20f;
	float defCompRatio = log(defaultLOD);
	if (defCompRatio < (float)cratios[cratios.size()-1]) defCompRatio = (float)cratios[cratios.size()-1];
	if (defCompRatio > (float)cratios[0]) defCompRatio = (float)cratios[0];
	//Now find closest integer, round slightly up
	int intRefLevel = (int)(defaultRefLevel+0.1);
	int intCompLevel = 0;
	//find the index of the smallest compression ratio denominator that is greater than the default;
	for (int i = 0; i<cratios.size(); i++){
		if(cratios[i] >= (int)(defCompRatio+0.5f)) intCompLevel = i;
	}
	*refLevel = intRefLevel;
	*lodLevel = intCompLevel;

}

//Following constructs vectors of lods and refLevels that passes through the default lod/ref 
//Returns the default fidelity level.
int EventRouter::orderLODRefs(int dim){
	//Determine array of cratios and max Ref levels.
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return -1;
	//Determine the no. of megabytes in the full data volume
	size_t dims[3];
	dataMgr->GetDim(dims,-1);
	float fullMBs = (float)dims[0]*(float)dims[1]*(float)dims[2]*4./1.e6;
	const vector<size_t> cratios = dataMgr->GetCRatios();
	int maxRefLevel = dataMgr->GetNumTransforms();
	
	//Create vectors of fidelity, cratios, ref levels, initialize with mins and maxs and default
	fidelityRefinements.clear();
	fidelityLODs.clear();
	int defLODIndx, defref;
	calcLODRefDefault(dim, fullMBs, &defLODIndx, &defref);
	//Initialize arrays with min and max values
	bool oneButton = false;
	if (maxRefLevel == 0 && cratios.size()==1) oneButton = true;
	fidelityLODs.push_back(0);
	fidelityRefinements.push_back(0);
	

	if (oneButton) return 0;
	//Now step through the 2D array of refinements and lodLevels, from origin to defaults
	int currLodLevel = 0;
	int currRefLevel = 0;
	bool firstHalf = true;
	int goalLodLevel = defLODIndx;
	int goalRefLevel = defref;
	int defaultFidelity = -1;
	for (int i = 0; i< (maxRefLevel+cratios.size()-1); i++){
		int currDistSq = (goalLodLevel - currLodLevel)*(goalLodLevel - currLodLevel) + 
			(goalRefLevel - currRefLevel)*(goalRefLevel - currRefLevel);
		if (currDistSq == 0){//have attained initial goal, the default lod and refinements
			assert(firstHalf);
			firstHalf = false;
			defaultFidelity = i;
			goalLodLevel = cratios.size()-1;
			goalRefLevel = maxRefLevel;
		}
		//increment either lod (x) or refinement (y) and see which is closer to goal:
		int xdistsq = (goalLodLevel - currLodLevel-1)*(goalLodLevel - currLodLevel-1) + 
			(goalRefLevel - currRefLevel)*(goalRefLevel - currRefLevel);
		int ydistsq = (goalLodLevel - currLodLevel)*(goalLodLevel - currLodLevel) + 
			(goalRefLevel - currRefLevel-1)*(goalRefLevel - currRefLevel-1);
		if (xdistsq < ydistsq) currLodLevel++;
		else currRefLevel++;
		fidelityLODs.push_back(currLodLevel);
		fidelityRefinements.push_back(currRefLevel);
	}
	assert(currLodLevel == cratios.size()-1);
	assert(currRefLevel == maxRefLevel);
	
	if (firstHalf) defaultFidelity = fidelityLODs.size()-1;
	assert(fidelityLODs.size() == maxRefLevel+cratios.size());
	return defaultFidelity;
}

//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the DVR params
void EventRouter::
saveTF(RenderParams* rParams){
	if (!DataStatus::getInstance()->getDataMgr()) return;
	SaveTFDialog* saveTFDialog = new SaveTFDialog(rParams,0);
	int rc = saveTFDialog->exec();
	if (rc == 1) fileSaveTF(rParams);
}
void EventRouter::
fileSaveTF(RenderParams* rParams){
	//Launch a file save dialog, open resulting file
    QString s = QFileDialog::getSaveFileName(0,
        	"Choose a filename to save the transfer function",
		Session::getInstance()->getTFFilePath().c_str(),
                "Vapor Transfer Functions (*.vtf)");
	//Did the user cancel?
	if (s.length()== 0) return;
	//Force the name to end with .vtf
	if (!s.endsWith(".vtf")){
		s += ".vtf";
	}
	
	ofstream fileout;
	fileout.open((const char*)s.toAscii());
	if (! fileout) {
		QString str("Unable to save to file: \n");
		str += s;
		MessageReporter::errorMsg((const char*) str.toAscii());
		return;
	}
	
	
	if (!((TransferFunction*)(rParams->GetMapperFunc()))->saveToFile(fileout)){//Report error if can't save to file
		QString str("Failed to write output file: \n");
		str += s;
		MessageReporter::errorMsg((const char*)str.toAscii());
		fileout.close();
		return;
	}
	fileout.close();
	Session::getInstance()->updateTFFilePath(&s);
}
void EventRouter::
loadInstalledTF(RenderParams* rParams, int varnum){
	//Get the path from the environment:
	vector <string> paths;
	paths.push_back("palettes");
	string palettes = GetAppPath("VAPOR", "share", paths);

	QString installPath = palettes.c_str();
	fileLoadTF(rParams,varnum, (const char*) installPath.toAscii(),false);
}
void EventRouter::
loadTF(RenderParams* rParams, int varnum){
	//If there are no TF's currently in Session, just launch file load dialog.
	
	if (Session::getInstance()->getNumTFs() > 0){
		LoadTFDialog* loadTFDialog = new LoadTFDialog(this);
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
	if ((DataStatus::getInstance()->getNumSessionVariables() + DataStatus::getInstance()->getNumSessionVariables2D())<=0) return;
	
	//Open a file load dialog
	
    QString s = QFileDialog::getOpenFileName(0,
                    "Choose a transfer function file to open",
                    startPath,
                    "Vapor Transfer Functions (*.vtf)");
	//Null string indicates nothing selected.
	if (s.length() == 0) return;
	//Force the name to end with .vtf
	if (!s.endsWith(".vtf")){
		s += ".vtf";
	}
	
	ifstream is;
	
	is.open((const char*)s.toAscii());

	if (!is){//Report error if you can't open the file
		QString str("Unable to open file: \n");
		str+= s;
		MessageReporter::errorMsg((const char*)str.toAscii());
		return;
	}
	//Start the history save:
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "Load Transfer Function from file");
	
	TransferFunction* t = TransferFunction::loadFromFile(is, rParams);
	if (!t){//Report error if can't load
		QString str("Error loading transfer function. /nFailed to convert input file: \n ");
		str += s;
		MessageReporter::errorMsg((const char*)str.toAscii());
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

	
vector<QAction*>* EventRouter::makeWebHelpActions(const char* texts[], const char* urls[]){
	std::vector<QAction*>* actionVector = new std::vector<QAction*>;
	for (int i = 0;; i++){
		const char* currText = texts[i];
		if (strcmp(currText,"<>") == 0) break;
		QAction* currAction = new QAction(QString(currText),MainForm::getInstance());
		QUrl myqurl(urls[i]);
		QVariant qv(myqurl);
		currAction->setData(qv);
		actionVector->push_back(currAction);
	}
	return actionVector;
}
void EventRouter::updateFidelity(RenderParams* rp, QComboBox* lodCombo, QComboBox* refinementCombo){
	if (!rp->GetIgnoreFidelity()){
		
		int defIndx = rp->GetFidelityLevel();
		int lod = fidelityLODs[defIndx];
		int refLevel = fidelityRefinements[defIndx];
		lodCombo->setCurrentIndex(lod);
		refinementCombo->setCurrentIndex(refLevel);
		rp->SetRefinementLevel(refLevel);
		rp->SetCompressionLevel(lod);
		QRadioButton* activeButton = (QRadioButton*)fidelityButtons->button(defIndx);
		activeButton->setChecked(true);
		
	} else {
		int numRefs = rp->GetRefinementLevel();
		if(numRefs <= refinementCombo->count())
			refinementCombo->setCurrentIndex(numRefs);
		lodCombo->setCurrentIndex(rp->GetCompressionLevel());
	}
}
void EventRouter::setupFidelity(int dim, QHBoxLayout* fidelityLayout,
	QGroupBox* fidelityBox, RenderParams* dParams, bool useDefault){
	
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	const vector<size_t> cRatios = dataMgr->GetCRatios();
	int deflt = orderLODRefs(dim);
	int numButtons = fidelityLODs.size();
	QHBoxLayout* hbox = new QHBoxLayout;
	//fidelityLayout->removeWidget(fidelityBox);
	delete fidelityBox;// this also deletes buttongroup
	fidelityBox = new QGroupBox("low .. Fidelity .. high");
	fidelityButtons = new QButtonGroup(fidelityBox);
	for (int i = 0; i<numButtons; i++){
		QRadioButton * rd = new QRadioButton(fidelityBox);
		hbox->addWidget(rd);
		fidelityButtons->addButton(rd, i);
		QString refLevel = "Refinement "+QString::number(fidelityRefinements[i]);
		QString LOD = "\nLOD "+QString::number(cRatios[fidelityLODs[i]])+":1";
		QString tt = refLevel+LOD;
		rd->setToolTip(tt);
		if (i == deflt) rd->setChecked(true);
	}
	if(useDefault){
		//set the default fidelity to be closest to the preference setting, when initially setting up tab
		dParams->SetFidelityLevel(deflt);
	}
	if (dParams->GetFidelityLevel() >= fidelityLODs.size()) dParams->SetFidelityLevel(fidelityLODs.size()-1);
	fidelityBox->setToolTip("Click a button to specify the fidelity (both LOD and refinement)\n Each button has a tooltip indicating its associated LOD and refinement.");
	fidelityBox->setLayout(hbox);
	fidelityLayout->addWidget(fidelityBox);
	fidelityBox->adjustSize();
	return;
}
//User clicks on SetDefault button, need to make current fidelity settings the default.
void EventRouter::setFidelityDefault(int dim, RenderParams* dParams){
	
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	int lodSet = dParams->GetCompressionLevel();
	int refSet = dParams->GetRefinementLevel();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	float regionMBs = rParams->fullRegionMBs(-1);
	Session* ses = Session::getInstance();
	//Adjust lod, ref based on actual region size
	const vector<size_t> comprs = dataMgr->GetCRatios();
	float lodDefault = (float)comprs[lodSet]/regionMBs;
	float refDefault = (float)refSet+log(regionMBs)/log(2.);
	//Set defaultLOD3d and defaultRefinement3D values
	if (dim == 3){
		float oldLODDefault = ses->getDefaultLODFidelity3D();
		float oldRefDefault = ses->getDefaultRefinementFidelity3D();
		if (oldLODDefault != lodDefault || oldRefDefault != refDefault){
			ses->setDefaultLODFidelity3D(lodDefault);
			ses->setDefaultRefinementFidelity3D(refDefault);
			UserPreferences::requestSave();
		}
	} else {
		float oldLODDefault = ses->getDefaultLODFidelity2D();
		float oldRefDefault = ses->getDefaultRefinementFidelity2D();
		if (oldLODDefault != lodDefault || oldRefDefault != refDefault){
			ses->setDefaultLODFidelity2D(lodDefault);
			ses->setDefaultRefinementFidelity2D(refDefault);
			UserPreferences::requestSave();
		}
	}
	
	//Clear ignoreFidelity flag
	dParams->SetIgnoreFidelity(false);
	VizWinMgr::getInstance()->forceFidelityUpdate();
	//Need undo/redo to include preference settings!
}
