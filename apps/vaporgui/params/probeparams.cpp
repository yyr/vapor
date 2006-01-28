//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		probeparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Implementation of the probeparams class
//		This contains all the parameters required to support the
//		probe renderer.  Embeds a transfer function and a
//		transfer function editor.
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include <qlineedit.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qlistbox.h>
#include <qapplication.h>
#include <qcursor.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "glutil.h"
#include "probeparams.h"
#include "probeframe.h"
#include "glprobewindow.h"
#include "probetab.h" 
#include "mainform.h"
#include "glbox.h"
#include "vizwin.h"
#include "params.h"
#include "vizwinmgr.h"
#include "panelcommand.h"
#include "session.h"
#include "params.h"
#include "transferfunction.h"
#include "tfeditor.h"
#include "tfframe.h"
#include "messagereporter.h"
#include "tabmanager.h"
#include "histo.h"
#include "animationparams.h"

#include <math.h>
#include <vapor/Metadata.h>


using namespace VAPoR;
const string ProbeParams::_editModeAttr = "TFEditMode";
const string ProbeParams::_histoStretchAttr = "HistoStretchFactor";
const string ProbeParams::_variableSelectedAttr = "VariableSelected";
const string ProbeParams::_geometryTag = "ProbeGeometry";
const string ProbeParams::_probeMinAttr = "ProbeMin";
const string ProbeParams::_probeMaxAttr = "ProbeMax";
const string ProbeParams::_cursorCoordsAttr = "CursorCoords";
const string ProbeParams::_phiAttr = "PhiAngle";
const string ProbeParams::_thetaAttr = "ThetaAngle";
const string ProbeParams::_numTransformsAttr = "NumTransforms";


ProbeParams::ProbeParams(int winnum) : Params(winnum){
	thisParamType = ProbeParamsType;
	myProbeTab = MainForm::getInstance()->getProbeTab();
	numVariables = 0;
	probeTexture = 0;
	probeDirty = true;
	restart();
	
}
ProbeParams::~ProbeParams(){
	
	if (savedCommand) delete savedCommand;
	
	if (transFunc){
		for (int i = 0; i< numVariables; i++){
			delete transFunc[i];  //will delete editor
		}
		delete transFunc;
	}
	
}

void ProbeParams::
setTab(ProbeTab* tab) {
	myProbeTab = tab;
	for (int i = 0; i<numVariables; i++){
		transFunc[i]->getEditor()->setFrame(myProbeTab->ProbeTFFrame);
	}
}
//Deepcopy requires cloning tf and tfeditor
Params* ProbeParams::
deepCopy(){
	ProbeParams* newParams = new ProbeParams(*this);
	//Clone the map bounds arrays:
	int numVars = Max (numVariables, 1);
	newParams->minColorEditBounds = new float[numVars];
	newParams->maxColorEditBounds = new float[numVars];
	newParams->minOpacEditBounds = new float[numVars];
	newParams->maxOpacEditBounds = new float[numVars];
	for (int i = 0; i<numVars; i++){
		newParams->minColorEditBounds[i] = minColorEditBounds[i];
		newParams->maxColorEditBounds[i] = maxColorEditBounds[i];
		newParams->minOpacEditBounds[i] = minOpacEditBounds[i];
		newParams->maxOpacEditBounds[i] = maxOpacEditBounds[i];
	}

	//Clone the Transfer Function and the TFEditor
	newParams->transFunc = new TransferFunction*[numVariables];
	for (int i = 0; i<numVariables; i++){
		newParams->transFunc[i] = new TransferFunction(*transFunc[i]);
		//clone the tfe, hook it to the trans func
		TFEditor* newTFEditor = new TFEditor(*(TFEditor*)(transFunc[i]->getEditor()));
		newParams->connectMapperFunction(newParams->transFunc[i],newTFEditor); 
	}
	//Probe texture must be recreated when needed
	newParams->probeTexture = 0;
	newParams->probeDirty = true;
	//never keep the SavedCommand:
	newParams->savedCommand = 0;
	newParams->attachedFlow = 0;
	newParams->histogramList = 0;
	newParams->numHistograms = 0;
	newParams->seedAttached = false;
	return newParams;
}
//Method called when undo/redo changes params:
void ProbeParams::
makeCurrent(Params* prevParams, bool newWin) {
	
	VizWinMgr::getInstance()->setProbeParams(vizNum, this);

	updateDialog();
	ProbeParams* formerParams = (ProbeParams*)prevParams;
	//Check if the enabled and/or Local settings changed:
	if (formerParams->enabled != enabled || formerParams->local != local || newWin){
		updateRenderer(formerParams->enabled, formerParams->local, newWin);
	}
	setDatarangeDirty();
	setClutDirty();
	probeDirty = true;
}

void ProbeParams::
refreshCtab() {
	((TransferFunction*)getMapperFunc())->makeLut((float*)ctab);
}
	
//Set the tab consistent with the probeparams data here
//
void ProbeParams::updateDialog(){
	
	QString strn;
	Session::getInstance()->blockRecording();
	//Don't respond to textChange that this method generates:
	guiSetTextChanged(false);
	myProbeTab->ProbeTFFrame->setEditor(getTFEditor());
	myProbeTab->probeTextureFrame->setParams(this);
	myProbeTab->EnableDisable->setCurrentItem((enabled) ? 1 : 0);
	
	
	myProbeTab->refinementCombo->setMaxCount(maxNumRefinements+1);
	myProbeTab->refinementCombo->setCurrentItem(numRefinements);
	

	//Set the selection in the variable listbox
	for (int i = 0; i< numVariables; i++){
		if (variableNames.size() > (unsigned int)i){
			if (myProbeTab->variableListBox->isSelected(i) != variableSelected[i])
				myProbeTab->variableListBox->setSelected(i, variableSelected[i]);
		} else assert(0);
	}
	//Set sliders and text:
	myProbeTab->histoScaleEdit->setText(QString::number(histoStretchFactor));
	for (int i = 0; i< 3; i++){
		textToSlider(i, (probeMin[i]+probeMax[i])*0.5f,probeMax[i]-probeMin[i]);
	}
	myProbeTab->xCenterEdit->setText(QString::number((probeMax[0]+probeMin[0])*0.5f));
	myProbeTab->yCenterEdit->setText(QString::number((probeMax[1]+probeMin[1])*0.5f));
	myProbeTab->zCenterEdit->setText(QString::number((probeMax[2]+probeMin[2])*0.5f));
	myProbeTab->xSizeEdit->setText(QString::number(probeMax[0]-probeMin[0]));
	myProbeTab->ySizeEdit->setText(QString::number(probeMax[1]-probeMin[1]));
	myProbeTab->zSizeEdit->setText(QString::number(probeMax[2]-probeMin[2]));
	myProbeTab->thetaEdit->setText(QString::number(theta));
	myProbeTab->phiEdit->setText(QString::number(phi));
	float* selectedPoint = getSelectedPoint();
	myProbeTab->selectedXLabel->setText(QString::number(selectedPoint[0]));
	myProbeTab->selectedYLabel->setText(QString::number(selectedPoint[1]));
	myProbeTab->selectedZLabel->setText(QString::number(selectedPoint[2]));
	myProbeTab->attachSeedCheckbox->setChecked(seedAttached);
	float val = calcCurrentValue(selectedPoint);
	if (val == OUT_OF_BOUNDS)
		myProbeTab->valueMagLabel->setText(QString(" "));
	else myProbeTab->valueMagLabel->setText(QString::number(val));

	if (isLocal())
		myProbeTab->LocalGlobal->setCurrentItem(1);
	else 
		myProbeTab->LocalGlobal->setCurrentItem(0);

	updateMapBounds();
	float sliderVal = 1.f;
	if (getTFEditor())
		sliderVal = getTFEditor()->getOpacityScaleFactor();
	QToolTip::add(myProbeTab->opacityScaleSlider,"Opacity Scale Value "+QString::number(sliderVal*sliderVal));
	sliderVal = 256.f*(1.f-sliderVal);
	myProbeTab->opacityScaleSlider->setValue((int) sliderVal);
	setBindButtons();
	
	//Set the mode buttons:
	
	if (editMode){
		
		myProbeTab->editButton->setOn(true);
		myProbeTab->navigateButton->setOn(false);
	} else {
		myProbeTab->editButton->setOn(false);
		myProbeTab->navigateButton->setOn(true);
	}
		
	myProbeTab->probeTextureFrame->setTextureSize(probeMax[0]-probeMin[0],probeMax[1]-probeMin[1]);
	if(getTFEditor())getTFEditor()->setDirty();
	myProbeTab->ProbeTFFrame->update();
	myProbeTab->update();
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
		
}
//Update all the panel state associated with textboxes.
//Performed whenever a textbox changes and user pressed "enter"
//
void ProbeParams::
updatePanelState(){
	QString strn;
	
	theta = myProbeTab->thetaEdit->text().toFloat();
	phi = myProbeTab->phiEdit->text().toFloat();
	histoStretchFactor = myProbeTab->histoScaleEdit->text().toFloat();

	//Get slider positions from text boxes:
		
	float boxCtr = myProbeTab->xCenterEdit->text().toFloat();
	float boxSize = myProbeTab->xSizeEdit->text().toFloat();
	probeMin[0] = boxCtr - 0.5*boxSize;
	probeMax[0] = boxCtr + 0.5*boxSize;
	textToSlider(0, boxCtr, boxSize);
	boxCtr = myProbeTab->yCenterEdit->text().toFloat();
	boxSize = myProbeTab->ySizeEdit->text().toFloat();
	probeMin[1] = boxCtr - 0.5*boxSize;
	probeMax[1] = boxCtr + 0.5*boxSize;
	textToSlider(1, boxCtr, boxSize);
	boxCtr = myProbeTab->zCenterEdit->text().toFloat();
	boxSize = myProbeTab->zSizeEdit->text().toFloat();
	probeMin[2] = boxCtr - 0.5*boxSize;
	probeMax[2] = boxCtr + 0.5*boxSize;
	textToSlider(2, boxCtr, boxSize);
	myProbeTab->probeTextureFrame->setTextureSize(probeMax[0]-probeMin[0],probeMax[1]-probeMin[1]);
	probeDirty = true;
	if (numVariables > 0) {
		((TransferFunction*)getMapperFunc())->setMinMapValue(myProbeTab->leftMappingBound->text().toFloat());
		((TransferFunction*)getMapperFunc())->setMaxMapValue(myProbeTab->rightMappingBound->text().toFloat());
	
		setDatarangeDirty();
		getTFEditor()->setDirty();
		myProbeTab->update();
		myProbeTab->probeTextureFrame->update();
	}

	guiSetTextChanged(false);
	
	//If we are in probe mode, force a rerender of all windows using the probe:
	if (MainForm::getInstance()->getCurrentMouseMode() == Command::probeMode){
		VizWinMgr::getInstance()->refreshProbe(this);
	}

	
}

//Set slider position, based on text change. 
//Requirement is that center is inside full domain.
//
void ProbeParams::
textToSlider(int coord, float newCenter, float newSize){
	//force the new center to fit in the full domain,
	
	
	bool centerChanged = false;
	const float* extents = Session::getInstance()->getExtents();
	float regMin = extents[coord];
	float regMax = extents[coord+3];
	if (newCenter < regMin) {
		newCenter = regMin;
		centerChanged = true;
	}
	if (newCenter > regMax) {
		newCenter = regMax;
		centerChanged = true;
	}
	
	probeMin[coord] = newCenter - newSize*0.5f; 
	probeMax[coord] = newCenter + newSize*0.5f; 
	
	int sliderSize = (int)(0.5f+ 256.f*newSize/(regMax - regMin));
	int sliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	int oldSliderSize, oldSliderCenter;
	switch(coord) {
		case 0:
			oldSliderSize = myProbeTab->xSizeSlider->value();
			oldSliderCenter = myProbeTab->xCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myProbeTab->xSizeSlider->setValue(sliderSize);
			
			if (oldSliderCenter != sliderCenter)
				myProbeTab->xCenterSlider->setValue(sliderCenter);
			if(centerChanged) myProbeTab->xCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 1:
			oldSliderSize = myProbeTab->ySizeSlider->value();
			oldSliderCenter = myProbeTab->yCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myProbeTab->ySizeSlider->setValue(sliderSize);
			
			if (oldSliderCenter != sliderCenter)
				myProbeTab->yCenterSlider->setValue(sliderCenter);
			if(centerChanged) myProbeTab->yCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 2:
			oldSliderSize = myProbeTab->zSizeSlider->value();
			oldSliderCenter = myProbeTab->zCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myProbeTab->zSizeSlider->setValue(sliderSize);
			
			if (oldSliderCenter != sliderCenter)
				myProbeTab->zCenterSlider->setValue(sliderCenter);
			if(centerChanged) myProbeTab->zCenterEdit->setText(QString::number(newCenter));
			
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	probeDirty=true;
	myProbeTab->update();
	return;
}
//Set text when a slider changes.
//
void ProbeParams::
sliderToText(int coord, int slideCenter, int slideSize){
	
	//force the center to fit in the region.  
	const float* extents = Session::getInstance()->getExtents();
	float regMin = extents[coord];
	float regMax = extents[coord+3];
	float newSize = slideSize*(regMax-regMin)/256.f;
	float newCenter = regMin+ slideCenter*(regMax-regMin)/256.f;
	
	probeMin[coord] = newCenter - newSize*0.5f; 
	probeMax[coord] = newCenter + newSize*0.5f;
	float* selectedPoint = getSelectedPoint();
	
	switch(coord) {
		case 0:
			myProbeTab->xSizeEdit->setText(QString::number(newSize));
			myProbeTab->xCenterEdit->setText(QString::number(newCenter));
			myProbeTab->selectedXLabel->setText(QString::number(selectedPoint[coord]));
			break;
		case 1:
			myProbeTab->ySizeEdit->setText(QString::number(newSize));
			myProbeTab->yCenterEdit->setText(QString::number(newCenter));
			myProbeTab->selectedYLabel->setText(QString::number(selectedPoint[coord]));
			break;
		case 2:
			myProbeTab->zSizeEdit->setText(QString::number(newSize));
			myProbeTab->zCenterEdit->setText(QString::number(newCenter));
			myProbeTab->selectedZLabel->setText(QString::number(selectedPoint[coord]));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	myProbeTab->probeTextureFrame->setTextureSize(probeMax[0]-probeMin[0],probeMax[1]-probeMin[1]);
	myProbeTab->update();
	//force a new render with new Probe data
	setProbeDirty();
	return;
}	




/*
 * Method to be invoked after the user has moved the right or left bounds
 * (e.g. From the TFE editor. ) 
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void ProbeParams::
updateMapBounds(){
	QString strn;
	int currentTimeStep = VizWinMgr::getInstance()->getAnimationParams(vizNum)->getCurrentFrameNumber();
	myProbeTab->minDataBound->setText(strn.setNum(getDataMinBound(currentTimeStep)));
	myProbeTab->maxDataBound->setText(strn.setNum(getDataMaxBound(currentTimeStep)));
	if (getMapperFunc()){
		myProbeTab->leftMappingBound->setText(strn.setNum(getMapperFunc()->getMinColorMapValue(),'g',4));
		myProbeTab->rightMappingBound->setText(strn.setNum(getMapperFunc()->getMaxColorMapValue(),'g',4));
	} else {
		myProbeTab->leftMappingBound->setText("0.0");
		myProbeTab->rightMappingBound->setText("1.0");
	}
	setDatarangeDirty();
	setProbeDirty();
}

void ProbeParams::
setClut(const float newTable[256][4]){
	for (int i = 0; i< 256; i++) {
		for (int j = 0; j< 4; j++){
			ctab[i][j] = newTable[i][j];
		}
	}
	setClutDirty();
}
//Make probe match region.  Responds to button in region panel
void ProbeParams::
guiCopyRegionToProbe(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "copy region to probe");
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	for (int i = 0; i< 3; i++){
		probeMin[i] = rParams->getRegionMin(i);
		probeMax[i] = rParams->getRegionMax(i);
	}
	
	setProbeDirty();
	PanelCommand::captureEnd(cmd,this);
	VizWinMgr::getInstance()->refreshProbe(this);
	
}
//Change mouse mode to specified value
//0,1,2 correspond to edit, zoom, pan
void ProbeParams::
guiSetEditMode(bool mode){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "set edit/navigate mode");
	setEditMode(mode);
	PanelCommand::captureEnd(cmd, this);
}
void ProbeParams::
guiSetAligned(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "align tf in edit frame");
	setMinEditBound(getMinColorMapBound());
	setMaxEditBound(getMaxColorMapBound());
	if(getTFEditor())getTFEditor()->setDirty();
	myProbeTab->ProbeTFFrame->update();
	PanelCommand::captureEnd(cmd, this);
}
void ProbeParams::
guiSetEnabled(bool value){
	
	confirmText(false);
	assert(value != enabled);
	PanelCommand* cmd = PanelCommand::captureStart(this, "toggle probe enabled");
	setEnabled(value);
	//if we are disabling, must destroy the latest texture
	if (!enabled && probeTexture){
		delete probeTexture;
		probeTexture = 0;
	}
	
	PanelCommand::captureEnd(cmd, this);
	//Need to rerender the texture:
	setProbeDirty(true);
	//and refresh the gui
	updateDialog();
}


//Respond to a change in opacity scale factor
void ProbeParams::
guiSetOpacityScale(int val){
	if (!getTFEditor()) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "modify opacity scale slider");
	setOpacityScale( ((float)(256-val))/256.f);
	float sliderVal = getOpacityScale();
	QToolTip::add(myProbeTab->opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	setClutDirty();
	getTFEditor()->setDirty();
	setProbeDirty();
	myProbeTab->ProbeTFFrame->update();
	
	PanelCommand::captureEnd(cmd,this);
	VizWinMgr::getInstance()->refreshProbe(this);
}
float ProbeParams::getOpacityScale() {
	return (getTFEditor() ? getTFEditor()->getOpacityScaleFactor() : 1.f );
}
void ProbeParams::setOpacityScale(float val) {
	if (getTFEditor()) getTFEditor()->setOpacityScaleFactor(val);
}
//Respond to a change in transfer function (from color selection or mouse down/release events)
//These are just for undo/redo.  Also may need to update visualizer and/or editor
//
void ProbeParams::
guiStartChangeMapFcn(char* str){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	savedCommand = PanelCommand::captureStart(this, str);
}
void ProbeParams::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand,this);
	savedCommand = 0;
	setProbeDirty();
}

void ProbeParams::
guiBindColorToOpac(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "bind Color to Opacity");
	getTFEditor()->bindColorToOpac();
	PanelCommand::captureEnd(cmd, this);
}
void ProbeParams::
guiBindOpacToColor(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "bind Opacity to Color");
	getTFEditor()->bindOpacToColor();
	PanelCommand::captureEnd(cmd, this);
}
//Make the probe center at selectedPoint.  Shrink size if necessary.
//Reset sliders and text as appropriate.  Equivalent to typing in the values
void ProbeParams::guiCenterProbe(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "Center Probe to Selected Point");
	float* selectedPoint = getSelectedPoint();
	for (int i = 0; i<3; i++)
		textToSlider(i,selectedPoint[i], probeMax[i]-probeMin[i]);
	PanelCommand::captureEnd(cmd, this);
	updateDialog();
	setProbeDirty();

}
//Following method sets up (or releases) a connection to the Flow 
void ProbeParams::
guiAttachSeed(bool attach, FlowParams* fParams){
	confirmText(false);
	//Don't capture the attach/detach event.
	//This cannot be easily undone/redone because it requires maintaining the
	//state of both the flowparams and the probeparams.
	//But we will capture the successive seed moves that occur while
	//the seed is attached.
	if (attach){ 
		attachedFlow = fParams;
		seedAttached = true;
		
	} else {
		seedAttached = false;
		attachedFlow = 0;
	} 
}
//Respond to an update of the variable listbox.  set the appropriate bits
void ProbeParams::
guiChangeVariables(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "change probe-selected variable(s)");
	int firstVar = -1;
	int numSelected = 0;
	for (int i = 0; i< numVariables; i++){
		if (myProbeTab->variableListBox->isSelected(i)){
			variableSelected[i] = true;
			if(firstVar == -1) firstVar = i;
			numSelected++;
		}
		else 
			variableSelected[i] = false;
	}
	//If nothing is selected, select the first one:
	if (firstVar == -1) {
		myProbeTab->variableListBox->setSelected(0, true);
		firstVar = 0;
		numSelected = 1;
	}
	numVariablesSelected = numSelected;
	firstVarNum = firstVar;
	//reset the editing display range shown on the tab, 
	//this also sets dirty flag
	updateMapBounds();
	//Force a redraw of tfframe 
	if (getTFEditor()) {
		getTFEditor()->setDirty();
		myProbeTab->ProbeTFFrame->setEditor(getTFEditor());
		connectMapperFunction(getMapperFunc(),getTFEditor());
	}
		
	
	PanelCommand::captureEnd(cmd, this);
	setClutDirty();
	myProbeTab->update();
	setProbeDirty();
}
void ProbeParams::
guiSetXCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide probe X center");
	setXCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setProbeDirty();
	
}
void ProbeParams::
guiSetYCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide probe Y center");
	setYCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setProbeDirty();
	
}
void ProbeParams::
guiSetZCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide probe Z center");
	setZCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setProbeDirty();

}
void ProbeParams::
guiSetXSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide probe X size");
	setXSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	myProbeTab->probeTextureFrame->setTextureSize(probeMax[0]-probeMin[0],probeMax[1]-probeMin[1]);
	setProbeDirty();

}
void ProbeParams::
guiSetYSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide probe Y size");
	setYSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	myProbeTab->probeTextureFrame->setTextureSize(probeMax[0]-probeMin[0],probeMax[1]-probeMin[1]);
	setProbeDirty();

}
void ProbeParams::
guiSetZSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide probe Z size");
	setZSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setProbeDirty();

}
void ProbeParams::
guiSetNumRefinements(int n){
	
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "set number Refinements for probe");
	if (Session::getInstance()->getDataStatus()) {
		maxNumRefinements = Session::getInstance()->getDataStatus()->getNumTransforms();
		if (n > maxNumRefinements) {
			MessageReporter::warningMsg("%s","Invalid number of Refinements for current data");
			n = maxNumRefinements;
			myProbeTab->refinementCombo->setCurrentItem(n);
		}
	} else if (n > maxNumRefinements) maxNumRefinements = n;
	setNumRefinements(n);
	PanelCommand::captureEnd(cmd, this);
	setProbeDirty();
}
	
void ProbeParams::
setBindButtons(){
	bool enable = false;
	if (getTFEditor())
		enable = getTFEditor()->canBind();
	myProbeTab->OpacityBindButton->setEnabled(enable);
	myProbeTab->ColorBindButton->setEnabled(enable);
}
void ProbeParams::setClutDirty(){ 
	//Eventually we need to notify the rendering window:
	//VizWinMgr::getInstance()->setClutDirty(this);
	//For now just force a redraw of the tab:
	clutDirty = true;
	if (myProbeTab) myProbeTab->update();
}

/* Handle the change of status associated with change of enablement and change
 * of local/global.  If we are enabling global, a renderer must be created in every
 * global window, including active one.  If we are enabling local, only active one is created.
 * If we change from local to global, (no change in enablement) then new renderers are
 * created for every additional global window.  Similar for disable.
 * It can occur that both enablement and local/global change, if the local and global enablement
 * are different, during a local/global change
 * This assumes that the VizWinMgr already is set with the current (new) local/global
 * dvr settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void ProbeParams::
updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow){
	
	bool newLocal = local;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	if (newWindow) {
		prevEnabled = false;
		wasLocal = true;
		newLocal = true;
	}
	//The actual enabled state of "this" depends on whether we are local or global.
	bool nowEnabled = enabled;
	if (!local) nowEnabled = vizWinMgr->getGlobalParams(Params::ProbeParamsType)->isEnabled();
	
	if (prevEnabled == nowEnabled && wasLocal == local) return;
	/*
	VizWin* viz = 0;
	if(getVizNum() >= 0){//Find the viz that this applies to:
		//Note that this is only for the cases below where one particular
		//visualizer is needed
		viz = vizWinMgr->getVizWin(getVizNum());
	} 
	
	//Four cases to consider:
	//1.  change of local/global with unchanged disabled renderer; do nothing.
	// If change of local/global with enabled renderer, just force refresh:
	
	if (prevEnabled == nowEnabled) {
		if (!prevEnabled) return;
		setClutDirty();
		setDatarangeDirty();
		VizWinMgr::getInstance()->setRegionDirty(this);
		//Need to dirty the region, too, since variable can change as a 
		//consequence of changing local/global
		return;
	}
	
	//2.  Change of disable->enable with unchanged local renderer.  Create a new renderer in active window.
	// Also applies to double change: disable->enable and local->global 
	// Also applies to disable->enable with global->local
	//3.  change of disable->enable with unchanged global renderer.  Create new renderers in all global windows, 
	//    including active window.
	
	
	//5.  Change of enable->disable with unchanged global , disable all global renderers, provided the
	//   VizWinMgr already has the current local/global probe settings
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	//  change of enable->disable with global->local.  (Must disable the local renderer)
	//  change of enable->disable with local->global (Must disable the local renderer)
	
	//For a new renderer

	
	if (nowEnabled && !prevEnabled && newLocal){//For case 2.:  create a renderer in the active window:

		VolumizerRenderer* myProbe = new VolumizerRenderer(viz);
		viz->appendRenderer(myProbe, ProbeParamsType);

		//force the renderer to refresh region data  (why?)
		//Maybe should use setRegionDirty(this)????
		viz->setRegionDirty(true);
		setClutDirty();
		setDatarangeDirty();
		
		//Quit 
		return;
	}
	
	if (!newLocal && nowEnabled){ //case 3: create renderers in all  global windows, then return
		for (int i = 0; i<MAXVIZWINS; i++){
			
			viz = vizWinMgr->getVizWin(i);
			if (viz && !vizWinMgr->getProbeParams(i)->isLocal()){
				VolumizerRenderer* myProbe = new VolumizerRenderer(viz);
				viz->appendRenderer(myProbe, ProbeParamsType);
				//force the renderer to refresh region data (??)
				viz->setRegionDirty(true);
				setClutDirty();
				setDatarangeDirty();
			}
		}
		return;
	}
	if (!nowEnabled && prevEnabled && !newLocal && !wasLocal) { //case 5., disable all global renderers
		for (int i = 0; i<MAXVIZWINS; i++){
			viz = vizWinMgr->getVizWin(i);
			if (viz && !vizWinMgr->getProbeParams(i)->isLocal()){
				viz->removeRenderer(ProbeParamsType);
			}
		}
		return;
	}
	assert(prevEnabled && !nowEnabled && (newLocal ||(newLocal != wasLocal))); //case 6, disable local only
	viz->removeRenderer(ProbeParamsType);
*/
	return;
}
//Initialize for new metadata.  Keep old transfer functions
//
void ProbeParams::
reinit(bool doOverride){
	int i;
	const Metadata* md = Session::getInstance()->getCurrentMetadata();
	const float* extents = Session::getInstance()->getExtents();
	setMaxNumRefinements(md->GetNumTransforms());
	//Set up the numRefinements combo
	
	myProbeTab->refinementCombo->setMaxCount(maxNumRefinements+1);
	myProbeTab->refinementCombo->clear();
	for (i = 0; i<= maxNumRefinements; i++){
		myProbeTab->refinementCombo->insertItem(QString::number(i));
	}
	
	//Either set the probe bounds to a default size in the center of the domain, or 
	//try to use the previous bounds:
	if (doOverride){
		for (int i = 0; i<3; i++){
			float probeRadius = 0.1f*(extents[i+3] - extents[i]);
			float probeMid = 0.5f*(extents[i+3] + extents[i]);
			if (i<2) {
				probeMin[i] = probeMid - probeRadius;
				probeMax[i] = probeMid + probeRadius;
			} else {
				probeMin[i] = probeMax[i] = probeMid;
			}
		}
		//default values for phi, theta, attachedSeed, cursorPosition
		phi = 0.f;
		theta = 0.f;
		seedAttached = false;
		cursorCoords[0] = cursorCoords[1] = 0.0f;
		numRefinements = 0;
	} else {
		//Force the probe size to be no larger than the domain, and 
		//force the probe center to be inside the domain
		for (i = 0; i<3; i++){
			if (probeMax[i] - probeMin[i] > (extents[i+3]-extents[i]))
				probeMax[i] = probeMin[i] + extents[i+3]-extents[i];
			float center = 0.5f*(probeMin[i]+probeMax[i]);
			if (center < extents[i]) {
				probeMin[i] += (extents[i]-center);
				probeMax[i] += (extents[i]-center);
			}
			if (center > extents[i+3]) {
				probeMin[i] += (extents[i+3]-center);
				probeMax[i] += (extents[i+3]-center);
			}
			if(probeMax[i] < probeMin[i]) 
				probeMax[i] = probeMin[i];
		}
		if (numRefinements > maxNumRefinements) numRefinements = maxNumRefinements;
	}
	//Get the variable names:
	variableNames = md->GetVariableNames();
	int newNumVariables = md->GetVariableNames().size();
	
	//See if current firstVarNum is valid
	//if not, reset to first variable that is present:
	if (!Session::getInstance()->getDataStatus()->variableIsPresent(firstVarNum)){
		firstVarNum = -1;
		for (i = 0; i<newNumVariables; i++) {
			if (Session::getInstance()->getDataStatus()->variableIsPresent(i)){
				firstVarNum = i;
				break;
			}
		}
	}
	if (firstVarNum == -1){
		MessageReporter::errorMsg("Probe Params: No data in specified dataset");
		for (i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete transFunc;
		numVariables = 0;
		return;
	}
	//Set up the selected variables.
	if (doOverride){//default is to only select the first variable.
		variableSelected.clear();
		variableSelected.resize(newNumVariables, false);
	} else {
		if (newNumVariables != numVariables){
			variableSelected.resize(newNumVariables, false);
		} 
	}
	variableSelected[firstVarNum] = true;

	//Set the names in the variable listbox
	myProbeTab->variableListBox->clear();
	for (i = 0; i< newNumVariables; i++){
		if (variableNames.size() > (unsigned int)i){
			const std::string& s = variableNames.at(i);
			//Direct conversion of std::string& to QString doesn't seem to work
			//Maybe std was not enabled when QT was built?
			const QString& text = QString(s.c_str());
			myProbeTab->variableListBox->insertItem(text, i);
		} else assert(0);
	}

	//Create new arrays to hold bounds and transfer functions:
	TransferFunction** newTransFunc = new TransferFunction*[newNumVariables];
	float* newMinEdit = new float[newNumVariables];
	float* newMaxEdit = new float[newNumVariables];
	//If we are overriding previous values, delete the transfer functions, create new ones.
	//Set the map bounds to the actual bounds in the data
	if (doOverride){
		for (i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		//Create new transfer functions, their editors, hook them up:
		
		for (i = 0; i<newNumVariables; i++){
			newTransFunc[i] = new TransferFunction(this, 8);
			//Initialize to be fully opaque:
			newTransFunc[i]->setOpaque();
			//create new tfe, hook it to the trans func
			TFEditor* newTFEditor = new TFEditor(newTransFunc[i], myProbeTab->ProbeTFFrame);
			connectMapperFunction(newTransFunc[i], newTFEditor);
			newTransFunc[i]->setMinMapValue(Session::getInstance()->getDefaultDataMin(i));
			newTransFunc[i]->setMaxMapValue(Session::getInstance()->getDefaultDataMax(i));
			newMinEdit[i] = Session::getInstance()->getDefaultDataMin(i);
			newMaxEdit[i] = Session::getInstance()->getDefaultDataMax(i);
		}
	} else { 
		//attempt to make use of existing transfer functions, edit ranges.
		//delete any that are no longer referenced
		for (i = 0; i<newNumVariables; i++){
			if(i<numVariables){
				newTransFunc[i] = transFunc[i];
				newMinEdit[i] = minColorEditBounds[i];
				newMaxEdit[i] = maxColorEditBounds[i];
			} else { //create new tfe, hook it to the trans func
				newTransFunc[i] = new TransferFunction(this, 8);
				//Initialize to be fully opaque:
				newTransFunc[i]->setOpaque();
				TFEditor* newTFEditor = new TFEditor(newTransFunc[i], myProbeTab->ProbeTFFrame);
				connectMapperFunction(newTransFunc[i], newTFEditor);
				newTransFunc[i]->setMinMapValue(Session::getInstance()->getDefaultDataMin(i));
				newTransFunc[i]->setMaxMapValue(Session::getInstance()->getDefaultDataMax(i));
				newMinEdit[i] = Session::getInstance()->getDefaultDataMin(i);
				newMaxEdit[i] = Session::getInstance()->getDefaultDataMax(i);
			}
		}
			//Delete trans funcs (and associated tfe's that are no longer referenced.
		for (i = newNumVariables; i<numVariables; i++){
			delete transFunc[i];
		}
	} //end if(doOverride)
	//Make sure edit bounds are valid
	for(i = 0; i<newNumVariables; i++){
		if (newMinEdit[i] >= newMaxEdit[i]){
			newMinEdit[i] = Session::getInstance()->getDefaultDataMin(i);
			newMaxEdit[i] = Session::getInstance()->getDefaultDataMax(i);
		}
		//And check again...
		if (newMinEdit[i] >= newMaxEdit[i]){
			newMinEdit[i] = 0.f;
			newMaxEdit[i] = 1.f;
		}
	}
	//Hook up new stuff
	delete minColorEditBounds;
	delete maxColorEditBounds;
	delete transFunc;
	minColorEditBounds = newMinEdit;
	maxColorEditBounds = newMaxEdit;
	//And clone the color edit bounds to use as opac edit bounds:
	minOpacEditBounds = new float[newNumVariables];
	maxOpacEditBounds = new float[newNumVariables];
	for (i = 0; i<newNumVariables; i++){
		minOpacEditBounds[i] = minColorEditBounds[i];
		maxOpacEditBounds[i] = maxColorEditBounds[i];
	}

	transFunc = newTransFunc;
	
	numVariables = newNumVariables;
	bool wasEnabled = enabled;
	setEnabled(false);
	//Always disable  don't change local/global 
	updateRenderer(wasEnabled, isLocal(), false);
	
	setClutDirty();
	setDatarangeDirty();
	if(numVariables>0) getTFEditor()->setDirty();
	//If probe is the current front tab, and if it applies to the active visualizer,
	//update its values
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myProbeTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getProbeParams(viznum)))
			updateDialog();
	}
	probeDirty = true;
}
//Initialize to default state
//
void ProbeParams::
restart(){
	histoStretchFactor = 1.f;
	firstVarNum = 0;
	if (probeTexture) delete probeTexture;
	probeTexture = 0;
	savedCommand = 0;
	
	if(numVariables > 0){
		for (int i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete transFunc;
	}
	numVariables = 0;
	transFunc = 0;
	cursorCoords[0] = cursorCoords[1] = 0.0f;
	//Initialize the mapping bounds to [0,1] until data is read
	
	if (minColorEditBounds) delete minColorEditBounds;
	if (maxColorEditBounds) delete maxColorEditBounds;
	if (minOpacEditBounds) delete minOpacEditBounds;
	if (maxOpacEditBounds) delete maxOpacEditBounds;
	
	
	minColorEditBounds = new float[1];
	maxColorEditBounds = new float[1];
	minColorEditBounds[0] = 0.f;
	maxColorEditBounds[0] = 1.f;
	minOpacEditBounds = new float[1];
	maxOpacEditBounds = new float[1];
	minOpacEditBounds[0] = 0.f;
	maxOpacEditBounds[0] = 1.f;
	currentDatarange[0] = 0.f;
	currentDatarange[1] = 1.f;
	editMode = true;   //default is edit mode
	savedCommand = 0;
	setEnabled(false);
	setClutDirty();
	setDatarangeDirty();
	firstVarNum = 0;
	numVariables = 0;
	numVariablesSelected = 0;

	probeDirty = false;
	numRefinements = 0;
	maxNumRefinements = 10;
	theta = 0.f;
	phi = 0.f;
	for (int i = 0; i<3; i++){
		probeMin[i] = 0.f;
		probeMax[i] = 1.f;
		selectPoint[i] = 0.f;
	}
	seedAttached = false;
	attachedFlow = 0;
	//If probe is the current front tab, and if it applies to the active visualizer,
	//update its values
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myProbeTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getProbeParams(viznum)))
			updateDialog();
	}
}
//Method to invalidate a datarange, and to force a rendering
//with new data quantization
void ProbeParams::
setDatarangeDirty()
{
	if (!getMapperFunc()) return;
	if (currentDatarange[0] != getMapperFunc()->getMinColorMapValue() ||
		currentDatarange[1] != getMapperFunc()->getMaxColorMapValue()){
			currentDatarange[0] = getMapperFunc()->getMinColorMapValue();
			currentDatarange[1] = getMapperFunc()->getMaxColorMapValue();
			//Eventually we may need to notify the rendering window that
			//the datarange has changed...
			//VizWinMgr::getInstance()->setDataRangeDirty(this);
	}
}

//Respond to user request to load/save TF
//Assumes name is valid
//
void ProbeParams::
sessionLoadTF(QString* name){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "Load Transfer Function from Session");
	
	//Get the transfer function from the session:
	
	std::string s(name->ascii());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	hookupTF(tf, firstVarNum);
	PanelCommand::captureEnd(cmd, this);
	setProbeDirty();
}
void ProbeParams::
fileLoadTF(){
	
	//Open a file load dialog
	
    QString s = QFileDialog::getOpenFileName(
                    Session::getInstance()->getTFFilePath().c_str(),
                    "Vapor Transfer Functions (*.vtf)",
                    myProbeTab,
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
	PanelCommand* cmd = PanelCommand::captureStart(this, "Load Transfer Function from file");
	
	TransferFunction* t = TransferFunction::loadFromFile(is);
	if (!t){//Report error if can't load
		QString str("Error loading transfer function. /nFailed to convert input file: \n ");
		str += s;
		MessageReporter::errorMsg(str.ascii());
		//Don't put this into history!
		delete cmd;
		return;
	}

	hookupTF(t, firstVarNum);
	PanelCommand::captureEnd(cmd, this);
	//Remember the path to the file:
	Session::getInstance()->updateTFFilePath(&s);
	setProbeDirty();
}
//Hook up the new transfer function in specified slot,
//Delete the old one.  This is called whenever a new tf is loaded.
//
void ProbeParams::
hookupTF(TransferFunction* tf, int index){

	//Create a new TFEditor
	TFEditor* newTFEditor = new TFEditor(tf, myProbeTab->ProbeTFFrame);
	if (transFunc[index]) delete transFunc[index];
	transFunc[index] = tf;
	connectMapperFunction(tf, newTFEditor);
	myProbeTab->ProbeTFFrame->setEditor(newTFEditor);
	newTFEditor->reset();
	minColorEditBounds[index] = tf->getMinMapValue();
	maxColorEditBounds[index] = tf->getMaxMapValue();
	//reset the editing display range, this also sets dirty flag
	updateMapBounds();
	//Force a redraw of tfframe
	newTFEditor->setDirty();
	myProbeTab->ProbeTFFrame->update();	
	setDatarangeDirty();
	setClutDirty();
}
// Setup pointers between transfer function, editor, and this:
//
void ProbeParams::
connectMapperFunction(MapperFunction* tf, MapEditor* tfe){
	tf->setEditor(tfe);
	tfe->setFrame(myProbeTab->ProbeTFFrame);
	tfe->setMapperFunction(tf);
	tf->setParams(this);
	tfe->setColorVarNum(firstVarNum);
	tfe->setOpacVarNum(firstVarNum);
	//myProbeTab->probeTextureFrame->setParams(this);
}
void ProbeParams::
fileSaveTF(){
	//Launch a file save dialog, open resulting file
    QString s = QFileDialog::getSaveFileName(
					Session::getInstance()->getTFFilePath().c_str(),
                    "Vapor Transfer Functions (*.vtf)",
                    myProbeTab,
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
	
	
	
	if (!((TransferFunction*)getMapperFunc())->saveToFile(fileout)){//Report error if can't save to file
		QString str("Failed to write output file: \n");
		str += s;
		MessageReporter::errorMsg(str.ascii());
		fileout.close();
		return;
	}
	fileout.close();
	Session::getInstance()->updateTFFilePath(&s);
}
// Force the tframe to update
//
void ProbeParams::
refreshTFFrame(){
	getTFEditor()->setDirty();
	myProbeTab->ProbeTFFrame->update();	
}

//Handlers for Expat parsing.
//
bool ProbeParams::
elementStartHandler(ExpatParseMgr* pm, int depth , std::string& tagString, const char **attrs){
	static int parsedVarnum;
	
	int i;
	if (StrCmpNoCase(tagString, _probeParamsTag) == 0) {
		
		int newNumVariables = 0;
		//If it's a Probe tag, obtain 6 attributes (2 are from Params class)
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
			else if (StrCmpNoCase(attribName, _numVariablesAttr) == 0) {
				ist >> newNumVariables;
			}
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				if (value == "true") setLocal(true); else setLocal(false);
			}
			else if (StrCmpNoCase(attribName, _histoStretchAttr) == 0){
				float histStretch;
				ist >> histStretch;
				setHistoStretch(histStretch);
			}
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
				if (numRefinements > maxNumRefinements) maxNumRefinements = numRefinements;
			}
			else if (StrCmpNoCase(attribName, _editModeAttr) == 0){
				if (value == "true") setEditMode(true); 
				else setEditMode(false);
			}
			else return false;
		}
		//Create space for the variables:
		int numVars = Max (newNumVariables, 1);
		if (minColorEditBounds) delete minColorEditBounds;
		minColorEditBounds = new float[numVars];
		if (maxColorEditBounds) delete maxColorEditBounds;
		maxColorEditBounds = new float[numVars];
		if (minOpacEditBounds) delete minOpacEditBounds;
		minOpacEditBounds = new float[numVars];
		if (maxOpacEditBounds) delete maxOpacEditBounds;
		maxOpacEditBounds = new float[numVars];
		variableNames.clear();
		variableSelected.clear();
		for (i = 0; i<newNumVariables; i++){
			variableNames.push_back("");
			variableSelected.push_back(false);
		}
		//Setup with default values, in case not specified:
		for (i = 0; i< newNumVariables; i++){
			minColorEditBounds[i] = 0.f;
			maxColorEditBounds[i] = 1.f;
		}

		//create default Transfer Function and TFEditor
		//Are they gone?
		if (transFunc){
			for (int j = 0; j<numVariables; j++){
				delete transFunc[j];
			}
			delete transFunc;
		}
		numVariables = newNumVariables;
		transFunc = new TransferFunction*[numVariables];
		//Create default transfer functions and editors
		for (int j = 0; j<numVariables; j++){
			transFunc[j] = new TransferFunction(this, 8);
			TFEditor* newTFEditor = new TFEditor(transFunc[j],myProbeTab->ProbeTFFrame);
			connectMapperFunction(transFunc[j], newTFEditor);
		}
		
		
		return true;
	}
	//Parse a Variable:
	else if (StrCmpNoCase(tagString, _variableTag) == 0) {
		parsedVarnum = 0;
		float leftEdit = 0.f;
		float rightEdit = 1.f;
		bool varSelected = false;
		float opacFac = 1.f;
		string varName;
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			
			if (StrCmpNoCase(attribName, _variableNumAttr) == 0) {
				ist >> parsedVarnum;
				if (parsedVarnum < 0 || parsedVarnum >= numVariables) {
					pm->parseError("Invalid variable number %d", parsedVarnum);
				}
			}
			else if (StrCmpNoCase(attribName, _leftEditBoundAttr) == 0) {
				ist >> leftEdit;
			}
			else if (StrCmpNoCase(attribName, _rightEditBoundAttr) == 0) {
				ist >> rightEdit;
			}
			else if (StrCmpNoCase(attribName, _variableNameAttr) == 0){
				ist >> varName;
			}
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
			}
			else if (StrCmpNoCase(attribName, _variableSelectedAttr) == 0){
				if (value == "true") {
					varSelected = true;
				}
			}
			else if (StrCmpNoCase(attribName, _opacityScaleAttr) == 0){
				ist >> opacFac;
			}
			else return false;
		}
		// Now set the values obtained from attribute parsing.
	
		variableNames[parsedVarnum] =  varName;
		variableSelected[parsedVarnum] = varSelected;
		setMinColorEditBound(leftEdit,parsedVarnum);
		setMaxColorEditBound(rightEdit,parsedVarnum);
		transFunc[parsedVarnum]->getEditor()->setOpacityScaleFactor(opacFac);
		
		return true;
	}
	//Parse a transferFunction
	//Note we are relying on parsedvarnum obtained by previous start handler:
	else if (StrCmpNoCase(tagString, TransferFunction::_transferFunctionTag) == 0) {
		//Need to "push" to transfer function parser.
		//That parser will "pop" back to probeparams when done.
		pm->pushClassStack(transFunc[parsedVarnum]);
		transFunc[parsedVarnum]->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	}
	//Parse the geometry node
	else if (StrCmpNoCase(tagString, _geometryTag) == 0) {
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			
			if (StrCmpNoCase(attribName, _thetaAttr) == 0) {
				ist >> theta;
			}
			else if (StrCmpNoCase(attribName, _phiAttr) == 0) {
				ist >> phi;
			}
			else if (StrCmpNoCase(attribName, _probeMinAttr) == 0) {
				ist >> probeMin[0];ist >> probeMin[1];ist >> probeMin[2];
			}
			else if (StrCmpNoCase(attribName, _probeMaxAttr) == 0) {
				ist >> probeMax[0];ist >> probeMax[1];ist >> probeMax[2];
			}
			else if (StrCmpNoCase(attribName, _cursorCoordsAttr) == 0) {
				ist >> cursorCoords[0];ist >> cursorCoords[1];
			}
			else return false;
		}
		return true;
	}
	else return false;
}
//The end handler needs to pop the parse stack, not much else
bool ProbeParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	
	if (StrCmpNoCase(tag, _probeParamsTag) == 0) {
		//Determine the first selected variable:
		firstVarNum = 0;
		int i;
		for (i = 0; i<numVariables; i++){
			if (variableSelected[i]){
				firstVarNum = i;
				break;
			}
		}
		if (i == numVariables) variableSelected[0] = true;

		//Align the editor
		setMinEditBound(getMinColorMapBound());
		setMaxEditBound(getMaxColorMapBound());
		//If we are current, update the tab panel after setting up the variable listbox
		
		if (isCurrent()) {
			QListBox* listBox = myProbeTab->variableListBox;
			listBox->clear();	
			//Set the names in the variable listbox
			for (i = 0; i< numVariables; i++){
				const std::string& s = variableNames.at(i);
				//Direct conversion of std::string& to QString doesn't seem to work
				//Maybe std was not enabled when QT was built?
				const QString& text = QString(s.c_str());
				listBox->insertItem(text, i);
				listBox->setSelected(i, variableSelected[i]);
			}
			updateDialog();
		}
		//If this is a probeparams, need to
		//pop the parse stack.  The caller will need to save the resulting
		//transfer function (i.e. this)
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0) {
		return true;
	} else if (StrCmpNoCase(tag, _variableTag) == 0){
		return true;
	} else if (StrCmpNoCase(tag, _geometryTag) == 0){
		return true;
	} else {
		pm->parseError("Unrecognized end tag in ProbeParams %s",tag.c_str());
		return false; 
	}
}

//Method to construct Xml for state saving
XmlNode* ProbeParams::
buildNode() {
	//Construct the probe node
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
	oss << (long)numVariables;
	attrs[_numVariablesAttr] = oss.str();

	oss.str(empty);
	oss << (long)numRefinements;
	attrs[_numTransformsAttr] = oss.str();

	oss.str(empty);
	if (editMode)
		oss << "true";
	else 
		oss << "false";
	attrs[_editModeAttr] = oss.str();
	oss.str(empty);
	oss << (double)getHistoStretch();
	attrs[_histoStretchAttr] = oss.str();
	
	XmlNode* probeNode = new XmlNode(_probeParamsTag, attrs, 3);

	//Now add children:  
	//Create the Variables nodes
	for (int i = 0; i<numVariables; i++){
		attrs.clear();
	
		oss.str(empty);
		oss << (long)i;
		attrs[_variableNumAttr] = oss.str();

		oss.str(empty);
		oss << variableNames[i];
		attrs[_variableNameAttr] = oss.str();

		oss.str(empty);
		if (variableSelected[i])
			oss << "true";
		else 
			oss << "false";
		attrs[_variableSelectedAttr] = oss.str();

		oss.str(empty);
		oss << (double)minColorEditBounds[i];
		attrs[_leftEditBoundAttr] = oss.str();

		oss.str(empty);
		oss << (double)maxColorEditBounds[i];
		attrs[_rightEditBoundAttr] = oss.str();

		oss.str(empty);
		oss << (double)transFunc[i]->getEditor()->getOpacityScaleFactor();
		attrs[_opacityScaleAttr] = oss.str();

		XmlNode* varNode = new XmlNode(_variableTag,attrs,1);

		//Create a transfer function node, add it as child
		XmlNode* tfNode = transFunc[i]->buildNode(empty);
		varNode->AddChild(tfNode);
		probeNode->AddChild(varNode);
	}
	//Now do geometry node:
	attrs.clear();
	oss.str(empty);
	oss << (double)phi;
	attrs[_phiAttr] = oss.str();
	oss.str(empty);
	oss << (double) theta;
	attrs[_thetaAttr] = oss.str();
	oss.str(empty);
	oss << (double)probeMin[0]<<" "<<(double)probeMin[1]<<" "<<(double)probeMin[2];
	attrs[_probeMinAttr] = oss.str();
	oss.str(empty);
	oss << (double)probeMax[0]<<" "<<(double)probeMax[1]<<" "<<(double)probeMax[2];
	attrs[_probeMaxAttr] = oss.str();
	oss.str(empty);
	oss << (double)cursorCoords[0]<<" "<<(double)cursorCoords[1];
	attrs[_cursorCoordsAttr] = oss.str();
	probeNode->NewChild(_geometryTag, attrs, 0);
	return probeNode;
}

TFEditor* ProbeParams::getTFEditor(){
	return (numVariables > 0 ? (TFEditor*)transFunc[firstVarNum]->getEditor() : 0);
}
MapperFunction* ProbeParams::getMapperFunc() {
	return (numVariables > 0 ? transFunc[firstVarNum] : 0);
}

void ProbeParams::setMinColorMapBound(float val){
	getMapperFunc()->setMinColorMapValue(val);
}
void ProbeParams::setMaxColorMapBound(float val){
	getMapperFunc()->setMaxColorMapValue(val);
}


void ProbeParams::setMinOpacMapBound(float val){
	getMapperFunc()->setMinOpacMapValue(val);
}
void ProbeParams::setMaxOpacMapBound(float val){
	getMapperFunc()->setMaxOpacMapValue(val);
}

//When the center slider moves, set the ProbeMin and ProbeMax
void ProbeParams::
setXCenter(int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(0, sliderval, myProbeTab->xSizeSlider->value());
	setProbeDirty();
}
void ProbeParams::
setYCenter(int sliderval){
	sliderToText(1, sliderval, myProbeTab->ySizeSlider->value());
	setProbeDirty();
}
void ProbeParams::
setZCenter(int sliderval){
	sliderToText(2, sliderval, myProbeTab->zSizeSlider->value());
	setProbeDirty();
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void ProbeParams::
setXSize(int sliderval){
	sliderToText(0, myProbeTab->xCenterSlider->value(),sliderval);
	setProbeDirty();
}
void ProbeParams::
setYSize(int sliderval){
	sliderToText(1, myProbeTab->yCenterSlider->value(),sliderval);
	setProbeDirty();
}
void ProbeParams::
setZSize(int sliderval){
	sliderToText(2, myProbeTab->zCenterSlider->value(),sliderval);
	setProbeDirty();
}

unsigned char* ProbeParams::
getProbeTexture(){
	if (!enabled) return 0;
	if (!probeDirty)
		return probeTexture;
	Session* ses = Session::getInstance();
	if (!ses->getDataMgr()) return 0;
	if (!probeTexture) {
		probeTexture = new unsigned char[128*128*4];
	}
	float transformMatrix[12];
	
	//Set up to transform from probe into volume:
	buildCoordTransform(transformMatrix);
	
	//Get the data dimensions (at this resolution):
	int dataSize[3];
	const size_t totTransforms = ses->getCurrentMetadata()->GetNumTransforms();
	//Start by initializing extents
	for (int i = 0; i< 3; i++){
		dataSize[i] = (ses->getDataStatus()->getFullDataSize(i))>>(totTransforms - numRefinements);
	}
	//Determine the integer extents of the containing cube, truncate to
	//valid integer coords:

	
	size_t blkMin[3], blkMax[3];
	int coordMin[3], coordMax[3];
	getBoundingBox(blkMin, blkMax, coordMin, coordMax);
	int bSize =  *(ses->getCurrentMetadata()->GetBlockSize());
	
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then do rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	int timeStep = VizWinMgr::getInstance()->getAnimationParams(vizNum)->getCurrentFrameNumber();
	//Now obtain all of the volumes needed for this probe:
	int totVars = 0;
	for (unsigned int varnum = 0; varnum < variableNames.size(); varnum++){
		if (!variableSelected[varnum]) continue;
		volData[totVars] = getContainingVolume(blkMin, blkMax, varnum, timeStep);
		totVars++;
	}
	//Now calculate the texture.
	//
	//For each pixel, map it into the volume.
	//The blkMin values tell you the offset to use.
	//The blkMax values tell how to stride through the data
	//The coordMin and coordMax values are used to check validity
	//We first map the coords in the probe to the volume.  
	//Then we map the volume into the region provided by dataMgr
	//This is done for each of the variables,
	//The RMS of the result is then mapped using the transfer function.
	float clut[256*4];
	transFunc[firstVarNum]->makeLut(clut);
	float probeCoord[3];
	float dataCoord[3];
	int arrayCoord[3];
	const float* extents = ses->getExtents();
	//Can ignore depth, just mapping center plane
	probeCoord[2] = 0.f;
	
	//Loop over pixels in texture
	for (int iy = 0; iy < 128; iy++){
		//Map iy to a value between -1 and 1
		probeCoord[1] = -1.f + 2.f*(float)iy/127.f;
		for (int ix = 0; ix < 128; ix++){
			
			//find the coords that the texture maps to
			//probeCoord is the coord in the probe, dataCoord is in data volume 
			probeCoord[0] = -1.f + 2.f*(float)ix/127.f;
			vtransform(probeCoord, transformMatrix, dataCoord);
			for (int i = 0; i<3; i++){
				arrayCoord[i] = (int) (0.5f+((float)dataSize[i])*(dataCoord[i] - extents[i])/(extents[i+3]-extents[i]));
			}
			
			//Make sure the transformed coords are in the probe region
			//This should only fail if they aren't even in the full volume.
			bool dataOK = true;
			for (int i = 0; i< 3; i++){
				if (arrayCoord[i] < coordMin[i] ||
					arrayCoord[i] > coordMax[i] ) {
						dataOK = false;
					} //outside; color it black
			}
			
			if(dataOK) { //find the coordinate in the data array
				int xyzCoord = (arrayCoord[0] - blkMin[0]*bSize) +
					(arrayCoord[1] - blkMin[1]*bSize)*(bSize*(blkMax[0]-blkMin[0]+1)) +
					(arrayCoord[2] - blkMin[2]*bSize)*(bSize*(blkMax[1]-blkMin[1]+1))*(bSize*(blkMax[0]-blkMin[0]+1));
				float varVal;
				

				//use the intDataCoords to index into the loaded data
				if (totVars == 1) varVal = volData[0][xyzCoord];
				else { //Add up the squares of the variables
					varVal = 0.f;
					for (int k = 0; k<totVars; k++){
						varVal += volData[k][xyzCoord]*volData[k][xyzCoord];
					}
					varVal = sqrt(varVal);
				}
				
				//Use the transfer function to map the data:
				int lutIndex = transFunc[firstVarNum]->mapFloatToIndex(varVal);
				
				probeTexture[4*(ix+128*iy)] = (unsigned char)(0.5+ clut[4*lutIndex]*255.f);
				probeTexture[4*(ix+128*iy)+1] = (unsigned char)(0.5+ clut[4*lutIndex+1]*255.f);
				probeTexture[4*(ix+128*iy)+2] = (unsigned char)(0.5+ clut[4*lutIndex+2]*255.f);
				probeTexture[4*(ix+128*iy)+3] = (unsigned char)(0.5+ clut[4*lutIndex+3]*255.f);
				
			}
			else {//point out of region
				probeTexture[4*(ix+128*iy)] = 0;
				probeTexture[4*(ix+128*iy)+1] = 0;
				probeTexture[4*(ix+128*iy)+2] = 0;
				probeTexture[4*(ix+128*iy)+3] = 0;
			}

		}//End loop over ix
	}//End loop over iy
	
	probeDirty = false;
	return probeTexture;
}
	
float* ProbeParams::
getContainingVolume(size_t blkMin[3], size_t blkMax[3], int varNum, int timeStep){
	//Get the region (int coords) associated with the specified variable at the
	//current probe extents
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//	qWarning("Fetching region: %d %d %d , %d %d %d ",blkMin[0],blkMin[1],blkMin[2],
	//	blkMax[0],blkMax[1],blkMax[2]);
	float* reg = Session::getInstance()->getDataMgr()->GetRegion((size_t)timeStep,
		variableNames[varNum].c_str(),
		numRefinements, blkMin, blkMax, 0);
	QApplication::restoreOverrideCursor();
	return reg;
}

//Find the smallest box containing the probe, in block coords, at current refinement level.
//We also need the actual box coords (min and max) to check for valid coords in the block.
//Note that the box region may be strictly smaller than the block region
//
void ProbeParams::getBoundingBox(size_t boxMinBlk[3], size_t boxMaxBlk[3], int boxMin[3], int boxMax[3]){
	//Determine the smallest axis-aligned cube that contains the probe.  This is
	//obtained by mapping all 8 corners into the space
	Session* ses = Session::getInstance();
	const size_t *bs = ses->getCurrentMetadata()->GetBlockSize();
	float transformMatrix[12];
	//Set up to transform from probe into volume:
	buildCoordTransform(transformMatrix);
	float dataSize[3];
	const size_t totTransforms = ses->getCurrentMetadata()->GetNumTransforms();
	const float* extents = ses->getExtents();
	//Start by initializing extents, and variables that will be min,max
	for (int i = 0; i< 3; i++){
		dataSize[i] = (ses->getDataStatus()->getFullDataSize(i))>>(totTransforms - numRefinements);
		boxMin[i] = dataSize[i]-1;
		boxMax[i] = 0;
	}
	
	for (int corner = 0; corner< 8; corner++){
		int intCoord[3];
		int intResult[3];
		float startVec[3], resultVec[3];
		intCoord[0] = corner%2;
		intCoord[1] = (corner/2)%2;
		intCoord[2] = (corner/4)%2;
		for (int i = 0; i<3; i++)
			startVec[i] = -1.f + (float)(2.f*intCoord[i]);
		// calculate the mapping of this corner,
		vtransform(startVec, transformMatrix, resultVec);
		
		// then make sure the container includes it:
		for(int i = 0; i< 3; i++){
			intResult[i] = (int) (0.5f+(float)dataSize[i]*(resultVec[i]- extents[i])/(extents[i+3]-extents[i]));
		
			if(intResult[i]<boxMin[i]) boxMin[i] = intResult[i];
			if(intResult[i]>boxMax[i]) boxMax[i] = intResult[i];
		}
		
		}
	for (int i = 0; i< 3; i++){
		if(boxMin[i] < 0) boxMin[i] = 0;
		if(boxMax[i] > dataSize[i]-1) boxMax[i] = dataSize[i] - 1;
		if (boxMin[i] > boxMax[i]) boxMax[i] = boxMin[i];
		//And make the block coords:
		boxMinBlk[i] = boxMin[i]/ (*bs);
		boxMaxBlk[i] = boxMax[i]/ (*bs);
	}
	
	return;
}

//Find the smallest extents containing the probe, 
//Similar to above, but using extents
void ProbeParams::calcContainingBoxExtentsInCube(float* bigBoxExtents){
	//Determine the smallest axis-aligned cube that contains the probe.  This is
	//obtained by mapping all 8 corners into the space.
	//It will not necessarily fit inside the unit cube.
	float corners[8][3];
	calcBoxCorners(corners);
	
	float boxMin[3],boxMax[3];
	int crd, cor;
	
	//initialize extents, and variables that will be min,max
	for (crd = 0; crd< 3; crd++){
		boxMin[crd] = 1.e30f;
		boxMax[crd] = -1.e30f;
	}
	
	
	for (cor = 0; cor< 8; cor++){
		
		
		//make sure the container includes it:
		for(crd = 0; crd< 3; crd++){
			if (corners[cor][crd]<boxMin[crd]) boxMin[crd] = corners[cor][crd];
			if (corners[cor][crd]>boxMax[crd]) boxMax[crd] = corners[cor][crd];
		}
	}
	//Now convert the min,max back into extents in unit cube:
	
	const float* fullExtents = Session::getInstance()->getExtents();
	
	float maxSize = Max(Max(fullExtents[3]-fullExtents[0],fullExtents[4]-fullExtents[1]),fullExtents[5]-fullExtents[2]);
	for (crd = 0; crd<3; crd++){
		bigBoxExtents[crd] = (boxMin[crd] - fullExtents[crd])/maxSize;
		bigBoxExtents[crd+3] = (boxMax[crd] - fullExtents[crd])/maxSize;
	}
	return;
}
// Map the cursor coords into world space,
// refreshing the selected point.  CursorCoords go from -1 to 1
//
void ProbeParams::mapCursor(){
	//Get the transform matrix:
	float transformMatrix[12];
	float probeCoord[3];
	buildCoordTransform(transformMatrix);
	//The cursor sits in the z=0 plane of the probe box coord system.
	//x is reversed because we are looking from the opposite direction (?)
	probeCoord[0] = -cursorCoords[0];
	probeCoord[1] = cursorCoords[1];
	probeCoord[2] = 0.f;
	
	vtransform(probeCoord, transformMatrix, selectPoint);
}

void ProbeParams::setProbeDirty(bool dirty){ 
	probeDirty = dirty;
	if (!probeDirty) return;
	if(myProbeTab)myProbeTab->probeTextureFrame->updateGLWindow();
	VizWinMgr::getInstance()->refreshProbe(this);
}

//Save undo/redo state when user grabs a probe handle, or maybe a probe face (later)
//
void ProbeParams::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(this,  "slide probe handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshProbe(this);
}
//The Manip class will have already changed the box...
void ProbeParams::
captureMouseUp(){
	myProbeTab->probeTextureFrame->setTextureSize(probeMax[0]-probeMin[0],probeMax[1]-probeMin[1]);
	probeDirty = true;
	//Update the tab if it's in front:
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myProbeTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getProbeParams(viznum)))
			updateDialog();
	}
	
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, this);
	savedCommand = 0;
	//Force a rerender, so we will see the unselected face:
	VizWinMgr::getInstance()->refreshProbe(this);
}

//Save undo/redo state when user clicks cursor
//
void ProbeParams::
guiStartCursorMove(){
	confirmText(false);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(this,  "move probe cursor");
}
void ProbeParams::
guiEndCursorMove(){
	//Update the selected point:
	//If we are connected to a seed, move it:
	if (seedIsAttached() && attachedFlow){
		attachedFlow->guiMoveLastSeed(getSelectedPoint());
	}
	//Update the tab, it's in front:
	updateDialog();
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, this);
	savedCommand = 0;
}

//calculate the variable, or rms of the variables, at a specific point.
//Returns the OUT_OF_BOUNDS flag if point is not (in region and in probe).
//In order to exploit the datamgr cache, always obtain the data from the same
//regions that are accessed in building the probe texture.
//


float ProbeParams::
calcCurrentValue(float point[3]){
	if (numVariables <= 0) return OUT_OF_BOUNDS;
	Session* ses = Session::getInstance();
	if (!ses->getDataMgr()) return 0.f;
	if (!enabled) return 0.f;
	int arrayCoord[3];
	const float* extents = ses->getExtents();

	//Get the data dimensions (at current resolution):
	int dataSize[3];
	const size_t totTransforms = ses->getCurrentMetadata()->GetNumTransforms();
	for (int i = 0; i< 3; i++){
		dataSize[i] = (ses->getDataStatus()->getFullDataSize(i))>>(totTransforms - numRefinements);
	}
	
	//Find the region that contains the probe.
	size_t blkMin[3], blkMax[3];
	int coordMin[3], coordMax[3];
	getBoundingBox(blkMin, blkMax, coordMin, coordMax);
	for (int i = 0; i< 3; i++){
		if ((point[i] < extents[i]) || (point[i] > extents[i+3])) return OUT_OF_BOUNDS;
		arrayCoord[i] = (int) (0.5f+((float)dataSize[i])*(point[i] - extents[i])/(extents[i+3]-extents[i]));
		//Make sure the transformed coords are in the probe region
		if (arrayCoord[i] < coordMin[i] || arrayCoord[i] > coordMax[i] ) {
			return OUT_OF_BOUNDS;
		} 
	}
	
	int bSize =  *(ses->getCurrentMetadata()->GetBlockSize());
	
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then do rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	int timeStep = VizWinMgr::getInstance()->getAnimationParams(vizNum)->getCurrentFrameNumber();
	//Now obtain all of the volumes needed for this probe:
	int totVars = 0;
	for (unsigned int varnum = 0; varnum < variableNames.size(); varnum++){
		if (!variableSelected[varnum]) continue;
		volData[totVars] = getContainingVolume(blkMin, blkMax, varnum, timeStep);
		totVars++;
	}
			
	int xyzCoord = (arrayCoord[0] - blkMin[0]*bSize) +
		(arrayCoord[1] - blkMin[1]*bSize)*(bSize*(blkMax[0]-blkMin[0]+1)) +
		(arrayCoord[2] - blkMin[2]*bSize)*(bSize*(blkMax[1]-blkMin[1]+1))*(bSize*(blkMax[0]-blkMin[0]+1));
	
	float varVal;
	//use the int xyzCoord to index into the loaded data
	if (totVars == 1) varVal = volData[0][xyzCoord];
	else { //Add up the squares of the variables
		varVal = 0.f;
		for (int k = 0; k<totVars; k++){
			varVal += volData[k][xyzCoord]*volData[k][xyzCoord];
		}
		varVal = sqrt(varVal);
	}
	delete volData;
	return varVal;
}
//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
Histo* ProbeParams::getHistogram(bool mustGet){
	if (!histogramList){
		if (!mustGet) return 0;
		histogramList = new Histo*[numVariables];
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
	}
	if (histogramList[firstVarNum]) return histogramList[firstVarNum];
	
	if (!mustGet) return 0;
	histogramList[firstVarNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	refreshHistogram();
	return histogramList[firstVarNum];
	
}
//Obtain a new histogram for the current selected variables.
//Save it at the position associated with firstVarNum
void ProbeParams::
refreshHistogram(){
	Session* ses = Session::getInstance();
	if (!ses->getDataMgr()) return;
	if (!histogramList){
		histogramList = new Histo*[numVariables];
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
	}
	if (!histogramList[firstVarNum]){
		histogramList[firstVarNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	}
	Histo* histo = histogramList[firstVarNum];
	histo->reset(256,currentDatarange[0],currentDatarange[1]);
	//create the smallest containing box
	size_t blkMin[3],blkMax[3];
	int boxMin[3],boxMax[3];
	getBoundingBox(blkMin, blkMax, boxMin, boxMax);

	int bSize =  *(ses->getCurrentMetadata()->GetBlockSize());
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then histogram rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	int timeStep = VizWinMgr::getInstance()->getAnimationParams(vizNum)->getCurrentFrameNumber();
	//Now obtain all of the volumes needed for this probe:
	int totVars = 0;
	for (int varnum = 0; varnum < (int)variableNames.size(); varnum++){
		if (!variableSelected[varnum]) continue;
		assert(varnum >= firstVarNum);
		volData[totVars] = getContainingVolume(blkMin, blkMax, varnum, timeStep);
		totVars++;
	}

	//Get the data dimensions (at current resolution):
	int dataSize[3];
	float gridSpacing[3];
	const size_t totTransforms = ses->getCurrentMetadata()->GetNumTransforms();
	const float* extents = ses->getExtents();
	for (int i = 0; i< 3; i++){
		dataSize[i] = (ses->getDataStatus()->getFullDataSize(i))>>(totTransforms - numRefinements);
		gridSpacing[i] = (extents[i+3]-extents[i])/(float)(dataSize[i]-1);
		if (boxMin[i]< 0) boxMin[i] = 0;
		if (boxMax[i] >= dataSize[i]) boxMax[i] = dataSize[i] - 1;
	}
	float voxSize = vlength(gridSpacing);

	//Prepare for test by finding corners and normals to box:
	float corner[8][3];
	float normals[6][3];
	float vec1[3], vec2[3];

	//Get box that is slightly fattened, to ensure nondegenerate normals
	calcBoxCorners(corner, 0.5*voxSize);
	//The first 6 corners are reference points for testing
	//the 6 normal vectors are outward pointing from these points
	//Normals are calculated as if cube were axis aligned but this is of 
	//course not really true, just gives the right orientation
	//
	// +Z normal: (c2-c0)X(c1-c0)
	vsub(corner[2],corner[0],vec1);
	vsub(corner[1],corner[0],vec2);
	vcross(vec1,vec2,normals[0]);
	vnormal(normals[0]);
	// -Y normal: (c5-c1)X(c0-c1)
	vsub(corner[5],corner[1],vec1);
	vsub(corner[0],corner[1],vec2);
	vcross(vec1,vec2,normals[1]);
	vnormal(normals[1]);
	// +Y normal: (c6-c2)X(c3-c2)
	vsub(corner[6],corner[2],vec1);
	vsub(corner[3],corner[2],vec2);
	vcross(vec1,vec2,normals[2]);
	vnormal(normals[2]);
	// -X normal: (c7-c3)X(c1-c3)
	vsub(corner[7],corner[3],vec1);
	vsub(corner[1],corner[3],vec2);
	vcross(vec1,vec2,normals[3]);
	vnormal(normals[3]);
	// +X normal: (c6-c4)X(c0-c4)
	vsub(corner[6],corner[4],vec1);
	vsub(corner[0],corner[4],vec2);
	vcross(vec1,vec2,normals[4]);
	vnormal(normals[4]);
	// -Z normal: (c7-c5)X(c4-c5)
	vsub(corner[7],corner[5],vec1);
	vsub(corner[4],corner[5],vec2);
	vcross(vec1,vec2,normals[5]);
	vnormal(normals[5]);

	float distval;
	float xyz[3];
	//int lastxyz = -1;
	//int incount = 0;
	//int outcount = 0;
	//Now loop over the grid points in the bounding box
	for (int k = boxMin[2]; k <= boxMax[2]; k++){
		xyz[2] = extents[2] + (((float)k)/(float)(dataSize[2]-1))*(extents[5]-extents[2]);
		for (int j = boxMin[1]; j <= boxMax[1]; j++){
			xyz[1] = extents[1] + (((float)j)/(float)(dataSize[1]-1))*(extents[4]-extents[1]);
			for (int i = boxMin[0]; i <= boxMax[0]; i++){
				xyz[0] = extents[0] + (((float)i)/(float)(dataSize[0]-1))*(extents[3]-extents[0]);
				
				//test if x,y,z is in probe:
				if ((distval = distanceToCube(xyz, normals, corner))<0.0005f*voxSize){
					//incount++;
					//Point is (almost) inside.
					//Evaluate the variable(s):
					int xyzCoord = (i - blkMin[0]*bSize) +
						(j - blkMin[1]*bSize)*(bSize*(blkMax[0]-blkMin[0]+1)) +
						(k - blkMin[2]*bSize)*(bSize*(blkMax[1]-blkMin[1]+1))*(bSize*(blkMax[0]-blkMin[0]+1));
					//qWarning(" sampled coord %d",xyzCoord);
					
				
					
					assert(xyzCoord >= 0);
					assert(xyzCoord < (int)((blkMax[0]-blkMin[0]+1)*(blkMax[1]-blkMin[1]+1)*(blkMax[2]-blkMin[2]+1)*bSize*bSize*bSize));
					float varVal;
					//use the int xyzCoord to index into the loaded data
					if (totVars == 1) varVal = volData[0][xyzCoord];
					else { //Add up the squares of the variables
						varVal = 0.f;
						for (int d = 0; d<totVars; d++){
							varVal += volData[d][xyzCoord]*volData[d][xyzCoord];
						}
						varVal = sqrt(varVal);
					}
					histo->addToBin(varVal);
				
				} 
				//else {
				//	outcount++;
				//}
			}
		}
	}

}

		
