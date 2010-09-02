
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
#include "vapor/Version.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace VetsUtil;
using namespace VAPoR;
const string ParamsIso::_shortName = "Iso";
const string ParamsIso::_IsoValueTag = "IsoValue";
const string ParamsIso::_IsoControlTag = "IsoControl";
const string ParamsIso::_ColorMapTag = "ColorMap";
const string ParamsIso::_NormalOnOffTag = "NormalOnOff";
const string ParamsIso::_ConstantColorTag = "ConstantColor";
const string ParamsIso::_HistoBoundsTag = "HistoBounds";
const string ParamsIso::_MapBoundsTag = "MapBounds";
const string ParamsIso::_HistoScaleTag = "HistoScale";
const string ParamsIso::_MapHistoScaleTag = "MapHistoScale";
const string ParamsIso::_SelectedPointTag = "SelectedPoint";
const string ParamsIso::_RefinementLevelTag = "RefinementLevel";
const string ParamsIso::_VisualizerNumTag = "VisualizerNum";
const string ParamsIso::_VariableNameTag = "VariableName";
const string ParamsIso::_MapVariableNameTag = "MapVariableName";
const string ParamsIso::_NumBitsTag = "NumVoxelBits";
const string ParamsIso::_editBoundsTag = "EditBounds";
const string ParamsIso::_mapEditModeTag = "MapEditMode";
const string ParamsIso::_isoEditModeTag = "IsoEditMode";
int ParamsIso::defaultBitsPerVoxel = 8;
namespace {
	const string IsoName = "IsosurfaceParams";
};

ParamsIso::ParamsIso(
	XmlNode *parent, int winnum
) : RenderParams(parent, Params::_isoParamsTag, winnum) {
	
	
	noIsoControlTags = false;
	restart();
}


ParamsIso::~ParamsIso() {
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
	int varNum = ds->getActiveVarNum3D(GetIsoVariableName());
	int mapVarNum = ds->getActiveVarNum3D(GetMapVariableName());
	//See if current variable name is valid.  It needs to be in the metadata.
	//if not, reset to first variable that is present in metadata:
	if (varNum < 0) 
	{
		SetIsoVariableName(ds->getActiveVarName3D(0));
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
		SetNumBits(defaultBitsPerVoxel);
	} else {  //Try to use existing values
		if (numrefs > maxNumRefinements) numrefs = maxNumRefinements;
		if (numrefs < 0) numrefs = 0;
	}
	SetRefinementLevel(numrefs);
	//Set up the compression level.  Whether or not override is true, make sure
	//That the compression level is valid.  If override is true set it to 0;
	if (doOverride) SetCompressionLevel(0);
	else {
		int numCompressions = 0;
	
		if (ds->getDataMgr()) {
			numCompressions = ds->getDataMgr()->GetCRatios().size();
		}
		//For backwards compatibility:
		if (!GetRootNode()->HasElementLong(_CompressionLevelTag)) SetCompressionLevel(0);
		else if (GetCompressionLevel() >= numCompressions){
			SetCompressionLevel(numCompressions-1);
		}
	}

	//Create arrays of pointers to IsoControls and TransferFunctions, and bounds
	assert(totNumVariables > 0);
	TransferFunction** newTransFunc = new TransferFunction*[totNumVariables];
	IsoControl** newIsoControls = new IsoControl*[totNumVariables];
	float** boundsArrays = new float*[totNumVariables];

	//If we are overriding previous values, delete the existing transfer functions, replace with new ones.
	//Set the map bounds to the actual bounds in the data
	if (doOverride){
		for (int i = 0; i<totNumVariables; i++){
			boundsArrays[i] = new float[4];
			
			//will need to set the iso value:
			float dataMin = ds->getDefaultDataMin(i);
			float dataMax = ds->getDefaultDataMax(i);
			if (dataMin == dataMax){
				dataMin -= 0.f; dataMax += 0.5;
			}
			boundsArrays[i][0] = boundsArrays[i][1] = dataMin;
			boundsArrays[i][2] = boundsArrays[i][3] = dataMax;
			newTransFunc[i] = new TransferFunction(this, 8);
			newTransFunc[i]->setOpaque();
			newIsoControls[i] = new IsoControl(this, 8);
			newTransFunc[i]->setMinMapValue(dataMin);
			newTransFunc[i]->setMaxMapValue(dataMax);
			newTransFunc[i]->setVarNum(i);
			newIsoControls[i]->setVarNum(i);
			newIsoControls[i]->setMinHistoValue(dataMin);
			newIsoControls[i]->setMaxHistoValue(dataMax);
			newIsoControls[i]->setIsoValue(0.5*(dataMin+dataMax));
		}
	} else {
		//attempt to make use of existing transfer functions, isocontrols, etc.
		//delete any that are no longer referenced
		
		for (int i = 0; i<totNumVariables; i++){
			boundsArrays[i] = new float[4];
			if(i<GetNumVariables()){ //make copy of existing ones, don't set their root nodes yet
				newTransFunc[i] = (TransferFunction*)GetTransFunc(i)->deepCopy(0);
				newIsoControls[i] = (IsoControl*)GetIsoControl(i)->deepCopy(0);
				newTransFunc[i]->hookup(this, i, i);
				
				newIsoControls[i]->setParams(this);
				boundsArrays[i][0] = GetMinIsoEdit(i);
				boundsArrays[i][1] = GetMinMapEdit(i);
				boundsArrays[i][2] = GetMaxIsoEdit(i);
				boundsArrays[i][3] = GetMaxMapEdit(i);
			} else { //create new tfs, isocontrols
				
				newIsoControls[i] = new IsoControl(this, 8);
				newIsoControls[i]->setMinHistoValue(ds->getDefaultDataMin(i));
				newIsoControls[i]->setMaxHistoValue(ds->getDefaultDataMax(i));
				newIsoControls[i]->setIsoValue(0.5f*(ds->getDefaultDataMin(i)+ds->getDefaultDataMax(i)));
				newIsoControls[i]->setVarNum(i);
				newIsoControls[i]->setParams(this);
				newTransFunc[i] = new TransferFunction(this, 8);
				newTransFunc[i]->setOpaque();
				newTransFunc[i]->setMinMapValue(ds->getDefaultDataMin(i));
				newTransFunc[i]->setMaxMapValue(ds->getDefaultDataMax(i));
				newTransFunc[i]->setVarNum(i);
				boundsArrays[i][0] = boundsArrays[i][1] = ds->getDefaultDataMin(i);
				boundsArrays[i][2] = boundsArrays[i][3] = ds->getDefaultDataMax(i);
				//For backwards compatibility, if we have read iso from an old session:
				if (noIsoControlTags){
					newIsoControls[i]->setIsoValue(oldIsoValue);
					newIsoControls[i]->setMinHistoValue(oldHistoBounds[0]);
					newIsoControls[i]->setMaxHistoValue(oldHistoBounds[1]);
				}
			}
			
		}
		
	} //end if(doOverride)

	//Delete all existing variable nodes
	GetRootNode()->GetNode(_variablesTag)->DeleteAll();
	
	assert(GetRootNode()->GetNode(_variablesTag)->GetNumChildren() == 0);
	//Extend the histo ranges to include the isovalues:
	for (int i = 0; i<totNumVariables; i++){
		float leftside = newIsoControls[i]->getMinHistoValue();
		float rightside = newIsoControls[i]->getMaxHistoValue();
		float width = rightside - leftside;
		float isoval = newIsoControls[i]->getIsoValue();
		if (isoval < leftside + 0.01*width) {
			newIsoControls[i]->setMinHistoValue(isoval - 0.01*width);
			boundsArrays[i][0] = isoval - 0.01*width;
		}
		if (isoval > rightside - 0.01*width){
			newIsoControls[i]->setMaxHistoValue(isoval + 0.01*width);
			boundsArrays[i][2] = isoval + 0.01*width;
		}
	}
	//Create new variable nodes, add them to the tree
	ParamNode* varsNode = GetRootNode()->GetNode(_variablesTag);
	for (int i = 0; i<totNumVariables; i++){
		std::string& varname = ds->getVariableName(i);
		ParamNode* varNode = new ParamNode(varname, 2);
		varsNode->AddChild(varNode);
		ParamNode* tfNode = new ParamNode(TransferFunction::_transferFunctionTag);
		ParamNode* isoNode = new ParamNode(_IsoControlTag);
		
		varNode->AddRegisteredNode(TransferFunction::_transferFunctionTag,tfNode,newTransFunc[i]);
		varNode->AddRegisteredNode(_IsoControlTag,isoNode,newIsoControls[i]);
		vector<double>* bounds = new vector<double>;
		for (int k = 0; k<4; k++)
			bounds->push_back(boundsArrays[i][k]);
		varNode->SetElementDouble(_editBoundsTag, *bounds);
		delete boundsArrays[i];
		
	}
	assert(GetRootNode()->GetNode(_variablesTag)->GetNumChildren() == totNumVariables);
	delete newTransFunc;
	delete newIsoControls;
	delete boundsArrays;
	
	if (doOverride) {
		SetHistoStretch(1.0);
		SetIsoHistoStretch(1.0);
		float col[4] = {1.f, 1.f, 1.f, 1.f};
		SetConstantColor(col);
	}
	initializeBypassFlags();
	
	return true;
}

void ParamsIso::restart() {
	
	
	SetNormalOnOff(1);
	SetNumBits(defaultBitsPerVoxel);

	SetHistoStretch(1.0);
	SetIsoHistoStretch(1.0);

	float pnt[3] = {0.0, 0.0, 0.0};
	SetSelectedPoint(pnt);
	SetRefinementLevel(0);
	SetCompressionLevel(0);
	SetVisualizerNum(vizNum);
	SetIsoVariableName("none");
	SetMapVariableName("Constant");

	setMapEditMode(1);
	setIsoEditMode(1);
	
	setEnabled(false);
	float default_color[4] = {1.0, 1.0, 1.0, 1.0};
	SetConstantColor(default_color);
	//Delete any child nodes
	GetRootNode()->DeleteAll();
	
	//Create a default variable node:
	ParamNode* pNode = new ParamNode(_variablesTag, 1);
	GetRootNode()->AddNode(_variablesTag, pNode);
	//Put a default variable node in it:
	ParamNode* vNode = new ParamNode("none", 3);
	pNode->AddNode("none",vNode);
	TransferFunction *tf = (TransferFunction*)TransferFunction::CreateDefaultInstance();
	vNode->AddRegisteredNode(TransferFunction::_transferFunctionTag,tf->GetRootNode(),tf);
	IsoControl* iControl = (IsoControl*)IsoControl::CreateDefaultInstance();
	vNode->AddRegisteredNode(_IsoControlTag,iControl->GetRootNode(),iControl);
	vector<double>* bounds = new vector<double>;
	bounds->push_back(0.00);
	bounds->push_back(0.00);
	bounds->push_back(1.00);
	bounds->push_back(1.00);
	vNode->SetElementDouble(_editBoundsTag, *bounds);
	
}
void ParamsIso::setDefaultPrefs(){
	defaultBitsPerVoxel = 8;
}
//Hook up the new transfer function in specified slot,
//Delete the old one.  This is called whenever a new tf is loaded.
//
void ParamsIso::
hookupTF(TransferFunction* tf, int index){

	vector<string> path;
	path.push_back(_variablesTag);
	DataStatus* ds = DataStatus::getInstance();
	path.push_back(ds->getVariableName(index));
	path.push_back(TransferFunction::_transferFunctionTag);
	
	tf->hookup(this,index,index);
	int newindex = GetRootNode()->ReplaceNode(path, tf->GetRootNode());
	
	assert(newindex >= 0);

	
}
MapperFunction* ParamsIso::getMapperFunc() {
	return ((GetMapVariableNum()>=0) ? GetTransFunc(GetMapVariableNum()) : 0);
}
IsoControl* ParamsIso::getIsoControl(){
	return ((GetNumVariables() > 0 ) ? GetIsoControl(GetIsoVariableNum()) : 0);
}

void ParamsIso::setMinColorMapBound(float val){
	getMapperFunc()->setMinColorMapValue(val);
	GetRootNode()->SetFlagDirty(_MapBoundsTag);
	setAllBypass(false);
}
void ParamsIso::setMaxColorMapBound(float val){
	getMapperFunc()->setMaxColorMapValue(val);
	GetRootNode()->SetFlagDirty(_MapBoundsTag);
	setAllBypass(false);
}

void ParamsIso::setMinOpacMapBound(float val){
	getMapperFunc()->setMinOpacMapValue(val);
	GetRootNode()->SetFlagDirty(_MapBoundsTag);
	setAllBypass(false);
}
void ParamsIso::setMaxOpacMapBound(float val){
	getMapperFunc()->setMaxOpacMapValue(val);
	GetRootNode()->SetFlagDirty(_MapBoundsTag);
	setAllBypass(false);
}


void ParamsIso::SetIsoValue(double value) {
	double oldVal = GetIsoValue();
	if (oldVal == value) return;
	getIsoControl()->setIsoValue(value);
	GetRootNode()->SetFlagDirty(_IsoValueTag);
	setAllBypass(false);
}

double ParamsIso::GetIsoValue() {
	return (GetNumVariables()>0) ? getIsoControl()->getIsoValue() : 0.f;
}

//Note:  Following dirty flags are not actually associated with tags or nodes in the xml.
//The flags must be set when appropriate changes are made in the isoControls or transfer functions
void ParamsIso::RegisterIsoValueDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlag(_IsoValueTag, df);
}
void ParamsIso::RegisterColorMapDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlag(_ColorMapTag, df);
}
void ParamsIso::RegisterMapBoundsDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlag(_MapBoundsTag, df);
}
void ParamsIso::RegisterHistoBoundsDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlag(_HistoBoundsTag, df);
}

void ParamsIso::SetNormalOnOff(bool flag) {
	vector <long> valvec(1, flag);
	GetRootNode()->SetElementLong(_NormalOnOffTag, valvec);
}

bool ParamsIso::GetNormalOnOff() {
	vector <long> valvec = GetRootNode()->GetElementLong(_NormalOnOffTag);
	return((bool) valvec[0]);
}
bool ParamsIso::getMapEditMode(){
	long val = (GetRootNode()->GetElementLong(_mapEditModeTag))[0];
	return (bool)val;
}
void ParamsIso::setMapEditMode(bool flag) {
	GetRootNode()->SetElementLong(_mapEditModeTag, (long)flag);
}
bool ParamsIso::getIsoEditMode(){
	long val = (GetRootNode()->GetElementLong(_isoEditModeTag))[0];
	return (bool)val;
}
void ParamsIso::setIsoEditMode(bool flag) {
	GetRootNode()->SetElementLong(_isoEditModeTag, (long)flag);
}
void ParamsIso::RegisterNormalOnOffDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlag(_NormalOnOffTag, df);
}

void ParamsIso::SetConstantColor(float rgba[4]) {
	vector <double> valvec(4,0);
	for (int i=0; i<4; i++) {
		valvec[i] = rgba[i];
	}
	GetRootNode()->SetElementDouble(_ConstantColorTag, valvec);
}

const float *ParamsIso::GetConstantColor() {
	vector <double> valvec = GetRootNode()->GetElementDouble(_ConstantColorTag);
	for (int i=0; i<4; i++) _constcolorbuf[i] = valvec[i];
	return(_constcolorbuf);
}

void ParamsIso::RegisterConstantColorDirtyFlag(ParamNode::DirtyFlag *df) {
	GetRootNode()->RegisterDirtyFlag(_ConstantColorTag, df);
}

 void ParamsIso::SetHistoBounds(float bnds[2]){
	IsoControl* isoContr = getIsoControl();
	if(!isoContr) return;
	if(isoContr->getMinHistoValue() == bnds[0] &&
		isoContr->getMaxHistoValue() == bnds[1]) return;
	isoContr->setMinHistoValue(bnds[0]);
	isoContr->setMaxHistoValue(bnds[1]);
	GetRootNode()->SetFlagDirty(_HistoBoundsTag);
	setAllBypass(false);
 }
 
 void ParamsIso::SetMapBounds(float bnds[2]){
	MapperFunction* mapFunc = getMapperFunc();
	if(!mapFunc) return;
	if(mapFunc->getMinColorMapValue() == bnds[0] &&
		mapFunc->getMaxColorMapValue() == bnds[1]) return;
	mapFunc->setMinColorMapValue(bnds[0]);
	mapFunc->setMaxColorMapValue(bnds[1]);
	mapFunc->setMinOpacMapValue(bnds[0]);
	mapFunc->setMaxOpacMapValue(bnds[1]);
	SetFlagDirty(_MapBoundsTag);
	setAllBypass(false);
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
 //Use _mapperBounds to return tf bounds 
 const float* ParamsIso::GetMapBounds(){
	 if (!getMapperFunc() || GetMapVariableNum() < 0){
		 _mapperBounds[0] = 0.f;
		 _mapperBounds[1] = 1.f;
	 } else {
		_mapperBounds[0]=getMapperFunc()->getMinColorMapValue();
		_mapperBounds[1]=getMapperFunc()->getMaxColorMapValue();
	 }
	return( _mapperBounds);
 }

 void ParamsIso::SetIsoHistoStretch(float scale){
	vector <double> valvec(1, (double)scale);
	GetRootNode()->SetElementDouble(_HistoScaleTag, valvec);
 }

 float ParamsIso::GetIsoHistoStretch(){
	vector<double> valvec = GetRootNode()->GetElementDouble(_HistoScaleTag);
	return (float)valvec[0];
 }
 void ParamsIso::SetHistoStretch(float scale){
	vector <double> valvec(1, (double)scale);
	GetRootNode()->SetElementDouble(_MapHistoScaleTag, valvec);
 }

 float ParamsIso::GetHistoStretch(){
	vector<double> valvec = GetRootNode()->GetElementDouble(_MapHistoScaleTag);
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
	vector<long> valvec = GetRootNode()->GetElementLong(_RefinementLevelTag);
	return (int)valvec[0];
 }
 void ParamsIso::RegisterRefinementDirtyFlag(ParamNode::DirtyFlag *df){
	GetRootNode()->RegisterDirtyFlag(_RefinementLevelTag, df);
}
int ParamsIso::GetCompressionLevel(){
	vector<long> valvec = GetRootNode()->GetElementLong(_CompressionLevelTag);
	return (int)valvec[0];
 }
void ParamsIso::SetCompressionLevel(int level){
	 vector<long> valvec(1,(long)level);
	 GetRootNode()->SetElementLong(_CompressionLevelTag,valvec);
 }
 void ParamsIso::RegisterCompressionDirtyFlag(ParamNode::DirtyFlag *df){
	GetRootNode()->RegisterDirtyFlag(_CompressionLevelTag, df);
}
 void ParamsIso::SetVisualizerNum(int viznum){
	vector<long> valvec(1,(long)viznum);
	GetRootNode()->SetElementLong(_VisualizerNumTag,valvec);
	vizNum = viznum;
 }
 int ParamsIso::GetVisualizerNum(){
	vector<long> valvec = GetRootNode()->GetElementLong(_RefinementLevelTag);
	return (int)valvec[0];
 }
 int ParamsIso::GetNumBits(){
	vector<long> valvec = GetRootNode()->GetElementLong(_NumBitsTag);
	if (valvec.size() == 0){
		//For backwards compatibility, insert default value:
		SetNumBits(defaultBitsPerVoxel);
		return defaultBitsPerVoxel;
	}
	return (int)valvec[0];
 }
 void ParamsIso::SetNumBits(int numbits){
	vector<long> valvec(1,(long)numbits);
	GetRootNode()->SetElementLong(_NumBitsTag,valvec);
 }
void ParamsIso::RegisterNumBitsDirtyFlag(ParamNode::DirtyFlag *df){
	GetRootNode()->RegisterDirtyFlag(_NumBitsTag, df);
}

 void ParamsIso::SetIsoVariableName(const string& varName){
	 GetRootNode()->SetElementString(_VariableNameTag, varName);
	 SetFlagDirty(_HistoBoundsTag);
	 SetFlagDirty(_IsoValueTag);
	 setAllBypass(false);
 }
 const string& ParamsIso::GetIsoVariableName(){
	 return GetRootNode()->GetElementString(_VariableNameTag);
 }
 void ParamsIso::RegisterVariableDirtyFlag(ParamNode::DirtyFlag *df){
	GetRootNode()->RegisterDirtyFlag(_VariableNameTag, df);
}
void ParamsIso::SetMapVariableName(const string& varName){
	 GetRootNode()->SetElementString(_MapVariableNameTag, varName);
	 SetFlagDirty(_MapBoundsTag);
	 SetFlagDirty(_ColorMapTag);
	 setAllBypass(false);
 }
 const string& ParamsIso::GetMapVariableName(){
	 return GetRootNode()->GetElementString(_MapVariableNameTag);
 }
 void ParamsIso::RegisterMapVariableDirtyFlag(ParamNode::DirtyFlag *df){
	GetRootNode()->RegisterDirtyFlag(_MapVariableNameTag, df);
}
void ParamsIso::
refreshCtab() {
	if (getMapperFunc())
		((TransferFunction*)getMapperFunc())->makeLut((float*)ctab);
}

//Following performs parsing for old versions (prior to 1.6)
//Parsing for 1.6 relies on ParamsBase to do everything
//Parsing of Iso node needs to handle tags for Variable,
//TransferFunction and IsoControl.
//Defers to parent for handling other stuff.
bool ParamsIso::elementStartHandler(
        ExpatParseMgr* pm, int depth, string& tag, const char ** attrs
) {
	DataStatus* ds = DataStatus::getInstance();
	const std::string& parsingVersion = ds->getSessionVersion();
	int gt = Version::Compare(parsingVersion, "1.5.2");
	if (gt > 0) return ParamsBase::elementStartHandler(pm, depth, tag, attrs);

	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	state->user_defined = 0;

	if (StrCmpNoCase(tag, _variableTag) == 0){
		_parseDepth++;
		//Variable tag:  create a variable 
		// The supported attributes are the variable name and opacity scale
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
		//Make sure that the required variables node exists:

		ParamNode* varsNode;
		if (!GetRootNode()->HasChild(_variablesTag)){
			varsNode = new ParamNode(_variablesTag);
			GetRootNode()->AddNode(_variablesTag,varsNode);
		} else {
			varsNode = GetRootNode()->GetNode(_variablesTag);
		}
		//See if the varsNode has a child for the current variable name;
		//If not, add it.
		if (!varsNode->HasChild(varName)){
			ParamNode* varNode = new ParamNode(varName, 2);
			varsNode->AddNode(varName,varNode);
		}
		
		return true;
	} else if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0){
		_parseDepth++;
		//Now create the transfer function, and have it parse its state:
		TransferFunction* tf = new TransferFunction(this, 8);
		tf->setVarNum(parsingVarNum);
		ParamNode* tfNode = new ParamNode(TransferFunction::_transferFunctionTag,0);
		tfNode->SetParamsBase(tf);
		string varname = ds->getVariableName(parsingVarNum);
		ParamNode* varNode = GetRootNode()->GetNode(_variablesTag)->GetNode(varname);
		varNode->AddNode(TransferFunction::_transferFunctionTag,tfNode);
		
		pm->pushClassStack(tf);
		//defer to the transfer function to do its own parsing:
		tf->elementStartHandler(pm, depth, tag, attrs);

		//Make sure there's a double element to work with:
		if (!varNode->HasElementDouble(_editBoundsTag)){
			vector<double> valvec;
			for (int q = 0; q<4; q++) valvec.push_back(0.);
			varNode->SetElementDouble(_editBoundsTag,valvec);
		}
		SetMinMapEdit(parsingVarNum, tf->getMinMapValue());
		SetMaxMapEdit(parsingVarNum, tf->getMaxMapValue());
		return true;
	} else if (StrCmpNoCase(tag, _IsoControlTag) == 0){
		_parseDepth++;
		noIsoControlTags = false;
		//Now create the iso control, and have it parse its state:
		IsoControl* ic = new IsoControl(this, 8);
		ic->setVarNum(parsingVarNum);
		ParamNode* icNode = new ParamNode(_IsoControlTag,0);
		icNode->SetParamsBase(ic);
		string varname = ds->getVariableName(parsingVarNum);
		ParamNode* varNode = GetRootNode()->GetNode(_variablesTag)->GetNode(varname);
		varNode->AddNode(_IsoControlTag,icNode);

		pm->pushClassStack(ic);
		//defer to the isocontrol  to do its own parsing:
		ic->elementStartHandler(pm, depth, tag, attrs);
		//Make sure there's a double element to work with:
		if (!varNode->HasElementDouble(_editBoundsTag)){
			vector<double> valvec;
			for (int q = 0; q<4; q++) valvec.push_back(0.);
			varNode->SetElementDouble(_editBoundsTag,valvec);
		}
		SetMinIsoEdit(parsingVarNum, ic->getMinHistoValue());
		SetMaxIsoEdit(parsingVarNum, ic->getMaxHistoValue());
		return true;
	} else { //Defer to parent class for iso parameter parsing
	    
		noIsoControlTags = true;
		return ParamsBase::elementStartHandler(pm, depth, tag, attrs);
	}
}
// At completion of parsing, need pop back from tf or isocontrol parsing
bool ParamsIso::elementEndHandler(ExpatParseMgr* pm, int depth, string& tag) {

	const std::string& parsingVersion = DataStatus::getInstance()->getSessionVersion();
	int gt = Version::Compare(parsingVersion, "1.5.2");
	if (gt > 0) return ParamsBase::elementEndHandler(pm, depth, tag);

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

	else {
		if (_parseDepth == 1 && noIsoControlTags) {//For backwards compatibility..
				//Get the value of the isocontrol stuff from the obsolete tags:
				
			const vector <double> &histBnds = GetRootNode()->GetElementDouble(_HistoBoundsTag);
			const vector <double> &isoval = GetRootNode()->GetElementDouble(_IsoValueTag);
			oldIsoValue = isoval[0];
			oldHistoBounds[0] = histBnds[0];
			oldHistoBounds[1] = histBnds[1];
		}
		
		return ParamsBase::elementEndHandler(pm, depth, tag);
	}

}
float ParamsIso::getOpacityScale() 
{
  if (GetNumVariables()>0)
  {
    return GetTransFunc(GetMapVariableNum())->getOpacityScaleFactor();
  }

  return 1.0;
}
int ParamsIso::GetNumVariables() {
	if (!GetRootNode()->HasChild(_variablesTag)) return 0;
	return(GetRootNode()->GetNode(_variablesTag)->GetNumChildren());
}
void ParamsIso::setOpacityScale(float val) 
{
  if (GetNumVariables()>0)
  {
    GetTransFunc(GetMapVariableNum())->setOpacityScaleFactor(val);
  }
}
bool ParamsIso::isOpaque(){
	if(GetMapVariableNum() < 0) {
		if(GetConstantColor()[3] < 0.99f) return true;
		else return false;
	}
	if (!GetTransFunc(GetMapVariableNum())) return true;
	if(GetTransFunc(GetMapVariableNum())->isOpaque() && getOpacityScale() > 0.99f) return true;
	return false;
}
TransferFunction* ParamsIso::GetTransFunc(int sesVarNum){
	DataStatus* ds = DataStatus::getInstance();
	const string& str = ds->getVariableName(sesVarNum);
	if (str.length() == 0) return 0;
	return (TransferFunction*)(GetRootNode()->GetNode(_variablesTag)->GetNode(str)->GetNode(TransferFunction::_transferFunctionTag)->GetParamsBase());
}
IsoControl* ParamsIso::GetIsoControl(int sesVarNum){
	DataStatus* ds = DataStatus::getInstance();
	const string& str = ds->getVariableName(sesVarNum);
	if (str.length() == 0) return 0;
	return (IsoControl*)(GetRootNode()->GetNode(_variablesTag)->GetNode(str)->GetNode(_IsoControlTag)->GetParamsBase());
}

