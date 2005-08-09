//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		params.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the Params class.
//		This is an abstract class for all the tabbed panel params classes.
//		Supports functionality common to all the tabbed panel params.
//
#include <cstring>
#include <qwidget.h>
#include "mainform.h"
#include "vizwinmgr.h"
#include "panelcommand.h"
#include "session.h"
#include "assert.h"
#include "params.h"


using namespace VAPoR;

const string Params::_dvrParamsTag = "DvrPanelParameters";
const string Params::_regionParamsTag = "RegionPanelParameters";
const string Params::_animationParamsTag = "AnimationPanelParameters";
const string Params::_viewpointParamsTag = "ViewpointPanelParameters";
const string Params::_flowParamsTag = "FlowPanelParameters";
const string Params::_localAttr = "Local";
const string Params::_vizNumAttr = "VisualizerNum";
const string Params::_numVariablesAttr = "NumVariables";
//Methods to find the "other" params in a local/global switch:
//
Params* Params::
getCorrespondingGlobalParams() {
	return VizWinMgr::getInstance()->getGlobalParams(thisParamType);
}
Params* Params::
getCorrespondingLocalParams() {
	return VizWinMgr::getInstance()->getLocalParams(thisParamType);
}
//To make a local->global change, must copy the local params.
//To make a global->local change, must copy global params
//
void Params::
guiSetLocal(bool lg){
	if (textChangedFlag) confirmText(false);
	/*
	if (lg) { //To convert global to local, "this" may be local or global
		PanelCommand* cmd;
		if(vizNum == -1){
			cmd = PanelCommand::captureStart(this,  "set Global to Local");
		} else {
			cmd = PanelCommand::captureStart(this->getCorrespondingGlobalParams(),  "set Global to Local");
		}
		//always set the local flag on "this"
		//
		setLocal(lg);
		PanelCommand::captureEnd(cmd, this->getCorrespondingLocalParams());
	} else { //To convert local to global, "this" is local:
		assert (vizNum >= 0);
		PanelCommand* cmd = PanelCommand::captureStart(this,  "set Local to Global");
		setLocal(lg);
		PanelCommand::captureEnd(cmd, this->getCorrespondingGlobalParams());
	} 
	*/
	PanelCommand* cmd;
	Params* localParams = getCorrespondingLocalParams();
	if (lg){
		cmd = PanelCommand::captureStart(localParams,  "set Global to Local");
	}
	else cmd = PanelCommand::captureStart(localParams,  "set Local to Global");
	localParams->setLocal(lg);
	PanelCommand::captureEnd(cmd, localParams);
}
	
void Params::confirmText(bool render){
	if (!textChangedFlag) return;
	PanelCommand* cmd = PanelCommand::captureStart(this, "edit text");
	updatePanelState();
	PanelCommand::captureEnd(cmd, this);
	//since only the text has changed, we know that the enabled and local settings
	//are unchanged:
	//
	if (render) updateRenderer(enabled, local, false);
}
QString& Params::paramName(Params::ParamType type){
	switch (type){
		
		case(ViewpointParamsType):
			return *(new QString("Viewpoint"));
		case(RegionParamsType):
			return *(new QString("Region"));
		case(IsoParamsType):
			return *(new QString("Isosurface"));
		case(FlowParamsType):
			return *(new QString("Flow"));
		case(DvrParamsType):
			return *(new QString("DVR"));
		case(ContourParamsType):
			return *(new QString("Contours"));
		case(AnimationParamsType):
			return *(new QString("Animation"));
		case (UnknownParamsType):
		default:
			return *(new QString("Unknown"));
	}
	
}
