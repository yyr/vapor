//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		regionparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Implements the RegionParams class.
//		This class supports parameters associted with the
//		region panel
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include "regionparams.h"
#include "regiontab.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "tabmanager.h"
#include "glutil.h"
#include "messagereporter.h"

using namespace VAPoR;

const string RegionParams::_regionCenterTag = "RegionCenter";
const string RegionParams::_regionSizeTag = "RegionSize";
const string RegionParams::_maxSizeAttr = "MaxSizeSlider";
const string RegionParams::_numTransAttr = "NumTrans";

RegionParams::RegionParams(int winnum): Params(winnum){
	thisParamType = RegionParamsType;
	myRegionTab = MainForm::getInstance()->getRegionTab();
	restart();
}
Params* RegionParams::
deepCopy(){
	//Just make a shallow copy, since there's nothing (yet) extra
	//
	RegionParams* p = new RegionParams(*this);
	//never keep the SavedCommand:
	p->savedCommand = 0;
	return (Params*)(p);
}

RegionParams::~RegionParams(){
	if (savedCommand) delete savedCommand;
}
void RegionParams::
makeCurrent(Params* ,bool) {
	
	VizWinMgr::getInstance()->setRegionParams(vizNum, this);
	//Also update current Tab.  It's probably visible.
	//
	updateDialog();
	setDirty();

}

//Insert values from params into tab panel
//
void RegionParams::updateDialog(){
	
	
	QString strng;
	Session::getInstance()->blockRecording();
	myRegionTab->numTransSpin->setMaxValue(maxNumTrans);
	myRegionTab->numTransSpin->setMinValue(minNumTrans);
	myRegionTab->numTransSpin->setValue(numTrans);
	int dataMaxSize = Max(fullSize[0],Max(fullSize[1],fullSize[2]));
	myRegionTab->maxSizeSlider->setMaxValue(dataMaxSize);
	myRegionTab->maxSizeSlider->setValue(maxSize);
	myRegionTab->maxSizeEdit->setText(strng.setNum(maxSize));
	
	myRegionTab->xCenterSlider->setMaxValue(fullSize[0]);
	myRegionTab->yCenterSlider->setMaxValue(fullSize[1]);
	myRegionTab->zCenterSlider->setMaxValue(fullSize[2]);
	myRegionTab->xSizeSlider->setMaxValue(fullSize[0]);
	myRegionTab->ySizeSlider->setMaxValue(fullSize[1]);
	myRegionTab->zSizeSlider->setMaxValue(fullSize[2]);

	//Enforce the requirement that center +- size/2 fits in range
	//
	for (int i = 0; i<3; i++){
		if(centerPosition[i] + (1+regionSize[i])/2 >= fullSize[i])
			centerPosition[i] = fullSize[i] - (1+regionSize[i])/2 -1;
		if(centerPosition[i] < (1+regionSize[i])/2)
			centerPosition[i] = (1+regionSize[i])/2;
		setCurrentExtents(i);
	}

	myRegionTab->xCntrEdit->setText(strng.setNum(centerPosition[0]));
	myRegionTab->xCenterSlider->setValue(centerPosition[0]);
	myRegionTab->yCntrEdit->setText(strng.setNum(centerPosition[1]));
	myRegionTab->yCenterSlider->setValue(centerPosition[1]);
	myRegionTab->zCntrEdit->setText(strng.setNum(centerPosition[2]));
	myRegionTab->zCenterSlider->setValue(centerPosition[2]);

	myRegionTab->xSizeEdit->setText(strng.setNum(regionSize[0]));
	myRegionTab->xSizeSlider->setValue(regionSize[0]);
	myRegionTab->ySizeEdit->setText(strng.setNum(regionSize[1]));
	myRegionTab->ySizeSlider->setValue(regionSize[1]);
	myRegionTab->zSizeEdit->setText(strng.setNum(regionSize[2]));
	myRegionTab->zSizeSlider->setValue(regionSize[2]);

	myRegionTab->minXFull->setText(strng.setNum(fullDataExtents[0]));
	myRegionTab->minYFull->setText(strng.setNum(fullDataExtents[1]));
	myRegionTab->minZFull->setText(strng.setNum(fullDataExtents[2]));
	myRegionTab->maxXFull->setText(strng.setNum(fullDataExtents[3]));
	myRegionTab->maxYFull->setText(strng.setNum(fullDataExtents[4]));
	myRegionTab->maxZFull->setText(strng.setNum(fullDataExtents[5]));
	
	
	

	if (isLocal())
		myRegionTab->LocalGlobal->setCurrentItem(1);
	else 
		myRegionTab->LocalGlobal->setCurrentItem(0);
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
}

//Update all the panel state associated with textboxes.
//Then enforce consistency requirements
//Whenever this affects a slider, move it.
//
void RegionParams::
updatePanelState(){
	
	
	centerPosition[0] = myRegionTab->xCntrEdit->text().toFloat();
	centerPosition[1] = myRegionTab->yCntrEdit->text().toFloat();
	centerPosition[2] = myRegionTab->zCntrEdit->text().toFloat();
	regionSize[0] = myRegionTab->xSizeEdit->text().toFloat();
	regionSize[1] = myRegionTab->ySizeEdit->text().toFloat();
	regionSize[2] = myRegionTab->zSizeEdit->text().toFloat();
	int oldSize = maxSize;
	maxSize = myRegionTab->maxSizeEdit->text().toInt();
	//If maxSize changed, need to attempt to adjust everything:
	//If this is an increase, try to increase other sliders:
	//
	bool rgChanged[3];
	rgChanged[0]=rgChanged[1]=rgChanged[2]=false;
	if (oldSize < maxSize){
		for (int i = 0; i< 3; i++) {
			if (regionSize[i] < maxSize) {
				regionSize[i] = maxSize;
				rgChanged[i]=true;
			}
		}
	} else if (maxSize < oldSize) { //likewise for decrease:
		for (int i = 0; i< 3; i++) {
			if (regionSize[i] > maxSize){ 
				regionSize[i] = maxSize;
				rgChanged[i]=true;
			}
		}
	}
	for(int i = 0; i< 3; i++)
		if(enforceConsistency(i))rgChanged[i]=true;
	
	if(myRegionTab->xCenterSlider->value() != centerPosition[0])
		myRegionTab->xCenterSlider->setValue(centerPosition[0]);
	if(myRegionTab->yCenterSlider->value() != centerPosition[1])
		myRegionTab->yCenterSlider->setValue(centerPosition[1]);
	if(myRegionTab->zCenterSlider->value() != centerPosition[2])
		myRegionTab->zCenterSlider->setValue(centerPosition[2]);
	if (myRegionTab->xSizeSlider->value() != regionSize[0])
		myRegionTab->xSizeSlider->setValue(regionSize[0]);
	if (myRegionTab->ySizeSlider->value() != regionSize[1])
		myRegionTab->ySizeSlider->setValue(regionSize[1]);
	if (myRegionTab->zSizeSlider->value() != regionSize[2])
		myRegionTab->zSizeSlider->setValue(regionSize[2]);
	if (myRegionTab->maxSizeSlider->value() != maxSize)
		myRegionTab->maxSizeSlider->setValue(maxSize);
	if (rgChanged[0]){
			myRegionTab->xCntrEdit->setText(QString::number(centerPosition[0]));
			myRegionTab->xSizeEdit->setText(QString::number(regionSize[0]));
			setCurrentExtents(0);
	}
	if (rgChanged[1]){
			myRegionTab->yCntrEdit->setText(QString::number(centerPosition[1]));
			myRegionTab->ySizeEdit->setText(QString::number(regionSize[1]));
			setCurrentExtents(1);
	}
	if (rgChanged[2]){
			myRegionTab->zCntrEdit->setText(QString::number(centerPosition[2]));
			myRegionTab->zSizeEdit->setText(QString::number(regionSize[2]));
			setCurrentExtents(2);
	}
	setDirty();
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
}
//Must Set this region as well as any others that are sharing
//RegionDirty refers to the latest region settings not yet visualized.
//
void RegionParams::
setDirty(){
	VizWinMgr::getInstance()->setRegionDirty(this);
}
//Method to make center and size values legitimate for specified dimension
//Returns true if anything changed.
//ignores maxSize, since it's just a guideline.
//
bool RegionParams::
enforceConsistency(int dim){
	bool rc = false;
	//The region size must be positive, i.e. at least
	//2 in transformed coords
	if (regionSize[dim]< (1<<(numTrans+1)))
		regionSize[dim] = (1<<(numTrans+1));
	if (regionSize[dim]>fullSize[dim]) {
		regionSize[dim] =fullSize[dim]; 
		rc = true;
	}
	if (regionSize[dim] > maxSize) {
		maxSize = regionSize[dim]; 
	}
	if (centerPosition[dim]<(1+regionSize[dim])/2) {
		centerPosition[dim]= (1+regionSize[dim])/2;
		rc = true;
	}
	if (centerPosition[dim]+(1+regionSize[dim])/2 > fullSize[dim]){
		centerPosition[dim]= fullSize[dim]-(1+regionSize[dim])/2;
		rc = true;
	}
	return rc;
}
//Methods in support of undo/redo:
//

void RegionParams::
guiSetNumTrans(int n){
	confirmText(false);
	
	int newNumTrans = validateNumTrans(n);
	if (newNumTrans != n) {
		MessageReporter::warningMsg("%s","Invalid number of Transforms for current region, data cache size");
		myRegionTab->numTransSpin->setValue(numTrans);
	}
	PanelCommand* cmd = PanelCommand::captureStart(this, "set number of Transformations");
	setNumTrans(newNumTrans);
	PanelCommand::captureEnd(cmd, this);
}
//See if the number of trans is ok.  If not, return an OK value
int RegionParams::
validateNumTrans(int n){
	//if we have a dataMgr, see if we can really handle this new numtrans:
	if (!Session::getInstance()->getDataMgr()) return n;
	
	int min_dim[3], max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	float minFull[3], maxFull[3], extents[6];
	calcRegionExtents(min_dim, max_dim, min_bdim, max_bdim, n, minFull, maxFull,  extents);
	//Size needed for data assumes blocksize = 2**5, 6 bytes per voxel, times 2.
	size_t newFullMB = (max_bdim[0]-min_bdim[0]+1)*(max_bdim[1]-min_bdim[1]+1)*(max_bdim[2]-min_bdim[2]+1);
	//right shift by 20 for megavoxels
	newFullMB >>= 20;
	//Multiply by 6 for 6 bytes per voxel
	newFullMB *= 6;
	while (newFullMB >= Session::getInstance()->getCacheMB()){
		//find  and return a legitimate value.  Each time we increase n by 1,
		//we decrease the size needed by 8
		n--;
		newFullMB >>= 3;
	}	
	return n;
}
	//If we passed that test, then go ahead and change the numTrans.

//Move the region center to specified coords, shrink it if necessary
void RegionParams::
guiSetCenter(float* coords){
	PanelCommand* cmd = PanelCommand::captureStart(this,  "move region center");
	for (int i = 0; i< 3; i++){
		float coord = coords[i];
		float fullMin = getFullDataExtent(i);
		float fullMax = getFullDataExtent(i+3);
		if (coord < fullMin) coord = fullMin;
		if (coord > fullMax) coord = fullMax;
		float regSize = getRegionMax(i) - getRegionMin(i);
		if (coord + 0.5f*regSize > fullMax) regSize = 2.f*(fullMax - coord);
		if (coord - 0.5f*regSize < fullMin) regSize = 2.f*(coord - fullMin);
		setRegionMax(i, coord + 0.5f*regSize);
		setRegionMin(i, coord - 0.5f*regSize);
	}
	PanelCommand::captureEnd(cmd, this);
	VizWinMgr::getInstance()->setRegionDirty(this);
}
//Following are set when slider is released:
//
void RegionParams::
guiSetXCenter(int n){
	confirmText(false);
	
	//Was there a change?
	//
	if (n!= centerPosition[0]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, "change region x-center");
		centerPosition[0] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(0)){
			myRegionTab->xCenterSlider->setValue(centerPosition[0]);
		}
		myRegionTab->xCntrEdit->setText(QString::number(centerPosition[0]));
		setCurrentExtents(0);
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd,this);
	}
	setDirty();
}

void RegionParams::
guiSetXSize(int n){
	confirmText(false);
	
	//Was there a change?
	if (n!= regionSize[0]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, "change region x-size");
		regionSize[0] = n;
		//If new position is invalid, move the slider back where it belongs, and
		//potentially move other slider as well
		//
		if (enforceConsistency(0)){
			myRegionTab->xSizeSlider->setValue(regionSize[0]);
			myRegionTab->xCenterSlider->setValue(centerPosition[0]);
			myRegionTab->xCntrEdit->setText(QString::number(centerPosition[0]));
		}
		myRegionTab->xSizeEdit->setText(QString::number(regionSize[0]));
		setCurrentExtents(0);
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd,this);
	}
	setDirty();
}
void RegionParams::
guiSetYCenter(int n){
	confirmText(false);
	
	//Was there a change?
	//
	if (n!= centerPosition[1]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, "change region y-center");
		centerPosition[1] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(1)){
			myRegionTab->yCenterSlider->setValue(centerPosition[1]);
		}
		myRegionTab->yCntrEdit->setText(QString::number(centerPosition[1]));
		setCurrentExtents(1);
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd,this);
	}
	setDirty();
}

void RegionParams::
guiSetYSize(int n){
	confirmText(false);
	
	//Was there a change?
	if (n!= regionSize[1]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, "change region y-size");
		regionSize[1] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(1)){
			myRegionTab->ySizeSlider->setValue(regionSize[1]);
			myRegionTab->yCenterSlider->setValue(centerPosition[1]);
			myRegionTab->yCntrEdit->setText(QString::number(centerPosition[1]));
		}
		myRegionTab->ySizeEdit->setText(QString::number(regionSize[1]));
		setCurrentExtents(1);
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd, this);
	}
	setDirty();
}
void RegionParams::
guiSetZCenter(int n){
	confirmText(false);
	
	//Was there a change?
	//
	if (n!= centerPosition[2]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, "change region z-center");
		centerPosition[2] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(2)){
			myRegionTab->zCenterSlider->setValue(centerPosition[2]);
		}
		myRegionTab->zCntrEdit->setText(QString::number(centerPosition[2]));
		setCurrentExtents(2);
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd, this);
	}
	setDirty();
}

void RegionParams::
guiSetZSize(int n){
	confirmText(false);
	
	//Was there a change?
	if (n!= regionSize[2]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, "change region z-size");
		regionSize[2] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(2)){
			myRegionTab->zSizeSlider->setValue(regionSize[2]);
			myRegionTab->zCenterSlider->setValue(centerPosition[2]);
			myRegionTab->zCntrEdit->setText(QString::number(centerPosition[2]));
		}
		myRegionTab->zSizeEdit->setText(QString::number(regionSize[2]));
		setCurrentExtents(2);
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd, this);
	}
	setDirty();
}
void RegionParams::
guiSetMaxSize(int n){
	confirmText(false);
	int oldSize = maxSize;
	bool didChange[3];
	//Was there a change?
	//
	if (n!= maxSize) {
		PanelCommand* cmd = PanelCommand::captureStart(this, "change region Max Size");
		maxSize = n;
		didChange[0] = didChange[1] = didChange[2] = false;
		//If this is an increase, try to increase other sliders:
		//
		if (oldSize < maxSize){
			for (int i = 0; i< 3; i++) {
				if (regionSize[i] < maxSize) {
					regionSize[i] = maxSize;
					didChange[i] = true;
				}
			}
		} else { //likewise for decrease:
			for (int i = 0; i< 3; i++) {
				if (regionSize[i] > maxSize){ 
					regionSize[i] = maxSize;
					didChange[i] = true;
				}
			}
		}
		//Then enforce consistency:
		//
		if (enforceConsistency(0)||didChange[0]){
			myRegionTab->xSizeSlider->setValue(regionSize[0]);
			myRegionTab->xCenterSlider->setValue(centerPosition[0]);
			myRegionTab->xSizeEdit->setText(QString::number(regionSize[0]));
			myRegionTab->xCntrEdit->setText(QString::number(centerPosition[0]));
			setCurrentExtents(0);
		}
		if (enforceConsistency(1)||didChange[1]){
			myRegionTab->ySizeSlider->setValue(regionSize[1]);
			myRegionTab->yCenterSlider->setValue(centerPosition[1]);
			myRegionTab->ySizeEdit->setText(QString::number(regionSize[1]));
			myRegionTab->yCntrEdit->setText(QString::number(centerPosition[1]));
			setCurrentExtents(1);
		}
		if (enforceConsistency(2)||didChange[2]){
			myRegionTab->zSizeSlider->setValue(regionSize[2]);
			myRegionTab->zSizeEdit->setText(QString::number(regionSize[2]));
			myRegionTab->zCenterSlider->setValue(centerPosition[2]);
			myRegionTab->zCntrEdit->setText(QString::number(centerPosition[2]));
			setCurrentExtents(2);
		}
		
		myRegionTab->maxSizeEdit->setText(QString::number(maxSize));
		//Ignore the resulting textChanged events:
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd, this);
	}
	setDirty();
}
//Reset region settings to initial state
void RegionParams::
restart(){
	numTrans = 0;
	maxNumTrans = 9;
	minNumTrans = 0;
	maxSize = 1024;
	
	
	savedCommand = 0;
	for (int i = 0; i< 3; i++){
		centerPosition[i] = 512;
		regionSize[i] = 1024;
		fullSize[i] = 1024;
		fullDataExtents[i] = Session::getInstance()->getExtents(i);
		fullDataExtents[i+3] = Session::getInstance()->getExtents(i+3);
	}
	//If this params is currently being displayed, 
	//force the current displayed tab to be reset to values
	//consistent with the params
	//
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myRegionTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getRegionParams(viznum)))
			updateDialog();
	}
}
//Reinitialize region settings, session has changed:
void RegionParams::
reinit(bool doOverride){
	int i;
	const Metadata* md = Session::getInstance()->getCurrentMetadata();
	//Setup the global region parameters based on bounds in Metadata
	const size_t* dataDim = md->GetDimension();
	
	//Note:  It's OK to cast to int here:
	int nx = (int)dataDim[0];
	int ny = (int)dataDim[1];
	int nz = (int)dataDim[2];
	
	setFullSize(0, nx);
	setFullSize(1, ny);
	setFullSize(2, nz);
	int nlevels = md->GetNumTransforms();
	int minTrans = Session::getInstance()->getDataStatus()->minXFormPresent();
	if(minTrans < 0) minTrans = 0; 
	int maxTrans = nlevels;
	setMinNumTrans(minTrans);
	setMaxNumTrans(maxTrans);
	if (doOverride) {
		//Start with minTrans, make sure it works with current data
		numTrans = minTrans;
		numTrans = validateNumTrans(numTrans);
		for (i = 0; i< 3; i++) {
			setRegionSize(i,fullSize[i]);
		}
	} else {
		//Make sure the numTrans value is available in this dataset
		if (numTrans > maxNumTrans) numTrans = maxNumTrans;
		if (numTrans < minNumTrans) numTrans = minNumTrans;
		//Make sure we really can use the specified numTrans.
		numTrans = validateNumTrans(numTrans);
		for (i = 0; i< 3; i++) enforceConsistency(i);
	}
	
	//Data extents (user coords) are presented read-only in gui
	//
	setDataExtents(Session::getInstance()->getExtents());
	//If this params is currently being displayed, 
	//force the current displayed tab to be reset to values
	//consistent with the params
	//
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myRegionTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getRegionParams(viznum)))
			updateDialog();
	}
	setDirty();
}
void RegionParams::
setCurrentExtents(int coord){
	double regionMin, regionMax;
	
	regionMin = fullDataExtents[coord] + (fullDataExtents[coord+3]-fullDataExtents[coord])*
		(centerPosition[coord] - regionSize[coord]*.5)/(double)(fullSize[coord]);
	regionMax = fullDataExtents[coord] + (fullDataExtents[coord+3]-fullDataExtents[coord])*
		(centerPosition[coord] + regionSize[coord]*.5)/(double)(fullSize[coord]);
	switch (coord){
		case(0):
			myRegionTab->minXRegion->setText(QString::number(regionMin));
			myRegionTab->maxXRegion->setText(QString::number(regionMax));
			break;
		case (1):
			myRegionTab->minYRegion->setText(QString::number(regionMin));
			myRegionTab->maxYRegion->setText(QString::number(regionMax));
			break;
		case (2):
			myRegionTab->minZRegion->setText(QString::number(regionMin));
			myRegionTab->maxZRegion->setText(QString::number(regionMax));
			break;
		default:
			assert(0);
	}
}
void RegionParams::setRegionMin(int coord, float minval){
	int regionCrd = (int)(fullSize[coord]*((minval - fullDataExtents[coord])/(fullDataExtents[coord+3]- fullDataExtents[coord])));
	if (regionCrd < 0) regionCrd = 0;
	if (regionCrd >= centerPosition[coord]+regionSize[coord]/2)
		regionCrd = centerPosition[coord]+regionSize[coord]/2 -1;
	int newSize = centerPosition[coord]+regionSize[coord]/2 - regionCrd;
	int newCenter = centerPosition[coord]+regionSize[coord]/2 - newSize/2;
	centerPosition[coord] = newCenter;
	regionSize[coord] = newSize;
}
void RegionParams::setRegionMax(int coord, float maxval){
	int regionCrd = (int)(fullSize[coord]*((maxval - fullDataExtents[coord])/(fullDataExtents[coord+3]- fullDataExtents[coord])));
	if (regionCrd > fullSize[coord]) regionCrd = fullSize[coord];
	if (regionCrd <= centerPosition[coord]-regionSize[coord]/2)
		regionCrd = centerPosition[coord]-regionSize[coord]/2 +1;
	int newSize = regionCrd - (centerPosition[coord]-regionSize[coord]/2);
	int newCenter = centerPosition[coord]-regionSize[coord]/2 + newSize/2;
	centerPosition[coord] = newCenter;
	regionSize[coord] = newSize;
}

//Save undo/redo state when user grabs a rake handle
//
void RegionParams::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(this,  "slide region handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshRegion(this);
}
void RegionParams::
captureMouseUp(){
	//Update the tab if it's in front:
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myRegionTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getRegionParams(viznum)))
			updateDialog();
	}
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, this);
	//Set region  dirty
	setDirty();
	savedCommand = 0;
	//Force a rerender:
	VizWinMgr::getInstance()->refreshRegion(this);
	
}

//Utility to calculate extents based on proposed numtransforms.
//Uses current region size and center.  Does not change regionParams state.
//Requires an existing dataMgr
void RegionParams::
calcRegionExtents(int min_dim[3], int max_dim[3], size_t min_bdim[3], size_t max_bdim[3], 
				  int numxforms, float minFull[3], float maxFull[3], float extents[6])
{
	int i;
	float fullExtent[6];
	const size_t *bs = Session::getInstance()->getCurrentMetadata()->GetBlockSize();
	for(i=0; i<3; i++) {
		int	s = getMaxNumTrans()-numxforms;
		min_dim[i] = (int) ((float) (getCenterPosition(i) >> s) - 0.5 
			- (((getRegionSize(i) >> s) / 2.0)-1.0));
		max_dim[i] = (int) ((float) (getCenterPosition(i) >> s) - 0.5 
			+ (((getRegionSize(i) >> s) / 2.0)));
		//Force these to be in data:
		int dim = (getFullSize(i)>>(getMaxNumTrans()-numxforms)) -1;
		if (max_dim[i] > dim) max_dim[i] = dim;
		//Make sure slab has nonzero thickness (this can only
		//be a problem while the mouse is pressed):
		//
		if (min_dim[i] >= max_dim[i]){
			if (max_dim[i] < 1){
				max_dim[i] = 1;
				min_dim[i] = 0;
			}
			else min_dim[i] = max_dim[i] - 1;
		}
		min_bdim[i] = min_dim[i] / bs[i];
		max_bdim[i] = max_dim[i] / bs[i];
	}
	
	for (i = 0; i< 3; i++){
		fullExtent[i] = getFullDataExtent(i);
		fullExtent[i+3] = getFullDataExtent(i+3);
	}
	float maxCoordRange = Max(fullExtent[3]-fullExtent[0],Max( fullExtent[4]-fullExtent[1], fullExtent[5]-fullExtent[2]));
	//calculate the geometric extents of the dimensions in the unit cube:
	//fit the full region adjacent to the coordinate planes.
	for (i = 0; i<3; i++) {
		int dim = (getFullSize(i)>>(getMaxNumTrans()-numxforms)) -1;
		assert (dim >= max_dim[i]);
		float extentRatio = (fullExtent[i+3]-fullExtent[i])/maxCoordRange;
		minFull[i] = 0.f;
		maxFull[i] = extentRatio;
		
		extents[i] = minFull[i] + ((float)min_dim[i]/(float)dim)*(maxFull[i]-minFull[i]);
		extents[i+3] = minFull[i] + ((float)max_dim[i]/(float)dim)*(maxFull[i]-minFull[i]);
	}
	return;
}
bool RegionParams::
elementStartHandler(ExpatParseMgr* pm, int /* depth*/ , std::string& tagString, const char **attrs){
	if (StrCmpNoCase(tagString, _regionParamsTag) == 0) {
		//If it's a region tag, save 4 attributes (2 are from Params class)
		//Do this by repeatedly pulling off the attribute name and value
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _vizNumAttr) == 0) {
				ist >> vizNum;
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				if (value == "true") setLocal(true); else setLocal(false);
			}
			else if (StrCmpNoCase(attribName, _maxSizeAttr) == 0) {
				ist >> maxSize;
			}
			else if (StrCmpNoCase(attribName, _numTransAttr) == 0) {
				ist >> numTrans;
			}
			else return false;
		}
		return true;
	}
	//prepare for center position, regionSize nodes
	else if ((StrCmpNoCase(tagString, _regionCenterTag) == 0) ||
		(StrCmpNoCase(tagString, _regionSizeTag) == 0) ) {
		//Should have a long type attribute
		string attribName = *attrs;
		attrs++;
		string value = *attrs;

		ExpatStackElement *state = pm->getStateStackTop();
		
		state->has_data = 1;
		if (StrCmpNoCase(attribName, _typeAttr) != 0) {
			pm->parseError("Invalid attribute : %s", attribName.c_str());
			return false;
		}
		if (StrCmpNoCase(value, _longType) != 0) {
			pm->parseError("Invalid type : %s", value.c_str());
			return false;
		}
		state->data_type = value;
		return true;  
	}
	else return false;
}
bool RegionParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	if (StrCmpNoCase(tag, _regionParamsTag) == 0) {
		//If this is a regionparams, need to
		//pop the parse stack.  
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, _regionCenterTag) == 0){
		vector<long> posn = pm->getLongData();
		centerPosition[0] = posn[0];
		centerPosition[1] = posn[1];
		centerPosition[2] = posn[2];
		return true;
	} else if (StrCmpNoCase(tag, _regionSizeTag) == 0){
		vector<long> sz = pm->getLongData();
		regionSize[0] = sz[0];
		regionSize[1] = sz[1];
		regionSize[2] = sz[2];
		return true;
	}
	else {
		pm->parseError("Unrecognized end tag in RegionParams %s",tag.c_str());
		return false;  
	}
}
XmlNode* RegionParams::
buildNode(){
	//Construct the region node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	if (local)
		oss << "true";
	else 
		oss << "false";
	attrs[_localAttr] = oss.str();

	oss.str(empty);
	oss << (long)numTrans;
	attrs[_numTransAttr] = oss.str();

	oss.str(empty);
	oss << (long)maxSize;
	attrs[_maxSizeAttr] = oss.str();

	XmlNode* regionNode = new XmlNode(_regionParamsTag, attrs, 2);

	//Now add children:  
	
	vector<long> longvec;
	int i;
	longvec.clear();
	for (i = 0; i< 3; i++){
		longvec.push_back((long) centerPosition[i]);
	}
	regionNode->SetElementLong(_regionCenterTag,longvec);
	longvec.clear();
	for (i = 0; i< 3; i++){
		longvec.push_back((long) regionSize[i]);
	}
	regionNode->SetElementLong(_regionSizeTag, longvec);	
		
	return regionNode;
}
