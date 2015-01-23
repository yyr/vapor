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
#include "glutil.h"
#include "eventrouter.h"
#include "params.h"
#include "renderer.h"
#include "histo.h"
#include "mapperfunction.h"
#include "mainform.h"
#include "vapor/ControlExecutive.h"
#include <qurl.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qmessagebox.h>
#include <qfileinfo.h>
#include <QFileDialog>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include "qbuttongroup.h"
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

//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
//Boolean flag is only used by isoeventrouter version
Histo* EventRouter::getHistogram(RenderParams* renParams, bool mustGet, bool){
	
	if (currentHistogram && !mustGet) return currentHistogram;
	if (!mustGet) return 0;
	string varname = renParams->GetVariableName();
	
	
	MapperFunction* mapFunc = renParams->GetMapperFunc();
	
	if (currentHistogram) delete currentHistogram;
	
	currentHistogram = new Histo(256,mapFunc->getMinMapValue(),mapFunc->getMaxMapValue());
	refreshHistogram(renParams);
	return currentHistogram;
	
}
void EventRouter::refresh2DHistogram(RenderParams* tParams){
	
	string varname = tParams->GetVariableName();
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;
	size_t timeStep = (size_t)VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	if(tParams->doBypass(timeStep)) return;
	float minRange = tParams->GetMapperFunc()->getMinMapValue();
	float maxRange = tParams->GetMapperFunc()->getMaxMapValue();
	if (!currentHistogram)
		currentHistogram = new Histo(256, minRange, maxRange);
	else currentHistogram->reset(256,minRange,maxRange);
	RegularGrid* histoGrid;
	int actualRefLevel = tParams->GetRefinementLevel();
	int lod = tParams->GetCompressionLevel();
	vector<string>varnames;
	varnames.push_back(varname);
	double exts[6];
	tParams->GetBox()->GetLocalExtents(exts);
	const vector<double>& userExts = dataMgr->GetExtents(timeStep);
	for (int i = 0; i<3; i++){
		exts[i]+=userExts[i];
		exts[i+3]+=userExts[i];
	}
	int rc = Renderer::getGrids( timeStep, varnames, exts, &actualRefLevel, &lod, &histoGrid);
	
	if(!rc) return;

	histoGrid->SetInterpolationOrder(0);	
	
	float v;
	RegularGrid *rg_const = (RegularGrid *) histoGrid;   
	RegularGrid::Iterator itr;
	
	for (itr = rg_const->begin(); itr!=rg_const->end(); ++itr) {
		v = *itr;
		if (v == histoGrid->GetMissingValue()) continue;
		currentHistogram->addToBin(v);
	}
	
	delete histoGrid;

}
//Calculate histogram for a planar slice of data, such as in
//the probe or the isolines.
void EventRouter::
calcSliceHistogram(RenderParams*p, int ts, Histo* histo){
	
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;

	if (p->doBypass(ts)) return;

	int actualRefLevel = p->GetRefinementLevel();
	int lod = p->GetCompressionLevel();
		
	const vector<double>&userExts = dataMgr->GetExtents((size_t)ts);
	RegularGrid* probeGrid;
	vector<string>varnames;
	varnames.push_back(p->GetVariableName());
	double extents[6];
	
	p->GetBox()->calcContainingStretchedBoxExtents(extents,true);

	for (int i = 0; i<3; i++){
		extents[i] += userExts[i];
		extents[i+3] += userExts[i];
	}
	
	int rc = Renderer::getGrids( ts, varnames, extents, &actualRefLevel, &lod, &probeGrid);
	
	if(!rc){
		return;
	}
	
	probeGrid->SetInterpolationOrder(0);

	double transformMatrix[12];
	//Set up to transform from probe into volume:
	p->GetBox()->buildLocalCoordTransform(transformMatrix, 0., -1);

	//Get the data dimensions (at this resolution):
	size_t dataSize[3];
	//Start by initializing extents
	
	dataMgr->GetDim(dataSize, actualRefLevel);

	const double* fullSizes = ds->getFullSizes();
	//Now calculate the histogram
	//
	//For each voxel, map it into the volume.
	
	
	//We first map the coords in the probe to the volume.  
	//Then we map the volume into the region provided by dataMgr
	
	double probeCoord[3];
	double dataCoord[3];
	
	float extExtents[6]; //Extend extents 1/2 voxel on each side so no bdry issues.
	for (int i = 0; i<3; i++){
		float mid = (fullSizes[i])*0.5;
		float halfExtendedSize = fullSizes[i]*0.5*(1.f+dataSize[i])/(float)(dataSize[i]);
		extExtents[i] = mid - halfExtendedSize;
		extExtents[i+3] = mid + halfExtendedSize;
	}
	
	// To determine the grid resolution to histogram, find out the change
	// in grid coordinate along each edge of the probe box.  Map each corner of the box to grid coordinates at current refinement level:
	// First map them to user coordinates, then convert these user coordinates to grid coordinates.

	//icor will contain the integer coordinates of each of the 8 corners of the probe box.
	int icor[8][3];
	for (int cornum = 0; cornum < 8; cornum++){
		// coords relative to (-1,1)
		probeCoord[2] = -1.f + 2.f*(float)(cornum/4);
		probeCoord[1] = -1.f + 2.f*(float)((cornum/2)%2);
		probeCoord[0] = -1.f + 2.f*(float)(cornum%2);
		//Then transform to values in data 
		vtransform(probeCoord, transformMatrix, dataCoord);
		//Then get array coords.
		// icor[k][dir] indicates the integer (data grid) coordinate of corner k along data grid axis dir
		for (int i = 0; i<3; i++){
			icor[cornum][i] = (size_t) (0.5f+(float)dataSize[i]*dataCoord[i]/fullSizes[i]);
		}
	}
	//Find the resolution along each axis of the probe
	//for each probe axis direction,  find the difference of each of the integer coordinates of the probe, across the probe in that direction.
	//Because the data is layered, try all 4 edges for each direction.  The various edges are identified by cornum increasing by 4,1,or 2 starting at
	//(0,1,2,3), (0,2,4,6), (0,1,4,5)
	// Once the fastest varying coordinate is known, subdivide that axis to match the resolution of that coordinate.
	// difference of a coordinate across a probe-axis direction is determined by the change in data-grid coordinates going from one face to the
	// opposite face of the probe.

	int gridRes[3] = {0,0,0};
	int difmax = -1;
	for (int dir = 0; dir<3;dir++){
		
		//four differences in the data-grid z direction, for data grid coordinate dir
		difmax = Max(difmax, abs(icor[0][dir]-icor[4][dir]));
		difmax = Max(difmax, abs(icor[1][dir]-icor[5][dir]));
		difmax = Max(difmax, abs(icor[2][dir]-icor[6][dir]));
		difmax = Max(difmax, abs(icor[3][dir]-icor[7][dir]));

	}
	gridRes[2] = difmax+1;
	difmax = -1;
	for (int dir = 0; dir<3;dir++){
		//four differences in the data-grid y direction
		difmax = Max(difmax, abs(icor[0][dir]-icor[2][dir]));
		difmax = Max(difmax, abs(icor[1][dir]-icor[3][dir]));
		difmax = Max(difmax, abs(icor[4][dir]-icor[6][dir]));
		difmax = Max(difmax, abs(icor[5][dir]-icor[7][dir]));
	}
	gridRes[1] = difmax+1;
	difmax = -1;
	for (int dir = 0; dir<3;dir++){
		//four differences in the data-grid x direction
		difmax = Max(difmax, abs(icor[0][dir]-icor[1][dir]));
		difmax = Max(difmax, abs(icor[2][dir]-icor[3][dir]));
		difmax = Max(difmax, abs(icor[4][dir]-icor[5][dir]));
		difmax = Max(difmax, abs(icor[6][dir]-icor[7][dir]));
	}
	gridRes[0] = difmax+1;
		
	//Now gridRes represents the number of samples to take in each direction across the probe
	//Use the region reader to calculate coordinates in volume

	//Loop over pixels in texture.  Pixel centers map to edges of probe
	for (int iz = 0; iz < gridRes[2]; iz++){
		if (gridRes[2] == 1) probeCoord[2] = 0.;
		else probeCoord[2] = -1. + 2.*iz/(float)(gridRes[2]-1);
		for (int iy = 0; iy < gridRes[1]; iy++){
			//Map iy to a value between -1 and 1
			if (gridRes[1] == 1) probeCoord[1] = 0.f; 
			else probeCoord[1] = -1.f + 2.f*(float)iy/(float)(gridRes[1]-1);
			for (int ix = 0; ix < gridRes[0]; ix++){
				if (gridRes[0] == 1) probeCoord[0] = 0.5f;
				else probeCoord[0] = -1.f + 2.f*(float)ix/(float)(gridRes[0]-1);
				vtransform(probeCoord, transformMatrix, dataCoord);
				//find the coords that the texture maps to
				//probeCoord is the coord in the probe, dataCoord is in data volume 
				bool dataOK = true;
				for (int i = 0; i< 3; i++){
					if (dataCoord[i] < extExtents[i] || dataCoord[i] > extExtents[i+3]) dataOK = false;
					dataCoord[i] += userExts[i]; //Convert to user coordinates.
				}
				float varVal;
				if(dataOK) { //find the coordinate in the data array
				
					varVal = probeGrid->GetValue(dataCoord[0],dataCoord[1],dataCoord[2]);
					if (varVal == probeGrid->GetMissingValue()) dataOK = false;
				}
				if (dataOK) {				
					// Add this sample to the histogram
					histo->addToBin(varVal);
				}
				// otherwise ignore this sample...
			}//End loop over ix
		}//End loop over iy
	}//End loop over iz;
	
	dataMgr->UnlockGrid(probeGrid);
	delete probeGrid;
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
void EventRouter::updateFidelity(QGroupBox* fidelityBox, RenderParams* rp, QComboBox* lodCombo, QComboBox* refinementCombo){
	QPalette pal = QPalette(fidelityBox->palette());
	if (!rp->GetIgnoreFidelity()){
		pal.setColor(QPalette::WindowText, Qt::black);
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
		pal.setColor(QPalette::WindowText, Qt::gray);
		int numRefs = rp->GetRefinementLevel();
		if(numRefs <= refinementCombo->count())
			refinementCombo->setCurrentIndex(numRefs);
		lodCombo->setCurrentIndex(rp->GetCompressionLevel());
	}
	fidelityBox->setPalette(pal);
}
void EventRouter::setupFidelity(int dim, QHBoxLayout* fidelityLayout,
	QGroupBox* fidelityBox, RenderParams* dParams, bool useDefault){
	
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	const vector<size_t> cRatios = dataMgr->GetCRatios();
	int deflt = orderLODRefs(dim);
	int numButtons = fidelityLODs.size();
	if (!fidelityButtons) fidelityButtons = new QButtonGroup(fidelityBox);
	QHBoxLayout* hlay = (QHBoxLayout*)fidelityBox->layout();
	if (!hlay) hlay = new QHBoxLayout(fidelityBox);
	hlay->setAlignment(Qt::AlignHCenter);

	QList<QAbstractButton*> btns = fidelityButtons->buttons();
	for (int i = 0; i<btns.size(); i++){
		fidelityButtons->removeButton(btns[i]);
		hlay->removeWidget(btns[i]);
		delete btns[i];
	}
	for (int i = 0; i<numButtons; i++){
		QRadioButton * rd = new QRadioButton(fidelityBox);
		hlay->addWidget(rd);
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
	RenderParams* defaultRenParams = (RenderParams*)Params::GetDefaultParams(dParams->GetParamsBaseTypeId());
	defaultRenParams->SetFidelityLevel(deflt);
	if (dParams->GetFidelityLevel() >= fidelityLODs.size()) dParams->SetFidelityLevel(fidelityLODs.size()-1);
	fidelityBox->adjustSize();
	return;
}
//User clicks on SetDefault button, need to make current fidelity settings the default.
void EventRouter::setFidelityDefault(int dim, RenderParams* dParams){
	/*  Can't do this until we have preferences....
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	int lodSet = dParams->GetCompressionLevel();
	int refSet = dParams->GetRefinementLevel();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	float regionMBs = rParams->fullRegionMBs(-1);
	
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
	*/
}
void EventRouter::calcLODRefDefault(int dim, float regMBs, int* lodLevel, int* refLevel){
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	
	const vector<size_t> cratios = dataMgr->GetCRatios();
	int maxRefLevel = dataMgr->GetNumTransforms();

	
	DataStatus* ds = DataStatus::getInstance();
	float defaultRefLevel, defaultLOD;
	if (dim == 3){//adjust defaults based on region size and dimension
		defaultRefLevel = ds->getDefaultRefinementFidelity3D()-log(regMBs)/log(2.);
		defaultLOD = ds->getDefaultLODFidelity3D()*regMBs;
	} else {
		defaultRefLevel = ds->getDefaultRefinementFidelity2D()-log(regMBs)/log(2.);
		defaultLOD = ds->getDefaultLODFidelity2D()*regMBs;
	}
	if (defaultRefLevel < 0) defaultRefLevel = 0;
	if (defaultRefLevel > maxRefLevel) defaultRefLevel = maxRefLevel;
	//defaultLOD is the default compression ratio
	if (defaultLOD < 1) defaultLOD = 1.;
	
	if (defaultLOD < (float)cratios[cratios.size()-1]) defaultLOD = (float)cratios[cratios.size()-1];
	if (defaultLOD > (float)cratios[0]) defaultLOD = (float)cratios[0];
	//Now find closest integer, round slightly up
	int intRefLevel = (int)(defaultRefLevel+0.1);
	int intCompLevel = 0;
	//find the index of the smallest compression ratio denominator that is closest to the default;
	float dist=1.e30f;
	for (int i = 0; i<cratios.size(); i++){
		if (abs((float)cratios[i] - defaultLOD) < dist) {
			dist = abs((float)cratios[i] - defaultLOD);
			intCompLevel = i;
		}
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
