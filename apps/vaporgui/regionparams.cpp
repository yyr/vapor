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
#include "params.h"
#include "vapor/Metadata.h"
#include "tabmanager.h"
#include "glutil.h"
#include "messagereporter.h"

using namespace VAPoR;

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
	myRegionTab->numTransSpin->setMinValue(minNumTrans);
	myRegionTab->numTransSpin->setMaxValue(maxNumTrans);
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
//Ignores MaxSize, since it's just a guideline.
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
guiCenterFull(ViewpointParams* vpparams){
	confirmText(false);
	//Undo/redo is done with vpparams
	vpparams->guiCenterFullRegion(this);
}
void RegionParams::
guiCenterRegion(ViewpointParams* vpparams){
	confirmText(false);
	//Undo/redo is done with vpparams
	vpparams->guiCenterSubRegion(this);
}
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
	//Size needed for data assumes blocksize = 2**5, 4 bytes per voxel, times 2.
	size_t newFullMB = (max_bdim[0]-min_bdim[0]+1)*(max_bdim[1]-min_bdim[1]+1)*(max_bdim[2]-min_bdim[2]+1);
	//Left shift by 18, for bytes and  right shift by 20 for mbytes
	newFullMB >>= 2;
	while (newFullMB >= Session::getInstance()->getCacheMB()){
		//find  and return a legitimate value.  Each time we increase n by 1,
		//we decrease the size needed by 8
		n++;
		newFullMB >>= 3;
	}	
	return n;
}
	//If we passed that test, then go ahead and change the numTrans.
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
	numTrans = 5;
	maxNumTrans = 9;
	minNumTrans = 0;
	maxSize = 256;
	fullDataExtents.resize(6);
	selectedFaceNum = -1;
	faceDisplacement = 0.f;
	savedCommand = 0;
	for (int i = 0; i< 3; i++){
		centerPosition[i] = 128;
		regionSize[i] = 256;
		fullSize[i] = 256;
		fullDataExtents[i] = 0.;
		fullDataExtents[i+3] = 1.;
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
	setMaxSize(Max(Max(nx, ny), nz));
	setFullSize(0, nx);
	setFullSize(1, ny);
	setFullSize(2, nz);
	int nlevels = md->GetNumTransforms();
	
	setMinNumTrans(Session::getInstance()->getDataStatus()->minXFormPresent());
	setMaxNumTrans(nlevels);
	if (doOverride) {
		numTrans = maxNumTrans;
		for (i = 0; i< 3; i++) {
			setRegionSize(i,fullSize[i]);
		}
	} else {
		if (numTrans> nlevels) numTrans = maxNumTrans;
		if (numTrans < minNumTrans) numTrans = minNumTrans;
		//Make sure we really can use the specified numTrans.
		numTrans = validateNumTrans(numTrans);
		for (i = 0; i< 3; i++) enforceConsistency(i);
	}
	
	//Data extents (user coords) are presented read-only in gui
	//
	setDataExtents(md->GetExtents());
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

//Grab a region face:
//
void RegionParams::
captureMouseDown(int faceNum, float camPos[3], float dirVec[3]){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(this,  "slide region boundary");
	selectedFaceNum = faceNum;
	faceDisplacement = 0.f;
	//Calculate intersection of ray with specified plane
	if (!rayCubeIntersect(dirVec, camPos, faceNum, initialSelectionRay))
		selectedFaceNum = -1;
	//The selection ray is the vector from the camera to the intersection point,
	//So subtract the camera position
	vsub(initialSelectionRay, camPos, initialSelectionRay);
	setDirty();
}

void RegionParams::
captureMouseUp(){
	//If no change, nothing to do, except
	//nullify unDo state
	if (faceDisplacement ==0.f || (selectedFaceNum < 0)) {
		if (savedCommand) {
			delete savedCommand;
			savedCommand = 0;
		}
		
	} else { //terminate dragging
		int coord = (5-selectedFaceNum)/2;
		if (selectedFaceNum %2) {
			setRegionMax(coord, getRegionMax(coord)+faceDisplacement);
		} else {
			setRegionMin(coord, getRegionMin(coord)+faceDisplacement);
		}
		
		enforceConsistency(coord);
		updateDialog();
	}
	faceDisplacement = 0.f;
	selectedFaceNum = -1;
	setDirty();
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, this);
	savedCommand = 0;
	
}

//Intersect the ray with the specified face, determining the intersection
//in world coordinates.  Note that meaning of faceNum is specified in 
//renderer.cpp
// Faces of the cube are numbered 0..5 based on view from pos z axis:
// back, front, bottom, top, left, right
// return false if no intersection
//
bool RegionParams::
rayCubeIntersect(float ray[3], float cameraPos[3], int faceNum, float intersect[3]){
	double val;
	int coord;// = (5-faceNum)/2;
	// if (faceNum%2) val = getRegionMin(coord); else val = getRegionMax(coord);
	switch (faceNum){
		case(0): //back; z = zmin
			val = getRegionMin(2);
			coord = 2;
			break;
		case(1): //front; z = zmax
			val = getRegionMax(2);
			coord = 2;
			break;
		case(2): //bot; y = min
			val = getRegionMin(1);
			coord = 1;
			break;
		case(3): //top; y = max
			val = getRegionMax(1);
			coord = 1;
			break;
		case(4): //left; x = min
			val = getRegionMin(0);
			coord = 0;
			break;
		case(5): //right; x = max
			val = getRegionMax(0);
			coord = 0;
			break;
		default:
			return false;
	}
	if (ray[coord] == 0.0) return false;
	float param = (val - cameraPos[coord])/ray[coord];
	for (int i = 0; i<3; i++){
		intersect[i] = cameraPos[i]+param*ray[i];
	}
	return true;
}


//Slide the face based on mouse move from previous capture.  
//Requires new direction vector associated with current mouse position
//The new face position requires finding the planar displacement such that 
//the ray (in the scene) associated with the new mouse position is as near
//as possible to the line projected from the original mouse position in the
//direction of planar motion
//displacement from the original intersection is as close as possible to the 
void RegionParams::
slideCubeFace(float movedRay[3]){
	float normalVector[3] = {0.f,0.f,0.f};
	float q[3], r[3], w[3];
	int coord = (5 - selectedFaceNum)/2;
	assert(selectedFaceNum >= 0);
	if (selectedFaceNum < 0) return;
	normalVector[coord] = 1.f;
	
	//Calculate W:
	vcopy(movedRay, w);
	vnormal(w);
	float scaleFactor = 1.f/vdot(w,normalVector);
	//Calculate q:
	vmult(w, scaleFactor, q);
	vsub(q, normalVector, q);
	
	//Calculate R:
	scaleFactor *= vdot(initialSelectionRay, normalVector);
	vmult(w, scaleFactor, r);
	vsub(r, initialSelectionRay, r);

	float denom = vdot(q,q);
	faceDisplacement = 0.f;
	if (denom != 0.f)
		faceDisplacement = -vdot(q,r)/denom;
	
	//Make sure the faceDisplacement is OK.  Not allowed to
	//extent face beyond end of data, nor beyond opposite face.
	//First, convert to a displacement in cube coordinates.  
	//Then, see what voxel coordinate 
	
	double regionMin = fullDataExtents[coord] + (fullDataExtents[coord+3]-fullDataExtents[coord])*
		(centerPosition[coord] - regionSize[coord]*.5)/(double)(fullSize[coord]);
	double regionMax = fullDataExtents[coord] + (fullDataExtents[coord+3]-fullDataExtents[coord])*
		(centerPosition[coord] + regionSize[coord]*.5)/(double)(fullSize[coord]);
	if (selectedFaceNum%2) { //Are we moving min or max?
		//Moving max, since selectedFace is odd:
		if (regionMax + faceDisplacement > fullDataExtents[coord+3])
			faceDisplacement = fullDataExtents[coord+3] - regionMax;
		if (regionMax + faceDisplacement < regionMin)
			faceDisplacement = regionMin - regionMax;
	} else { //Moving region min:
		if (regionMin + faceDisplacement > regionMax)
			faceDisplacement = regionMax - regionMin;
		if (regionMin + faceDisplacement < fullDataExtents[coord])
			faceDisplacement = fullDataExtents[coord] - regionMin;
	}
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
	int bs = Session::getInstance()->getCurrentMetadata()->GetBlockSize();
	for(i=0; i<3; i++) {
		int	s = numxforms;
		min_dim[i] = (int) ((float) (getCenterPosition(i) >> s) - 0.5 
			- (((getRegionSize(i) >> s) / 2.0)-1.0));
		max_dim[i] = (int) ((float) (getCenterPosition(i) >> s) - 0.5 
			+ (((getRegionSize(i) >> s) / 2.0)));
		//Force these to be in data:
		int dim = (getFullSize(i)>>numxforms) -1;
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
		min_bdim[i] = min_dim[i] / bs;
		max_bdim[i] = max_dim[i] / bs;
	}
	
	for (i = 0; i< 3; i++){
		fullExtent[i] = getFullDataExtent(i);
		fullExtent[i+3] = getFullDataExtent(i+3);
	}
	float maxCoordRange = Max(fullExtent[3]-fullExtent[0],Max( fullExtent[4]-fullExtent[1], fullExtent[5]-fullExtent[2]));
	//calculate the geometric extents of the dimensions in the unit cube:
	//fit the full region adjacent to the coordinate planes.
	for (i = 0; i<3; i++) {
		int dim = (getFullSize(i)>>numxforms) -1;
		assert (dim >= max_dim[i]);
		float extentRatio = (fullExtent[i+3]-fullExtent[i])/maxCoordRange;
		minFull[i] = 0.f;
		maxFull[i] = extentRatio;
		
		extents[i] = minFull[i] + ((float)min_dim[i]/(float)dim)*(maxFull[i]-minFull[i]);
		extents[i+3] = minFull[i] + ((float)max_dim[i]/(float)dim)*(maxFull[i]-minFull[i]);
	}
	return;
}
	
