
//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2006	                        *
//          University Corporation for Atmospheric Research             *
//                      All Rights Reserved                             *
//                                                                      *
//***********************************************************************
//
//	File:		ParamsIso.h
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Tue Dec 5 14:23:51 MST 2006
//
//	Description:	
//
//


#include "ParamsIso.h"

using namespace VetsUtil;
using namespace VAPoR;

const string ParamsIso::_IsoValueTag = "IsoValue";
const string ParamsIso::_NormalOnOffTag = "NormalOnOff";
const string ParamsIso::_ConstantColorTag = "ConstantColor";

namespace {
	const string IsoName = "IsosurfaceParams";
};

ParamsIso::ParamsIso(
	XmlNode *parent
) : RenderParams(parent, IsoName) {

	restart();
}


ParamsIso::~ParamsIso() {}


void ParamsIso::restart() {
	SetIsoValue(1.0);
	SetNormalOnOff(0);

	float default_color[4] = {1.0, 1.0, 1.0, 1.0};
	SetConstantColor(default_color);
}

void ParamsIso::SetIsoValue(double value) {
	vector <double> valvec(1, value);
	GetRootNode()->SetElementDouble(_IsoValueTag, valvec);
}

double ParamsIso::GetIsoValue() {
	vector <double> &valvec = GetRootNode()->GetElementDouble(_IsoValueTag);
	return(valvec[0]);
}

void ParamsIso::RegisterIsoValueDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlagDouble(_IsoValueTag, df);
}

void ParamsIso::SetNormalOnOff(bool flag) {
	vector <long> valvec(1, flag);
	GetRootNode()->SetElementLong(_NormalOnOffTag, valvec);
}

bool ParamsIso::GetNormalOnOff() {
	vector <long> &valvec = GetRootNode()->GetElementLong(_NormalOnOffTag);
	return((bool) valvec[0]);
}

void ParamsIso::RegisterNormalOnOffDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlagLong(_NormalOnOffTag, df);
}


void ParamsIso::SetConstantColor(float rgba[4]) {
	vector <double> valvec(4,0);
	for (int i=0; i<4; i++) {
		valvec[i] = rgba[i];
	}
	GetRootNode()->SetElementDouble(_ConstantColorTag, valvec);
}

const float *ParamsIso::GetConstantColor() {
	vector <double> &valvec = GetRootNode()->GetElementDouble(_ConstantColorTag);

	for (int i=0; i<4; i++) _constcolorbuf[i] = valvec[i];
	return(_constcolorbuf);
}

void ParamsIso::RegisterConstantColorDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlagDouble(_ConstantColorTag, df);
}



