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
const string RegionParams::_regionMinTag = "RegionMin";
const string RegionParams::_regionMaxTag = "RegionMax";

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
	
	Session::getInstance()->blockRecording();
	for (int i = 0; i< 3; i++){
		textToSlider(i, (regionMin[i]+regionMax[i])*0.5f,
			regionMax[i]-regionMin[i]);
	}
	myRegionTab->xSizeEdit->setText(QString::number(regionMax[0]-regionMin[0],'g', 4));
	myRegionTab->xCntrEdit->setText(QString::number(0.5f*(regionMax[0]+regionMin[0]),'g',5));
	myRegionTab->ySizeEdit->setText(QString::number(regionMax[1]-regionMin[1],'g', 4));
	myRegionTab->yCntrEdit->setText(QString::number(0.5f*(regionMax[1]+regionMin[1]),'g',5));
	myRegionTab->zSizeEdit->setText(QString::number(regionMax[2]-regionMin[2],'g', 4));
	myRegionTab->zCntrEdit->setText(QString::number(0.5f*(regionMax[2]+regionMin[2]),'g',5));
	
	
	if (isLocal())
		myRegionTab->LocalGlobal->setCurrentItem(1);
	else 
		myRegionTab->LocalGlobal->setCurrentItem(0);
	guiSetTextChanged(false);
	refreshRegionInfo();
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
}
//Set slider position, based on text change. 
//
void RegionParams::
textToSlider(int coord, float newCenter, float newSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	bool centerChanged = false;
	bool sizeChanged = false;
	const float* extents = Session::getInstance()->getExtents();
	float regMax = extents[coord+3];
	float regMin = extents[coord];
	if (newSize > regMax-regMin){
		newSize = regMax-regMin;
		sizeChanged = true;
	}
	if (newSize < 0.f) {
		newSize = 0.f;
		sizeChanged = true;
	}
	if (newCenter < regMin) {
		newCenter = regMin;
		centerChanged = true;
	}
	if (newCenter > regMax) {
		newCenter = regMax;
		centerChanged = true;
	}
	if ((newCenter - newSize*0.5f) < regMin){
		newCenter = regMin+ newSize*0.5f;
		centerChanged = true;
	}
	if ((newCenter + newSize*0.5f) > regMax){
		newCenter = regMax- newSize*0.5f;
		centerChanged = true;
	}
	
	regionMin[coord] = newCenter - newSize*0.5f; 
	regionMax[coord] = newCenter + newSize*0.5f; 
	int sliderSize = (int)(0.5f+ 256.f*newSize/(regMax - regMin));
	int sliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	int oldSliderSize, oldSliderCenter;
	switch(coord) {
		case 0:
			oldSliderSize = myRegionTab->xSizeSlider->value();
			oldSliderCenter = myRegionTab->xCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myRegionTab->xSizeSlider->setValue(sliderSize);
			if(sizeChanged) myRegionTab->xSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				myRegionTab->xCenterSlider->setValue(sliderCenter);
			if(centerChanged) myRegionTab->xCntrEdit->setText(QString::number(newCenter));
			
			break;
		case 1:
			oldSliderSize = myRegionTab->ySizeSlider->value();
			oldSliderCenter = myRegionTab->yCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myRegionTab->ySizeSlider->setValue(sliderSize);
			if(sizeChanged) myRegionTab->ySizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				myRegionTab->yCenterSlider->setValue(sliderCenter);
			if(centerChanged) myRegionTab->yCntrEdit->setText(QString::number(newCenter));
			
			break;
		case 2:
			oldSliderSize = myRegionTab->zSizeSlider->value();
			oldSliderCenter = myRegionTab->zCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myRegionTab->zSizeSlider->setValue(sliderSize);
			if(sizeChanged) myRegionTab->zSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				myRegionTab->zCenterSlider->setValue(sliderCenter);
			if(centerChanged) myRegionTab->zCntrEdit->setText(QString::number(newCenter));
			
			break;
		default:
			assert(0);
	}
	
	guiSetTextChanged(false);
	myRegionTab->update();
	return;
}
//Set text when a slider changes.
//Move the center if the size is too big
void RegionParams::
sliderToText(int coord, int slideCenter, int slideSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	const float* extents = Session::getInstance()->getExtents();
	float regMin = extents[coord];
	float regMax = extents[coord+3];
	bool sliderChanged = false;
	
	float newSize = slideSize*(regMax-regMin)/256.f;
	float newCenter = regMin+ slideCenter*(regMax-regMin)/256.f;
	
	if (newCenter < regMin) {
		newCenter = regMin;
	}
	if (newCenter > regMax) {
		newCenter = regMax;
	}
	if ((newCenter - newSize*0.5f) < regMin){
		newCenter = regMin+ newSize*0.5f;
		sliderChanged = true;
	}
	if ((newCenter + newSize*0.5f) > regMax){
		newCenter = regMax- newSize*0.5f;
		sliderChanged = true;
	}
	regionMin[coord] = newCenter - newSize*0.5f; 
	regionMax[coord] = newCenter + newSize*0.5f; 
	
	int newSliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	//Always need to change text.  Possibly also change slider if it was moved
	switch(coord) {
		case 0:
			if (sliderChanged) 
				myRegionTab->xCenterSlider->setValue(newSliderCenter);
			myRegionTab->xSizeEdit->setText(QString::number(newSize));
			myRegionTab->xCntrEdit->setText(QString::number(newCenter));
			break;
		case 1:
			if (sliderChanged) 
				myRegionTab->yCenterSlider->setValue(newSliderCenter);
			myRegionTab->ySizeEdit->setText(QString::number(newSize));
			myRegionTab->yCntrEdit->setText(QString::number(newCenter));
			break;
		case 2:
			if (sliderChanged) 
				myRegionTab->zCenterSlider->setValue(newSliderCenter);
			myRegionTab->zSizeEdit->setText(QString::number(newSize));
			myRegionTab->zCntrEdit->setText(QString::number(newCenter));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	myRegionTab->update();
	//force a new render with new flow data
	setDirty();
	return;
}	
void RegionParams::
refreshRegionInfo(){
	//First setup the timestep & refinement components
	//Make sure that the refinement levels are valid for the specified timestep.
	//If not, correct them.  If there is no data at specified timestep,
	//Then don't show anything in refinementCombo
	size_t bs = 32;
	int mdVarNum = myRegionTab->variableCombo->currentItem();
	//This index is only relevant to metadata numbering
	Session* ses = Session::getInstance();
	
	int timeStep = myRegionTab->timestepSpin->value();
	//Distinguish between the actual data available and the numtransforms
	//in the metadata.  If the data isn't there, we will display blanks
	//in the "selected" area.
	int maxRefLevel = 10;
	int numTrans = 10;
	int varNum = -1;
	std::string varName;
	if (ses->getDataStatus()) {
		varName = ses->getCurrentMetadata()->GetVariableNames()[mdVarNum];
		varNum = ses->getSessionVariableNum(varName);
		maxRefLevel = ses->getDataStatus()->maxXFormPresent(varNum, timeStep);
		numTrans = ses->getDataStatus()->getNumTransforms();
	}

	int refLevel = myRegionTab->refinementCombo->currentItem();

	const float* fullDataExtents = ses->getExtents();

	myRegionTab->fullMinXLabel->setText(QString::number(fullDataExtents[0], 'g', 5));
	myRegionTab->fullMinYLabel->setText(QString::number(fullDataExtents[1], 'g', 5));
	myRegionTab->fullMinZLabel->setText(QString::number(fullDataExtents[2], 'g', 5));
	myRegionTab->fullMaxXLabel->setText(QString::number(fullDataExtents[3], 'g', 5));
	myRegionTab->fullMaxYLabel->setText(QString::number(fullDataExtents[4], 'g', 5));
	myRegionTab->fullMaxZLabel->setText(QString::number(fullDataExtents[5], 'g', 5));
	myRegionTab->fullSizeXLabel->setText(QString::number(fullDataExtents[3]-fullDataExtents[0], 'g', 5));
	myRegionTab->fullSizeYLabel->setText(QString::number(fullDataExtents[4]-fullDataExtents[1], 'g', 5));
	myRegionTab->fullSizeZLabel->setText(QString::number(fullDataExtents[5]-fullDataExtents[2], 'g', 5));

	myRegionTab->minXFullLabel->setText(QString::number(fullDataExtents[0],'g',5));
	myRegionTab->minYFullLabel->setText(QString::number(fullDataExtents[1],'g',5));
	myRegionTab->minZFullLabel->setText(QString::number(fullDataExtents[2],'g',5));
	myRegionTab->maxXFullLabel->setText(QString::number(fullDataExtents[3],'g',5));
	myRegionTab->maxYFullLabel->setText(QString::number(fullDataExtents[4],'g',5));
	myRegionTab->maxZFullLabel->setText(QString::number(fullDataExtents[5],'g',5));

	//For now, the min and max var extents are the whole thing:

	myRegionTab->minVarXLabel->setText(QString::number(fullDataExtents[0],'g',5));
	myRegionTab->minVarYLabel->setText(QString::number(fullDataExtents[1],'g',5));
	myRegionTab->minVarZLabel->setText(QString::number(fullDataExtents[2],'g',5));
	myRegionTab->maxVarXLabel->setText(QString::number(fullDataExtents[3],'g',5));
	myRegionTab->maxVarYLabel->setText(QString::number(fullDataExtents[4],'g',5));
	myRegionTab->maxVarZLabel->setText(QString::number(fullDataExtents[5],'g',5));

	myRegionTab->minXSelectedLabel->setText(QString::number(regionMin[0],'g',5));
	myRegionTab->minYSelectedLabel->setText(QString::number(regionMin[1],'g',5));
	myRegionTab->minZSelectedLabel->setText(QString::number(regionMin[2],'g',5));
	myRegionTab->maxXSelectedLabel->setText(QString::number(regionMax[0],'g',5));
	myRegionTab->maxYSelectedLabel->setText(QString::number(regionMax[1],'g',5));
	myRegionTab->maxZSelectedLabel->setText(QString::number(regionMax[2],'g',5));

	//Now produce the corresponding voxel coords:
	size_t min_dim[3], max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	
	// if region isn't valid just don't show the bounds:
	if (refLevel <= maxRefLevel){
		getRegionVoxelCoords(refLevel,min_dim,max_dim,min_bdim,max_bdim);
		myRegionTab->minXVoxSelectedLabel->setText(QString::number(min_dim[0]));
		myRegionTab->minYVoxSelectedLabel->setText(QString::number(min_dim[1]));
		myRegionTab->minZVoxSelectedLabel->setText(QString::number(min_dim[2]));
		myRegionTab->maxXVoxSelectedLabel->setText(QString::number(max_dim[0]));
		myRegionTab->maxYVoxSelectedLabel->setText(QString::number(max_dim[1]));
		myRegionTab->maxZVoxSelectedLabel->setText(QString::number(max_dim[2]));
	} else { 
		myRegionTab->minXVoxSelectedLabel->setText("");
		myRegionTab->minYVoxSelectedLabel->setText("");
		myRegionTab->minZVoxSelectedLabel->setText("");
		myRegionTab->maxXVoxSelectedLabel->setText("");
		myRegionTab->maxYVoxSelectedLabel->setText("");
		myRegionTab->maxZVoxSelectedLabel->setText("");
	}

	if (ses->getDataStatus()){
		//Entire region size in voxels
		for (int i = 0; i<3; i++){
			max_dim[i] = ((ses->getDataStatus()->getFullDataSize(i))>>(numTrans - refLevel))-1;
			max_bdim[i] = (max_dim[i]/bs);
			min_bdim[i] = 0;
		}
	}
	myRegionTab->minXVoxFullLabel->setText("0");
	myRegionTab->minYVoxFullLabel->setText("0");
	myRegionTab->minZVoxFullLabel->setText("0");
	myRegionTab->maxXVoxFullLabel->setText(QString::number(max_dim[0]));
	myRegionTab->maxYVoxFullLabel->setText(QString::number(max_dim[1]));
	myRegionTab->maxZVoxFullLabel->setText(QString::number(max_dim[2]));

	//Find the variable voxel limits:
	int rc = -1;
	if (refLevel <= maxRefLevel && ses->getDataMgr())
		rc = ses->getDataMgr()->GetValidRegion(timeStep, varName.c_str(),refLevel, min_dim, max_dim);
	if (rc>= 0)	{
		myRegionTab->minXVoxVarLabel->setText(QString::number(min_dim[0]));
		myRegionTab->minYVoxVarLabel->setText(QString::number(min_dim[1]));
		myRegionTab->minZVoxVarLabel->setText(QString::number(min_dim[2]));
		myRegionTab->maxXVoxVarLabel->setText(QString::number(max_dim[0]));
		myRegionTab->maxYVoxVarLabel->setText(QString::number(max_dim[1]));
		myRegionTab->maxZVoxVarLabel->setText(QString::number(max_dim[2]));
	} else {
		myRegionTab->minXVoxVarLabel->setText("");
		myRegionTab->minYVoxVarLabel->setText("");
		myRegionTab->minZVoxVarLabel->setText("");
		myRegionTab->maxXVoxVarLabel->setText("");
		myRegionTab->maxYVoxVarLabel->setText("");
		myRegionTab->maxZVoxVarLabel->setText("");
	}
	

	if (ses->getCurrentMetadata())
		bs = *(ses->getCurrentMetadata()->GetBlockSize());
	//Size needed for data assumes blocksize = 2**5, 6 bytes per voxel, times 2.
	float newFullMB = (float)(bs*bs*bs*(max_bdim[0]-min_bdim[0]+1)*(max_bdim[1]-min_bdim[1]+1)*(max_bdim[2]-min_bdim[2]+1));
	
	
	//divide by 1 million for megabytes, mult by 4 for 4 bytes per voxel:
	newFullMB /= 262144.f;

	
	myRegionTab->selectedDataSizeLabel->setText(QString::number(newFullMB,'g',8));
}

//Update all the panel state associated with textboxes.
//Currently that's just the textboxes associated with sliders.
//enforce consistency requirements
//Whenever this affects a slider, move it.
//
void RegionParams::
updatePanelState(){
	float centerPos[3], regSize[3];
	centerPos[0] = myRegionTab->xCntrEdit->text().toFloat();
	centerPos[1] = myRegionTab->yCntrEdit->text().toFloat();
	centerPos[2] = myRegionTab->zCntrEdit->text().toFloat();
	regSize[0] = myRegionTab->xSizeEdit->text().toFloat();
	regSize[1] = myRegionTab->ySizeEdit->text().toFloat();
	regSize[2] = myRegionTab->zSizeEdit->text().toFloat();
	for (int i = 0; i<3; i++)
		textToSlider(i,centerPos[i],regSize[i]);
	refreshRegionInfo();
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

//Methods in support of undo/redo:
//
//Make region match probe.  Responds to button in region panel
void RegionParams::
guiCopyProbeToRegion(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "copy probe to region");
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
	for (int i = 0; i< 3; i++){
		regionMin[i] = pParams->getProbeMin(i);
		regionMax[i] = pParams->getProbeMax(i);
	}
	//Note:  the probe may not fit in the region.  UpdateDialog will fix this:
	updateDialog();
	setDirty();
	PanelCommand::captureEnd(cmd,this);
	
}
//Make region match rake.  Responds to button in region panel
void RegionParams::
guiCopyRakeToRegion(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "copy rake to region");
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::FlowParamsType);
	for (int i = 0; i< 3; i++){
		regionMin[i] = fParams->getSeedRegionMin(i);
		regionMax[i] = fParams->getSeedRegionMax(i);
	}
	updateDialog();
	setDirty();
	PanelCommand::captureEnd(cmd,this);
	
}
void RegionParams::
guiSetVarNum(int n){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "set variable");
	infoVarNum = n;
	refreshRegionInfo();
	PanelCommand::captureEnd(cmd, this);

}
void RegionParams::guiSetTimeStep(int n){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "set time step");
	infoTimeStep = n;
	refreshRegionInfo();
	PanelCommand::captureEnd(cmd, this);
}
void RegionParams::
guiSetNumRefinements(int n){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "set number of Refinements");
	infoNumRefinements = n;
	refreshRegionInfo();
	PanelCommand::captureEnd(cmd, this);
}
//See if the number of trans is ok.  If not, return an OK value
int RegionParams::
validateNumTrans(int n){
	//if we have a dataMgr, see if we can really handle this new numtrans:
	if (!Session::getInstance()->getDataMgr()) return n;
	
	size_t min_dim[3], max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	getRegionVoxelCoords(n,min_dim,max_dim,min_bdim,max_bdim);
	
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
	const float* extents = Session::getInstance()->getExtents();
	for (int i = 0; i< 3; i++){
		float coord = coords[i];
		float fullMin = extents[i];
		float fullMax = extents[i+3];
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
void RegionParams::
guiSetXCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide region X center");
	setXCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty();
	VizWinMgr::getInstance()->setRegionDirty(this);
}
//Following are set when slider is released:
void RegionParams::
guiSetYCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide region Y center");
	setYCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty();
	
}
//Following are set when slider is released:
void RegionParams::
guiSetZCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide region Z center");
	setZCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty();
	
}

void RegionParams::
guiSetXSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide region X size");
	setXSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty();
}
void RegionParams::
guiSetYSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide region Y size");
	setYSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty();
}
void RegionParams::
guiSetZSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide region Z size");
	setZSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty();
}
//When the center slider moves, set the seedBoxMin and seedBoxMax
void RegionParams::
setXCenter(int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(0, sliderval, myRegionTab->xSizeSlider->value());
	refreshRegionInfo();
	setDirty();
}
void RegionParams::
setYCenter(int sliderval){
	sliderToText(1, sliderval, myRegionTab->ySizeSlider->value());
	refreshRegionInfo();
	setDirty();
}
void RegionParams::
setZCenter(int sliderval){
	sliderToText(2, sliderval, myRegionTab->zSizeSlider->value());
	refreshRegionInfo();
	setDirty();
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void RegionParams::
setXSize(int sliderval){
	sliderToText(0, myRegionTab->xCenterSlider->value(),sliderval);
	refreshRegionInfo();
	setDirty();
}
void RegionParams::
setYSize(int sliderval){
	sliderToText(1, myRegionTab->yCenterSlider->value(),sliderval);
	refreshRegionInfo();
	setDirty();
}
void RegionParams::
setZSize(int sliderval){
	sliderToText(2, myRegionTab->zCenterSlider->value(),sliderval);
	refreshRegionInfo();
	setDirty();
}
void RegionParams::
guiSetMaxSize(){
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(this, "change region size to max");
	const float* fullDataExtents = Session::getInstance()->getExtents();
	for (int i = 0; i<3; i++){
		regionMin[i] = fullDataExtents[i];
		regionMax[i] = fullDataExtents[i+3];
	}
	updateDialog();
	PanelCommand::captureEnd(cmd, this);

	setDirty();
}
//Reset region settings to initial state
void RegionParams::
restart(){
	infoNumRefinements = 0; 
	infoVarNum = 0;
	infoTimeStep = 0;

	const float* fullDataExtents = Session::getInstance()->getExtents();
	
	savedCommand = 0;
	for (int i = 0; i< 3; i++){
		regionMin[i] = fullDataExtents[i];
		regionMax[i] = fullDataExtents[i+3];
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
//Reinitialize region settings, session has changed
//Need to force the regionMin, regionMax to be OK.
void RegionParams::
reinit(bool doOverride){
	int i;
	const Metadata* md = Session::getInstance()->getCurrentMetadata();
	const float* extents = Session::getInstance()->getExtents();
	
	if (doOverride) {
		for (i = 0; i< 3; i++) {
			regionMin[i] = extents[i];
			regionMax[i] = extents[i+3];
		}
	} else {
		//Just force them to fit in current volume 
		for (i = 0; i< 3; i++) {
			if (regionMin[i] > regionMax[i]) 
				regionMax[i] = regionMin[i];
			if (regionMin[i] > extents[i+3])
				regionMin[i] = extents[i+3];
			if (regionMin[i] < extents[i])
				regionMin[i] = extents[i];
			if (regionMax[i] > extents[i+3])
				regionMax[i] = extents[i+3];
			if (regionMax[i] < extents[i])
				regionMax[i] = extents[i];
		}
	}
	if (!local){
		//Set up the combo boxes in the gui based on info in the session:
		const vector<string>& varNames = md->GetVariableNames();
		myRegionTab->variableCombo->clear();
		for (i = 0; i<(int)varNames.size(); i++)
			myRegionTab->variableCombo->insertItem(varNames[i].c_str());
	}
	int mints = Session::getInstance()->getMinTimestep();
	int maxts = Session::getInstance()->getMaxTimestep();

	myRegionTab->timestepSpin->setMinValue(mints);
	myRegionTab->timestepSpin->setMaxValue(maxts);

	int numRefinements = md->GetNumTransforms();
	myRegionTab->refinementCombo->setMaxCount(numRefinements+1);
	myRegionTab->refinementCombo->clear();
	for (i = 0; i<= numRefinements; i++){
		myRegionTab->refinementCombo->insertItem(QString::number(i));
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
	setDirty();
}

void RegionParams::setRegionMin(int coord, float minval){
	const float* fullDataExtents = Session::getInstance()->getExtents();
	if (minval < fullDataExtents[coord]) minval = fullDataExtents[coord];
	if (minval > fullDataExtents[coord+3]) minval = fullDataExtents[coord+3];
	if (minval > regionMax[coord]) minval = regionMax[coord];
	regionMin[coord] = minval;
}
void RegionParams::setRegionMax(int coord, float maxval){
	const float* fullDataExtents = Session::getInstance()->getExtents();
	if (maxval < fullDataExtents[coord]) maxval = fullDataExtents[coord];
	if (maxval > fullDataExtents[coord+3]) maxval = fullDataExtents[coord+3];
	if (maxval < regionMin[coord]) maxval = regionMin[coord];
	regionMax[coord] = maxval;
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
//static method to do conversion to box coords (probably based on available
//coords, that may be smaller than region coords)
//
void RegionParams::
convertToBoxExtentsInCube(int refLevel, size_t min_dim[3], size_t max_dim[3], float extents[6]){
	double fullExtents[6];
	double subExtents[6];
	Session* ses = Session::getInstance();
	const size_t fullMin[3] = {0,0,0};
	size_t fullMax[3];
	int maxRef = Session::getInstance()->getDataStatus()->getNumTransforms();
	const size_t* fullDims = ses->getFullDataDimensions();
	for (int i = 0; i<3; i++) fullMax[i] = (fullDims[i]>>(maxRef-refLevel)) - 1;

	ses->mapVoxelToUserCoords(refLevel, fullMin, fullExtents);
	ses->mapVoxelToUserCoords(refLevel, fullMax, fullExtents+3);
	ses->mapVoxelToUserCoords(refLevel, min_dim, subExtents);
	ses->mapVoxelToUserCoords(refLevel, max_dim, subExtents+3);
	
	double maxSize = Max(Max(fullExtents[3]-fullExtents[0],fullExtents[4]-fullExtents[1]),fullExtents[5]-fullExtents[2]);
	for (int i = 0; i<3; i++){
		extents[i] = (float)((subExtents[i] - fullExtents[i])/maxSize);
		extents[i+3] = (float)((subExtents[i+3] - fullExtents[i])/maxSize);
	}
	
}
	
bool RegionParams::
getAvailableVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3], size_t timestep, const int* varNums, int numVars){
	//First determine the bounds specified in this RegionParams
	int i;
	bool retVal = true;
	float fullExtents[6];
	const size_t *bs ;
	const size_t* dataSize;
	int maxTrans;
	Session* ses = Session::getInstance();
	//Special case before there is any data...
	if (!ses->getCurrentMetadata()){
		for (i = 0; i<3; i++) {
			min_dim[i] = 0;
			max_dim[i] = (1024>>numxforms) -1;
			min_bdim[i] = 0;
			max_bdim[i] = (32 >>numxforms) -1;
		}
		return false;
	}
	//Check that the data exists for this timestep and refinement:
	for (i = 0; i<numVars; i++){
		if(ses->getDataStatus()->maxXFormPresent(varNums[i],timestep) < numxforms)
			return false;
	}
	
	maxTrans = ses->getCurrentMetadata()->GetNumTransforms();
	dataSize = ses->getFullDataDimensions();
	bs = ses->getCurrentMetadata()->GetBlockSize();
	ses->getExtents(numxforms, fullExtents);
	size_t temp_min[3], temp_max[3];
	for(i=0; i<3; i++) {
		int	s = maxTrans-numxforms;
		int arraySide = (dataSize[i] >> s) - 1;
		min_dim[i] = (int) (0.5f+(regionMin[i] - fullExtents[i])*(float)arraySide/
			(fullExtents[i+3]-fullExtents[i]));
		max_dim[i] = (int) (0.5f+(regionMax[i] - fullExtents[i])*(float)arraySide/
			(fullExtents[i+3]-fullExtents[i]));

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
	}
	//Now intersect with available bounds based on variables:
	for (int varIndex = 0; varIndex < numVars; varIndex++){
		const string varName = ses->getVariableName(varNums[varIndex]);
		int rc = ses->getDataMgr()->GetValidRegion(timestep, varName.c_str(),numxforms, temp_min, temp_max);
		if (rc < 0) retVal = false;
		else for (i = 0; i< 3; i++){
			if (min_dim[i] < temp_min[i]) min_dim[i] = temp_min[i];
			if (max_dim[i] > temp_max[i]) max_dim[i] = temp_max[i];
			//Again check for validity:
			if (min_dim[i] >= max_dim[i]) retVal = false;
		}
	}
	for (i = 0; i<3; i++){	
		min_bdim[i] = min_dim[i] / bs[i];
		max_bdim[i] = max_dim[i] / bs[i];
	}
	return retVal;
}

void RegionParams::
getRegionVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3]){
	int i;
	const float* fullExtents;
	const size_t *bs ;
	const size_t* dataSize;
	int maxTrans;
	if (!Session::getInstance()->getCurrentMetadata()){
		for (i = 0; i<3; i++) {
			min_dim[i] = 0;
			max_dim[i] = (1024>>numxforms) -1;
			min_bdim[i] = 0;
			max_bdim[i] = (32 >>numxforms) -1;
		}
		return;
	}
	
	maxTrans = Session::getInstance()->getCurrentMetadata()->GetNumTransforms();
	dataSize = Session::getInstance()->getCurrentMetadata()->GetDimension();
	bs = Session::getInstance()->getCurrentMetadata()->GetBlockSize();
	fullExtents = Session::getInstance()->getExtents();
	for(i=0; i<3; i++) {
		int	s = maxTrans-numxforms;
		int arraySide = (dataSize[i] >> s) - 1;
		min_dim[i] = (int) (0.5f+(regionMin[i] - fullExtents[i])*(float)arraySide/
			(fullExtents[i+3]-fullExtents[i]));
		max_dim[i] = (int) (0.5f+(regionMax[i] - fullExtents[i])*(float)arraySide/
			(fullExtents[i+3]-fullExtents[i]));
		
		
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
	return;
}

/*
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
*/
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
			/*  No longer support these attributes:
			else if (StrCmpNoCase(attribName, _maxSizeAttr) == 0) {
				ist >> maxSize;
			}
			else if (StrCmpNoCase(attribName, _numTransAttr) == 0) {
				ist >> numTrans;
			}
			*/
			else return false;
		}
		return true;
	}
	//Prepare for region min/max
	else if ((StrCmpNoCase(tagString, _regionMinTag) == 0) ||
		(StrCmpNoCase(tagString, _regionMaxTag) == 0) ) {
		//Should have a double type attribute
		string attribName = *attrs;
		attrs++;
		string value = *attrs;

		ExpatStackElement *state = pm->getStateStackTop();
		
		state->has_data = 1;
		if (StrCmpNoCase(attribName, _typeAttr) != 0) {
			pm->parseError("Invalid attribute : %s", attribName.c_str());
			return false;
		}
		if (StrCmpNoCase(value, _doubleType) != 0) {
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
	} 
	else if (StrCmpNoCase(tag, _regionMinTag) == 0){
		vector<double> bound = pm->getDoubleData();
		regionMin[0] = bound[0];
		regionMin[1] = bound[1];
		regionMin[2] = bound[2];
		return true;
	} else if (StrCmpNoCase(tag, _regionMaxTag) == 0){
		vector<double> bound = pm->getDoubleData();
		regionMax[0] = bound[0];
		regionMax[1] = bound[1];
		regionMax[2] = bound[2];
		return true;
	} 
	// no longer support these...
	else if (StrCmpNoCase(tag, _regionCenterTag) == 0){
		//vector<long> posn = pm->getLongData();
		//centerPosition[0] = posn[0];
		//centerPosition[1] = posn[1];
		//centerPosition[2] = posn[2];
		return true;
	} else if (StrCmpNoCase(tag, _regionSizeTag) == 0){
		//vector<long> sz = pm->getLongData();
		//regionSize[0] = sz[0];
		//regionSize[1] = sz[1];
		//regionSize[2] = sz[2];
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

	XmlNode* regionNode = new XmlNode(_regionParamsTag, attrs, 2);

	//Now add children:  
	
	vector<double> bounds;
	int i;
	bounds.clear();
	for (i = 0; i< 3; i++){
		bounds.push_back((double) regionMin[i]);
	}
	regionNode->SetElementDouble(_regionMinTag,bounds);
	bounds.clear();
	for (i = 0; i< 3; i++){
		bounds.push_back((double) regionMax[i]);
	}
	regionNode->SetElementDouble(_regionMaxTag,bounds);
		
	return regionNode;
}
