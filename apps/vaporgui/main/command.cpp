//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		command.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements three small subclasses of the Command class including 
//		MouseModeCommand, TabChangeCommand, ColorChangeCommand
//
#include "command.h"
#include "params.h"
#include "session.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "vizfeatureparams.h"
#include "userpreferences.h"
#include "pythonpipeline.h"
#include "assert.h"
#include <qaction.h>
using namespace VAPoR;
//Constructor:  called when a new command is issued.
MouseModeCommand::MouseModeCommand(int oldMode, int newMode){
	//Make a copy of previous panel:
	previousMode = oldMode;
	currentMode = newMode;
	
	description = QString("change mode: ");
	description += modeName(oldMode);
	description += " to ";
	description += modeName(newMode);
}

void MouseModeCommand::unDo(){
	Session::getInstance()->blockRecording();
	MainForm::getInstance()->setMouseMode(previousMode);
	Session::getInstance()->unblockRecording();
}
void MouseModeCommand::reDo(){
	Session::getInstance()->blockRecording();
	MainForm::getInstance()->setMouseMode(currentMode);
	Session::getInstance()->unblockRecording();
}

const char* MouseModeCommand::
modeName(int t){
	return GLWindow::getModeName(t).c_str();
}

TabChangeCommand::TabChangeCommand(Params::ParamsBaseType oldTab, Params::ParamsBaseType newTab){
	//Make a copy of previous panel:
	previousTab = oldTab;
	currentTab = newTab;
	
	
	description = QString(" change tab ");
	description += tabName(oldTab);
	description += " to ";
	description += tabName(newTab);
}

void TabChangeCommand::unDo(){
	Session::getInstance()->blockRecording();
	const string& tag = Params::GetTagFromType(previousTab);
	MainForm::showTab(tag);
	Session::getInstance()->unblockRecording();
}

void TabChangeCommand::reDo(){
	Session::getInstance()->blockRecording();
	const string& tag = Params::GetTagFromType(currentTab);
	MainForm::showTab(tag);
	Session::getInstance()->unblockRecording();
}

const char* TabChangeCommand::
tabName(Params::ParamsBaseType t){
	return Params::paramName(t).c_str();
}
//VizFeatureCommand methods:

VizFeatureCommand::VizFeatureCommand(VizFeatureParams* prevFeatures, const char* descr, int vizNum){
	//Make a copy of previous features
	previousFeatures = new VizFeatureParams(*prevFeatures);
	currentFeatures = 0;
	windowNum = vizNum;
	QString vizName = VizWinMgr::getInstance()->getVizWinName(vizNum);
	description = QString(descr) + " on " + (vizName);	
}

VizFeatureCommand::~VizFeatureCommand(){
	if (previousFeatures) delete previousFeatures;
	if (currentFeatures) delete currentFeatures;
}
void VizFeatureCommand::
setNext(VizFeatureParams* nextFeatures){
	currentFeatures = new VizFeatureParams(*nextFeatures);
}
void VizFeatureCommand::unDo(){
	Session::getInstance()->blockRecording();
	previousFeatures->applyToViz(windowNum);
	Session::getInstance()->unblockRecording();
}

void VizFeatureCommand::reDo(){
	Session::getInstance()->blockRecording();
	currentFeatures->applyToViz(windowNum);
	Session::getInstance()->unblockRecording();
}

VizFeatureCommand* VizFeatureCommand::
captureStart(VizFeatureParams* p, const char* description, int viznum){
	if (!Session::getInstance()->isRecording()) return 0;
	VizFeatureCommand* cmd = new VizFeatureCommand(p, description, viznum);
	return cmd;
}
void VizFeatureCommand::
captureEnd(VizFeatureCommand* vCom, VizFeatureParams *p) {
	if (!vCom) return;
	if (!Session::getInstance()->isRecording()) return;
	vCom->setNext(p);
	Session::getInstance()->addToHistory(vCom);
}

//PreferencesCommand methods
PreferencesCommand::PreferencesCommand(UserPreferences* prevPrefs, const char* descr){
	//Make a copy of previous Prefs
	previousPrefs = prevPrefs->clone();
	currentPrefs = 0;
	description = QString(descr);	
}

PreferencesCommand::~PreferencesCommand(){
	if (previousPrefs) delete previousPrefs;
	if (currentPrefs) delete currentPrefs;
}
void PreferencesCommand::
setNext(UserPreferences* nextPrefs){
	currentPrefs = nextPrefs->clone();
}
void PreferencesCommand::unDo(){
	Session::getInstance()->blockRecording();
	previousPrefs->applyToState(false);
	Session::getInstance()->unblockRecording();
}

void PreferencesCommand::reDo(){
	Session::getInstance()->blockRecording();
	currentPrefs->applyToState(false);
	Session::getInstance()->unblockRecording();
}

PreferencesCommand* PreferencesCommand::
captureStart(UserPreferences* p, const char* description){
	if (!Session::getInstance()->isRecording()) return 0;
	PreferencesCommand* cmd = new PreferencesCommand(p, description);
	return cmd;
}
void PreferencesCommand::
captureEnd(PreferencesCommand* vCom, UserPreferences *p) {
	if (!vCom) return;
	if (!Session::getInstance()->isRecording()) return;
	vCom->setNext(p);
	Session::getInstance()->addToHistory(vCom);
}
ScriptCommand::ScriptCommand(const char* descr) {
	description = QString(descr);
}
ScriptCommand::~ScriptCommand(){
	prev2DInputs.clear();
	prev3DInputs.clear();
	next2DInputs.clear();
	next3DInputs.clear();
	prev2DOutputs.clear();
	prev3DOutputs.clear();
	next2DOutputs.clear();
	next3DOutputs.clear();
}
ScriptCommand* ScriptCommand::captureStart(int scriptId, const char* descr){
	if (!Session::getInstance()->isRecording()) return 0;
	ScriptCommand* cmd = new ScriptCommand(descr);
	cmd->prevId = scriptId;
	//cases are: 0 : new script
	//		>0 edit existing script
	//		-1 edit startup script
	if (scriptId > 0){ //edit existing script
		cmd->prevProgram = DataStatus::getDerivedScript(scriptId);
		for (int i = 0; i< DataStatus::getDerived2DInputVars(scriptId).size(); i++)
			cmd->prev2DInputs.push_back(DataStatus::getDerived2DInputVars(scriptId)[i]);
		for (int i = 0; i< DataStatus::getDerived3DInputVars(scriptId).size(); i++)
			cmd->prev3DInputs.push_back(DataStatus::getDerived3DInputVars(scriptId)[i]);
		for (int i = 0; i< DataStatus::getDerived2DOutputVars(scriptId).size(); i++)
			cmd->prev2DOutputs.push_back(DataStatus::getDerived2DOutputVars(scriptId)[i]);
		for (int i = 0; i< DataStatus::getDerived3DOutputVars(scriptId).size(); i++)
			cmd->prev3DOutputs.push_back(DataStatus::getDerived3DOutputVars(scriptId)[i]);
	} else if (scriptId == -1){
		cmd->prevProgram = PythonPipeLine::getStartupScript();
	}
	return cmd;
}
void ScriptCommand::captureEnd(ScriptCommand* cmd, int scriptId){
	cmd->nextId = scriptId;
	//Distinguish between edit, new, delete based on id.  
	//nextID > 0 for edit or new;
	//	 -1 for startup, -2 on delete.
	//A new script has scriptId=0 initially then the new id afterwards.
	if (scriptId > 0) { // completed edit of script, or new script.  It may have a new id
		// must save new program etc:
		cmd->nextProgram = DataStatus::getDerivedScript(scriptId);
		for (int i = 0; i< DataStatus::getDerived2DInputVars(scriptId).size(); i++)
			cmd->next2DInputs.push_back(DataStatus::getDerived2DInputVars(scriptId)[i]);
		for (int i = 0; i< DataStatus::getDerived3DInputVars(scriptId).size(); i++)
			cmd->next3DInputs.push_back(DataStatus::getDerived3DInputVars(scriptId)[i]);
		for (int i = 0; i< DataStatus::getDerived2DOutputVars(scriptId).size(); i++)
			cmd->next2DOutputs.push_back(DataStatus::getDerived2DOutputVars(scriptId)[i]);
		for (int i = 0; i< DataStatus::getDerived3DOutputVars(scriptId).size(); i++)
			cmd->next3DOutputs.push_back(DataStatus::getDerived3DOutputVars(scriptId)[i]);
	} else if (scriptId == -1) { //completed edit of startup script:
		cmd->nextProgram = PythonPipeLine::getStartupScript();
	} else if (scriptId == -2) { //Deleted script
		//nothing to do
	} 
	Session::getInstance()->addToHistory(cmd);
		
}
void ScriptCommand::unDo(){
	Session::getInstance()->blockRecording();
	DataStatus* ds = DataStatus::getInstance();
	if (nextId == -2) {//undo delete
		prevId = ds->addDerivedScript(prev2DInputs, prev2DOutputs, prev3DInputs, prev3DOutputs, prevProgram);
		VizWinMgr::getInstance()->reinitializeVariables();
	} else if (prevId > 0) { //undo edit 
		//Update existing script.  Note that the id can change
		//Need to remove old pipeline, too.
		VizWinMgr* vizMgr = VizWinMgr::getInstance();
		//Disable renderers that use the variables in the script
		vizMgr->disableRenderers(next2DOutputs, next3DOutputs);
		prevId = ds->replaceDerivedScript(nextId, prev2DInputs, prev2DOutputs, prev3DInputs, prev3DOutputs, prevProgram);
		assert(prevId > 0);
		//refresh the visualizers:
		
		vizMgr->refreshRenderData();
	} else if (prevId == 0) { //undo create new program
		VizWinMgr::getInstance()->disableRenderers(next2DOutputs,next3DOutputs);
		ds->removeDerivedScript(nextId);
		VizWinMgr::getInstance()->reinitializeVariables();
	} else if (prevId == -1) { //undo change to startup program
		PythonPipeLine::setStartupScript(prevProgram);
		VizWinMgr::getInstance()->refreshRenderData();
	}
	Session::getInstance()->unblockRecording();
}
void ScriptCommand::reDo(){
	Session::getInstance()->blockRecording();
	DataStatus* ds = DataStatus::getInstance();
	if (nextId == -2){ //redo delete
		VizWinMgr::getInstance()->disableRenderers(prev2DOutputs,prev3DOutputs);
		ds->removeDerivedScript(prevId);
		VizWinMgr::getInstance()->reinitializeVariables();
	} else if (nextId > 0) {//redo edit:
		VizWinMgr* vizMgr = VizWinMgr::getInstance();
		//Disable renderers that use the variables in the script
		vizMgr->disableRenderers(prev2DOutputs, prev3DOutputs);
		nextId = ds->replaceDerivedScript(prevId, next2DInputs, next2DOutputs, next3DInputs, next3DOutputs, nextProgram);
		assert(nextId > 0);
		//refresh the visualizers:
		vizMgr->refreshRenderData();
	} else if (prevId == 0) { //redo create new:
		nextId = ds->addDerivedScript(next2DInputs, next2DOutputs, next3DInputs, next3DOutputs, nextProgram);
		VizWinMgr::getInstance()->reinitializeVariables();
	} else if (prevId == -1) { //redo change to startup program:
		PythonPipeLine::setStartupScript(nextProgram);
		VizWinMgr::getInstance()->refreshRenderData();
	}
	Session::getInstance()->unblockRecording();
		
}
		







