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
#include "assert.h"
#include <qaction.h>
using namespace VAPoR;
//Constructor:  called when a new command is issued.
MouseModeCommand::MouseModeCommand(GLWindow::mouseModeType oldMode, GLWindow::mouseModeType newMode){
	//Make a copy of previous panel:
	previousMode = oldMode;
	currentMode = newMode;
	
	description = QString("change mode");
	description += modeName(oldMode);
	description += "to";
	description += modeName(newMode);
}

void MouseModeCommand::unDo(){
	Session::getInstance()->blockRecording();
	switch (previousMode){
		case GLWindow::navigateMode:
			MainForm::getInstance()->navigationAction->setOn(true);
			break;
		case GLWindow::regionMode:
			MainForm::getInstance()->regionSelectAction->setOn(true);
			break;
		case GLWindow::probeMode:
			MainForm::getInstance()->probeAction->setOn(true);
			break;
		case GLWindow::twoDDataMode:
			MainForm::getInstance()->twoDDataAction->setOn(true);
			break;
		case GLWindow::twoDImageMode:
			MainForm::getInstance()->twoDImageAction->setOn(true);
			break;
		case GLWindow::rakeMode:
			MainForm::getInstance()->rakeAction->setOn(true);
			break;
		case GLWindow::lightMode:
			MainForm::getInstance()->moveLightsAction->setOn(true);
			break;
		
		default:
			assert(0);
	}
	Session::getInstance()->unblockRecording();
}
void MouseModeCommand::reDo(){
	Session::getInstance()->blockRecording();
	switch (currentMode){
		case GLWindow::navigateMode:
			MainForm::getInstance()->navigationAction->setOn(true);
			break;
		case GLWindow::regionMode:
			MainForm::getInstance()->regionSelectAction->setOn(true);
			break;
		case GLWindow::probeMode:
			MainForm::getInstance()->probeAction->setOn(true);
			break;
		case GLWindow::twoDDataMode:
			MainForm::getInstance()->twoDDataAction->setOn(true);
			break;
		case GLWindow::twoDImageMode:
			MainForm::getInstance()->twoDImageAction->setOn(true);
			break;
		case GLWindow::rakeMode:
			MainForm::getInstance()->rakeAction->setOn(true);
			break;
		case GLWindow::lightMode:
			MainForm::getInstance()->moveLightsAction->setOn(true);
			break;
		
		default:
			assert(0);
	}
	Session::getInstance()->unblockRecording();
}

const char* MouseModeCommand::
modeName(GLWindow::mouseModeType t){
	switch (t){
		case GLWindow::navigateMode:
			return " navigate ";
		case GLWindow::regionMode:
			return " region-set ";
		case GLWindow::probeMode:
			return " probe-set ";
		case GLWindow::rakeMode:
			return " rake-set ";
		case GLWindow::twoDDataMode:
			return " 2d planar-set ";
		case GLWindow::twoDImageMode:
			return " image planar-set ";
		case GLWindow::lightMode:
			return " light-move ";
		default:  
			assert(0);
			return 0;
	}
}

TabChangeCommand::TabChangeCommand(Params::ParamType oldTab, Params::ParamType newTab){
	//Make a copy of previous panel:
	previousTab = oldTab;
	currentTab = newTab;
	
	
	description = QString("change tab");
	description += tabName(oldTab);
	description += "to";
	description += tabName(newTab);
}

void TabChangeCommand::unDo(){
	Session::getInstance()->blockRecording();
	switch (previousTab){
		case Params::UnknownParamsType:
			MainForm::getInstance()->viewpoint();
			break;
		case Params::ViewpointParamsType:
			MainForm::getInstance()->viewpoint();
			break;
		case Params::RegionParamsType:
			MainForm::getInstance()->region();
			break;
		
		case Params::FlowParamsType:
			MainForm::getInstance()->launchFlowTab();
			break;
		case Params::DvrParamsType:
			MainForm::getInstance()->renderDVR();
			break;
		case Params::IsoParamsType:
			MainForm::getInstance()->launchIsoTab();
			break;
		case Params::ProbeParamsType:
			MainForm::getInstance()->launchProbeTab();
			break;
		case Params::TwoDDataParamsType:
			MainForm::getInstance()->launchTwoDDataTab();
			break;
		case Params::TwoDImageParamsType:
			MainForm::getInstance()->launchTwoDImageTab();
			break;
		case Params::AnimationParamsType:
			MainForm::getInstance()->animationParams();
			break;
		default:
			assert(0);
	}
	Session::getInstance()->unblockRecording();
}

void TabChangeCommand::reDo(){
	Session::getInstance()->blockRecording();
	switch (currentTab){
		case Params::UnknownParamsType:
			MainForm::getInstance()->viewpoint();
			break;
		case Params::ViewpointParamsType:
			MainForm::getInstance()->viewpoint();
			break;
		case Params::RegionParamsType:
			MainForm::getInstance()->region();
			break;
		
		case Params::FlowParamsType:
			MainForm::getInstance()->launchFlowTab();
			break;
		case Params::DvrParamsType:
			MainForm::getInstance()->renderDVR();
			break;
		case Params::IsoParamsType:
			MainForm::getInstance()->launchIsoTab();
			break;
		case Params::ProbeParamsType:
			MainForm::getInstance()->launchProbeTab();
			break;
		case Params::TwoDDataParamsType:
			MainForm::getInstance()->launchTwoDDataTab();
			break;
		case Params::TwoDImageParamsType:
			MainForm::getInstance()->launchTwoDImageTab();
			break;
		case Params::AnimationParamsType:
			MainForm::getInstance()->animationParams();
			break;
		default:
			assert(0);
	}
	Session::getInstance()->unblockRecording();
}

const char* TabChangeCommand::
tabName(Params::ParamType t){
	switch (t){
		case Params::UnknownParamsType:
			return " none ";
		case Params::ViewpointParamsType:
			return "viewpoint";
		case Params::RegionParamsType:
			return " region ";
		case Params::DvrParamsType:
			return " dvr ";
		case Params::IsoParamsType:
			return " Iso ";
		case Params::AnimationParamsType:
			return " animation ";
		case Params::FlowParamsType:
			return " flow ";
		case Params::ProbeParamsType:
			return " probe ";
		case Params::TwoDDataParamsType:
			return " 2D ";
		case Params::TwoDImageParamsType:
			return " Image ";
		default:  
			assert(0);
			return 0;
	}
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
captureStart(VizFeatureParams* p, char* description, int viznum){
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
	previousPrefs->applyToState();
	Session::getInstance()->unblockRecording();
}

void PreferencesCommand::reDo(){
	Session::getInstance()->blockRecording();
	currentPrefs->applyToState();
	Session::getInstance()->unblockRecording();
}

PreferencesCommand* PreferencesCommand::
captureStart(UserPreferences* p, char* description){
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






