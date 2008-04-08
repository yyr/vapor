
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
#include "transferfunction.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace VetsUtil;
using namespace VAPoR;

const string ParamsIso::_IsoValueTag = "IsoValue";
const string ParamsIso::_IsoControlTag = "IsoControl";
const string ParamsIso::_ColorMapTag = "ColorMap";
const string ParamsIso::_NormalOnOffTag = "NormalOnOff";
const string ParamsIso::_ConstantColorTag = "ConstantColor";
const string ParamsIso::_HistoBoundsTag = "HistoBounds";
const string ParamsIso::_HistoScaleTag = "HistoScale";
const string ParamsIso::_MapHistoScaleTag = "MapHistoScale";
const string ParamsIso::_SelectedPointTag = "SelectedPoint";
const string ParamsIso::_RefinementLevelTag = "RefinementLevel";
const string ParamsIso::_VisualizerNumTag = "VisualizerNum";
const string ParamsIso::_VariableNameTag = "VariableName";
const string ParamsIso::_MapVariableNameTag = "MapVariableName";
const string ParamsIso::_NumBitsTag = "NumVoxelBits";

namespace {
	const string IsoName = "IsosurfaceParams";
};

ParamsIso::ParamsIso(
	XmlNode *parent, int winnum
) : RenderParams(parent, Params::_isoParamsTag, winnum) {
	thisParamType = IsoParamsType;
	
	minIsoEditBounds = 0;
	maxIsoEditBounds = 0;
	restart();
}


ParamsIso::~ParamsIso() {
	for (int i = 0; i<transFunc.size(); i++){
		delete transFunc[i];
	}
	for (int i = 0; i<isoControls.size(); i++){
		delete isoControls[i];
	}
}
//clone the xml node, (since buildNode() will eventually
//destroy the node it uses).  Then add isoControl and TF nodes
XmlNode* ParamsIso::buildNode(){
	XmlNode* clonedNode = new XmlNode(*GetRootNode());
	
	// For each session variable, create a variable node with appropriate attributes:
	for (int i = 0; i<numSessionVariables; i++){
		string varName = DataStatus::getInstance()->getVariableName(i);
		//Create a "Variable" node
		ostringstream oss;
		string empty;
		map <string, string> varAttrs;
		varAttrs[_variableNameAttr] = varName;
		
		oss.str(empty);
		oss << (double)(transFunc[i]->getOpacityScaleFactor());
		varAttrs[_opacityScaleAttr] = oss.str();

		XmlNode *variableNode = new XmlNode(_variableTag, varAttrs);
		variableNode->AddChild(transFunc[i]->buildNode(""));
		variableNode->AddChild(isoControls[i]->buildNode(""));
		//Use XMLNode version of AddChild to add multiple children with same tag
		clonedNode->XmlNode::AddChild(variableNode);
	}
	return clonedNode;
}

//Initialize for new metadata.  Keep old transfer functions
//
// For each new variable in the metadata create a variable child node, and build the
// associated isoControl and transfer function nodes.
bool ParamsIso::
reinit(bool doOverride){

	DataStatus* ds = DataStatus::getInstance();
	
	int totNumVariables = ds->getNumSessionVariables();
	if (totNumVariables <= 0) return false;
	int varNum = ds->getMetadataVarNum(GetVariableName());
	int mapVarNum = ds->getMetadataVarNum(GetMapVariableName());
	//See if current variable name is valid.  It needs to be in the metadata.
	//if not, reset to first variable that is present in metadata:
	if (varNum < 0) 
	{
		SetVariableName(ds->getMetadataVarName(0));
		varNum = 0;
	}
	//use constant if variable is not available
	if (mapVarNum < 0) 
	{
		SetMapVariableName("Constant");
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
	//Create arrays of IsoControls and TransferFunctions
	assert(totNumVariables > 0);
	TransferFunction** newTransFunc = new TransferFunction*[totNumVariables];
	float* newMinMapEdit = new float[totNumVariables];
	float* newMaxMapEdit = new float[totNumVariables];
	float* newMinIsoEdit = new float[totNumVariables];
	float* newMaxIsoEdit = new float[totNumVariables];
	IsoControl** newIsoControls = new IsoControl*[totNumVariables];

	//If we are overriding previous values, delete the transfer functions, create new ones.
	//Set the map bounds to the actual bounds in the data
	if (doOverride){
		for (int i = 0; i<totNumVariables; i++){
			//will need to set the iso value:
			float dataMin = DataStatus::getInstance()->getDefaultDataMin(i);
			float dataMax = DataStatus::getInstance()->getDefaultDataMax(i);
			newTransFunc[i] = new TransferFunction(this, 8);
			newIsoControls[i] = new IsoControl(this, 8);
			newTransFunc[i]->setMinMapValue(DataStatus::getInstance()->getDefaultDataMin(i));
			newTransFunc[i]->setMaxMapValue(DataStatus::getInstance()->getDefaultDataMax(i));
			newMinIsoEdit[i] = DataStatus::getInstance()->getDefaultDataMin(i);
			newMaxIsoEdit[i] = DataStatus::getInstance()->getDefaultDataMax(i);
			newMinMapEdit[i] = DataStatus::getInstance()->getDefaultDataMin(i);
			newMaxMapEdit[i] = DataStatus::getInstance()->getDefaultDataMax(i);
			newIsoControls[i]->setMinHistoValue(DataStatus::getInstance()->getDefaultDataMin(i));
            newIsoControls[i]->setMaxHistoValue(DataStatus::getInstance()->getDefaultDataMax(i));
			newTransFunc[i]->setVarNum(i);
			newIsoControls[i]->setVarNum(i);
			newIsoControls[i]->setIsoValue(0.5*(dataMin+dataMax));
		}
	} else {
		//attempt to make use of existing transfer functions
		//delete any that are no longer referenced
		//Copy tf bounds to edit bounds
		for (int i = 0; i<totNumVariables; i++){
			if(i<numSessionVariables){ //make copy of existing ones
				newTransFunc[i] = transFunc[i];
				newIsoControls[i] = isoControls[i];
				newMinMapEdit[i] = transFunc[i]->getMinMapValue();
				newMaxMapEdit[i] = transFunc[i]->getMaxMapValue();
				newMinIsoEdit[i] = isoControls[i]->getMinHistoValue();
				newMaxIsoEdit[i] = isoControls[i]->getMaxHistoValue();
			} else { //create new tfs, isocontrols
				
				newIsoControls[i] = new IsoControl(this, 8);
				newIsoControls[i]->setMinHistoValue(DataStatus::getInstance()->getDefaultDataMin(i));
				newIsoControls[i]->setMaxHistoValue(DataStatus::getInstance()->getDefaultDataMax(i));
				newIsoControls[i]->setVarNum(i);
				newTransFunc[i] = new TransferFunction(this, 8);
				newTransFunc[i]->setMinMapValue(DataStatus::getInstance()->getDefaultDataMin(i));
				newTransFunc[i]->setMaxMapValue(DataStatus::getInstance()->getDefaultDataMax(i));
				newMinMapEdit[i] = DataStatus::getInstance()->getDefaultDataMin(i);
				newMaxMapEdit[i] = DataStatus::getInstance()->getDefaultDataMax(i);
				newMinIsoEdit[i] = DataStatus::getInstance()->getDefaultDataMin(i);
				newMaxIsoEdit[i] = DataStatus::getInstance()->getDefaultDataMax(i);
				newTransFunc[i]->setVarNum(i);
			}
			
		}
			//Delete trans funcs that are not in the current session
		for (int i = totNumVariables; i<numSessionVariables; i++){
			delete transFunc[i];
			delete isoControls[i];
		}
	} //end if(doOverride)
	//Now put the new transfer functions and isocontrols into
	//the appropriate arrays:
	transFunc.clear();
	isoControls.clear();
	for (int i = 0; i<totNumVariables; i++){
		transFunc.push_back(newTransFunc[i]);
		isoControls.push_back(newIsoControls[i]);
	}
	delete newTransFunc;
	delete newIsoControls;
	if(minColorEditBounds) delete minColorEditBounds;
	if(maxColorEditBounds) delete maxColorEditBounds;
	if(minOpacEditBounds) delete minOpacEditBounds;
	if(maxOpacEditBounds) delete maxOpacEditBounds;
	if(minIsoEditBounds) delete minIsoEditBounds;
	if(maxIsoEditBounds) delete maxIsoEditBounds;
	minColorEditBounds = newMinMapEdit;
	maxColorEditBounds = newMaxMapEdit;
	minIsoEditBounds = newMinIsoEdit;
	maxIsoEditBounds = newMaxIsoEdit;
	//And clone the color edit bounds to use as opac edit bounds:
	minOpacEditBounds = new float[totNumVariables];
	maxOpacEditBounds = new float[totNumVariables];
	for (int i = 0; i<totNumVariables; i++){
		minOpacEditBounds[i] = minColorEditBounds[i];
		maxOpacEditBounds[i] = maxColorEditBounds[i];
	}
	numSessionVariables = totNumVariables;
	if (doOverride) {
		SetHistoStretch(1.0);
		SetIsoHistoStretch(1.0);
		float col[4] = {1.f, 1.f, 1.f, 1.f};
		SetConstantColor(col);
	}
	
	return true;
}

void ParamsIso::restart() {
	
	
	for (int i = 0; i< numSessionVariables; i++){
		delete transFunc[i];
		delete isoControls[i];
	}
	transFunc.clear();
	isoControls.clear();
	numSessionVariables = 0;
	
	SetNormalOnOff(1);
	SetNumBits(8);

	SetHistoStretch(1.0);
	SetIsoHistoStretch(1.0);

	float pnt[3] = {0.0, 0.0, 0.0};
	SetSelectedPoint(pnt);

	SetRefinementLevel(0);

	SetVisualizerNum(vizNum);

	SetVariableName("N/A");

	SetMapVariableName("Constant");


	//Initialize the mapping bounds to [0,1] until data is read
	
	if (minColorEditBounds) delete minColorEditBounds;
	if (maxColorEditBounds) delete maxColorEditBounds;
	if (minOpacEditBounds) delete minOpacEditBounds;
	if (maxOpacEditBounds) delete maxOpacEditBounds;
	if (minIsoEditBounds) delete minIsoEditBounds;
	if (maxIsoEditBounds) delete maxIsoEditBounds;
	
	
	minColorEditBounds = new float[1];
	maxColorEditBounds = new float[1];
	minColorEditBounds[0] = 0.f;
	maxColorEditBounds[0] = 1.f;
	minOpacEditBounds = new float[1];
	maxOpacEditBounds = new float[1];
	minOpacEditBounds[0] = 0.f;
	maxOpacEditBounds[0] = 1.f;
	minIsoEditBounds = new float[1];
	maxIsoEditBounds = new float[1];
	minIsoEditBounds[0] = 0.f;
	maxIsoEditBounds[0] = 1.f;
	
	mapEditMode = true;   //default is edit mode
	isoEditMode = true;
	
	setEnabled(false);
	float default_color[4] = {1.0, 1.0, 1.0, 1.0};
	SetConstantColor(default_color);
	
}
//Hook up the new transfer function in specified slot,
//Delete the old one.  This is called whenever a new tf is loaded.
//
void ParamsIso::
hookupTF(TransferFunction* tf, int index){

	//Create a new TFEditor
	if (transFunc[index]) delete transFunc[index];
	transFunc[index] = tf;

	minColorEditBounds[index] = tf->getMinMapValue();
	maxColorEditBounds[index] = tf->getMaxMapValue();
	tf->setParams(this);
	int varnum = DataStatus::getInstance()->getSessionVariableNum(GetMapVariableName());
	tf->setColorVarNum(varnum);
	tf->setOpacVarNum(varnum);
}
MapperFunction* ParamsIso::getMapperFunc() {
	return ((GetMapVariableNum()>=0) ? transFunc[GetMapVariableNum()] : 0);
}
IsoControl* ParamsIso::getIsoControl(){
	return ((numSessionVariables > 0 && isoControls.size() > 0) ? isoControls[GetIsoVariableNum()] : 0);
}

void ParamsIso::setMinColorMapBound(float val){
	getMapperFunc()->setMinColorMapValue(val);
	_rootParamNode->SetNodeDirty(_IsoControlTag);
}
void ParamsIso::setMaxColorMapBound(float val){
	getMapperFunc()->setMaxColorMapValue(val);
	_rootParamNode->SetNodeDirty(_IsoControlTag);
}


void ParamsIso::setMinOpacMapBound(float val){
	getMapperFunc()->setMinOpacMapValue(val);
	_rootParamNode->SetNodeDirty(_IsoControlTag);
}
void ParamsIso::setMaxOpacMapBound(float val){
	getMapperFunc()->setMaxOpacMapValue(val);
	_rootParamNode->SetNodeDirty(_ColorMapTag);
}


void ParamsIso::SetIsoValue(double value) {
	getIsoControl()->setIsoValue(value);
	_rootParamNode->SetNodeDirty(_IsoControlTag);
}

double ParamsIso::GetIsoValue() {
	return (isoControls.size()>0) ? getIsoControl()->getIsoValue() : 0.f;
}

void ParamsIso::RegisterIsoControlDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlagNode(_IsoControlTag, df);
}

void ParamsIso::RegisterColorMapDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlagNode(_ColorMapTag, df);
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
	IsoControl* isoContr = getIsoControl();
	isoContr->setMinHistoValue(bnds[0]);
	isoContr->setMaxHistoValue(bnds[1]);
	_rootParamNode->SetNodeDirty(_IsoControlTag);
 }
 const float* ParamsIso::GetHistoBounds(){
	 if (!getIsoControl()){
		 _histoBounds[0] = 0.f;
		 _histoBounds[1] = 1.f;
	 } else {
		_histoBounds[0]=getIsoControl()->getMinHistoValue();
		_histoBounds[1]=getIsoControl()->getMaxHistoValue();
	 }
	return( _histoBounds);
 }
 //Use _histoBounds to return tf bounds 
 const float* ParamsIso::GetMapBounds(){
	 if (!getMapperFunc() || GetMapVariableNum() < 0){
		 _histoBounds[0] = 0.f;
		 _histoBounds[1] = 1.f;
	 } else {
		_histoBounds[0]=getMapperFunc()->getMinOpacMapValue();
		_histoBounds[1]=getMapperFunc()->getMaxOpacMapValue();
	 }
	return( _histoBounds);
 }
	
 void ParamsIso::SetIsoHistoStretch(float scale){
	vector <double> valvec(1, (double)scale);
	GetRootNode()->SetElementDouble(_HistoScaleTag, valvec);
 }

 float ParamsIso::GetIsoHistoStretch(){
	vector<double>& valvec = GetRootNode()->GetElementDouble(_HistoScaleTag);
	return (float)valvec[0];
 }
 void ParamsIso::SetHistoStretch(float scale){
	vector <double> valvec(1, (double)scale);
	GetRootNode()->SetElementDouble(_MapHistoScaleTag, valvec);
 }

 float ParamsIso::GetHistoStretch(){
	vector<double>& valvec = GetRootNode()->GetElementDouble(_MapHistoScaleTag);
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
 int ParamsIso::GetNumBits(){
	vector<long>& valvec = GetRootNode()->GetElementLong(_NumBitsTag);
	if (valvec.size() == 0){
		//For backwards compatibility, insert default value:
		SetNumBits(8);
		return 8;
	}
	return (int)valvec[0];
 }
 void ParamsIso::SetNumBits(int numbits){
	vector<long> valvec(1,(long)numbits);
	GetRootNode()->SetElementLong(_NumBitsTag,valvec);
 }
void ParamsIso::RegisterNumBitsDirtyFlag(ParamNode::DirtyFlag *df){
	GetRootNode()->RegisterDirtyFlagLong(_NumBitsTag, df);
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
void ParamsIso::SetMapVariableName(const string& varName){
	 GetRootNode()->SetElementString(_MapVariableNameTag, varName);
 }
 const string& ParamsIso::GetMapVariableName(){
	 return GetRootNode()->GetElementString(_MapVariableNameTag);
 }
 void ParamsIso::RegisterMapVariableDirtyFlag(ParamNode::DirtyFlag *df){
	GetRootNode()->RegisterDirtyFlagLong(_MapVariableNameTag, df);
}
void ParamsIso::
refreshCtab() {
	if (getMapperFunc())
		((TransferFunction*)getMapperFunc())->makeLut((float*)ctab);
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
	//Copy the edit bounds:
	newParams->minOpacEditBounds = new float[numSessionVariables];
	newParams->maxOpacEditBounds = new float[numSessionVariables];
	newParams->minColorEditBounds = new float[numSessionVariables];
	newParams->maxColorEditBounds = new float[numSessionVariables];
	newParams->minIsoEditBounds = new float[numSessionVariables];
	newParams->maxIsoEditBounds = new float[numSessionVariables];
	for (int i = 0; i<numSessionVariables; i++){
		newParams->minOpacEditBounds[i] = minOpacEditBounds[i];
		newParams->maxOpacEditBounds[i] = maxOpacEditBounds[i];
		newParams->minColorEditBounds[i] = minColorEditBounds[i];
		newParams->maxColorEditBounds[i] = maxColorEditBounds[i];
		newParams->minIsoEditBounds[i] = minIsoEditBounds[i];
		newParams->maxIsoEditBounds[i] = maxIsoEditBounds[i];
	}
	
	//clone the IsoControls and Transfer Functions:
	newParams->transFunc.clear();
	newParams->isoControls.clear();
	assert(transFunc.size() == isoControls.size());
	for (int i = 0; i<isoControls.size(); i++){
		newParams->isoControls.push_back(new IsoControl(*isoControls[i]));
		newParams->transFunc.push_back(new TransferFunction(*transFunc[i]));
		newParams->transFunc[i]->setParams(newParams);
		newParams->isoControls[i]->setParams(newParams);
	}
	return (RenderParams*)newParams;
}

void ParamsIso::updateHistoBounds(){
	if(!getIsoControl()) return;
	float newBnds[2];
	newBnds[0] = getIsoControl()->getMinOpacMapValue();
	newBnds[1] = getIsoControl()->getMaxOpacMapValue();
	SetHistoBounds(newBnds);
}

//Parsing of Iso node needs to handle tags for Variable,
//TransferFunction and IsoControl.
//Defers to parent for handling other stuff.
bool ParamsIso::elementStartHandler(
        ExpatParseMgr* pm, int depth, string& tag, const char ** attrs
) {
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	state->user_defined = 0;

	if (StrCmpNoCase(tag, _variableTag) == 0){
		_parseDepth++;
		//Variable tag:  create a variable 
		// The only supported attribute is the variable name:
		string varName;
		
		float opacFac = 1.f;
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			
			if (StrCmpNoCase(attribName, _variableNameAttr) == 0){
				ist >> varName;
			}
			else if (StrCmpNoCase(attribName, _opacityScaleAttr) == 0){
				ist >> opacFac;
			}
			else return false;
		}
	
		//Add this variable to the session variable names, and set up
		//this variable in the ParamsIso
		parsingVarNum = DataStatus::mergeVariableName(varName);
		if (parsingVarNum >= numSessionVariables) numSessionVariables = parsingVarNum+1;
		
		//Make sure we have a place for it:
		if (transFunc.size() < parsingVarNum +1){
			int currentSize = transFunc.size();
			for (int i = currentSize; i < parsingVarNum +1; i++){
				//Insert nulls
				transFunc.push_back(0);
			}
		}
		if (transFunc[parsingVarNum]) {
			delete transFunc[parsingVarNum];
			transFunc[parsingVarNum] = 0;
		}
		if (isoControls.size() < parsingVarNum +1){
			int currentSize = isoControls.size();
			for (int i = currentSize; i < parsingVarNum +1; i++){
				//Insert nulls
				isoControls.push_back(0);
			}
		}
		if (isoControls[parsingVarNum]) {
			delete isoControls[parsingVarNum];
			isoControls[parsingVarNum] = 0;
		}
		return true;
	} else if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0){
		_parseDepth++;
		//Now create the transfer function, and have it parse its state:
		TransferFunction* tf = new TransferFunction(this, 8);
		transFunc[parsingVarNum] = tf;
		tf->setVarNum(parsingVarNum);
		pm->pushClassStack(transFunc[parsingVarNum]);
		//defer to the transfer function to do its own parsing:
		tf->elementStartHandler(pm, depth, tag, attrs);
		
		return true;
	} else if (StrCmpNoCase(tag, _IsoControlTag) == 0){
		_parseDepth++;
		//Now create the iso control, and have it parse its state:
		IsoControl* ic = new IsoControl(this, 8);
		isoControls[parsingVarNum] = ic;
		ic->setVarNum(parsingVarNum);
		pm->pushClassStack(isoControls[parsingVarNum]);
		//defer to the transfer function to do its own parsing:
		ic->elementStartHandler(pm, depth, tag, attrs);
		
		return true;
	} else { //Defer to parent class for iso parameter parsing
		numSessionVariables = 0;
		return ParamsBase::elementStartHandler(pm, depth, tag, attrs);
	}
}
// At completion of parsing, need pop back from tf or isocontrol parsing
bool ParamsIso::elementEndHandler(ExpatParseMgr* pm, int depth, string& tag) {

	if (_parseDepth == 3){
		//popped back from TF or IsoControl parsing.  
		if ((StrCmpNoCase(tag,_IsoControlTag) == 0)||
			(StrCmpNoCase(tag,TransferFunction::_transferFunctionTag) == 0)){
			_parseDepth--;
			Pop();
			return true;
		} 
		else return false;
		
	}

	else return ParamsBase::elementEndHandler(pm, depth, tag);

}
float ParamsIso::getOpacityScale() 
{
  if (numSessionVariables)
  {
    return transFunc[GetMapVariableNum()]->getOpacityScaleFactor();
  }

  return 1.0;
}

void ParamsIso::setOpacityScale(float val) 
{
  if (numSessionVariables)
  {
    transFunc[GetMapVariableNum()]->setOpacityScaleFactor(val);
  }
}
