
//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2006							    *
//          University Corporation for Atmospheric Research             *
//                      All Rights Reserved                             *
//                                                                      *
//***********************************************************************
//
//	File:		ParamsIso.cpp
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
#include "mapperfunction.h"

using namespace VetsUtil;
using namespace VAPoR;

const string ParamsIso::_IsoValueTag = "IsoValue";
const string ParamsIso::_NormalOnOffTag = "NormalOnOff";
const string ParamsIso::_ConstantColorTag = "ConstantColor";
const string ParamsIso::_HistoBoundsTag = "HistoBounds";
const string ParamsIso::_HistoScaleTag = "HistoScale";
const string ParamsIso::_SelectedPointTag = "SelectedPoint";
const string ParamsIso::_RefinementLevelTag = "RefinementLevel";
const string ParamsIso::_VisualizerNumTag = "VisualizerNum";
const string ParamsIso::_VariableNameTag = "VariableName";

namespace {
	const string IsoName = "IsosurfaceParams";
};

ParamsIso::ParamsIso(
	XmlNode *parent, int winnum
) : RenderParams(parent, Params::_isoParamsTag, winnum) {
	thisParamType = IsoParamsType;
	dummyMapperFunc = new MapperFunction(this, 8);
	minOpacEditBounds = new float[1];
	maxOpacEditBounds = new float[1];
	//Need to do this better:
	OpacityMap* foo = dummyMapperFunc->getOpacityMap(0);
    dummyMapperFunc->deleteOpacityMap(foo);
	
	restart();
}


ParamsIso::~ParamsIso() {}

XmlNode* ParamsIso::buildNode(){
	return GetRootNode();
}
//Initialize for new metadata.  Keep old transfer functions
//
bool ParamsIso::
reinit(bool doOverride){

	DataStatus* ds = DataStatus::getInstance();
	
	int totNumVariables = ds->getNumMetadataVariables();
	if (totNumVariables <= 0) return false;
	int varNum = ds->getMetadataVarNum(GetVariableName());
	//See if current variable name is valid.  It needs to be in the metadata.
	//if not, reset to first variable that is present:
	if (varNum < 0) 
	{
		SetVariableName(ds->getMetadataVarName(0));
		varNum = 0;
	}
	
	//Set up the numRefinements. 
	int maxNumRefinements = ds->getNumTransforms();
	int numrefs = GetRefinementLevel();
	if (doOverride) { 
		numrefs = 0;
	} else {  //Try to use existing value
		if (numrefs > maxNumRefinements) numrefs = maxNumRefinements;
		if (numrefs < 0) numrefs = 0;
	}
	SetRefinementLevel(numrefs);
	
	//Make sure histo bounds and iso Value are valid
	const float* curBounds = GetHistoBounds();
	float bnds[2];
	bnds[0] = curBounds[0]; bnds[1] = curBounds[1];
	

	float isoval = GetIsoValue();
	float dataMin = DataStatus::getInstance()->getDefaultDataMin(varNum);
	float dataMax = DataStatus::getInstance()->getDefaultDataMax(varNum);
	if (doOverride) {
		bnds[0] = dataMin;
		bnds[1] = dataMax;
		isoval = 0.5f*(bnds[0]+bnds[1]);
		SetHistoBounds(bnds);
		SetIsoValue(isoval);
	} else {
		//only modify the existing bounds and isovalue if they 
		//lie outside the data bounds:
		float newVal = isoval;
		if (newVal < dataMin) newVal = dataMin;
		if (newVal > dataMax) newVal = dataMax;
		if (newVal != isoval) SetIsoValue(newVal);
		if (bnds[0] < dataMin) bnds[0] = dataMin;
		if (bnds[1] > dataMax) bnds[1] = dataMax;
		if (bnds[0] > bnds[1]){
			bnds[0] = dataMin; bnds[1] = dataMax;
		}
		if (bnds[0] != curBounds[0] || bnds[1] != curBounds[1])
			SetHistoBounds(bnds);

	}
	minOpacEditBounds[0] = bnds[0];
	maxOpacEditBounds[0] = bnds[1];
	dummyMapperFunc->setMinOpacMapValue(bnds[0]);
	dummyMapperFunc->setMaxOpacMapValue(bnds[1]);
	if (doOverride) {
		SetHistoStretch(1.0);
		float col[4] = {1.f, 1.f, 1.f, 1.f};
		SetConstantColor(col);
	}
	return true;
}

void ParamsIso::restart() {
	SetIsoValue(1.0);
	SetNormalOnOff(0);

	float bnds[2] = {0.0, 1.0};
	SetHistoBounds(bnds);

	minOpacEditBounds[0] = 0.f;
	maxOpacEditBounds[0] = 1.f;

	SetHistoStretch(1.0);

	float pnt[3] = {0.0, 0.0, 0.0};
	SetSelectedPoint(pnt);

	SetRefinementLevel(0);

	SetVisualizerNum(vizNum);

	SetVariableName("N/A");

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


 void ParamsIso::SetHistoBounds(float bnds[2]){
	vector <double> valvec(2);
	valvec[0] = bnds[0]; valvec[1]= bnds[1];
	GetRootNode()->SetElementDouble(_HistoBoundsTag, valvec);
 }
 const float* ParamsIso::GetHistoBounds(){
	const vector <double> valvec = GetRootNode()->GetElementDouble(_HistoBoundsTag);
	_histoBounds[0]=(float)valvec[0];_histoBounds[1]=(float)valvec[1];
	return( _histoBounds);
 }
	
 void ParamsIso::SetHistoStretch(float scale){
	vector <double> valvec(1, (double)scale);
	GetRootNode()->SetElementDouble(_HistoScaleTag, valvec);
 }

 float ParamsIso::GetHistoStretch(){
	vector<double>& valvec = GetRootNode()->GetElementDouble(_HistoScaleTag);
	return (float)valvec[0];
 }

 void ParamsIso::SetSelectedPoint(const float pnt[3]){
	vector <double> valvec(3);
	valvec[0] = pnt[0]; valvec[1]= pnt[1]; valvec[2]= pnt[2];
	GetRootNode()->SetElementDouble(_SelectedPointTag, valvec);
 }
 const vector<double>& ParamsIso::GetSelectedPoint(){
	 return GetRootNode()->GetElementDouble(_SelectedPointTag);
 }

 void ParamsIso::SetRefinementLevel(int level){
	 vector<long> valvec(1,(long)level);
	 GetRootNode()->SetElementLong(_RefinementLevelTag,valvec);
 }
 int ParamsIso::GetRefinementLevel(){
	vector<long>& valvec = GetRootNode()->GetElementLong(_RefinementLevelTag);
	return (int)valvec[0];
 }
 void ParamsIso::RegisterRefinementDirtyFlag(ParamNode::DirtyFlag *df){
	GetRootNode()->RegisterDirtyFlagLong(_RefinementLevelTag, df);
}

 void ParamsIso::SetVisualizerNum(int viznum){
	vector<long> valvec(1,(long)viznum);
	GetRootNode()->SetElementLong(_VisualizerNumTag,valvec);
	vizNum = viznum;
 }
 int ParamsIso::GetVisualizerNum(){
	vector<long>& valvec = GetRootNode()->GetElementLong(_RefinementLevelTag);
	return (int)valvec[0];
 }

 void ParamsIso::SetVariableName(const string& varName){
	 GetRootNode()->SetElementString(_VariableNameTag, varName);
 }
 const string& ParamsIso::GetVariableName(){
	 return GetRootNode()->GetElementString(_VariableNameTag);
 }
 void ParamsIso::RegisterVariableDirtyFlag(ParamNode::DirtyFlag *df){
	GetRootNode()->RegisterDirtyFlagLong(_VariableNameTag, df);
}
 
RenderParams* ParamsIso::deepRCopy(){
	ParamsIso* newParams = new ParamsIso(*this);
	// Need to clone the xmlnode; 
	if (_rootParamNode) newParams->_rootParamNode = new ParamNode(*_rootParamNode);
	//Probably these are always the same when we clone...
	assert(_rootParamNode == _currentParamNode);
	if (_rootParamNode == _currentParamNode)
		newParams->_currentParamNode = newParams->_rootParamNode;
	else newParams->_currentParamNode = new ParamNode(*_currentParamNode);
	newParams->dummyMapperFunc = new MapperFunction(*dummyMapperFunc);
	newParams->dummyMapperFunc->setParams(newParams);
	newParams->minOpacEditBounds = new float[1];
	newParams->maxOpacEditBounds = new float[1];
	newParams->minOpacEditBounds[0] = minOpacEditBounds[0];
	newParams->maxOpacEditBounds[0] = maxOpacEditBounds[0];
	return (RenderParams*)newParams;
}
//Opacity map bounds are being used for histo bounds:
void ParamsIso::setMinOpacMapBound(float minhisto){
	const float* bnds = GetHistoBounds();
	float nbds[2];
	nbds[0] = minhisto;
	nbds[1] = bnds[1];
	SetHistoBounds(nbds);
	dummyMapperFunc->setMinOpacMapValue(minhisto);
	return;
}
void ParamsIso::setMaxOpacMapBound(float maxhisto){
	const float* bnds = GetHistoBounds();
	float nbds[2];
	nbds[1] = maxhisto;
	nbds[0] = bnds[0];
	SetHistoBounds(nbds);
	dummyMapperFunc->setMaxOpacMapValue(maxhisto);
	return;
}
void ParamsIso::updateHistoBounds(){
	float newBnds[2];
	newBnds[0] = dummyMapperFunc->getMinOpacMapValue();
	newBnds[1] = dummyMapperFunc->getMaxOpacMapValue();
	SetHistoBounds(newBnds);
}

