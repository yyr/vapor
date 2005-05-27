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
#include "assert.h"
#include <qaction.h>
using namespace VAPoR;
//Constructor:  called when a new command is issued.
MouseModeCommand::MouseModeCommand(mouseModeType oldMode, mouseModeType newMode){
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
		case navigateMode:
			MainForm::getInstance()->navigationAction->setOn(true);
			break;
		case regionMode:
			MainForm::getInstance()->regionSelectAction->setOn(true);
			break;
		case probeMode:
			MainForm::getInstance()->probeAction->setOn(true);
			break;
		case lightMode:
			MainForm::getInstance()->moveLightsAction->setOn(true);
			break;
		case contourMode:
			MainForm::getInstance()->contourAction->setOn(true);
			break;
		default:
			assert(0);
	}
	Session::getInstance()->unblockRecording();
}
void MouseModeCommand::reDo(){
	Session::getInstance()->blockRecording();
	switch (currentMode){
		case navigateMode:
			MainForm::getInstance()->navigationAction->setOn(true);
			break;
		case regionMode:
			MainForm::getInstance()->regionSelectAction->setOn(true);
			break;
		case probeMode:
			MainForm::getInstance()->probeAction->setOn(true);
			break;
		case lightMode:
			MainForm::getInstance()->moveLightsAction->setOn(true);
			break;
		case contourMode:
			MainForm::getInstance()->contourAction->setOn(true);
			break;
		default:
			assert(0);
	}
	Session::getInstance()->unblockRecording();
}

const char* MouseModeCommand::
modeName(mouseModeType t){
	switch (t){
		case navigateMode:
			return " navigate ";
		case regionMode:
			return " region-set ";
		case probeMode:
			return " probe-set ";
		case contourMode:
			return " contour-set ";
		case lightMode:
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
		case Params::IsoParamsType:
			MainForm::getInstance()->calcIsosurface();
			break;
		case Params::DvrParamsType:
			MainForm::getInstance()->renderDVR();
			break;
		case Params::ContourParamsType:
			MainForm::getInstance()->contourPlanes();
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
		case Params::IsoParamsType:
			MainForm::getInstance()->calcIsosurface();
			break;
		case Params::DvrParamsType:
			MainForm::getInstance()->renderDVR();
			break;
		case Params::ContourParamsType:
			MainForm::getInstance()->contourPlanes();
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
			return " viewpoint ";
		case Params::RegionParamsType:
			return " region ";
		case Params::IsoParamsType:
			return " isosurface ";
		case Params::DvrParamsType:
			return " dvr ";
		case Params::ContourParamsType:
			return " contours ";
		case Params::AnimationParamsType:
			return " animation ";
		default:  
			assert(0);
			return 0;
	}
}


ColorChangeCommand::ColorChangeCommand(QColor& oldColor, QColor& newColor, int vizNum){
	//Make a copy of previous panel:
	previousColor = oldColor;
	currentColor = newColor;
	windowNum = vizNum;
	QString* vizName;
	vizName = VizWinMgr::getInstance()->getVizWinName(vizNum);
	description = QString("change background color in " + (*vizName));
	
}

void ColorChangeCommand::unDo(){
	Session::getInstance()->blockRecording();
	VizWin* win = VizWinMgr::getInstance()->getVizWin(windowNum);
	win->setGLBackgroundColor(previousColor);
	Session::getInstance()->unblockRecording();
	win->updateGL();
}

void ColorChangeCommand::reDo(){
	Session::getInstance()->blockRecording();
	VizWin* win = VizWinMgr::getInstance()->getVizWin(windowNum);
	win->setGLBackgroundColor(currentColor);
	Session::getInstance()->unblockRecording();
	win->updateGL();
}









