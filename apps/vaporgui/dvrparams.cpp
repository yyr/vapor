//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		dvrparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implementation of the dvrparams class
//		This contains all the parameters required to support the
//		dvr renderer.  Includes a transfer function and a
//		transfer function editor.
//
#include "dvrparams.h"
#include "dvr.h"
#include "mainform.h"
#include "glbox.h"
#include "vizwin.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <vector>
#include <string>
#include "params.h"
#include "vizwinmgr.h"
#include "panelcommand.h"
#include "session.h"
#include "params.h"
#include "transferfunction.h"
#include "tfeditor.h"
#include "tfframe.h"
#include <qlabel.h>
#include <math.h>
#include <vapor/Metadata.h>

#ifdef VOLUMIZER
#include "volumizerrenderer.h"
#endif

using namespace VAPoR;

DvrParams::DvrParams(MainForm* mf, int winnum) : Params(mf, winnum){
	thisParamType = DvrParamsType;
	myDvrTab = mf->getDvrTab();
	

	varNum = 0;
	lightingOn = false;
	numBits = 8;
	diffuseCoeff = 0.8f;
	ambientCoeff = 0.5f;
	specularCoeff = 0.3f;
	specularExponent = 20;
	ambientAtten = .18f;
	specularAtten = .39f;
	diffuseAtten = 1.f;
	numVariables = 0;
	enabled = false;
	attenuationDirty = true;
	minDataValue = 0.f;
	maxDataValue = 1.f;
	rightTFLimit = 1.f;
	leftTFLimit = 0.f;

	//Hookup the editor to the frame in the dvr tab:
	myTransFunc = new TransferFunction(this, numBits);
	if(myDvrTab) myTFEditor = new TFEditor(this, myTransFunc, myDvrTab->DvrTFFrame, mf->getSession() );
	else myTFEditor = 0;
	clutDirty = true;
	savedCommand = 0;
	
}
DvrParams::~DvrParams(){
	delete myTransFunc;
	delete myTFEditor;
	if (savedCommand) delete savedCommand;
	
}
void DvrParams::
setDataRange(){
	rightTFLimit = Max(myTFEditor->getMaxEditValue(),
		Max(maxDataValue,myTransFunc->getMaxMapValue()));
	leftTFLimit = Min(myTFEditor->getMinEditValue(),
		Min(minDataValue, myTransFunc->getMinMapValue()));
}
void DvrParams::
setTab(Dvr* tab) {
	myDvrTab = tab;
	if (myTFEditor) delete myTFEditor;
	myTFEditor = new TFEditor(this, myTransFunc, myDvrTab->DvrTFFrame, mainWin->getSession() );
}
//Deepcopy requires cloning tf and tfeditor
Params* DvrParams::
deepCopy(){
	DvrParams* newParams = new DvrParams(*this);
	//Clone the Transfer Function (and TFEditor)??
	newParams->myTransFunc = new TransferFunction(*myTransFunc);
	//Need also to update pointers in this Transfer function:
	newParams->myTransFunc->setParams(newParams);
	//if(myDvrTab) newParams->myTFEditor = new TFEditor(newParams->myTransFunc, myDvrTab->DvrTFFrame, mainWin->getSession() );
	if (myDvrTab) {
		newParams->myTFEditor = new TFEditor(*myTFEditor);
		newParams->myTFEditor->setParams(newParams);
		newParams->myTransFunc->setEditor(newParams->myTFEditor);
		newParams->myTFEditor->setTransferFunction(newParams->myTransFunc);
	}
	//never keep the SavedCommand:
	newParams->savedCommand = 0;
	return newParams;
}
void DvrParams::
makeCurrent(Params* prevParams, bool newWin) {
	VizWinMgr* vwm = mainWin->getVizWinMgr();
	vwm->setDvrParams(vizNum, this);
	updateDialog();
	DvrParams* formerParams = (DvrParams*)prevParams;
	//Check if the enabled and/or Local settings changed:
	if (formerParams->enabled != enabled || formerParams->local != local || newWin){
		updateRenderer(formerParams->enabled, formerParams->local, newWin);
	}
}

void DvrParams::
refreshCtab() {
	myTransFunc->makeLut((float*)ctab);
}
	
//Set the tab consistent with the dvrparams data here:
void DvrParams::updateDialog(){
	
	QString strn;
	
	mainWin->getSession()->blockRecording();
	myDvrTab->EnableDisable->setCurrentItem((enabled) ? 1 : 0);
	myDvrTab->variableCombo->setCurrentItem(varNum);
	myDvrTab->variableCombo->setMaxCount(numVariables);
	//Set the names in the variable combo
	for (int i = 0; i< numVariables; i++){
		const std::string& s = variableNames.at(i);
		//Direct conversion of std:string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& text = QString(s.c_str());
		myDvrTab->variableCombo->changeItem(text,i);
	}
	myDvrTab->lightingCheckbox->setChecked(lightingOn);
	myDvrTab->numBitsSpin->setValue(numBits);
	myDvrTab->diffuseShading->setText(strn.setNum(diffuseCoeff, 'g', 3));
	myDvrTab->ambientShading->setText(strn.setNum(ambientCoeff, 'g', 3));
	myDvrTab->specularShading->setText(strn.setNum(specularCoeff, 'g', 3));
	myDvrTab->exponentShading->setText(strn.setNum(specularExponent));
	myDvrTab->diffuseAttenuation->setText(strn.setNum(diffuseAtten, 'g', 3));
	myDvrTab->ambientAttenuation->setText(strn.setNum(ambientAtten, 'g', 3));
	myDvrTab->specularAttenuation->setText(strn.setNum(specularAtten, 'g', 3));
	
	if (isLocal())
		myDvrTab->LocalGlobal->setCurrentItem(1);
	else 
		myDvrTab->LocalGlobal->setCurrentItem(0);

	TFFrame* myTFFrame = myDvrTab->DvrTFFrame;
	myTFFrame->setEditor(myTFEditor);
	float edCenter = 0.5f*(myTFEditor->getMinEditValue() +myTFEditor->getMaxEditValue());
	float edSize = (myTFEditor->getMaxEditValue() -myTFEditor->getMinEditValue());
	float mapCenter = 0.5f*(myTransFunc->getMinMapValue() +myTransFunc->getMaxMapValue());
	float mapSize = myTransFunc->getMaxMapValue() -myTransFunc->getMinMapValue();
	myDvrTab->editCenter->setText(strn.setNum(edCenter,'g', 4));
	myDvrTab->editSize->setText(strn.setNum(edSize,'g', 4));
	myDvrTab->editLeftLabel->setText(strn.setNum(myTFEditor->getMinEditValue(),'g', 4));
	myDvrTab->editRightLabel->setText(strn.setNum(myTFEditor->getMaxEditValue(),'g', 4));
	myDvrTab->leftMappingBound->setText(strn.setNum(myTransFunc->getMinMapValue(),'g',4));
	myDvrTab->rightMappingBound->setText(strn.setNum(myTransFunc->getMaxMapValue(),'g',4));
	myDvrTab->tfDomainCenterEdit->setText(strn.setNum(mapCenter, 'g', 4));
	myDvrTab->tfDomainSizeEdit->setText(strn.setNum(mapSize, 'g', 4));
	float sliderVal = myTFEditor->getHistoStretch();
	sliderVal = 1000.f - logf(sliderVal)/(HISTOSTRETCHCONSTANT*logf(2.f));
	myDvrTab->histoStretchSlider->setValue((int) sliderVal);
	setBindButtons();
	setTFESliders();
	myTFEditor->setDirty(true);
	myDvrTab->DvrTFFrame->update();
	guiSetTextChanged(false);
	mainWin->getSession()->unblockRecording();
		
}
//Update all the panel state associated with textboxes.
void DvrParams::
updatePanelState(){
	QString strn;
	enabled = myDvrTab->EnableDisable->currentItem();
	diffuseCoeff = myDvrTab-> diffuseShading->text().toFloat();
	ambientCoeff = myDvrTab->ambientShading->text().toFloat();
	specularCoeff = myDvrTab->specularShading->text().toFloat();
	specularExponent = myDvrTab->exponentShading->text().toFloat();
	diffuseAtten = myDvrTab->diffuseAttenuation->text().toFloat();
	ambientAtten = myDvrTab->ambientAttenuation->text().toFloat();
	specularAtten = myDvrTab->specularAttenuation->text().toFloat();
	
	myTFEditor->setMinEditValue(myDvrTab->editCenter->text().toFloat() -
		0.5f* myDvrTab->editSize->text().toFloat());
	myTFEditor->setMaxEditValue(myDvrTab->editCenter->text().toFloat() +
		0.5f* myDvrTab->editSize->text().toFloat());
	myDvrTab->editLeftLabel->setText(strn.setNum(myTFEditor->getMinEditValue(),'g', 4));
	myDvrTab->editRightLabel->setText(strn.setNum(myTFEditor->getMaxEditValue(),'g', 4));
	//Need to know whether the min/max values changed or the size
	float newMinMap = myDvrTab->leftMappingBound->text().toFloat();
	float newMaxMap = myDvrTab->rightMappingBound->text().toFloat();
	
	

	//if the left or right domain bounds changed, ignore changes in
	//the center/size text edits.  Ignore round-off errors
	if((fabs(myTransFunc->getMinMapValue() - newMinMap) > ROUND_OFF) ||
		(fabs(myTransFunc->getMaxMapValue() - newMaxMap) > ROUND_OFF)
	  )
	{
		myTransFunc->setMinMapValue(newMinMap);
		myTransFunc->setMaxMapValue(newMaxMap);
		myDvrTab->tfDomainCenterEdit->setText(strn.setNum(0.5f*(newMaxMap+newMinMap),'g',4));
		myDvrTab->tfDomainSizeEdit->setText(strn.setNum(newMaxMap-newMinMap,'g',4));
	} else {
		float newMapSize = myDvrTab->tfDomainSizeEdit->text().toFloat();
		float newMapCenter = myDvrTab->tfDomainCenterEdit->text().toFloat();
		myTransFunc->setMinMapValue(newMapCenter-.5f*newMapSize);
		myTransFunc->setMaxMapValue(newMapCenter+.5f*newMapSize);
		myDvrTab->leftMappingBound->setText(strn.setNum(newMapCenter-.5f*newMapSize,'g',4));
		myDvrTab->rightMappingBound->setText(strn.setNum(newMapCenter+.5f*newMapSize,'g',4));
	}

	setTFESliders();
	myTFEditor->setDirty(true);
	myDvrTab->DvrTFFrame->update();
	guiSetTextChanged(false);
	//updateRenderer();
}
//Set the TFE sliders consistent with the bounds settings in params
void DvrParams::
setTFESliders(){
	//first set the domain sliders.
	//The domain size slider has its max the full data range,
	//or the size of the domain, whichever is larger.
	setDataRange();
	float mapSize = (myTransFunc->getMaxMapValue()- myTransFunc->getMinMapValue());
	float relSize = mapSize/(rightTFLimit - leftTFLimit);
	
	myDvrTab->tfDomainSizeSlider->setValue((int)(relSize*1000.f));
	float centerMax = myTransFunc->getMaxMapValue();
	float centerMin = myTransFunc->getMinMapValue();

	float center = (centerMin+centerMax)*0.5f;
	int centerSlider;
	if (centerMin >= centerMax) centerSlider = 500;
	else centerSlider = (int)(1000.f * (center - centerMin)/(centerMax - centerMin));
	myDvrTab->tfDomainCenterSlider->setValue(centerSlider);
	
	//then set the edit window sliders:
	float size = (myTFEditor->getMaxEditValue()- myTFEditor->getMinEditValue());
	relSize = size/(rightTFLimit - leftTFLimit);
	
	myDvrTab->editSizeSlider->setValue((int)(relSize*1000.f));
	center = ((myTFEditor->getMaxEditValue()+ myTFEditor->getMinEditValue())*0.5f);
	
	centerSlider = (int)(1000.f * (center - leftTFLimit)/(rightTFLimit - leftTFLimit));
	myDvrTab->editCenterSlider->setValue(centerSlider);

}
/*
 * Method to be invoked after the user has moved the right or left bounds
 * From the TFE editor.  
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 * Then make the sliders update.
 */
void DvrParams::
updateTFBounds(){
	QString strn;
	float mapCenter = 0.5f*(myTransFunc->getMinMapValue() +myTransFunc->getMaxMapValue());
	float mapSize = myTransFunc->getMaxMapValue() -myTransFunc->getMinMapValue();
	myDvrTab->leftMappingBound->setText(strn.setNum(myTransFunc->getMinMapValue(),'g',4));
	myDvrTab->rightMappingBound->setText(strn.setNum(myTransFunc->getMaxMapValue(),'g',4));
	myDvrTab->tfDomainCenterEdit->setText(strn.setNum(mapCenter, 'g', 4));
	myDvrTab->tfDomainSizeEdit->setText(strn.setNum(mapSize, 'g', 4));
	setTFESliders();
}
void DvrParams::
setClut(const float newTable[256][4]){
	for (int i = 0; i< 256; i++) {
		for (int j = 0; j< 4; j++){
			ctab[i][j] = newTable[i][j];
		}
	}
	clutDirty = true;
}

void DvrParams::
guiSetEnabled(bool value){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"toggle dvr enabled");
	setEnabled(value);
	PanelCommand::captureEnd(cmd, this);
	//updateRenderer();
}
void DvrParams::
guiSetVarNum(int val){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"set dvr variable");
	setVarNum(val);
	PanelCommand::captureEnd(cmd, this);
	//updateRenderer();
}
void DvrParams::
guiSetNumBits(int val){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"set dvr number bits");
	setNumBits(val);
	PanelCommand::captureEnd(cmd, this);
	//updateRenderer();
}
void DvrParams::
guiSetLighting(bool val){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(), "toggle dvr lighting");
	setLighting(val);
	PanelCommand::captureEnd(cmd, this);
	//updateRenderer();
}

//The start and end of the slider move are captured in history.  Individual
//slider moves are not captured.
void DvrParams::
guiStartTFECenterSlide(){
	confirmText(false);
	if(savedCommand) return;
	savedCommand = PanelCommand::captureStart(this, mainWin->getSession(),"slide TF Edit window center");	
}
//Capture the event, adjust text boxes when the user stops sliding the center slider.
void DvrParams::
guiEndTFECenterSlide(int n){
	guiMoveTFECenter(n);
	PanelCommand::captureEnd(savedCommand,this);
	savedCommand = 0;
}

//When the center-slider moves:
void DvrParams::
guiMoveTFECenter(int n){
	
	int sizeVal = myDvrTab->editSizeSlider->value();
	
	float realHalfSize =  (((float)(sizeVal/2))/1000.f)*(rightTFLimit - leftTFLimit);	
	//Convert n to a float val:
	float centerRatio = (float)n/1000.f;
	float realCenter = leftTFLimit*(1.f-centerRatio) +
		centerRatio*rightTFLimit;
	myTFEditor->setMinEditValue(realCenter - realHalfSize);
	myTFEditor->setMaxEditValue(realCenter + realHalfSize);
	//Set the line edit
	myDvrTab->editCenter->setText(QString::number(realCenter));
	
	//make label values consistent:
	myDvrTab->editLeftLabel->setText(QString::number(realCenter - realHalfSize,'g',4));
	myDvrTab->editRightLabel->setText(QString::number(realCenter + realHalfSize,'g',4));
	
	//Ignore the resulting textChanged events:
	textChangedFlag = false;
	myTFEditor->setDirty(true);
	myDvrTab->DvrTFFrame->update();	
}
//Respond to a change in size slider

void DvrParams::
guiSetTFESize(int n){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"set TF editor display size");
	int centerVal = myDvrTab->editCenterSlider->value();
	
	//Convert n to a float val:
	float realCenter = leftTFLimit + ((float)centerVal/1000.f)*(rightTFLimit - leftTFLimit);
	float realHalfSize =  (((float)(n/2))/1000.f)*(rightTFLimit - leftTFLimit);	
	
	myTFEditor->setMinEditValue(realCenter - realHalfSize);
	myTFEditor->setMaxEditValue(realCenter + realHalfSize);
	//Set the line edits
	myDvrTab->editCenter->setText(QString::number(realCenter));
	myDvrTab->editSize->setText(QString::number(2.f*realHalfSize));
	
	//make label values consistent:
	myDvrTab->editLeftLabel->setText(QString::number(realCenter - realHalfSize,'g',4));
	myDvrTab->editRightLabel->setText(QString::number(realCenter + realHalfSize,'g',4));
	
	//Ignore the resulting textChanged event:
	textChangedFlag = false;
	myTFEditor->setDirty(true);
	myDvrTab->DvrTFFrame->update();
	PanelCommand::captureEnd(cmd,this);
}
//Routines that handle the tf domain center/size slides
void DvrParams::
guiStartTFDomainCenterSlide(){
	confirmText(false);
	if(savedCommand) return;
	savedCommand = PanelCommand::captureStart(this, mainWin->getSession(),"slide TF domain center");	
}
//Capture the event, adjust text boxes when the user stops sliding the center slider.
void DvrParams::
guiEndTFDomainCenterSlide(int n){
	guiMoveTFDomainCenter(n);
	PanelCommand::captureEnd(savedCommand,this);
	savedCommand = 0;
}

//When the domain center-slider moves:
void DvrParams::
guiMoveTFDomainCenter(int n){
	
	int sizeVal = myDvrTab->tfDomainSizeSlider->value();
	
	
	float realHalfSize =  (((float)(sizeVal/2))/1000.f)*(rightTFLimit - leftTFLimit);	
	//Convert n to a float val:
	float centerRatio = (float)n/1000.f;
	float realCenter = leftTFLimit*(1.f-centerRatio) +
		centerRatio*rightTFLimit;
	myTransFunc->setMinMapValue(realCenter - realHalfSize);
	myTransFunc->setMaxMapValue(realCenter + realHalfSize);
	//Set the line edits that are affected:
	myDvrTab->tfDomainCenterEdit->setText(QString::number(realCenter));
	myDvrTab->leftMappingBound->setText(QString::number(realCenter - realHalfSize,'g',4));
	myDvrTab->rightMappingBound->setText(QString::number(realCenter + realHalfSize,'g',4));
	
	//Ignore the resulting textChanged events:
	textChangedFlag = false;
	myTFEditor->setDirty(true);
	myDvrTab->DvrTFFrame->update();	
}
//Respond to a change in domain size slider

void DvrParams::
guiSetTFDomainSize(int n){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"set TF domain size");
	int centerVal = myDvrTab->tfDomainCenterSlider->value();
	
	//Convert n to a float val:
	float realCenter = leftTFLimit + ((float)centerVal/1000.f)*(rightTFLimit - leftTFLimit);
	float realHalfSize =  (((float)(n/2))/1000.f)*(rightTFLimit - leftTFLimit);	
	
	myTransFunc->setMinMapValue(realCenter - realHalfSize);
	myTransFunc->setMaxMapValue(realCenter + realHalfSize);
	//Set the line edits
	myDvrTab->tfDomainCenterEdit->setText(QString::number(realCenter));
	myDvrTab->tfDomainSizeEdit->setText(QString::number(2.f*realHalfSize));
	
	//make label values consistent:
	myDvrTab->leftMappingBound->setText(QString::number(realCenter - realHalfSize,'g',4));
	myDvrTab->rightMappingBound->setText(QString::number(realCenter + realHalfSize,'g',4));
	
	//Ignore the resulting textChanged event:
	textChangedFlag = false;
	myTFEditor->setDirty(true);
	myDvrTab->DvrTFFrame->update();
	PanelCommand::captureEnd(cmd,this);
}
//Respond to a change in histogram stretch factor
void DvrParams::
guiSetHistoStretch(int val){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"modify histogram stretch slider");
	myTFEditor->setHistoStretch( powf(2.f, HISTOSTRETCHCONSTANT*(1000.f - (float)val)));
	myTFEditor->setDirty(true);
	myDvrTab->DvrTFFrame->update();
	PanelCommand::captureEnd(cmd,this);
}
//Respond to a change in transfer function (from color selection or mouse down/release events)
//These are just for undo/redo
void DvrParams::
guiStartChangeTF(char* str){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	savedCommand = PanelCommand::captureStart(this, mainWin->getSession(),str);
}
void DvrParams::
guiEndChangeTF(){
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand,this);
	savedCommand = 0;
}

void DvrParams::
guiBindColorToOpac(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"bind Color to Opacity");
	myTFEditor->bindColorToOpac();
	PanelCommand::captureEnd(cmd, this);
}
void DvrParams::
guiBindOpacToColor(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"bind Opacity to Color");
	myTFEditor->bindOpacToColor();
	PanelCommand::captureEnd(cmd, this);
}
void DvrParams::
setBindButtons(){
	bool enable = myTFEditor->canBind();
	myDvrTab->OpacityBindButton->setEnabled(enable);
	myDvrTab->ColorBindButton->setEnabled(enable);
}
//Require that the TFE bounds are within the TF bounds. 
//Return true if anything is changed.
bool DvrParams::
enforceConsistency(){
	bool changed = false;
	//Make sure Map values are OK.
	
	if (myTransFunc->getMaxMapValue()<= myTransFunc->getMinMapValue()){
		changed = true;
		myTransFunc->setMaxMapValue( myTransFunc->getMinMapValue() + 1.e-20);
	}
	
	return changed;
}
void DvrParams::setClutDirty(bool dirty){ 
	clutDirty = dirty;
	if (dirty) {
		VizWinMgr* vizWinMgr = mainWin->getVizWinMgr();
		if (vizWinMgr) vizWinMgr->setDvrDirty(this);
	}
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
void DvrParams::
updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow){
	bool isLocal = local;
	if (newWindow) {
		prevEnabled = false;
		wasLocal = true;
		isLocal = true;
	}
	if (prevEnabled == enabled && wasLocal == local) return;
	VizWinMgr* vizWinMgr = mainWin->getVizWinMgr();
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
	//   VizWinMgr already has the current local/global dvr settings
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	//  change of enable->disable with global->local.  (Must disable the local renderer)
	//  change of enable->disable with local->global (Must disable the local renderer)
	
	//For a new renderer

	
	if (enabled && !prevEnabled){//For cases 2. or 3. :  create a renderer in the active window:
#ifdef VOLUMIZER
		VolumizerRenderer* myDvr = new VolumizerRenderer(viz);
		viz->addRenderer(myDvr);
#else
		GLBox* myBox = new GLBox (viz);
		viz->addRenderer(myBox);
#endif
		//force the renderer to refresh region data
		viz->setRegionDirty(true);
		//Quit if not case 3:
		if (wasLocal || isLocal) return;
	}
	
	if (!isLocal && enabled){ //case 3: create renderers in all *other* global windows, then return
		for (int i = 0; i<MAXVIZWINS; i++){
			if (i == activeViz) continue;
			viz = vizWinMgr->getVizWin(i);
			if (viz && !vizWinMgr->getDvrParams(i)->isLocal()){
#ifdef VOLUMIZER
				VolumizerRenderer* myDvr = new VolumizerRenderer(viz);
				viz->addRenderer(myDvr);
#else
				GLBox* myBox = new GLBox (viz);
				viz->addRenderer(myBox);
#endif
				//force the renderer to refresh region data
				viz->setRegionDirty(true);
			}
		}
		return;
	}
	if (!enabled && prevEnabled && !isLocal && !wasLocal) { //case 5., disable all global renderers
		for (int i = 0; i<MAXVIZWINS; i++){
			viz = vizWinMgr->getVizWin(i);
			if (viz && !vizWinMgr->getDvrParams(i)->isLocal()){
#ifdef VOLUMIZER
				viz->removeRenderer("VolumizerRenderer");
#else
				viz->removeRenderer("GLBox");
#endif
			}
		}
		return;
	}
	assert(prevEnabled && !enabled && (isLocal ||(isLocal != wasLocal))); //case 6, disable local only
#ifdef VOLUMIZER
	viz->removeRenderer("VolumizerRenderer");
#else
	viz->removeRenderer("GLBox");
#endif
	return;
}
//Respond to change in Metadata
//
void DvrParams::
reinit(){
	Metadata* md = mainWin->getSession()->getCurrentMetadata();
	//Get the variable names:
	variableNames = md->GetVariableNames();
	//reset to first variable:
	varNum = 0;
	numVariables = md->GetVariableNames().size();
}


	
