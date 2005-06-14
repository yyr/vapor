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

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "dvrparams.h"
#include "dvr.h" 
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

#include <math.h>
#include <vapor/Metadata.h>

#ifdef VOLUMIZER
#include "volumizerrenderer.h"
#endif



using namespace VAPoR;
const string DvrParams::_variableTag = "Variable";
const string DvrParams::_leftEditBoundAttr = "LeftEditBound";
const string DvrParams::_rightEditBoundAttr = "RightEditBound";
const string DvrParams::_numVariablesAttr = "NumVariables";
const string DvrParams::_variableNumAttr = "VariableNum";
const string DvrParams::_activeVariableNumAttr = "ActiveVariableNum";
const string DvrParams::_editModeAttr = "TFEditMode";
const string DvrParams::_histoStretchAttr = "HistoStretchFactor";
const string DvrParams::_variableNameAttr = "VariableName";


DvrParams::DvrParams(int winnum) : Params(winnum){
	thisParamType = DvrParamsType;
	myDvrTab = MainForm::getInstance()->getDvrTab();

	minEditBounds = 0;
	maxEditBounds = 0;
	numBits = 8;
	numVariables = 0;
	restart();
	
}
DvrParams::~DvrParams(){
	


	
	if (savedCommand) delete savedCommand;
	
	if (minEditBounds) delete minEditBounds;
	if (maxEditBounds) delete maxEditBounds;
	for (int i = 0; i< numVariables; i++){
		delete transFunc[i];  //destructor will delete TFEditor
	}
	if (transFunc) delete transFunc;
	
}

void DvrParams::
setTab(Dvr* tab) {
	myDvrTab = tab;
	for (int i = 0; i<numVariables; i++){
		transFunc[i]->getEditor()->setFrame(myDvrTab->DvrTFFrame);
	}
}
//Deepcopy requires cloning tf and tfeditor
Params* DvrParams::
deepCopy(){
	DvrParams* newParams = new DvrParams(*this);
	//Clone the map bounds arrays:
	int numVars = Max (numVariables, 1);
	newParams->minEditBounds = new float[numVars];
	newParams->maxEditBounds = new float[numVars];
	for (int i = 0; i<numVars; i++){
		
		newParams->minEditBounds[i] = minEditBounds[i];
		newParams->maxEditBounds[i] = maxEditBounds[i];
	}

	//Clone the Transfer Function and the TFEditor
	newParams->transFunc = new TransferFunction*[numVariables];
	for (int i = 0; i<numVariables; i++){
		newParams->transFunc[i] = new TransferFunction(*transFunc[i]);
		//clone the tfe, hook it to the trans func
		TFEditor* newTFEditor = new TFEditor(*(transFunc[i]->getEditor()));
		newParams->transFunc[i]->setEditor(newTFEditor);
		newParams->transFunc[i]->setParams(newParams);
		newTFEditor->setTransferFunction(newParams->transFunc[i]);
		newTFEditor->setFrame(myDvrTab->DvrTFFrame);
	}
	
	//never keep the SavedCommand:
	newParams->savedCommand = 0;
	return newParams;
}
//Method called when undo/redo changes params:
void DvrParams::
makeCurrent(Params* prevParams, bool newWin) {
	
	VizWinMgr::getInstance()->setDvrParams(vizNum, this);

	updateDialog();
	DvrParams* formerParams = (DvrParams*)prevParams;
	//Check if the enabled and/or Local settings changed:
	if (formerParams->enabled != enabled || formerParams->local != local || newWin){
		updateRenderer(formerParams->enabled, formerParams->local, newWin);
	}
	setDatarangeDirty();
	setClutDirty();
}

void DvrParams::
refreshCtab() {
	getTransFunc()->makeLut((float*)ctab);
}
	
//Set the tab consistent with the dvrparams data here
//
void DvrParams::updateDialog(){
	
	QString strn;
	Session::getInstance()->blockRecording();
	myDvrTab->DvrTFFrame->setEditor(getTFEditor());
	myDvrTab->EnableDisable->setCurrentItem((enabled) ? 1 : 0);
	
	//Set the names in the variable combo
	myDvrTab->variableCombo->clear();
	myDvrTab->variableCombo->setMaxCount(numVariables);
	for (int i = 0; i< numVariables; i++){
		if (variableNames.size() > (unsigned int)i){
			const std::string& s = variableNames.at(i);
			//Direct conversion of std::string& to QString doesn't seem to work
			//Maybe std was not enabled when QT was built?
			const QString& text = QString(s.c_str());
			myDvrTab->variableCombo->insertItem(text);
		} else myDvrTab->variableCombo->insertItem("");
	}
	myDvrTab->variableCombo->setCurrentItem(varNum);
	
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
	myTFFrame->setEditor(getTFEditor());

	updateTFBounds();
	float sliderVal = 1.f;
	if (getTFEditor())
		sliderVal = getHistoStretch();
	sliderVal = 1000.f - logf(sliderVal)/(HISTOSTRETCHCONSTANT*logf(2.f));
	myDvrTab->histoStretchSlider->setValue((int) sliderVal);
	setBindButtons();
	
	//Set the mode buttons:
	
	if (editMode){
		
		myDvrTab->editButton->setOn(true);
		myDvrTab->navigateButton->setOn(false);
	} else {
		myDvrTab->editButton->setOn(false);
		myDvrTab->navigateButton->setOn(true);
	}
		
	if(getTFEditor())getTFEditor()->setDirty(true);
	myDvrTab->DvrTFFrame->update();
	myDvrTab->update();
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
		
}
//Update all the panel state associated with textboxes.
//Performed whenever a textbox changes
//
void DvrParams::
updatePanelState(){
	QString strn;
	enabled = myDvrTab->EnableDisable->currentItem();
	diffuseCoeff = myDvrTab-> diffuseShading->text().toFloat();
	ambientCoeff = myDvrTab->ambientShading->text().toFloat();
	specularCoeff = myDvrTab->specularShading->text().toFloat();
	specularExponent = (int)myDvrTab->exponentShading->text().toFloat();
	diffuseAtten = myDvrTab->diffuseAttenuation->text().toFloat();
	ambientAtten = myDvrTab->ambientAttenuation->text().toFloat();
	specularAtten = myDvrTab->specularAttenuation->text().toFloat();
	

	getTransFunc()->setMinMapValue(myDvrTab->leftMappingBound->text().toFloat());
	getTransFunc()->setMaxMapValue(myDvrTab->rightMappingBound->text().toFloat());
	
	setDatarangeDirty();
	getTFEditor()->setDirty(true);
	myDvrTab->DvrTFFrame->update();
	guiSetTextChanged(false);
	
}

//Change variable, plus other side-effects, updating tfe as well as tf.
//
void DvrParams::
setVarNum(int val) 
{
	varNum = val;
	
	//reset the editing display range shown on the tab, 
	//this also sets dirty flag
	updateTFBounds();
	//Force a redraw of tfframe
	getTFEditor()->setDirty(true);
	myDvrTab->DvrTFFrame->setEditor(getTFEditor());
	myDvrTab->DvrTFFrame->update();	
	setClutDirty();
	//Set region dirty, since new data is needed
	VizWinMgr::getInstance()->setRegionDirty(this);
}
/*
 * Method to be invoked after the user has moved the right or left bounds
 * (e.g. From the TFE editor. ) 
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void DvrParams::
updateTFBounds(){
	QString strn;
	myDvrTab->minDataBound->setText(strn.setNum(getDataMinBound()));
	myDvrTab->maxDataBound->setText(strn.setNum(getDataMaxBound()));
	if (getTransFunc()){
		myDvrTab->leftMappingBound->setText(strn.setNum(getTransFunc()->getMinMapValue(),'g',4));
		myDvrTab->rightMappingBound->setText(strn.setNum(getTransFunc()->getMaxMapValue(),'g',4));
	} else {
		myDvrTab->leftMappingBound->setText("0.0");
		myDvrTab->rightMappingBound->setText("1.0");
	}
	setDatarangeDirty();
}

void DvrParams::
setClut(const float newTable[256][4]){
	for (int i = 0; i< 256; i++) {
		for (int j = 0; j< 4; j++){
			ctab[i][j] = newTable[i][j];
		}
	}
	setClutDirty();
}
//Change mouse mode to specified value
//0,1,2 correspond to edit, zoom, pan
void DvrParams::
guiSetEditMode(bool mode){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "set edit/navigate mode");
	setEditMode(mode);
	PanelCommand::captureEnd(cmd, this);
}
void DvrParams::
guiSetAligned(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "align tf in edit frame");
	setMinEditBound(getMinMapBound());
	setMaxEditBound(getMaxMapBound());
	getTFEditor()->setDirty(true);
	myDvrTab->DvrTFFrame->update();
	PanelCommand::captureEnd(cmd, this);
}
void DvrParams::
guiSetEnabled(bool value){
	
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "toggle dvr enabled");
	setEnabled(value);
	PanelCommand::captureEnd(cmd, this);
	//updateRenderer(prevEnabled, local, false); (unnecessary; called by vizwinmgr)
}
void DvrParams::
guiSetVarNum(int val){
	if (val == varNum) return;
	//Only change the variable to a valid one:
	if (!Session::getInstance()->getDataStatus()->variableIsPresent(val)){
		MessageReporter::errorMsg( "Data for specified variable is not available");
		//Set it back to the old value.  This will generate an event that
		//we will ignore.
		myDvrTab->variableCombo->setCurrentItem(varNum);
		myDvrTab->update();
		return;
	}
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "set dvr variable");
	setVarNum(val);
	PanelCommand::captureEnd(cmd, this);
}

void DvrParams::
guiSetNumBits(int val){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "set dvr number bits");
	setNumBits(val);
	PanelCommand::captureEnd(cmd, this);
	
}
void DvrParams::
guiSetLighting(bool val){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "toggle dvr lighting");
	setLighting(val);
	PanelCommand::captureEnd(cmd, this);
}

//Respond to a change in histogram stretch factor
void DvrParams::
guiSetHistoStretch(int val){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "modify histogram stretch slider");
	setHistoStretch( powf(2.f, HISTOSTRETCHCONSTANT*(1000.f - (float)val)));
	getTFEditor()->setDirty(true);
	myDvrTab->DvrTFFrame->update();
	PanelCommand::captureEnd(cmd,this);
}
//Respond to a change in transfer function (from color selection or mouse down/release events)
//These are just for undo/redo.  Also may need to update visualizer and/or editor
//
void DvrParams::
guiStartChangeTF(char* str){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	savedCommand = PanelCommand::captureStart(this, str);
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
	PanelCommand* cmd = PanelCommand::captureStart(this, "bind Color to Opacity");
	getTFEditor()->bindColorToOpac();
	PanelCommand::captureEnd(cmd, this);
}
void DvrParams::
guiBindOpacToColor(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "bind Opacity to Color");
	getTFEditor()->bindOpacToColor();
	PanelCommand::captureEnd(cmd, this);
}
void DvrParams::
setBindButtons(){
	bool enable = false;
	if (getTFEditor())
		enable = getTFEditor()->canBind();
	myDvrTab->OpacityBindButton->setEnabled(enable);
	myDvrTab->ColorBindButton->setEnabled(enable);
}
void DvrParams::setClutDirty(){ 
	VizWinMgr::getInstance()->setClutDirty(this);
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
	bool newLocal = local;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	if (newWindow) {
		prevEnabled = false;
		wasLocal = true;
		newLocal = true;
	}
	//The actual enabled state of "this" depends on whether we are local or global.
	bool nowEnabled = enabled;
	if (!local) nowEnabled = vizWinMgr->getGlobalParams(Params::DvrParamsType)->isEnabled();
	
	if (prevEnabled == nowEnabled && wasLocal == local) return;
	
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
	//   VizWinMgr already has the current local/global dvr settings
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	//  change of enable->disable with global->local.  (Must disable the local renderer)
	//  change of enable->disable with local->global (Must disable the local renderer)
	
	//For a new renderer

	
	if (nowEnabled && !prevEnabled && newLocal){//For case 2.:  create a renderer in the active window:

		VolumizerRenderer* myDvr = new VolumizerRenderer(viz);
		viz->addRenderer(myDvr);

		//force the renderer to refresh region data  (why?)
		viz->setRegionDirty(true);
		setClutDirty();
		setDatarangeDirty();
		
		//Quit 
		return;
	}
	
	if (!newLocal && nowEnabled){ //case 3: create renderers in all  global windows, then return
		for (int i = 0; i<MAXVIZWINS; i++){
			
			viz = vizWinMgr->getVizWin(i);
			if (viz && !vizWinMgr->getDvrParams(i)->isLocal()){
				VolumizerRenderer* myDvr = new VolumizerRenderer(viz);
				viz->addRenderer(myDvr);
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
			if (viz && !vizWinMgr->getDvrParams(i)->isLocal()){
				viz->removeRenderer("VolumizerRenderer");
			}
		}
		return;
	}
	assert(prevEnabled && !nowEnabled && (newLocal ||(newLocal != wasLocal))); //case 6, disable local only
	viz->removeRenderer("VolumizerRenderer");

	return;
}
//Initialize for new metadata.  Keep old transfer functions
//
void DvrParams::
reinit(bool doOverride){
	int i;
	const Metadata* md = Session::getInstance()->getCurrentMetadata();
	//Get the variable names:
	variableNames = md->GetVariableNames();
	int newNumVariables = md->GetVariableNames().size();
	//See if current varNum is valid
	//reset to first variable that is present:
	if (!Session::getInstance()->getDataStatus()->variableIsPresent(varNum)){
		varNum = -1;
		for (i = 0; i<newNumVariables; i++) {
			if (Session::getInstance()->getDataStatus()->variableIsPresent(i)){
				varNum = i;
				break;
			}
		}
	}
	if (varNum == -1){
		MessageReporter::errorMsg("DVR Params: No data in specified dataset");
		for (i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete transFunc;
		numVariables = 0;
		return;
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
			newTransFunc[i] = new TransferFunction(this, numBits);
			//create new tfe, hook it to the trans func
			TFEditor* newTFEditor = new TFEditor(newTransFunc[i], myDvrTab->DvrTFFrame);
			newTransFunc[i]->setEditor(newTFEditor);
			newTransFunc[i]->setMinMapValue(Session::getInstance()->getDataRange(i)[0]);
			newTransFunc[i]->setMaxMapValue(Session::getInstance()->getDataRange(i)[1]);
			newMinEdit[i] = Session::getInstance()->getDataRange(i)[0];
			newMaxEdit[i] = Session::getInstance()->getDataRange(i)[1];
		}
	} else { 
		//attempt to make use of existing transfer functions, edit ranges.
		//delete any that are no longer referenced
		for (i = 0; i<newNumVariables; i++){
			if(i<numVariables){
				newTransFunc[i] = transFunc[i];
				newMinEdit[i] = minEditBounds[i];
				newMaxEdit[i] = maxEditBounds[i];
			} else { //create new tfe, hook it to the trans func
				newTransFunc[i] = new TransferFunction(this, numBits);
				TFEditor* newTFEditor = new TFEditor(transFunc[i], myDvrTab->DvrTFFrame);
				newTransFunc[i]->setEditor(newTFEditor);
				newTransFunc[i]->setMinMapValue(Session::getInstance()->getDataRange(i)[0]);
				newTransFunc[i]->setMaxMapValue(Session::getInstance()->getDataRange(i)[1]);
				newMinEdit[i] = Session::getInstance()->getDataRange(i)[0];
				newMaxEdit[i] = Session::getInstance()->getDataRange(i)[1];
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
			newMinEdit[i] = Session::getInstance()->getDataRange(i)[0];
			newMaxEdit[i] = Session::getInstance()->getDataRange(i)[1];
		}
		//And check again...
		if (newMinEdit[i] >= newMaxEdit[i]){
			newMinEdit[i] = 0.f;
			newMaxEdit[i] = 1.f;
		}
	}
	//Hook up new stuff
	delete minEditBounds;
	delete maxEditBounds;
	delete transFunc;
	minEditBounds = newMinEdit;
	maxEditBounds = newMaxEdit;
	transFunc = newTransFunc;
	
	numVariables = newNumVariables;
	bool wasEnabled = enabled;
	setEnabled(false);
	//Always disable  don't change local/global 
	updateRenderer(wasEnabled, isLocal(), false);
	
	setClutDirty();
	setDatarangeDirty();
	if(numVariables>0) getTFEditor()->setDirty(true);
	//If dvr is the current front tab, and if it applies to the active visualizer,
	//update its values
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myDvrTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getDvrParams(viznum)))
			updateDialog();
	}
}
//Initialize to default state
//
void DvrParams::
restart(){
	histoStretchFactor = 1.f;
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
	if(numVariables > 0){
		for (int i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete transFunc;
	}
	numVariables = 0;
	attenuationDirty = true;
	transFunc = 0;
	//Initialize the mapping bounds to [0,1] until data is read
	
	if (minEditBounds) delete minEditBounds;
	if (maxEditBounds) delete maxEditBounds;
	
	
	minEditBounds = new float[1];
	maxEditBounds = new float[1];
	minEditBounds[0] = 0.f;
	maxEditBounds[0] = 1.f;
	currentDatarange[0] = 0.f;
	currentDatarange[1] = 1.f;
	editMode = true;   //default is edit mode
	savedCommand = 0;
	setEnabled(false);
	setClutDirty();
	setDatarangeDirty();
	//If dvr is the current front tab, and if it applies to the active visualizer,
	//update its values
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myDvrTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getDvrParams(viznum)))
			updateDialog();
	}
}
//Method to invalidate a datarange, and to force a rendering
//with new data quantization
void DvrParams::
setDatarangeDirty()
{
	if (!getTransFunc()) return;
	if (currentDatarange[0] != getTransFunc()->getMinMapValue() ||
		currentDatarange[1] != getTransFunc()->getMaxMapValue()){
			currentDatarange[0] = getTransFunc()->getMinMapValue();
			currentDatarange[1] = getTransFunc()->getMaxMapValue();
			VizWinMgr::getInstance()->setDataRangeDirty(this);
	}
}

//Respond to user request to load/save TF
//Assumes name is valid
//
void DvrParams::
sessionLoadTF(QString* name){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "Load Transfer Function from Session");
	
	//Get the transfer function from the session:
	
	std::string s(name->ascii());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	hookupTF(tf, varNum);
	PanelCommand::captureEnd(cmd, this);
}
void DvrParams::
fileLoadTF(){
	
	//Open a file load dialog
	
    QString s = QFileDialog::getOpenFileName(
                    Session::getInstance()->getTFFilePath().c_str(),
                    "Vapor Transfer Functions (*.vtf)",
                    myDvrTab,
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

	hookupTF(t, varNum);
	PanelCommand::captureEnd(cmd, this);
	//Remember the path to the file:
	Session::getInstance()->updateTFFilePath(&s);
}
//Hook up the new transfer function in specified slot,
//Delete the old one.
//
void DvrParams::
hookupTF(TransferFunction* t, int index){

	//Create a new TFEditor
	TFEditor* newTFEditor = new TFEditor(t, myDvrTab->DvrTFFrame);
	t->setEditor(newTFEditor);
	minEditBounds[index] = t->getMinMapValue();
	maxEditBounds[index] = t->getMaxMapValue();
	t->setParams(this);
	newTFEditor->reset();
	delete transFunc[index];
	transFunc[index] = t;
	
	//reset the editing display range, this also sets dirty flag
	updateTFBounds();
	//Force a redraw of tfframe
	newTFEditor->setDirty(true);
	myDvrTab->DvrTFFrame->update();	
	setDatarangeDirty();
	setClutDirty();
}
void DvrParams::
fileSaveTF(){
	//Launch a file save dialog, open resulting file
    QString s = QFileDialog::getSaveFileName(
					Session::getInstance()->getTFFilePath().c_str(),
                    "Vapor Transfer Functions (*.vtf)",
                    myDvrTab,
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
	
	
	
	if (!getTransFunc()->saveToFile(fileout)){//Report error if can't save to file
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
void DvrParams::
refreshTFFrame(){
	getTFEditor()->setDirty(true);
	myDvrTab->DvrTFFrame->update();	
}
void DvrParams::setMinMapBound(float val){
	getTransFunc()->setMinMapValue(val);
}
void DvrParams::setMaxMapBound(float val){
	getTransFunc()->setMaxMapValue(val);
}

float DvrParams::getMinMapBound(){
	return getTransFunc()->getMinMapValue();
}
float DvrParams::getMaxMapBound(){
	return getTransFunc()->getMaxMapValue();
}
//Handlers for Expat parsing.
//
bool DvrParams::
elementStartHandler(ExpatParseMgr* pm, int depth , std::string& tagString, const char **attrs){
	static int parsedVarnum;
	int i;
	if (StrCmpNoCase(tagString, _dvrParamsTag) == 0) {
		int newNumVariables = 0;
		//If it's a Dvr tag, save 5 attributes (2 are from Params class)
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
			else if (StrCmpNoCase(attribName, _activeVariableNumAttr) == 0) {
				ist >> varNum;
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				if (value == "true") setLocal(true); else setLocal(false);
			}
			else if (StrCmpNoCase(attribName, _histoStretchAttr) == 0){
				float histStretch;
				ist >> histStretch;
				setHistoStretch(histStretch);
			}
			else if (StrCmpNoCase(attribName, _editModeAttr) == 0){
				if (value == "true") setEditMode(true); 
				else setEditMode(false);
			}
			else return false;
		}
		//Create space for the variables:
		int numVars = Max (newNumVariables, 1);
		if(minEditBounds) delete minEditBounds;
		minEditBounds = new float[numVars];
		if(maxEditBounds) delete maxEditBounds;
		maxEditBounds = new float[numVars];
		variableNames.clear();
		for (i = 0; i<newNumVariables; i++){
			variableNames.push_back("");
		}
		//Setup with default values, in case not specified:
		for (i = 0; i< newNumVariables; i++){
			minEditBounds[i] = 0.f;
			maxEditBounds[i] = 1.f;
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
			transFunc[j] = new TransferFunction(this, numBits);
			TFEditor* newTFEditor = new TFEditor(transFunc[j],myDvrTab->DvrTFFrame);
			transFunc[j]->setEditor(newTFEditor);
		}
		return true;
	}
	//Parse a Variable:
	else if (StrCmpNoCase(tagString, _variableTag) == 0) {
		parsedVarnum = 0;
		float leftEdit = 0.f;
		float rightEdit = 1.f;
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
			else return false;
		}
		// Now set the values obtained from attribute parsing.
	
		variableNames[parsedVarnum] =  varName;
		minEditBounds[parsedVarnum] = leftEdit;
		maxEditBounds[parsedVarnum] = rightEdit;
		return true;
	}
	//Parse a transferFunction
	//Note we are relying on parsedvarnum obtained by previous start handler:
	else if (StrCmpNoCase(tagString, TransferFunction::_transferFunctionTag) == 0) {
		//Need to "push" to transfer function parser.
		//That parser will "pop" back to dvrparams when done.
		pm->pushClassStack(transFunc[parsedVarnum]);
		transFunc[parsedVarnum]->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	}
	else return false;
}
//The end handler needs to pop the parse stack, nothing else
bool DvrParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	
	if (StrCmpNoCase(tag, _dvrParamsTag) == 0) {
		//If this is a dvrparams, need to
		//pop the parse stack.  The caller will need to save the resulting
		//transfer function (i.e. this)
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0) {
		return true;
	} else if (StrCmpNoCase(tag, _variableTag) == 0){
		return true;
	} else {
		pm->parseError("Unrecognized end tag in DVRParams %s",tag.c_str());
		return false;  //Could there be other end tags that we ignore??
	}
}

//Method to construct Xml for state saving
XmlNode* DvrParams::
buildNode() {
	//Construct the dvr node
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
	oss << (long)varNum;
	attrs[_activeVariableNumAttr] = oss.str();

	oss.str(empty);
	if (editMode)
		oss << "true";
	else 
		oss << "false";
	attrs[_editModeAttr] = oss.str();
	oss.str(empty);
	oss << (double)getHistoStretch();
	attrs[_histoStretchAttr] = oss.str();
	
	XmlNode* dvrNode = new XmlNode(_dvrParamsTag, attrs, 3);

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
		oss << (double)minEditBounds[i];
		attrs[_leftEditBoundAttr] = oss.str();

		oss.str(empty);
		oss << (double)maxEditBounds[i];
		attrs[_rightEditBoundAttr] = oss.str();

		XmlNode* varNode = new XmlNode(_variableTag,attrs,1);

		//Create a transfer function node, add it as child
		XmlNode* tfNode = transFunc[i]->buildNode(empty);
		varNode->AddChild(tfNode);
		dvrNode->AddChild(varNode);
	}
	return dvrNode;
}
void DvrParams::resetMinEditBounds(vector<double>& newBounds){
	if (minEditBounds) delete minEditBounds;
	minEditBounds = new float[newBounds.size()];
	
	for (unsigned int i = 0; i<newBounds.size(); i++){ 
		minEditBounds[i] = newBounds[i];
	}
}
void DvrParams::resetMaxEditBounds(vector<double>& newBounds){
	if (maxEditBounds) delete maxEditBounds;
	maxEditBounds = new float[newBounds.size()];
	for (unsigned int i = 0; i<newBounds.size(); i++) 
		maxEditBounds[i] = newBounds[i];
}
TFEditor* DvrParams::getTFEditor(){
	return (numVariables > 0 ? transFunc[varNum]->getEditor() : 0);
}
TransferFunction* DvrParams::getTransFunc() {
	return (numVariables > 0 ? transFunc[varNum] : 0);
}
