//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		renderparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2014
//
//	Description:	Implements the RenderParams class.
//		This is an abstract class for all the tabbed panel rendering params classes.
//		Supports functionality common to all the tabbed panel render params.
//
#include "glutil.h"	// Must be included first!!!
#include <vapor/XmlNode.h>
#include <cstring>
#include <string>
#include <vapor/MyBase.h>
#include "assert.h"
#include "params.h"
#include "datastatus.h"
#include "vizwinparams.h"
#include "renderparams.h"
#include <vapor/ParamNode.h>
#include <vapor/DataMgrV3_0.h>
#include <vapor/RegularGrid.h>
#include <vapor/errorcodes.h>
#include "Box.h"
#include <iostream>
#include <sstream>
#include <string>


using namespace VAPoR;

#include "command.h"

const string RenderParams::_EnabledTag = "Enabled";
const string RenderParams::_FidelityLevelTag = "FidelityLevel";
const string RenderParams::_IgnoreFidelityTag = "IgnoreFidelity";
const string RenderParams::_histoScaleTag = "HistoScale";
const string RenderParams::_editBoundsTag = "EditBounds";
const string RenderParams::_histoBoundsTag = "HistoBounds";
const string RenderParams::_RendererNameTag = "RendererName";

RenderParams::RenderParams(XmlNode *parent, const string &name, int winnum):Params(parent, name, winnum){
	SetLocal(true);
}


//Following methods adapted from ParamsBase.cpp
void RenderParams::initializeBypassFlags(){
	bypassFlags.clear();
	int numTimesteps = DataStatus::getInstance()->getNumTimesteps();
	bypassFlags.resize(numTimesteps, 0);
}
void RenderParams::setAllBypass(bool val){ 
	//set all bypass flags to either 0 or 2
	//when set to 0, indicates "never bypass"
	//when set to 2, indicates "always bypass"
	int ival = val ? 2 : 0;
	for (int i = 0; i<bypassFlags.size(); i++)
		bypassFlags[i] = ival;
}

int RenderParams::GetCompressionLevel(){
	const vector<long> defaultLevel(1,2);
	return GetValueLong(_CompressionLevelTag,defaultLevel);
 }
int RenderParams::SetCompressionLevel(int level){
	 vector<long> valvec(1,(long)level);
	 int rc = SetValueLong(_CompressionLevelTag,"Set compression level",valvec);
	 setAllBypass(false);
	 return rc;
}
int RenderParams::SetRefinementLevel(int level){
		
	if (level < 0 ) return -1;
	SetValueLong(_RefinementLevelTag, "Set refinement level",level);
	setAllBypass(false);
	return 0;
}
int RenderParams::GetRefinementLevel(){
		const vector<long>defaultRefinement(1,0);
		return (GetValueLong(_RefinementLevelTag,defaultRefinement));
}


void RenderParams::SetEnabled(bool val){
	long lval = (long)val;
	Command::blockCapture();
	SetValueLong(_EnabledTag,"enable/disable renderer",lval);
	Command::unblockCapture();
}
int RenderParams::GetFidelityLevel(){
	return GetValueLong(_FidelityLevelTag);
 }
void RenderParams::SetFidelityLevel(int level){
	 SetValueLong(_FidelityLevelTag,"Set Fidelity", level);
 }
bool RenderParams::GetIgnoreFidelity(){
	return (bool)GetValueLong(_IgnoreFidelityTag);
 }
void RenderParams::SetIgnoreFidelity(bool val){
	 SetValueLong(_IgnoreFidelityTag, "change fidelity usage", val);
 }
const string RenderParams::GetVariableName(){
	 return GetValueString(_VariableNameTag);
}

void RenderParams::SetVariableName(const string& varname){
	SetValueString(_VariableNameTag, "Specify variable name", varname);
}
const string RenderParams::GetRendererName(){
	 return GetValueString(_RendererNameTag);
}

void RenderParams::SetRendererName(const string& renname){
	SetValueString(_RendererNameTag, "Specify Renderer name", renname);
}
