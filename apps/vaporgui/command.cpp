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
//	Description:	Implements two subclasses of the Command class including 
//		MouseModeCommand and TabChangeCommand
//
#include "command.h"
#include "params.h"
#include "session.h"
#include "mainform.h"
#include "assert.h"
#include <qaction.h>
using namespace VAPoR;
//Constructor:  called when a new command is issued.
MouseModeCommand::MouseModeCommand(mouseModeType oldMode, mouseModeType newMode, Session* ses){
	//Make a copy of previous panel:
	previousMode = oldMode;
	currentMode = newMode;
	currentSession = ses;
	
	description = QString("change mode");
	description += modeName(oldMode);
	description += "to";
	description += modeName(newMode);
}

void MouseModeCommand::unDo(){
	currentSession->blockRecording();
	switch (previousMode){
		case navigateMode:
			currentSession->getMainWindow()->navigationAction->setOn(true);
			break;
		case regionMode:
			currentSession->getMainWindow()->regionSelectAction->setOn(true);
			break;
		case probeMode:
			currentSession->getMainWindow()->probeAction->setOn(true);
			break;
		case lightMode:
			currentSession->getMainWindow()->moveLightsAction->setOn(true);
			break;
		case contourMode:
			currentSession->getMainWindow()->contourAction->setOn(true);
			break;
		default:
			assert(0);
	}
	currentSession->unblockRecording();
}
void MouseModeCommand::reDo(){
	currentSession->blockRecording();
	switch (currentMode){
		case navigateMode:
			currentSession->getMainWindow()->navigationAction->setOn(true);
			break;
		case regionMode:
			currentSession->getMainWindow()->regionSelectAction->setOn(true);
			break;
		case probeMode:
			currentSession->getMainWindow()->probeAction->setOn(true);
			break;
		case lightMode:
			currentSession->getMainWindow()->moveLightsAction->setOn(true);
			break;
		case contourMode:
			currentSession->getMainWindow()->contourAction->setOn(true);
			break;
		default:
			assert(0);
	}
	currentSession->unblockRecording();
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

TabChangeCommand::TabChangeCommand(Params::ParamType oldTab, Params::ParamType newTab, Session* ses){
	//Make a copy of previous panel:
	previousTab = oldTab;
	currentTab = newTab;
	currentSession = ses;
	
	description = QString("change tab");
	description += tabName(oldTab);
	description += "to";
	description += tabName(newTab);
}

void TabChangeCommand::unDo(){
	currentSession->blockRecording();
	switch (previousTab){
		case Params::UnknownParamsType:
			currentSession->getMainWindow()->viewpoint();
			break;
		case Params::ViewpointParamsType:
			currentSession->getMainWindow()->viewpoint();
			break;
		case Params::RegionParamsType:
			currentSession->getMainWindow()->region();
			break;
		case Params::IsoParamsType:
			currentSession->getMainWindow()->calcIsosurface();
			break;
		case Params::DvrParamsType:
			currentSession->getMainWindow()->renderDVR();
			break;
		case Params::ContourParamsType:
			currentSession->getMainWindow()->contourPlanes();
			break;
		case Params::AnimationParamsType:
			currentSession->getMainWindow()->animationParams();
			break;
		default:
			assert(0);
	}
	currentSession->unblockRecording();
}

void TabChangeCommand::reDo(){
	currentSession->blockRecording();
	switch (currentTab){
		case Params::UnknownParamsType:
			currentSession->getMainWindow()->viewpoint();
			break;
		case Params::ViewpointParamsType:
			currentSession->getMainWindow()->viewpoint();
			break;
		case Params::RegionParamsType:
			currentSession->getMainWindow()->region();
			break;
		case Params::IsoParamsType:
			currentSession->getMainWindow()->calcIsosurface();
			break;
		case Params::DvrParamsType:
			currentSession->getMainWindow()->renderDVR();
			break;
		case Params::ContourParamsType:
			currentSession->getMainWindow()->contourPlanes();
			break;
		case Params::AnimationParamsType:
			currentSession->getMainWindow()->animationParams();
			break;
		default:
			assert(0);
	}
	currentSession->unblockRecording();
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





