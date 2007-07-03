//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		probeparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Implementation of the probeparams class
//		This contains all the parameters required to support the
//		probe renderer.  Embeds a transfer function and a
//		transfer function editor.
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif



#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "glutil.h"
#include "probeparams.h"
#include "regionparams.h"
#include "params.h"
#include "transferfunction.h"

#include "histo.h"
#include "animationparams.h"

#include <math.h>
#include "vapor/Metadata.h"
#include "vapor/DataMgr.h"
#include "vapor/errorcodes.h"


using namespace VAPoR;
const string ProbeParams::_editModeAttr = "TFEditMode";
const string ProbeParams::_histoStretchAttr = "HistoStretchFactor";
const string ProbeParams::_variableSelectedAttr = "VariableSelected";
const string ProbeParams::_geometryTag = "ProbeGeometry";
const string ProbeParams::_probeMinAttr = "ProbeMin";
const string ProbeParams::_probeMaxAttr = "ProbeMax";
const string ProbeParams::_cursorCoordsAttr = "CursorCoords";
const string ProbeParams::_phiAttr = "PhiAngle";
const string ProbeParams::_thetaAttr = "ThetaAngle";
const string ProbeParams::_psiAttr = "PsiAngle";
const string ProbeParams::_numTransformsAttr = "NumTransforms";
const string ProbeParams::_planarAttr = "Planar";


ProbeParams::ProbeParams(int winnum) : RenderParams(winnum){
	thisParamType = ProbeParamsType;
	numVariables = 0;
	probeTextures = 0;
	maxTimestep = 1;
	restart();
	
}
ProbeParams::~ProbeParams(){
	
	if (transFunc){
		for (int i = 0; i< numVariables; i++){
			delete transFunc[i];  //will delete editor
		}
		delete transFunc;
	}
	if (probeTextures) {
		for (int i = 0; i<= maxTimestep; i++){
			if (probeTextures[i]) delete probeTextures[i];
		}
		delete probeTextures;
	}
	
}


//Deepcopy requires cloning tf 
RenderParams* ProbeParams::
deepRCopy(){
	ProbeParams* newParams = new ProbeParams(*this);
	//Clone the map bounds arrays:
	int numVars = Max (numVariables, 1);
	newParams->minColorEditBounds = new float[numVars];
	newParams->maxColorEditBounds = new float[numVars];
	newParams->minOpacEditBounds = new float[numVars];
	newParams->maxOpacEditBounds = new float[numVars];
	for (int i = 0; i<numVars; i++){
		newParams->minColorEditBounds[i] = minColorEditBounds[i];
		newParams->maxColorEditBounds[i] = maxColorEditBounds[i];
		newParams->minOpacEditBounds[i] = minOpacEditBounds[i];
		newParams->maxOpacEditBounds[i] = maxOpacEditBounds[i];
	}

	//Clone the Transfer Functions
	newParams->transFunc = new TransferFunction*[numVariables];
	for (int i = 0; i<numVariables; i++){
		newParams->transFunc[i] = new TransferFunction(*transFunc[i]);
	}
	//Probe texture must be recreated when needed
	newParams->probeTextures = 0;
	
	//never keep the SavedCommand:
	
	return newParams;
}


void ProbeParams::
refreshCtab() {
	((TransferFunction*)getMapperFunc())->makeLut((float*)ctab);
}
	



void ProbeParams::
setClut(const float newTable[256][4]){
	for (int i = 0; i< 256; i++) {
		for (int j = 0; j< 4; j++){
			ctab[i][j] = newTable[i][j];
		}
	}
}


float ProbeParams::getOpacityScale() 
{
  if (numVariables)
  {
    return transFunc[firstVarNum]->getOpacityScaleFactor();
  }

  return 1.0;
}

void ProbeParams::setOpacityScale(float val) 
{
  if (numVariables)
  {
    return transFunc[firstVarNum]->setOpacityScaleFactor(val);
  }
}


//Initialize for new metadata.  Keep old transfer functions
//
bool ProbeParams::
reinit(bool doOverride){
	int i;
	
	const float* extents = DataStatus::getInstance()->getExtents();
	setMaxNumRefinements(DataStatus::getInstance()->getNumTransforms());
	//Set up the numRefinements combo
	
	
	//Either set the probe bounds to a default size in the center of the domain, or 
	//try to use the previous bounds:
	if (doOverride){
		for (int i = 0; i<3; i++){
			float probeRadius = 0.1f*(extents[i+3] - extents[i]);
			float probeMid = 0.5f*(extents[i+3] + extents[i]);
			if (i<2) {
				probeMin[i] = probeMid - probeRadius;
				probeMax[i] = probeMid + probeRadius;
			} else {
				probeMin[i] = probeMax[i] = probeMid;
			}
		}
		//default values for phi, theta,  cursorPosition
		phi = 0.f;
		theta = 0.f;
		psi = 0.f;
		
		cursorCoords[0] = cursorCoords[1] = 0.0f;
		numRefinements = 0;
	} else {
		//Force the probe size to be no larger than the domain, and 
		//force the probe center to be inside the domain
		for (i = 0; i<3; i++){
			if (probeMax[i] - probeMin[i] > (extents[i+3]-extents[i]))
				probeMax[i] = probeMin[i] + extents[i+3]-extents[i];
			float center = 0.5f*(probeMin[i]+probeMax[i]);
			if (center < extents[i]) {
				probeMin[i] += (extents[i]-center);
				probeMax[i] += (extents[i]-center);
			}
			if (center > extents[i+3]) {
				probeMin[i] += (extents[i+3]-center);
				probeMax[i] += (extents[i+3]-center);
			}
			if(probeMax[i] < probeMin[i]) 
				probeMax[i] = probeMin[i];
		}
		if (numRefinements > maxNumRefinements) numRefinements = maxNumRefinements;
	}
	//Get the variable names:

	int newNumVariables = DataStatus::getInstance()->getNumSessionVariables();
	
	//See if current firstVarNum is valid
	//if not, reset to first variable that is present:
	if (!DataStatus::getInstance()->variableIsPresent(firstVarNum)){
		firstVarNum = -1;
		for (i = 0; i<newNumVariables; i++) {
			if (DataStatus::getInstance()->variableIsPresent(i)){
				firstVarNum = i;
				break;
			}
		}
	}
	if (firstVarNum == -1){
		
		for (i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete transFunc;
		numVariables = 0;
		return false;
	}
	//Set up the selected variables.
	
	if (doOverride){//default is to only select the first variable.
		variableSelected.clear();
		variableSelected.resize(newNumVariables, false);
	} else {
		if (newNumVariables != numVariables){
			variableSelected.resize(newNumVariables, false);
		} 
		//Make sure that current selection is valid, by unselecting
		//anything that isn't there:
		for (int i = 0; i<newNumVariables; i++){
			if (variableSelected[i] && 
				!DataStatus::getInstance()->variableIsPresent(i))
				variableSelected[i] = false;
		}
	}
	variableSelected[firstVarNum] = true;

	
	
	//Create new arrays to hold bounds and transfer functions:
	TransferFunction** newTransFunc = new TransferFunction*[newNumVariables];
	float* newMinEdit = new float[newNumVariables];
	float* newMaxEdit = new float[newNumVariables];
	//If we are overriding previous values, delete the transfer functions, create new ones.
	//Set the map bounds to the actual bounds in the data
	if (doOverride){
		for (i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		//Create new transfer functions, their editors, hook them up:
		
		for (i = 0; i<newNumVariables; i++){
			newTransFunc[i] = new TransferFunction(this, 8);
			//Initialize to be fully opaque:
			newTransFunc[i]->setOpaque();

			newTransFunc[i]->setMinMapValue(DataStatus::getInstance()->getDefaultDataMin(i));
			newTransFunc[i]->setMaxMapValue(DataStatus::getInstance()->getDefaultDataMax(i));
			newMinEdit[i] = DataStatus::getInstance()->getDefaultDataMin(i);
			newMaxEdit[i] = DataStatus::getInstance()->getDefaultDataMax(i);

            newTransFunc[i]->setVarNum(i);
		}
	} else { 
		//attempt to make use of existing transfer functions, edit ranges.
		//delete any that are no longer referenced
		for (i = 0; i<newNumVariables; i++){
			if(i<numVariables){
				newTransFunc[i] = transFunc[i];
				newMinEdit[i] = minColorEditBounds[i];
				newMaxEdit[i] = maxColorEditBounds[i];
			} else { //create new tf
				newTransFunc[i] = new TransferFunction(this, 8);
				//Initialize to be fully opaque:
				newTransFunc[i]->setOpaque();

				newTransFunc[i]->setMinMapValue(DataStatus::getInstance()->getDefaultDataMin(i));
				newTransFunc[i]->setMaxMapValue(DataStatus::getInstance()->getDefaultDataMax(i));
				newMinEdit[i] = DataStatus::getInstance()->getDefaultDataMin(i);
				newMaxEdit[i] = DataStatus::getInstance()->getDefaultDataMax(i);
                newTransFunc[i]->setVarNum(i);
			}
		}
			//Delete trans funcs 
		for (i = newNumVariables; i<numVariables; i++){
			delete transFunc[i];
		}
	} //end if(doOverride)
	//Make sure edit bounds are valid
	for(i = 0; i<newNumVariables; i++){
		if (newMinEdit[i] >= newMaxEdit[i]){
			newMinEdit[i] = DataStatus::getInstance()->getDefaultDataMin(i);
			newMaxEdit[i] = DataStatus::getInstance()->getDefaultDataMax(i);
		}
		//And check again...
		if (newMinEdit[i] >= newMaxEdit[i]){
			newMinEdit[i] = 0.f;
			newMaxEdit[i] = 1.f;
		}
	}
	//Hook up new stuff
	delete minColorEditBounds;
	delete maxColorEditBounds;
	delete transFunc;
	minColorEditBounds = newMinEdit;
	maxColorEditBounds = newMaxEdit;
	//And clone the color edit bounds to use as opac edit bounds:
	minOpacEditBounds = new float[newNumVariables];
	maxOpacEditBounds = new float[newNumVariables];
	for (i = 0; i<newNumVariables; i++){
		minOpacEditBounds[i] = minColorEditBounds[i];
		maxOpacEditBounds[i] = maxColorEditBounds[i];
	}

	transFunc = newTransFunc;
	
	numVariables = newNumVariables;
	setEnabled(false);
	// set up the texture cache
	setProbeDirty();
	if (probeTextures) delete probeTextures;
	maxTimestep = DataStatus::getInstance()->getMaxTimestep();
	probeTextures = 0;
	return true;
}
//Initialize to default state
//
void ProbeParams::
restart(){
	planar = true;
	textureWidth = textureHeight = 0;
	histoStretchFactor = 1.f;
	firstVarNum = 0;
	setProbeDirty();
	if (probeTextures) delete probeTextures;
	probeTextures = 0;
	
	if(numVariables > 0){
		for (int i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete transFunc;
	}
	numVariables = 0;
	transFunc = 0;
	cursorCoords[0] = cursorCoords[1] = 0.0f;
	//Initialize the mapping bounds to [0,1] until data is read
	
	if (minColorEditBounds) delete minColorEditBounds;
	if (maxColorEditBounds) delete maxColorEditBounds;
	if (minOpacEditBounds) delete minOpacEditBounds;
	if (maxOpacEditBounds) delete maxOpacEditBounds;
	
	
	minColorEditBounds = new float[1];
	maxColorEditBounds = new float[1];
	minColorEditBounds[0] = 0.f;
	maxColorEditBounds[0] = 1.f;
	minOpacEditBounds = new float[1];
	maxOpacEditBounds = new float[1];
	minOpacEditBounds[0] = 0.f;
	maxOpacEditBounds[0] = 1.f;
	currentDatarange[0] = 0.f;
	currentDatarange[1] = 1.f;
	editMode = true;   //default is edit mode

	setEnabled(false);
	
	
	firstVarNum = 0;
	numVariables = 0;
	numVariablesSelected = 0;

	
	numRefinements = 0;
	maxNumRefinements = 10;
	theta = 0.f;
	phi = 0.f;
	psi = 0.f;
	for (int i = 0; i<3; i++){
		if (i < 2) probeMin[i] = 0.4f;
		else probeMin[i] = 0.5f;
		if(i<2) probeMax[i] = 0.6f;
		else probeMax[i] = 0.5f;
		selectPoint[i] = 0.5f;
	}
	
	
	
}




//Hook up the new transfer function in specified slot,
//Delete the old one.  This is called whenever a new tf is loaded.
//
void ProbeParams::
hookupTF(TransferFunction* tf, int index){

	if (transFunc[index]) delete transFunc[index];
	transFunc[index] = tf;

	minColorEditBounds[index] = tf->getMinMapValue();
	maxColorEditBounds[index] = tf->getMaxMapValue();
	tf->setParams(this);
    tf->setVarNum(firstVarNum);
}


//Handlers for Expat parsing.
//
bool ProbeParams::
elementStartHandler(ExpatParseMgr* pm, int depth , std::string& tagString, const char **attrs){
	
	static int parsedVarNum = -1;
	int i;
	if (StrCmpNoCase(tagString, _probeParamsTag) == 0) {
		
		int newNumVariables = 0;
		//If it's a Probe tag, obtain 6 attributes (2 are from Params class)
		//Do this by repeatedly pulling off the attribute name and value
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _vizNumAttr) == 0) {
				ist >> vizNum;
			}
			else if (StrCmpNoCase(attribName, _numVariablesAttr) == 0) {
				ist >> newNumVariables;
			}
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				//Ignore this
			}
			else if (StrCmpNoCase(attribName, _histoStretchAttr) == 0){
				float histStretch;
				ist >> histStretch;
				setHistoStretch(histStretch);
			}
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
				if (numRefinements > maxNumRefinements) maxNumRefinements = numRefinements;
			}
			else if (StrCmpNoCase(attribName, _editModeAttr) == 0){
				if (value == "true") setEditMode(true); 
				else setEditMode(false);
			}
			else if (StrCmpNoCase(attribName, _planarAttr) == 0){
				if (value == "true") setPlanar(true); 
				else setPlanar(false);
			}
			else return false;
		}
		//Create space for the variables:
		int numVars = Max (newNumVariables, 1);
		if (minColorEditBounds) delete minColorEditBounds;
		minColorEditBounds = new float[numVars];
		if (maxColorEditBounds) delete maxColorEditBounds;
		maxColorEditBounds = new float[numVars];
		if (minOpacEditBounds) delete minOpacEditBounds;
		minOpacEditBounds = new float[numVars];
		if (maxOpacEditBounds) delete maxOpacEditBounds;
		maxOpacEditBounds = new float[numVars];
		
		variableSelected.clear();
		for (i = 0; i<newNumVariables; i++){
			variableSelected.push_back(false);
		}
		//Setup with default values, in case not specified:
		for (i = 0; i< newNumVariables; i++){
			minColorEditBounds[i] = 0.f;
			maxColorEditBounds[i] = 1.f;
		}

		//create default Transfer Functions
		//Are they gone?
		if (transFunc){
			for (int j = 0; j<numVariables; j++){
				delete transFunc[j];
			}
			delete transFunc;
		}
		numVariables = newNumVariables;
		transFunc = new TransferFunction*[numVariables];
		//Create default transfer functions and editors
		for (int j = 0; j<numVariables; j++){
			transFunc[j] = new TransferFunction(this, 8);
            transFunc[j]->setVarNum(j);
		}
		
		
		return true;
	}
	//Parse a Variable:
	else if (StrCmpNoCase(tagString, _variableTag) == 0) {
		string parsedVarName;
		parsedVarName.clear();
		float leftEdit = 0.f;
		float rightEdit = 1.f;
		bool varSelected = false;
		float opacFac = 1.f;
		
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			
			if (StrCmpNoCase(attribName, _leftEditBoundAttr) == 0) {
				ist >> leftEdit;
			}
			else if (StrCmpNoCase(attribName, _rightEditBoundAttr) == 0) {
				ist >> rightEdit;
			}
			else if (StrCmpNoCase(attribName, _variableNameAttr) == 0){
				ist >> parsedVarName;
			}
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
			}
			else if (StrCmpNoCase(attribName, _variableSelectedAttr) == 0){
				if (value == "true") {
					varSelected = true;
				}
			}
			else if (StrCmpNoCase(attribName, _opacityScaleAttr) == 0){
				ist >> opacFac;
			}
			else return false;
		}
		// Now set the values obtained from attribute parsing.
	
		parsedVarNum = DataStatus::getInstance()->mergeVariableName(parsedVarName);
		variableSelected[parsedVarNum] = varSelected;
		setMinColorEditBound(leftEdit,parsedVarNum);
		setMaxColorEditBound(rightEdit,parsedVarNum);
		transFunc[parsedVarNum]->setOpacityScaleFactor(opacFac);
		
		return true;
	}
	//Parse a transferFunction
	//Note we are relying on parsedvarnum obtained by previous start handler:
	else if (StrCmpNoCase(tagString, TransferFunction::_transferFunctionTag) == 0) {
		//Need to "push" to transfer function parser.
		//That parser will "pop" back to probeparams when done.
		pm->pushClassStack(transFunc[parsedVarNum]);
		transFunc[parsedVarNum]->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	}
	//Parse the geometry node
	else if (StrCmpNoCase(tagString, _geometryTag) == 0) {
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			
			if (StrCmpNoCase(attribName, _thetaAttr) == 0) {
				ist >> theta;
			}
			else if (StrCmpNoCase(attribName, _phiAttr) == 0) {
				ist >> phi;
			}
			else if (StrCmpNoCase(attribName, _psiAttr) == 0) {
				ist >> psi;
			}
			else if (StrCmpNoCase(attribName, _probeMinAttr) == 0) {
				ist >> probeMin[0];ist >> probeMin[1];ist >> probeMin[2];
			}
			else if (StrCmpNoCase(attribName, _probeMaxAttr) == 0) {
				ist >> probeMax[0];ist >> probeMax[1];ist >> probeMax[2];
			}
			else if (StrCmpNoCase(attribName, _cursorCoordsAttr) == 0) {
				ist >> cursorCoords[0];ist >> cursorCoords[1];
			}
			else return false;
		}
		return true;
	}
	else return false;
}
//The end handler needs to pop the parse stack, not much else
bool ProbeParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	
	if (StrCmpNoCase(tag, _probeParamsTag) == 0) {
		//Determine the first selected variable:
		firstVarNum = 0;
		int i;
		for (i = 0; i<numVariables; i++){
			if (variableSelected[i]){
				firstVarNum = i;
				break;
			}
		}
		if (i == numVariables) variableSelected[0] = true;
		
        if (numVariables)
        {
          transFunc[firstVarNum]->setVarNum(firstVarNum);
        }
		//Align the editor
		setMinEditBound(getMinColorMapBound());
		setMaxEditBound(getMaxColorMapBound());
		
		
		//If this is a probeparams, need to
		//pop the parse stack.  The caller will need to save the resulting
		//transfer function (i.e. this)
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0) {
		return true;
	} else if (StrCmpNoCase(tag, _variableTag) == 0){
		return true;
	} else if (StrCmpNoCase(tag, _geometryTag) == 0){
		return true;
	} else {
		pm->parseError("Unrecognized end tag in ProbeParams %s",tag.c_str());
		return false; 
	}
}

//Method to construct Xml for state saving
XmlNode* ProbeParams::
buildNode() {
	//Construct the probe node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	oss << (long)numVariables;
	attrs[_numVariablesAttr] = oss.str();

	oss.str(empty);
	oss << (long)numRefinements;
	attrs[_numTransformsAttr] = oss.str();

	oss.str(empty);
	if (editMode)
		oss << "true";
	else 
		oss << "false";
	attrs[_editModeAttr] = oss.str();

	oss.str(empty);
	if (planar)
		oss << "true";
	else 
		oss << "false";
	attrs[_planarAttr] = oss.str();

	oss.str(empty);
	oss << (double)getHistoStretch();
	attrs[_histoStretchAttr] = oss.str();
	
	XmlNode* probeNode = new XmlNode(_probeParamsTag, attrs, 3);

	//Now add children:  
	//Create the Variables nodes
	for (int i = 0; i<numVariables; i++){
		attrs.clear();
	

		oss.str(empty);
		oss << DataStatus::getInstance()->getVariableName(i);
		attrs[_variableNameAttr] = oss.str();

		oss.str(empty);
		if (variableSelected[i])
			oss << "true";
		else 
			oss << "false";
		attrs[_variableSelectedAttr] = oss.str();

		oss.str(empty);
		oss << (double)minColorEditBounds[i];
		attrs[_leftEditBoundAttr] = oss.str();

		oss.str(empty);
		oss << (double)maxColorEditBounds[i];
		attrs[_rightEditBoundAttr] = oss.str();

		oss.str(empty);
		oss << (double)transFunc[i]->getOpacityScaleFactor();
		attrs[_opacityScaleAttr] = oss.str();

		XmlNode* varNode = new XmlNode(_variableTag,attrs,1);

		//Create a transfer function node, add it as child
		XmlNode* tfNode = transFunc[i]->buildNode(empty);
		varNode->AddChild(tfNode);
		probeNode->AddChild(varNode);
	}
	//Now do geometry node:
	attrs.clear();
	oss.str(empty);
	oss << (double)phi;
	attrs[_phiAttr] = oss.str();
	oss.str(empty);
	oss << (double)psi;
	attrs[_psiAttr] = oss.str();
	oss.str(empty);
	oss << (double) theta;
	attrs[_thetaAttr] = oss.str();
	oss.str(empty);
	oss << (double)probeMin[0]<<" "<<(double)probeMin[1]<<" "<<(double)probeMin[2];
	attrs[_probeMinAttr] = oss.str();
	oss.str(empty);
	oss << (double)probeMax[0]<<" "<<(double)probeMax[1]<<" "<<(double)probeMax[2];
	attrs[_probeMaxAttr] = oss.str();
	oss.str(empty);
	oss << (double)cursorCoords[0]<<" "<<(double)cursorCoords[1];
	attrs[_cursorCoordsAttr] = oss.str();
	probeNode->NewChild(_geometryTag, attrs, 0);
	return probeNode;
}

MapperFunction* ProbeParams::getMapperFunc() {
	return (numVariables > 0 ? transFunc[firstVarNum] : 0);
}

void ProbeParams::setMinColorMapBound(float val){
	getMapperFunc()->setMinColorMapValue(val);
}
void ProbeParams::setMaxColorMapBound(float val){
	getMapperFunc()->setMaxColorMapValue(val);
}


void ProbeParams::setMinOpacMapBound(float val){
	getMapperFunc()->setMinOpacMapValue(val);
}
void ProbeParams::setMaxOpacMapBound(float val){
	getMapperFunc()->setMaxOpacMapValue(val);
}
void ProbeParams::getContainingRegion(float regMin[3], float regMax[3]){
	//Determine the smallest axis-aligned cube that contains the probe.  This is
	//obtained by mapping all 8 corners into the space.
	//Note that this is just a floating point version of getBoundingBox(), below.
	float transformMatrix[12];
	//Set up to transform from probe (coords [-1,1]) into volume:
	buildCoordTransform(transformMatrix, 0.f);
	const float* extents = DataStatus::getInstance()->getExtents();
	//Start by initializing extents, and variables that will be min,max
	for (int i = 0; i< 3; i++){
		regMin[i] = 1.e30f;
		regMax[i] = -1.e30f;
	}
	
	for (int corner = 0; corner< 8; corner++){
		int intCoord[3];
		float startVec[3], resultVec[3];
		intCoord[0] = corner%2;
		intCoord[1] = (corner/2)%2;
		intCoord[2] = (corner/4)%2;
		for (int i = 0; i<3; i++)
			startVec[i] = -1.f + (float)(2.f*intCoord[i]);
		// calculate the mapping of this corner,
		vtransform(startVec, transformMatrix, resultVec);
		// force mapped corner to lie in the full extents, and then force box to contain the corner:
		for (int i = 0; i<3; i++) {
			if (resultVec[i] < extents[i]) resultVec[i] = extents[i];
			if (resultVec[i] > extents[i+3]) resultVec[i] = extents[i+3];
			if (resultVec[i] < regMin[i]) regMin[i] = resultVec[i];
			if (resultVec[i] > regMax[i]) regMax[i] = resultVec[i];
		}
	}
	return;
}



//Find the smallest box containing the probe, in block coords, at current refinement level.
//We also need the actual box coords (min and max) to check for valid coords in the block.
//Note that the box region may be strictly smaller than the block region
//
void ProbeParams::getBoundingBox(size_t boxMin[3], size_t boxMax[3]){
	//Determine the smallest axis-aligned cube that contains the probe.  This is
	//obtained by mapping all 8 corners into the space
	
	
	float transformMatrix[12];
	//Set up to transform from probe into volume:
	buildCoordTransform(transformMatrix, 0.f);
	size_t dataSize[3];
	const float* extents = DataStatus::getInstance()->getExtents();
	//Start by initializing extents, and variables that will be min,max
	for (int i = 0; i< 3; i++){
		dataSize[i] = DataStatus::getInstance()->getFullSizeAtLevel(numRefinements,i);
		boxMin[i] = dataSize[i]-1;
		boxMax[i] = 0;
	}
	
	for (int corner = 0; corner< 8; corner++){
		int intCoord[3];
		int intResult[3];
		float startVec[3], resultVec[3];
		intCoord[0] = corner%2;
		intCoord[1] = (corner/2)%2;
		intCoord[2] = (corner/4)%2;
		for (int i = 0; i<3; i++)
			startVec[i] = -1.f + (float)(2.f*intCoord[i]);
		// calculate the mapping of this corner,
		vtransform(startVec, transformMatrix, resultVec);
		
		// then make sure the container includes it:
		for(int i = 0; i< 3; i++){
			intResult[i] = (int) (0.5f+(float)dataSize[i]*(resultVec[i]- extents[i])/(extents[i+3]-extents[i]));
			if (intResult[i]< 0) intResult[i] = 0;
			if((size_t)intResult[i]<boxMin[i]) boxMin[i] = intResult[i];
			if((size_t)intResult[i]>boxMax[i]) boxMax[i] = intResult[i];
		}
	}
	return;
}
//Find the smallest box containing the probe, in block coords, at current refinement level
//and current time step.  Restrict it to the available data.
//
bool ProbeParams::
getAvailableBoundingBox(int timeStep, size_t boxMinBlk[3], size_t boxMaxBlk[3], size_t boxMin[3], size_t boxMax[3]){
	//Start with the bounding box for this refinement level:
	getBoundingBox(boxMin, boxMax);
	
	const size_t* bs = DataStatus::getInstance()->getCurrentMetadata()->GetBlockSize();
	size_t temp_min[3],temp_max[3];
	bool retVal = true;
	int i;
	//Now intersect with available bounds based on variables:
	for (int varIndex = 0; varIndex < DataStatus::getInstance()->getNumSessionVariables(); varIndex++){
		if (!variableSelected[varIndex]) continue;
		if (DataStatus::getInstance()->maxXFormPresent(varIndex,timeStep) < numRefinements){
			retVal = false;
			continue;
		} else {
			const string varName = DataStatus::getInstance()->getVariableName(varIndex);
			int rc = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetValidRegion(timeStep, varName.c_str(),numRefinements, temp_min, temp_max);
			if (rc < 0) {
				retVal = false;
			}
		}
		if(retVal) for (i = 0; i< 3; i++){
			if (boxMin[i] < temp_min[i]) boxMin[i] = temp_min[i];
			if (boxMax[i] > temp_max[i]) boxMax[i] = temp_max[i];
		}
	}
	//Now do the block dimensions:
	for (int i = 0; i< 3; i++){
		size_t dataSize = DataStatus::getInstance()->getFullSizeAtLevel(numRefinements,i);
		if(boxMax[i] > dataSize-1) boxMax[i] = dataSize - 1;
		if (boxMin[i] > boxMax[i]) boxMax[i] = boxMin[i];
		//And make the block coords:
		boxMinBlk[i] = boxMin[i]/ (*bs);
		boxMaxBlk[i] = boxMax[i]/ (*bs);
	}
	
	return retVal;
}

//Find the smallest stretched extents containing the probe, 
//Similar to above, but using stretched extents
void ProbeParams::calcContainingStretchedBoxExtentsInCube(float* bigBoxExtents){
	if(!DataStatus::getInstance()) return;
	//Determine the smallest axis-aligned cube that contains the probe.  This is
	//obtained by mapping all 8 corners into the space.
	//It will not necessarily fit inside the unit cube.
	float corners[8][3];
	calcBoxCorners(corners, 0.f);
	
	float boxMin[3],boxMax[3];
	int crd, cor;
	
	//initialize extents, and variables that will be min,max
	for (crd = 0; crd< 3; crd++){
		boxMin[crd] = 1.e30f;
		boxMax[crd] = -1.e30f;
	}
	
	
	for (cor = 0; cor< 8; cor++){
		
		
		//make sure the container includes it:
		for(crd = 0; crd< 3; crd++){
			if (corners[cor][crd]<boxMin[crd]) boxMin[crd] = corners[cor][crd];
			if (corners[cor][crd]>boxMax[crd]) boxMax[crd] = corners[cor][crd];
		}
	}
	//Now convert the min,max back into extents in unit cube:
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	const float* fullExtents = DataStatus::getInstance()->getStretchedExtents();
	
	float maxSize = Max(Max(fullExtents[3]-fullExtents[0],fullExtents[4]-fullExtents[1]),fullExtents[5]-fullExtents[2]);
	for (crd = 0; crd<3; crd++){
		bigBoxExtents[crd] = (boxMin[crd]*stretch[crd] - fullExtents[crd])/maxSize;
		bigBoxExtents[crd+3] = (boxMax[crd]*stretch[crd] - fullExtents[crd])/maxSize;
	}
	return;
}
// Map the cursor coords into world space,
// refreshing the selected point.  CursorCoords go from -1 to 1
//
void ProbeParams::mapCursor(){
	//Get the transform matrix:
	float transformMatrix[12];
	float probeCoord[3];
	buildCoordTransform(transformMatrix, 0.f);
	//The cursor sits in the z=0 plane of the probe box coord system.
	//x is reversed because we are looking from the opposite direction (?)
	probeCoord[0] = -cursorCoords[0];
	probeCoord[1] = cursorCoords[1];
	probeCoord[2] = 0.f;
	
	vtransform(probeCoord, transformMatrix, selectPoint);
}
//Clear out the cache
void ProbeParams::setProbeDirty(){
	if (probeTextures){
		for (int i = 0; i<=maxTimestep; i++){
			if (probeTextures[i]) {
				delete probeTextures[i];
				probeTextures[i] = 0;
			}
		}
	}
	textureWidth = textureHeight = 0;
}

float* ProbeParams::
getContainingVolume(size_t blkMin[3], size_t blkMax[3], int sessionVarNum, int timeStep){
	//Get the region (int coords) associated with the specified variable at the
	//current probe extents
	int numRefinements = getNumRefinements();
	int maxRes = DataStatus::getInstance()->maxXFormPresent(sessionVarNum,timeStep);
	if (maxRes < numRefinements){
		const char* vname = DataStatus::getInstance()->getVariableName(sessionVarNum).c_str();
		MyBase::SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,
			"Probe data unavailable for refinement level %d of variable %s, at current timestep",
			numRefinements, vname);
		return 0;
	}
	
	float* reg = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetRegion((size_t)timeStep,
		DataStatus::getInstance()->getVariableName(sessionVarNum).c_str(),
		numRefinements, blkMin, blkMax, 0);
	
	return reg;
}
//Calculate the probe texture (if it needs refreshing).
//It's kept (cached) in the probe params
//If nonzero texture dimensions are provided, then the cached image
//is not affected 
unsigned char* ProbeParams::
calcProbeTexture(int ts, int texWidth, int texHeight){
	if (!isEnabled()) return 0;
	
	if (!DataStatus::getInstance()->getDataMgr()) return 0;

	bool doCache = (texWidth == 0 && texHeight == 0);
	
	float transformMatrix[12];
	
	//Set up to transform from probe into volume:
	buildCoordTransform(transformMatrix, 0.f);
	int numRefinements = getNumRefinements();
	//Get the data dimensions (at this resolution):
	int dataSize[3];
	//Start by initializing extents
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)DataStatus::getInstance()->getFullSizeAtLevel(numRefinements,i);
	}
	//Determine the integer extents of the containing cube, truncate to
	//valid integer coords:

	
	size_t blkMin[3], blkMax[3];
	size_t coordMin[3], coordMax[3];
	
	getAvailableBoundingBox(ts,blkMin, blkMax, coordMin, coordMax);
	int bSize =  *(DataStatus::getInstance()->getCurrentMetadata()->GetBlockSize());

	float boxExts[6];
	RegionParams::convertToStretchedBoxExtentsInCube(numRefinements,coordMin, coordMax,boxExts); 
	int numMBs = RegionParams::getMBStorageNeeded(boxExts, boxExts+3, numRefinements);
	//Check how many variables are needed:
	int varCount = 0;
	for (int varnum = 0; varnum < (int)DataStatus::getInstance()->getNumSessionVariables(); varnum++){
		if (variableIsSelected(varnum)) varCount++;
	}
	int cacheSize = DataStatus::getInstance()->getCacheMB();
	if (numMBs*varCount > (int)(cacheSize*0.75)){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small for current probe and resolution.\n%s \n%s",
			"Lower the refinement level, reduce the probe size, or increase the cache size.",
			"Rendering has been disabled.");
		setEnabled(false);
		return 0;
	}
	
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then do rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	//Now obtain all of the volumes needed for this probe:
	int totVars = 0;
	for (int varnum = 0; varnum < DataStatus::getInstance()->getNumSessionVariables(); varnum++){
		if (!variableIsSelected(varnum)) continue;
		volData[totVars] = getContainingVolume(blkMin, blkMax, varnum, ts);
		if (!volData[totVars]) {
			return 0;
		}
		totVars++;
	}
	//Now calculate the texture.
	//
	//For each pixel, map it into the volume.
	//The blkMin values tell you the offset to use.
	//The blkMax values tell how to stride through the data
	//The coordMin and coordMax values are used to check validity
	//We first map the coords in the probe to the volume.  
	//Then we map the volume into the region provided by dataMgr
	//This is done for each of the variables,
	//The RMS of the result is then mapped using the transfer function.
	float clut[256*4];
	TransferFunction* transFunc = getTransFunc();
	assert(transFunc);
	transFunc->makeLut(clut);
	
	float probeCoord[3];
	float dataCoord[3];
	size_t arrayCoord[3];
	const float* extents = DataStatus::getInstance()->getExtents();
	//Can ignore depth, just mapping center plane
	probeCoord[2] = 0.f;

	if (doCache) {  //Need to determine appropriate texture dimensions
		//First, map the corners of texture, to determine appropriate
		//texture sizes and image aspect ratio:
		int icor[4][3];
		
		for (int cornum = 0; cornum < 4; cornum++){
			// coords relative to (-1,1)
			probeCoord[1] = -1.f + 2.f*(float)(cornum/2);
			probeCoord[0] = -1.f + 2.f*(float)(cornum%2);
			//Then transform to values in data 
			vtransform(probeCoord, transformMatrix, dataCoord);
			//Then get array coords:
			for (int i = 0; i<3; i++){
				icor[cornum][i] = (size_t) (0.5f+((float)dataSize[i])*(dataCoord[i] - extents[i])/(extents[i+3]-extents[i]));
			}
		}
		//To get texture width, take distance in array coords, get first power of 2
		//that exceeds integer dist, but at least 64.
		int distsq = (icor[0][0]-icor[1][0])*(icor[0][0]-icor[1][0]) + 
			(icor[0][1]-icor[1][1])*(icor[0][1]-icor[1][1])+
			(icor[0][2]-icor[1][2])*(icor[0][2]-icor[1][2]);
		int xdist = (int)sqrt((float)distsq);
		distsq = (icor[0][0]-icor[2][0])*(icor[0][0]-icor[2][0]) + 
			(icor[0][1]-icor[2][1])*(icor[0][1]-icor[2][1]) +
			(icor[0][2]-icor[2][2])*(icor[0][2]-icor[2][2]);
		int ydist = (int)sqrt((float)distsq);
		textureWidth = 1<<(VetsUtil::ILog2(xdist));
		textureHeight = 1<<(VetsUtil::ILog2(ydist));
		if (textureWidth < 64) textureWidth = 64;
		if (textureHeight < 64) textureHeight = 64;
		texWidth = textureWidth;
		texHeight = textureHeight;
	}
	
	unsigned char* probeTexture = new unsigned char[texWidth*texHeight*4];


	//Loop over pixels in texture
	for (int iy = 0; iy < texHeight; iy++){
		//Map iy to a value between -1 and 1
		probeCoord[1] = -1.f + 2.f*(float)iy/(float)(texHeight-1);
		for (int ix = 0; ix < texWidth; ix++){
			
			//find the coords that the texture maps to
			//probeCoord is the coord in the probe, dataCoord is in data volume 
			probeCoord[0] = -1.f + 2.f*(float)ix/(float)(texWidth-1);
			vtransform(probeCoord, transformMatrix, dataCoord);
			for (int i = 0; i<3; i++){
				arrayCoord[i] = (size_t) (0.5f+((float)dataSize[i])*(dataCoord[i] - extents[i])/(extents[i+3]-extents[i]));
			}
			
			//Make sure the transformed coords are in the probe region
			//This should only fail if they aren't even in the full volume.
			bool dataOK = true;
			for (int i = 0; i< 3; i++){
				if (dataCoord[i]< extents[i] || dataCoord[i] > extents[i+3])
					dataOK = false;
				if (arrayCoord[i] < coordMin[i] ||
					arrayCoord[i] > coordMax[i] ) {
						dataOK = false;
					} //outside; color it black
			}
			
			if(dataOK) { //find the coordinate in the data array
				int xyzCoord = (arrayCoord[0] - blkMin[0]*bSize) +
					(arrayCoord[1] - blkMin[1]*bSize)*(bSize*(blkMax[0]-blkMin[0]+1)) +
					(arrayCoord[2] - blkMin[2]*bSize)*(bSize*(blkMax[1]-blkMin[1]+1))*(bSize*(blkMax[0]-blkMin[0]+1));
				float varVal;
				

				//use the intDataCoords to index into the loaded data
				if (totVars == 1) varVal = volData[0][xyzCoord];
				else { //Add up the squares of the variables
					varVal = 0.f;
					for (int k = 0; k<totVars; k++){
						varVal += volData[k][xyzCoord]*volData[k][xyzCoord];
					}
					varVal = sqrt(varVal);
				}
				
				//Use the transfer function to map the data:
				int lutIndex = transFunc->mapFloatToIndex(varVal);
				
				probeTexture[4*(ix+texWidth*iy)] = (unsigned char)(0.5+ clut[4*lutIndex]*255.f);
				probeTexture[4*(ix+texWidth*iy)+1] = (unsigned char)(0.5+ clut[4*lutIndex+1]*255.f);
				probeTexture[4*(ix+texWidth*iy)+2] = (unsigned char)(0.5+ clut[4*lutIndex+2]*255.f);
				probeTexture[4*(ix+texWidth*iy)+3] = (unsigned char)(0.5+ clut[4*lutIndex+3]*255.f);
				
			}
			else {//point out of region
				probeTexture[4*(ix+texWidth*iy)] = 0;
				probeTexture[4*(ix+texWidth*iy)+1] = 0;
				probeTexture[4*(ix+texWidth*iy)+2] = 0;
				probeTexture[4*(ix+texWidth*iy)+3] = 0;
			}

		}//End loop over ix
	}//End loop over iy
	
	if (doCache) setProbeTexture(probeTexture,ts);
	return probeTexture;
}


