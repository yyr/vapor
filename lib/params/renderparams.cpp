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
#include <vapor/DataMgr.h>
#include <vapor/RegularGrid.h>
#include <vapor/errorcodes.h>
#include "Box.h"
#include <iostream>
#include <sstream>
#include <string>


using namespace VAPoR;

#include "command.h"

const string RenderParams::_EnabledTag = "Enabled";

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
		
		int maxref = DataStatus::getInstance()->getNumTransforms();
		if (level < 0 || level > maxref) return -1;
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