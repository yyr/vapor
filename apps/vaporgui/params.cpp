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
#include <qwidget.h>
#include "mainform.h"
#include "vizwinmgr.h"
#include "panelcommand.h"
#include "session.h"
#include "assert.h"
#include "params.h"
	
using namespace VAPoR;
//Methods to find the "other" params in a local/global switch:
//
Params* Params::
getCorrespondingGlobalParams() {
	return mainWin->getVizWinMgr()->getGlobalParams(thisParamType);
}
Params* Params::
getCorrespondingLocalParams() {
	return mainWin->getVizWinMgr()->getLocalParams(thisParamType);
}
//To make a local->global change, must copy the local params.
//To make a global->local change, must copy global params
//
void Params::
guiSetLocal(bool lg){
	if (textChangedFlag) confirmText(false);
	if (lg) { //To convert global to local, "this" may be local or global
		PanelCommand* cmd;
		if(vizNum == -1){
			cmd = PanelCommand::captureStart(this, mainWin->getSession(), "set Global to Local");
		} else {
			cmd = PanelCommand::captureStart(this->getCorrespondingGlobalParams(), mainWin->getSession(), "set Global to Local");
		}
		//always set the local flag on "this"
		//
		setLocal(lg);
		PanelCommand::captureEnd(cmd, this->getCorrespondingLocalParams());
	} else { //To convert local to global, "this" is local:
		assert (vizNum >= 0);
		PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(), "set Local to Global");
		setLocal(lg);
		PanelCommand::captureEnd(cmd, this->getCorrespondingGlobalParams());
	} 
}
	
void Params::confirmText(bool render){
	if (!textChangedFlag) return;
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(), "edit text");
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

	
