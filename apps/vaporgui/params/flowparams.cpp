//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2005
//
//	Description:  Implementation of flowparams class 
//
#include "flowparams.h"
#include "flowtab.h"
#include "flowrenderer.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "vapor/VaporFlow.h"
#include "glutil.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include "mainform.h"
#include "session.h"
#include "params.h"
#include "panelcommand.h"
#include "tabmanager.h"
#include "vizwinmgr.h"
#include "messagereporter.h"
using namespace VAPoR;
FlowParams::FlowParams(int winnum) : Params(winnum) {
	thisParamType = FlowParamsType;
	myFlowTab = MainForm::getInstance()->getFlowTab();
	enabled = false;
	//Set default values
	flowType = 0; //steady
	instance = 1;
	numTrans = 4; 
	maxNumTrans = 4; 
	minNumTrans = 0;
	numVariables = 0;
	variableNames.empty();
	varNum[0] = varNum [1] = varNum[2] = 0;
	integrationAccuracy = 0.5f;
	userTimeStepMultiplier = 1.0f;
	timeSamplingInterval = 1;
	minFrame = maxFrame = 1;
	

	randomGen = false;
	dirty = true;
	
	randomSeed = 1;
	seedBoxMin[0] = seedBoxMin[1] = seedBoxMin[2] = 0.f;
	seedBoxMax[0] = seedBoxMax[1] = seedBoxMax[2] = 1.f;
	regionMin[0] = regionMin[1] = regionMin[2] = 0.f;
	regionMax[0] = regionMax[1] = regionMax[2] = 1.f;
	generatorCount[0]=generatorCount[1]=generatorCount[2] = 1;

	allGeneratorCount = 1;
	seedTimeStart = 1; 
	seedTimeEnd = 100; 
	seedTimeIncrement = 1;
	currentDimension = 0;

	geometryType = 0;  //0= tube, 1=point, 2 = arrow
	objectsPerTimestep = 1.f;
	minAgeShown = 0;
	maxAgeShown = 100;
	shapeDiameter = 0.f;
	colorMappedEntity = 0; //0 = constant, 1=age, 2 = speed, 3+varnum = variable
	colorMapMin = 0.f; 
	colorMapMax = 1.f;
	myFlowLib = 0;
	//Set up flow data cache:
	flowData = 0;
	
	numSeedPoints = 1;
	numInjections = 1;
	
}

//Make a copy of  parameters:
Params* FlowParams::
deepCopy(){
	//For now, copy is not deep...
	FlowParams* newFlowParams = new FlowParams(*this);
	return (Params*)newFlowParams;
}

FlowParams::~FlowParams(){
	
}
void FlowParams::
makeCurrent(Params* prev, bool newWin) {
	
	VizWinMgr::getInstance()->setFlowParams(vizNum, this);
	updateDialog();
	//Need to create/destroy renderer if there's a change in local/global or enable/disable
	//or if the window is new
	//
	FlowParams* formerParams = (FlowParams*)prev;
	if (formerParams->enabled != enabled || formerParams->local != local || newWin){
		updateRenderer(formerParams->enabled, formerParams->local, newWin);
	}
}

void FlowParams::updateDialog(){
	
	Session::getInstance()->blockRecording();
	//Get the region corners from the current applicable region panel,
	//or the global one:
	
	myFlowTab->EnableDisable->setCurrentItem((enabled) ? 1 : 0);
	
	myFlowTab->flowTypeCombo->setCurrentItem(flowType);
	myFlowTab->numTransSpin->setMinValue(minNumTrans);
	myFlowTab->numTransSpin->setMaxValue(maxNumTrans);
	myFlowTab->numTransSpin->setValue(numTrans);
	myFlowTab->xCoordVarCombo->clear();
	myFlowTab->xCoordVarCombo->setMaxCount(numVariables);
	myFlowTab->yCoordVarCombo->clear();
	myFlowTab->yCoordVarCombo->setMaxCount(numVariables);
	myFlowTab->zCoordVarCombo->clear();
	myFlowTab->zCoordVarCombo->setMaxCount(numVariables);
	for (int i = 0; i< numVariables; i++){
		if (variableNames.size() > (unsigned int)i){
			const std::string& s = variableNames.at(i);
			//Direct conversion of std::string& to QString doesn't seem to work
			//Maybe std was not enabled when QT was built?
			const QString& text = QString(s.c_str());
			myFlowTab->xCoordVarCombo->insertItem(text);
			myFlowTab->yCoordVarCombo->insertItem(text);
			myFlowTab->zCoordVarCombo->insertItem(text);
		} else {
			myFlowTab->xCoordVarCombo->insertItem("");
			myFlowTab->yCoordVarCombo->insertItem("");
			myFlowTab->zCoordVarCombo->insertItem("");
		}
	}
	myFlowTab->xCoordVarCombo->setCurrentItem(varNum[0]);
	myFlowTab->yCoordVarCombo->setCurrentItem(varNum[1]);
	myFlowTab->zCoordVarCombo->setCurrentItem(varNum[2]);
	
	myFlowTab->timeSampleEdit->setEnabled(flowType == 1);
	myFlowTab->recalcButton->setEnabled(flowType == 1);
	myFlowTab->seedtimeIncrementEdit->setEnabled(flowType == 1);
	myFlowTab->seedtimeEndEdit->setEnabled(flowType == 1);

	myFlowTab->randomCheckbox->setChecked(randomGen);
	
	myFlowTab->randomSeedEdit->setEnabled(randomGen);
	myFlowTab->generatorDimensionCombo->setCurrentItem(currentDimension);

	//To set the seed region extents, find the region panel that applies in the current
	//active visualizer, make those region bounds be the limits of the seed sliders.
	//Then force the seed region to fit in there.
	int viznum = getVizNum();
	RegionParams* rParams;
	if (viznum >= 0) rParams = VizWinMgr::getInstance()->getRegionParams(viznum);
	else rParams = (RegionParams*)VizWinMgr::getInstance()->getGlobalParams(RegionParamsType);
	for (int i = 0; i< 3; i++){
		regionMin[i] = rParams->getRegionMin(i);
		regionMax[i] = rParams->getRegionMax(i);
		if (seedBoxMin[i]<regionMin[i]) seedBoxMin[i] = regionMin[i];
		if (seedBoxMax[i]>regionMax[i]) seedBoxMax[i] = regionMax[i];
		if (seedBoxMin[i]>seedBoxMax[i]) seedBoxMin[i] = seedBoxMax[i];
		textToSlider(i, (seedBoxMin[i]+seedBoxMax[i])*0.5f,
			seedBoxMax[i]-seedBoxMin[i]);
	}


	//Geometric parameters:
	myFlowTab->geometryCombo->setCurrentItem(geometryType);
	
	myFlowTab->colormapEntityCombo->setCurrentItem(colorMappedEntity);
	
	if (isLocal())
		myFlowTab->LocalGlobal->setCurrentItem(1);
	else 
		myFlowTab->LocalGlobal->setCurrentItem(0);

	if (randomGen){
		myFlowTab->dimensionLabel->setEnabled(false);
		myFlowTab->generatorDimensionCombo->setEnabled(false);
	} else {
		myFlowTab->dimensionLabel->setEnabled(true);
		myFlowTab->generatorDimensionCombo->setEnabled(true);
		myFlowTab->generatorDimensionCombo->setCurrentItem(currentDimension);
	}
	//Put all the setText messages here, so they won't trigger a textChanged message
	if (randomGen){
		myFlowTab->generatorCountEdit->setText(QString::number(allGeneratorCount));
	} else {
		myFlowTab->generatorCountEdit->setText(QString::number(generatorCount[currentDimension]));
	}
	myFlowTab->integrationAccuracyEdit->setText(QString::number(integrationAccuracy));
	myFlowTab->userTimestepEdit->setText(QString::number(userTimeStepMultiplier));
	myFlowTab->timeSampleEdit->setText(QString::number(timeSamplingInterval));
	myFlowTab->randomSeedEdit->setText(QString::number(randomSeed));
	myFlowTab->objectsPerTimestepEdit->setText(QString::number(objectsPerTimestep));
	myFlowTab->minAgeEdit->setText(QString::number(minAgeShown));
	myFlowTab->maxAgeEdit->setText(QString::number(maxAgeShown));
	myFlowTab->diameterEdit->setText(QString::number(shapeDiameter));
	myFlowTab->minColormapEdit->setText(QString::number(colorMapMin));
	myFlowTab->maxColormapEdit->setText(QString::number(colorMapMax));
	myFlowTab->xSizeEdit->setText(QString::number(seedBoxMax[0]-seedBoxMin[0],'g', 4));
	myFlowTab->xCenterEdit->setText(QString::number(0.5f*(seedBoxMax[0]+seedBoxMin[0]),'g',5));
	myFlowTab->ySizeEdit->setText(QString::number(seedBoxMax[1]-seedBoxMin[1],'g', 4));
	myFlowTab->yCenterEdit->setText(QString::number(0.5f*(seedBoxMax[1]+seedBoxMin[1]),'g',5));
	myFlowTab->zSizeEdit->setText(QString::number(seedBoxMax[2]-seedBoxMin[2],'g', 4));
	myFlowTab->zCenterEdit->setText(QString::number(0.5f*(seedBoxMax[2]+seedBoxMin[2]),'g',5));
	myFlowTab->seedtimeIncrementEdit->setText(QString::number(seedTimeIncrement));
	myFlowTab->seedtimeStartEdit->setText(QString::number(seedTimeStart));
	myFlowTab->seedtimeEndEdit->setText(QString::number(seedTimeEnd));
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
}

//Update all the panel state associated with textboxes, after user presses return
//
void FlowParams::
updatePanelState(){
	
	integrationAccuracy = myFlowTab->integrationAccuracyEdit->text().toFloat();
	if (integrationAccuracy < 0.f || integrationAccuracy > 1.f) {
		if (integrationAccuracy > 1.f) integrationAccuracy = 1.f;
		if (integrationAccuracy < 0.f) integrationAccuracy = 0.f;
		myFlowTab->integrationAccuracyEdit->setText(QString::number(integrationAccuracy));
	}

	userTimeStepMultiplier = myFlowTab->userTimestepEdit->text().toFloat();
	if (userTimeStepMultiplier < 1.e-30){
		userTimeStepMultiplier = 1.f;
		myFlowTab->userTimestepEdit->setText(QString::number(userTimeStepMultiplier));
	}
	timeSamplingInterval = myFlowTab->timeSampleEdit->text().toInt();
	if (timeSamplingInterval < 1){
		timeSamplingInterval = 1;
		myFlowTab->timeSampleEdit->setText(QString::number(timeSamplingInterval));
	}
	randomSeed = myFlowTab->randomSeedEdit->text().toUInt();

	//Get slider positions from text boxes:
	
	float boxCtr = myFlowTab->xCenterEdit->text().toFloat();
	float boxSize = myFlowTab->xSizeEdit->text().toFloat();
	seedBoxMin[0] = boxCtr - 0.5*boxSize;
	seedBoxMax[0] = boxCtr + 0.5*boxSize;
	textToSlider(0, boxCtr, boxSize);
	boxCtr = myFlowTab->yCenterEdit->text().toFloat();
	boxSize = myFlowTab->ySizeEdit->text().toFloat();
	seedBoxMin[1] = boxCtr - 0.5*boxSize;
	seedBoxMax[1] = boxCtr + 0.5*boxSize;
	textToSlider(1, boxCtr, boxSize);
	boxCtr = myFlowTab->zCenterEdit->text().toFloat();
	boxSize = myFlowTab->zSizeEdit->text().toFloat();
	seedBoxMin[2] = boxCtr - 0.5*boxSize;
	seedBoxMax[2] = boxCtr + 0.5*boxSize;
	textToSlider(2, boxCtr, boxSize);

	int genCount = myFlowTab->generatorCountEdit->text().toInt();
	if (genCount < 1) {
		genCount = 1;
		myFlowTab->generatorCountEdit->setText(QString::number(genCount));
	}
	if (randomGen) {
		allGeneratorCount = genCount;
	} else {
		generatorCount[currentDimension] = genCount;
	}
	
	seedTimeStart = myFlowTab->seedtimeStartEdit->text().toUInt();
	seedTimeEnd = myFlowTab->seedtimeEndEdit->text().toUInt(); 
	bool changed = false;
	if (seedTimeStart < minFrame) {seedTimeStart = minFrame; changed = true;}
	if (seedTimeEnd > maxFrame) {seedTimeEnd = maxFrame; changed = true;}
	if (seedTimeEnd < seedTimeStart) {seedTimeEnd = seedTimeStart; changed = true;}
	if (changed){
		myFlowTab->seedtimeStartEdit->setText(QString::number(seedTimeStart));
		myFlowTab->seedtimeEndEdit->setText(QString::number(seedTimeEnd));
	}

	seedTimeIncrement = myFlowTab->seedtimeIncrementEdit->text().toUInt();

	objectsPerTimestep = myFlowTab->objectsPerTimestepEdit->text().toFloat();
	if (objectsPerTimestep <= 0.f) {
		objectsPerTimestep = 1.f;
		myFlowTab->objectsPerTimestepEdit->setText(QString::number(objectsPerTimestep));
	}

	minAgeShown = myFlowTab->minAgeEdit->text().toUInt();
	maxAgeShown = myFlowTab->maxAgeEdit->text().toUInt();
	if (minAgeShown > maxAgeShown) {
		maxAgeShown = minAgeShown;
		myFlowTab->maxAgeEdit->setText(QString::number(maxAgeShown));
	}
	shapeDiameter = myFlowTab->diameterEdit->text().toFloat();
	if (shapeDiameter < 0.f) {
		shapeDiameter = 0.f;
		myFlowTab->diameterEdit->setText(QString::number(shapeDiameter));
	}
	colorMapMin = myFlowTab->minColormapEdit->text().toFloat();
	colorMapMax = myFlowTab->maxColormapEdit->text().toFloat();
	if (colorMapMin > colorMapMax){
		colorMapMax = colorMapMin;
		myFlowTab->maxColormapEdit->setText(QString::number(colorMapMax));
	}
	guiSetTextChanged(false);
	myFlowTab->update();
	setDirty(true);
	
}
//Reinitialize settings, session has changed:
void FlowParams::
reinit(bool doOverride){
	int i;
	const Metadata* md = Session::getInstance()->getCurrentMetadata();
	Session* session = Session::getInstance();
	int nlevels = md->GetNumTransforms();
	int minTrans = session->getDataStatus()->minXFormPresent();
	if(minTrans < 0) minTrans = nlevels; 
	setMinNumTrans(minTrans);
	setMaxNumTrans(nlevels);
	//Make min and max conform to new data:
	minFrame = (int)(session->getMinTimestep());
	maxFrame = (int)(session->getMaxTimestep());
	if (doOverride) {
		numTrans = maxNumTrans;
		seedTimeStart = minFrame;
		seedTimeEnd = maxFrame;
	} else {
		if (numTrans> nlevels) numTrans = maxNumTrans;
		if (numTrans < minNumTrans) numTrans = minNumTrans;
		//Make sure we really can use the specified numTrans.
		RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->
			getRegionParams(vizNum);
		numTrans = rParams->validateNumTrans(numTrans);
		if (seedTimeStart > maxFrame) seedTimeStart = maxFrame;
		if (seedTimeStart < minFrame) seedTimeStart = minFrame;
		if (seedTimeEnd > maxFrame) seedTimeEnd = maxFrame;
		if (seedTimeEnd < minFrame) seedTimeEnd = minFrame;
	}
	//Set up variables:
	//Get the variable names:
	variableNames = md->GetVariableNames();
	int newNumVariables = md->GetVariableNames().size();
	for (int dim = 0; dim < 3; dim++){
		//See if current varNums is valid
		//reset to first variable that is present:
		if (!Session::getInstance()->getDataStatus()->variableIsPresent(varNum[dim])){
			varNum[dim] = -1;
			for (i = 0; i<newNumVariables; i++) {
				if (Session::getInstance()->getDataStatus()->variableIsPresent(i)){
					varNum[dim] = i;
					break;
				}
			}
		}
	}
	if (varNum[0] == -1){
		MessageReporter::errorMsg("Flow Params: No data in specified dataset");
		numVariables = 0;
		return;
	}
	
	numVariables = newNumVariables;
	//Always disable
	bool wasEnabled = enabled;
	setEnabled(false);
	//don't change local/global 
	updateRenderer(wasEnabled, isLocal(), false);
	//If flow is the current front tab, and if it applies to the active visualizer,
	//update its values
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myFlowTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum == vizNum)
			updateDialog();
	}
	//force a new render with new flow data
	setDirty(true);
}
//Set slider position, based on text change. 
//
void FlowParams::
textToSlider(int coord, float newCenter, float newSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	bool centerChanged = false;
	bool sizeChanged = false;
	if (newSize > (regionMax[coord] - regionMin[coord])){
		newSize = (regionMax[coord] - regionMin[coord]);
		sizeChanged = true;
	}
	if (newSize < 0.f) {
		newSize = 0.f;
		sizeChanged = true;
	}
	if (newCenter < regionMin[coord]) {
		newCenter = regionMin[coord];
		centerChanged = true;
	}
	if (newCenter > regionMax[coord]) {
		newCenter = regionMax[coord];
		centerChanged = true;
	}
	if ((newCenter - newSize*0.5f) < regionMin[coord]){
		newCenter = regionMin[coord]+ newSize*0.5f;
		centerChanged = true;
	}
	if ((newCenter + newSize*0.5f) > regionMax[coord]){
		newCenter = regionMax[coord]- newSize*0.5f;
		centerChanged = true;
	}
	if (newSize <= 0.f && !randomGen){
		if (generatorCount[coord] != 1) {
			generatorCount[coord] = 1;
			if (currentDimension == coord)
				myFlowTab->generatorCountEdit->setText("1");
		}
	}
	seedBoxMin[coord] = newCenter - newSize*0.5f; 
	seedBoxMax[coord] = newCenter + newSize*0.5f; 
	int sliderSize = (int)(0.5f+ 256.f*newSize/(regionMax[coord] - regionMin[coord]));
	int sliderCenter = (int)(0.5f+ 256.f*(newCenter - regionMin[coord])/(regionMax[coord] - regionMin[coord]));
	int oldSliderSize, oldSliderCenter;
	switch(coord) {
		case 0:
			oldSliderSize = myFlowTab->xSizeSlider->value();
			oldSliderCenter = myFlowTab->xCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myFlowTab->xSizeSlider->setValue(sliderSize);
			if(sizeChanged) myFlowTab->xSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				myFlowTab->xCenterSlider->setValue(sliderCenter);
			if(centerChanged) myFlowTab->xCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 1:
			oldSliderSize = myFlowTab->ySizeSlider->value();
			oldSliderCenter = myFlowTab->yCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myFlowTab->ySizeSlider->setValue(sliderSize);
			if(sizeChanged) myFlowTab->ySizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				myFlowTab->yCenterSlider->setValue(sliderCenter);
			if(centerChanged) myFlowTab->yCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 2:
			oldSliderSize = myFlowTab->zSizeSlider->value();
			oldSliderCenter = myFlowTab->zCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myFlowTab->zSizeSlider->setValue(sliderSize);
			if(sizeChanged) myFlowTab->zSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				myFlowTab->zCenterSlider->setValue(sliderCenter);
			if(centerChanged) myFlowTab->zCenterEdit->setText(QString::number(newCenter));
			
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	myFlowTab->update();
	return;
}
//Set text when a slider changes.
//Move the center if the size is too big
//
void FlowParams::
sliderToText(int coord, int slideCenter, int slideSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	bool sliderChanged = false;
	
	float newSize = slideSize*(regionMax[coord]-regionMin[coord])/256.f;
	float newCenter = regionMin[coord]+ slideCenter*(regionMax[coord]-regionMin[coord])/256.f;
	
	if (newCenter < regionMin[coord]) {
		newCenter = regionMin[coord];
	}
	if (newCenter > regionMax[coord]) {
		newCenter = regionMax[coord];
	}
	if ((newCenter - newSize*0.5f) < regionMin[coord]){
		newCenter = regionMin[coord]+ newSize*0.5f;
		sliderChanged = true;
	}
	if ((newCenter + newSize*0.5f) > regionMax[coord]){
		newCenter = regionMax[coord]- newSize*0.5f;
		sliderChanged = true;
	}
	seedBoxMin[coord] = newCenter - newSize*0.5f; 
	seedBoxMax[coord] = newCenter + newSize*0.5f; 
	if (newSize <= 0.f && !randomGen){
		if (generatorCount[coord] != 1) {
			generatorCount[coord] = 1;
			if (currentDimension == coord)
				myFlowTab->generatorCountEdit->setText("1");
		}
	}
	int newSliderCenter = (int)(0.5f+ 256.f*(newCenter - regionMin[coord])/(regionMax[coord] - regionMin[coord]));
	//Always need to change text.  Possibly also change slider if it was moved
	switch(coord) {
		case 0:
			if (sliderChanged) 
				myFlowTab->xCenterSlider->setValue(newSliderCenter);
			myFlowTab->xSizeEdit->setText(QString::number(newSize));
			myFlowTab->xCenterEdit->setText(QString::number(newCenter));
			break;
		case 1:
			if (sliderChanged) 
				myFlowTab->yCenterSlider->setValue(newSliderCenter);
			myFlowTab->ySizeEdit->setText(QString::number(newSize));
			myFlowTab->yCenterEdit->setText(QString::number(newCenter));
			break;
		case 2:
			if (sliderChanged) 
				myFlowTab->zCenterSlider->setValue(newSliderCenter);
			myFlowTab->zSizeEdit->setText(QString::number(newSize));
			myFlowTab->zCenterEdit->setText(QString::number(newCenter));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	myFlowTab->update();
	//force a new render with new flow data
	setDirty(true);
	return;
}	




//Methods that record changes in the history:
//
void FlowParams::
guiSetEnabled(bool on){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "enable/disable flow render");
	setEnabled(on);
	PanelCommand::captureEnd(cmd, this);
}
void FlowParams::
guiSetFlowType(int typenum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set flow type");
	setFlowType(typenum);
	PanelCommand::captureEnd(cmd, this);
	
}
void FlowParams::
guiSetNumTrans(int n){
	confirmText(false);
	
	int newNumTrans = ((RegionParams*)(VizWinMgr::getInstance()->getRegionParams(vizNum)))->validateNumTrans(n);
	if (newNumTrans != n) {
		MessageReporter::warningMsg("%s","Invalid number of Transforms for current region, data cache size");
		myFlowTab->numTransSpin->setValue(newNumTrans);
	}
	PanelCommand* cmd = PanelCommand::captureStart(this, "set number of Transformations in Flow data");
	setNumTrans(newNumTrans);
	PanelCommand::captureEnd(cmd, this);
}
void FlowParams::
guiSetXVarNum(int varnum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set X field variable");
	setXVarNum(varnum);
	PanelCommand::captureEnd(cmd, this);
}
void FlowParams::
guiSetYVarNum(int varnum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set Y field variable");
	setYVarNum(varnum);
	PanelCommand::captureEnd(cmd, this);

}
void FlowParams::
guiSetZVarNum(int varnum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set Z field variable");
	setZVarNum(varnum);
	PanelCommand::captureEnd(cmd, this);

}
void FlowParams::
guiRecalc(){ //Not covered by redo/undo
	confirmText(false);
	//Need to implement!
}
void FlowParams::
guiSetRandom(bool rand){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "toggle random setting");
	setRandom(rand);
	//Also display the appropriate numGenerators
	int genCount = allGeneratorCount;
	if (!rand) genCount = generatorCount[currentDimension];
	myFlowTab->generatorCountEdit->setText(QString::number(genCount));
	guiSetTextChanged(false);
	myFlowTab->update();
	PanelCommand::captureEnd(cmd, this);
}
void FlowParams::
guiSetXCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator X center");
	setXCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty(true);
}
void FlowParams::
guiSetYCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator Y center");
	setYCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty(true);
}
void FlowParams::
guiSetZCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator Z center");
	setZCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty(true);
}
void FlowParams::
guiSetXSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator X size");
	setXSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty(true);
}
void FlowParams::
guiSetYSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator Y size");
	setYSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty(true);
}
void FlowParams::
guiSetZSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator Z size");
	setZSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setDirty(true);
}
void FlowParams::
guiSetFlowGeometry(int geomNum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set flow geometry type");
	setFlowGeometry(geomNum);
	PanelCommand::captureEnd(cmd, this);
	//Just force rerender
	VizWinMgr::getInstance()->setFlowDirty(this);
}
void FlowParams::
guiSetMapEntity( int entityNum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set flow colormap entity");
	setMapEntity(entityNum);
	PanelCommand::captureEnd(cmd, this);
	//Just force rerender
	VizWinMgr::getInstance()->setFlowDirty(this);
}
void FlowParams::
guiSetGeneratorDimension( int dimNum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "specify generator dimension");
	setCurrentDimension(dimNum);
	myFlowTab->generatorCountEdit->setText(QString::number(generatorCount[dimNum]));
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, this);
	myFlowTab->update();
	setDirty(true);
}
//When the center slider moves, set the seedBoxMin and seedBoxMax
void FlowParams::
setXCenter(int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(0, sliderval, myFlowTab->xSizeSlider->value());
	setDirty(true);
}
void FlowParams::
setYCenter(int sliderval){
	sliderToText(1, sliderval, myFlowTab->ySizeSlider->value());
	setDirty(true);
}
void FlowParams::
setZCenter(int sliderval){
	sliderToText(2, sliderval, myFlowTab->zSizeSlider->value());
	setDirty(true);
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void FlowParams::
setXSize(int sliderval){
	sliderToText(0, myFlowTab->xCenterSlider->value(),sliderval);
	setDirty(true);
}
void FlowParams::
setYSize(int sliderval){
	sliderToText(1, myFlowTab->yCenterSlider->value(),sliderval);
	setDirty(true);
}
void FlowParams::
setZSize(int sliderval){
	sliderToText(2, myFlowTab->zCenterSlider->value(),sliderval);
	setDirty(true);
}
	
/* Handle the change of status associated with change of enablement and change
 * of local/global.  If we are enabling global, a renderer must be created in every
 * global window, including active one.  If we are enabling local, only active one is created.
 * If we change from local to global, (no change in enablement) then new renderers are
 * created for every additional global window.  Similar for disable.
 * It can occur that both enablement and local/global change, if the local and global enablement
 * are different, during a local/global change
 * This assumes that the VizWinMgr already is set with the current (new) local/global
 * Flow settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void FlowParams::
updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow){
	bool isLocal = local;
	if (newWindow) {
		prevEnabled = false;
		wasLocal = true;
		isLocal = true;
	}
	if (prevEnabled == enabled && wasLocal == local) return;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	VizWin* viz = vizWinMgr->getActiveVisualizer();
	if (!viz) return;
	int activeViz = vizWinMgr->getActiveViz();
	assert(activeViz >= 0);
	
	//Four cases to consider:
	//1.  change of local/global with unchanged disabled renderer; do nothing.
	// Same for change of local/global with unchanged, enabled renderer
	
	if (prevEnabled == enabled) return;
	
	//2.  Change of disable->enable with unchanged local renderer.  Create a new renderer in active window.
	// Also applies to double change: disable->enable and local->global 
	// Also applies to disable->enable with global->local
	//3.  change of disable->enable with unchanged global renderer.  Create new renderers in all global windows, 
	//    including active window.
	//5.  Change of enable->disable with unchanged global , disable all global renderers, provided the
	//   VizWinMgr already has the current local/global Flow settings
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	//  change of enable->disable with global->local.  (Must disable the local renderer)
	//  change of enable->disable with local->global (Must disable the local renderer)
	
	//For a new renderer

	
	if (enabled && !prevEnabled){//For cases 2. or 3. :  create a renderer in the active window:
		FlowRenderer* myRenderer = new FlowRenderer (viz);
		viz->prependRenderer(myRenderer, Params::FlowParamsType);
		//Quit if not case 3:
		if (wasLocal || isLocal) return;
	}
	
	if (!isLocal && enabled){ //case 3: create renderers in all *other* global windows, then return
		for (int i = 0; i<MAXVIZWINS; i++){
			if (i == activeViz) continue;
			viz = VizWinMgr::getInstance()->getVizWin(i);
			if (viz && !vizWinMgr->getFlowParams(i)->isLocal()){
				FlowRenderer* myRenderer = new FlowRenderer (viz);
				viz->prependRenderer(myRenderer, Params::FlowParamsType);
			}
		}
		return;
	}
	if (!enabled && prevEnabled && !isLocal && !wasLocal) { //case 5., disable all global renderers
		for (int i = 0; i<MAXVIZWINS; i++){
			viz = VizWinMgr::getInstance()->getVizWin(i);
			if (viz && !vizWinMgr->getFlowParams(i)->isLocal()){
				viz->removeRenderer(Params::FlowParamsType);
			}
		}
		return;
	}
	assert(prevEnabled && !enabled && (isLocal ||(isLocal != wasLocal))); //case 6, disable local only
	viz->removeRenderer(Params::FlowParamsType);
	
	return;

}

void FlowParams::
setEnabled(bool on){
	if (enabled == on) return;
	enabled = on;
	if (!enabled){//delete existing flow lib
		if (myFlowLib) delete myFlowLib;
		myFlowLib = 0;
		return;
	}
	//create a new flow lib:
	assert(myFlowLib == 0);
	DataMgr* dataMgr = Session::getInstance()->getDataMgr();
	assert(dataMgr);
	myFlowLib = new VaporFlow(dataMgr);
	return;
	
}

float* FlowParams::
regenerateFlowData(){
	int min_dim[3], max_dim[3]; 
	size_t min_bdim[3], max_bdim[3];
	float minFull[3], maxFull[3], extents[6];
	if (!myFlowLib) return 0;
	if (flowData) delete flowData;
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//specify field components:
	const char* xVar = variableNames[varNum[0]].c_str();
	const char* yVar = variableNames[varNum[1]].c_str();
	const char* zVar = variableNames[varNum[2]].c_str();
	myFlowLib->SetFieldComponents(xVar, yVar, zVar);
	//If these flowparams are shared, we have a problem here; 
	//Different windows could have different regions
	RegionParams* rParams;
	if (vizNum < 0) {
		MessageReporter::warningMsg("FlowParams: Multiple region params may apply to flow");
		rParams = vizMgr->getRegionParams(vizMgr->getActiveViz());
	}
	else rParams = VizWinMgr::getInstance()->getRegionParams(vizNum);
	rParams->calcRegionExtents(min_dim, max_dim, min_bdim, max_bdim, 
		numTrans, minFull, maxFull, extents);
	myFlowLib->SetRegion(numTrans, min_bdim, max_bdim);
	myFlowLib->SetTimeStepInterval(seedTimeStart, maxFrame, timeSamplingInterval);
	myFlowLib->ScaleTimeStepSizes(userTimeStepMultiplier, 1./objectsPerTimestep);
	if (randomGen) {
		myFlowLib->SetRandomSeedPoints(seedBoxMin, seedBoxMax, allGeneratorCount);
		numSeedPoints = allGeneratorCount;
	} else {
		myFlowLib->SetRegularSeedPoints(seedBoxMin, seedBoxMax, generatorCount);
		numSeedPoints = generatorCount[0]*generatorCount[1]*generatorCount[2];
	}
	// setup integration parameters:
	float minIntegStep = (1.f - integrationAccuracy)* 5.f;//go from 0 to 5
	float maxIntegStep = 3.f*minIntegStep;
	myFlowLib->SetIntegrationParams(minIntegStep, maxIntegStep);
	//Parameters controlling flowDataAccess.  These are established each time
	//The flow data is regenerated:
	
	
	if (flowType == 0) { //steady
		numInjections = 1;
		maxPoints = maxAgeShown+1;
	} else {// determine largest possible number of injections
		numInjections = 1+ (seedTimeEnd - seedTimeStart)/seedTimeIncrement;
		//For unsteady flow, the minAge and maxAge give a window
		//on a longer timespan that potentially goes from 
		//the first seed start time to the last frame in the animation.
		//maxAge does not limit the length of the flow
		maxPoints = maxFrame - seedTimeStart+1;
	}
	flowData = new float[3*maxPoints*numSeedPoints*numInjections];

	/* For now, don't call the flowlib
	if (flowType == 0){ //steady
		myFlowLib->GenStreamLines(flowData, maxPoints, randomSeed);
	} else {
		myFlowLib->GenStreakLines(flowData, maxPoints, randomSeed, seedTimeStart, lastTime, seedTimeIncrement);
	}
	*/
	//test stream code, not using flowlib:
	if (flowType == 0){
		float* seeds = new float[3*numSeedPoints];
		float seedSep = (extents[3]-extents[0])*0.1f;
		//Put the seed(s) at the middle of the bottom:
		for (int j = 0; j < numSeedPoints; j++){
			seeds[3*j] = 0.5*(rParams->getFullDataExtent(0) + rParams->getFullDataExtent(3));
			seeds[3*j+1] = rParams->getFullDataExtent(1);
			seeds[3*j+2] = 0.5*(rParams->getFullDataExtent(2) + rParams->getFullDataExtent(5));
			//increment x coord by seedSep.
			seeds[3*j] += (seedSep*(float)j);
		}

		//Just specify a line up the center of the region
		float diag[3];
		diag[0] = 0.01f;
		diag[1] = rParams->getFullDataExtent(4) - rParams->getFullDataExtent(1);
		diag[2] = 0.01f;
		
		
		for (int j = 0; j< numSeedPoints; j++){
			for (int k = 0; k< maxPoints; k++) {
				for (int coord = 0; coord < 3; coord++){
					flowData[coord + 3*(k + maxPoints *j)] = 
						seeds[coord+3*j] + k*diag[coord]/maxPoints;
				}
			}
		}
		delete seeds;
	} else { //unsteady flow data, repeat same seed at each injectionTime
		float* seeds = new float[3*numSeedPoints];
		float seedSep = (extents[3]-extents[0])*0.1f;
		//Put the seed(s) at the middle of the bottom:
		for (int j = 0; j < numSeedPoints; j++){
			seeds[3*j] = 0.5*(rParams->getFullDataExtent(0) + rParams->getFullDataExtent(3));
			seeds[3*j+1] = rParams->getFullDataExtent(1);
			seeds[3*j+2] = 0.5*(rParams->getFullDataExtent(2) + rParams->getFullDataExtent(5));
			//increment x coord by seedSep.
			seeds[3*j] += (seedSep*(float)j);
		}

		//Just specify a line up the center of the region, slightly off to avoid aliasing
		float diag[3];
		
		
		for (int q = 0; q<numInjections; q++){
			//Make line tilt more (x and z) with each subsequent injection:
			diag[0] = 0.01f + 0.05*(float)q;
			diag[1] = 0.01f + rParams->getFullDataExtent(4) - rParams->getFullDataExtent(1);
			diag[2] = 0.01f + 0.05*(float)q;
			for (int j = 0; j< numSeedPoints; j++){
				for (int k = 0; k< maxPoints; k++) {
					for (int coord = 0; coord < 3; coord++){
						flowData[coord + 3*(k + maxPoints *(j + numSeedPoints*q))] = 
							seeds[coord+3*j] + k*diag[coord]/maxPoints;
					}
				}
			}
		}
		delete seeds;
	}
	//end test code
	//Clear the dirty flag (just for the one renderer).
	dirty = false;
	return flowData;
}
void FlowParams::setDirty(bool isDirty){
	dirty = isDirty;
	if (dirty) VizWinMgr::getInstance()->setFlowDirty(this);
}