//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Implementation of the twoDparams class
//		This contains all the parameters required to support the
//		twoD renderer.  Embeds a transfer function and a
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
#include "twoDparams.h"
#include "regionparams.h"
#include "params.h"
#include "transferfunction.h"

#include "histo.h"
#include "animationparams.h"

#include <math.h>
#include "vapor/Metadata.h"
#include "vapor/DataMgr.h"
#include "vapor/VDFIOBase.h"
#include "vapor/LayeredIO.h"
#include "vapor/errorcodes.h"


using namespace VAPoR;
const string TwoDParams::_editModeAttr = "TFEditMode";
const string TwoDParams::_histoStretchAttr = "HistoStretchFactor";
const string TwoDParams::_variableSelectedAttr = "VariableSelected";
const string TwoDParams::_geometryTag = "TwoDGeometry";
const string TwoDParams::_twoDMinAttr = "TwoDMin";
const string TwoDParams::_twoDMaxAttr = "TwoDMax";
const string TwoDParams::_cursorCoordsAttr = "CursorCoords";
const string TwoDParams::_terrainMapAttr = "MapToTerrain";
const string TwoDParams::_verticalDisplacementAttr = "VerticalDisplacement";

const string TwoDParams::_numTransformsAttr = "NumTransforms";

TwoDParams::TwoDParams(int winnum) : RenderParams(winnum){
	thisParamType = TwoDParamsType;
	numVariables = 0;
	twoDDataTextures = 0;
	maxTimestep = 1;
	restart();
	
}
TwoDParams::~TwoDParams(){
	
	if (transFunc){
		for (int i = 0; i< numVariables; i++){
			delete transFunc[i];  //will delete editor
		}
		delete transFunc;
	}
	if (twoDDataTextures) {
		for (int i = 0; i<= maxTimestep; i++){
			if (twoDDataTextures[i]) delete twoDDataTextures[i];
		}
		delete twoDDataTextures;
	}
	
	
}


//Deepcopy requires cloning tf 
RenderParams* TwoDParams::
deepRCopy(){
	TwoDParams* newParams = new TwoDParams(*this);
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
	//TwoD texture must be recreated when needed
	newParams->twoDDataTextures = 0;
	
	//never keep the SavedCommand:
	
	return newParams;
}


void TwoDParams::
refreshCtab() {
	((TransferFunction*)getMapperFunc())->makeLut((float*)ctab);
}
	



void TwoDParams::
setClut(const float newTable[256][4]){
	for (int i = 0; i< 256; i++) {
		for (int j = 0; j< 4; j++){
			ctab[i][j] = newTable[i][j];
		}
	}
}


float TwoDParams::getOpacityScale() 
{
  if (numVariables)
  {
    return transFunc[firstVarNum]->getOpacityScaleFactor();
  }

  return 1.0;
}

void TwoDParams::setOpacityScale(float val) 
{
  if (numVariables)
  {
    return transFunc[firstVarNum]->setOpacityScaleFactor(val);
  }
}


//Initialize for new metadata.  Keep old transfer functions
//
bool TwoDParams::
reinit(bool doOverride){
	int i;
	
	const float* extents = DataStatus::getInstance()->getExtents();
	setMaxNumRefinements(DataStatus::getInstance()->getNumTransforms());
	//Set up the numRefinements combo
	
	
	//Either set the twoD bounds to a default size in the center of the domain, or 
	//try to use the previous bounds:
	if (doOverride){
		for (int i = 0; i<3; i++){
			float twoDRadius = 0.1f*(extents[i+3] - extents[i]);
			float twoDMid = 0.5f*(extents[i+3] + extents[i]);
			if (i<2) {
				twoDMin[i] = twoDMid - twoDRadius;
				twoDMax[i] = twoDMid + twoDRadius;
			} else {
				twoDMin[i] = twoDMax[i] = twoDMid;
			}
		}
		
		cursorCoords[0] = cursorCoords[1] = 0.0f;
		numRefinements = 0;
	} else {
		//Force the twoD size to be no larger than the domain extents, and 
		//force the twoD center to be inside the domain.  Note that
		//because of rotation, the twoD max/min may not correspond
		//to the same extents.
		float maxExtents = Max(Max(extents[3]-extents[0],extents[4]-extents[1]),extents[5]-extents[2]);
		for (i = 0; i<3; i++){
			if (twoDMax[i] - twoDMin[i] > maxExtents)
				twoDMax[i] = twoDMin[i] + maxExtents;
			float center = 0.5f*(twoDMin[i]+twoDMax[i]);
			if (center < extents[i]) {
				twoDMin[i] += (extents[i]-center);
				twoDMax[i] += (extents[i]-center);
			}
			if (center > extents[i+3]) {
				twoDMin[i] += (extents[i+3]-center);
				twoDMax[i] += (extents[i+3]-center);
			}
			if(twoDMax[i] < twoDMin[i]) 
				twoDMax[i] = twoDMin[i];
		}
		if (numRefinements > maxNumRefinements) numRefinements = maxNumRefinements;
	}
	//Get the variable names:

	int newNumVariables = DataStatus::getInstance()->getNumSessionVariables2D();
	
	//See if current firstVarNum is valid
	//if not, reset to first 2D variable that is present:
	if (!DataStatus::getInstance()->variableIsPresent2D(firstVarNum)){
		firstVarNum = -1;
		for (i = 0; i<newNumVariables; i++) {
			if (DataStatus::getInstance()->variableIsPresent2D(i)){
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
		transFunc = 0;
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
				!DataStatus::getInstance()->variableIsPresent2D(i))
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

			newTransFunc[i]->setMinMapValue(DataStatus::getInstance()->getDefaultDataMin2D(i));
			newTransFunc[i]->setMaxMapValue(DataStatus::getInstance()->getDefaultDataMax2D(i));
			newMinEdit[i] = DataStatus::getInstance()->getDefaultDataMin2D(i);
			newMaxEdit[i] = DataStatus::getInstance()->getDefaultDataMax2D(i);

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

				newTransFunc[i]->setMinMapValue(DataStatus::getInstance()->getDefaultDataMin2D(i));
				newTransFunc[i]->setMaxMapValue(DataStatus::getInstance()->getDefaultDataMax2D(i));
				newMinEdit[i] = DataStatus::getInstance()->getDefaultDataMin2D(i);
				newMaxEdit[i] = DataStatus::getInstance()->getDefaultDataMax2D(i);
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
			newMinEdit[i] = DataStatus::getInstance()->getDefaultDataMin2D(i);
			newMaxEdit[i] = DataStatus::getInstance()->getDefaultDataMax2D(i);
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
	transFunc = 0;
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
	setTwoDDirty();
	if (twoDDataTextures) delete twoDDataTextures;
	
	maxTimestep = DataStatus::getInstance()->getMaxTimestep();
	twoDDataTextures = 0;

	return true;
}
//Initialize to default state
//
void TwoDParams::
restart(){
	
	verticalDisplacement = 0.f;
	mapToTerrain = false;

	textureSize[0] = textureSize[1] = 0;
	histoStretchFactor = 1.f;
	firstVarNum = 0;
	setTwoDDirty();
	if (twoDDataTextures) delete twoDDataTextures;
	twoDDataTextures = 0;
	
	
	
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
	
	for (int i = 0; i<3; i++){
		if (i < 2) twoDMin[i] = 0.4f;
		else twoDMin[i] = 0.5f;
		if(i<2) twoDMax[i] = 0.6f;
		else twoDMax[i] = 0.5f;
		selectPoint[i] = 0.5f;
	}
	
	
}



//Hook up the new transfer function in specified slot,
//Delete the old one.  This is called whenever a new tf is loaded.
//
void TwoDParams::
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
bool TwoDParams::
elementStartHandler(ExpatParseMgr* pm, int depth , std::string& tagString, const char **attrs){
	
	static int parsedVarNum = -1;
	int i;
	if (StrCmpNoCase(tagString, _twoDParamsTag) == 0) {
		
		int newNumVariables = 0;
		//If it's a TwoD tag, obtain 12 attributes (2 are from Params class)
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
			else if (StrCmpNoCase(attribName, _terrainMapAttr) == 0){
				if (value == "true") setMappedToTerrain(true); 
				else setMappedToTerrain(false);
			}
			else if (StrCmpNoCase(attribName, _verticalDisplacementAttr) == 0){
				ist >> verticalDisplacement;
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
	
		parsedVarNum = DataStatus::getInstance()->mergeVariableName2D(parsedVarName);
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
		//That parser will "pop" back to twoDparams when done.
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
			if (StrCmpNoCase(attribName, _twoDMinAttr) == 0) {
				ist >> twoDMin[0];ist >> twoDMin[1];ist >> twoDMin[2];
			}
			else if (StrCmpNoCase(attribName, _twoDMaxAttr) == 0) {
				ist >> twoDMax[0];ist >> twoDMax[1];ist >> twoDMax[2];
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
bool TwoDParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	
	if (StrCmpNoCase(tag, _twoDParamsTag) == 0) {
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
		
		
		//If this is a twoDparams, need to
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
		pm->parseError("Unrecognized end tag in TwoDParams %s",tag.c_str());
		return false; 
	}
}

//Method to construct Xml for state saving
XmlNode* TwoDParams::
buildNode() {
	//Construct the twoD node
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
	oss << (double)GetHistoStretch();
	attrs[_histoStretchAttr] = oss.str();

	oss.str(empty);
	oss << (double)getVerticalDisplacement();
	attrs[_verticalDisplacementAttr] = oss.str();

	oss.str(empty);
	if (isMappedToTerrain())
		oss << "true";
	else 
		oss << "false";
	attrs[_terrainMapAttr] = oss.str();


	XmlNode* twoDNode = new XmlNode(_twoDParamsTag, attrs, 3);

	//Now add children:  
	//Create the Variables nodes
	for (int i = 0; i<numVariables; i++){
		attrs.clear();
	

		oss.str(empty);
		oss << DataStatus::getInstance()->getVariableName2D(i);
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
		twoDNode->AddChild(varNode);
	}
	//Now do geometry node:
	attrs.clear();
	oss.str(empty);
	oss << (double)twoDMin[0]<<" "<<(double)twoDMin[1]<<" "<<(double)twoDMin[2];
	attrs[_twoDMinAttr] = oss.str();
	oss.str(empty);
	oss << (double)twoDMax[0]<<" "<<(double)twoDMax[1]<<" "<<(double)twoDMax[2];
	attrs[_twoDMaxAttr] = oss.str();
	oss.str(empty);
	oss << (double)cursorCoords[0]<<" "<<(double)cursorCoords[1];
	attrs[_cursorCoordsAttr] = oss.str();
	twoDNode->NewChild(_geometryTag, attrs, 0);
	return twoDNode;
}

MapperFunction* TwoDParams::getMapperFunc() {
	return (numVariables > 0 ? transFunc[firstVarNum] : 0);
}

void TwoDParams::setMinColorMapBound(float val){
	getMapperFunc()->setMinColorMapValue(val);
}
void TwoDParams::setMaxColorMapBound(float val){
	getMapperFunc()->setMaxColorMapValue(val);
}


void TwoDParams::setMinOpacMapBound(float val){
	getMapperFunc()->setMinOpacMapValue(val);
}
void TwoDParams::setMaxOpacMapBound(float val){
	getMapperFunc()->setMaxOpacMapValue(val);
}
void TwoDParams::getContainingRegion(float regMin[3], float regMax[3]){
	//Determine the smallest axis-aligned cube that contains the twoD.  This is
	//obtained by mapping all 8 corners into the space.
	//Note that this is just a floating point version of getBoundingBox(), below.
	float transformMatrix[12];
	//Set up to transform from twoD (coords [-1,1]) into volume:
	buildCoordTransform(transformMatrix, 0.f);
	const float* extents = DataStatus::getInstance()->getExtents();

	//Calculate the normal vector to the twoD plane:
	float zdir[3] = {0.f,0.f,1.f};
	float normEnd[3];  //This will be the unit normal
	float normBeg[3];
	float zeroVec[3] = {0.f,0.f,0.f};
	vtransform(zdir, transformMatrix, normEnd);
	vtransform(zeroVec,transformMatrix,normBeg);
	vsub(normEnd,normBeg,normEnd);
	vnormal(normEnd);

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



//Find the smallest box containing the twoD, in block coords, at current refinement level.
//We also need the actual box coords (min and max) to check for valid coords in the block.
//Note that the box region may be strictly smaller than the block region
//
void TwoDParams::getBoundingBox(int timestep, size_t boxMin[3], size_t boxMax[3], int numRefs){
	//Determine the smallest axis-aligned cube that contains the twoD.  This is
	//obtained by mapping all 8 corners into the space
	DataStatus* ds = DataStatus::getInstance();
	
	float transformMatrix[12];
	//Set up to transform from twoD into volume:
	buildCoordTransform(transformMatrix, 0.f);
	size_t dataSize[3];
	const float* extents = ds->getExtents();
	//Start by initializing extents, and variables that will be min,max
	for (int i = 0; i< 3; i++){
		dataSize[i] = ds->getFullSizeAtLevel(numRefs,i);
		boxMin[i] = dataSize[i]-1;
		boxMax[i] = 0;
	}
	//Get the regionReader to map coordinates:
	//Use the region reader to calculate coordinates in volume
	const VDFIOBase* myReader = ds->getRegionReader();
	if (ds->dataIsLayered()){
		RegionParams::setFullGridHeight(RegionParams::getFullGridHeight());
	}

	
	for (int corner = 0; corner< 8; corner++){
		int intCoord[3];
		size_t intResult[3];
		float startVec[3]; 
		double resultVec[3];
		intCoord[0] = corner%2;
		intCoord[1] = (corner/2)%2;
		intCoord[2] = (corner/4)%2;
		for (int i = 0; i<3; i++)
			startVec[i] = -1.f + (float)(2.f*intCoord[i]);
		// calculate the mapping of this corner,
		vtransform(startVec, transformMatrix, resultVec);
		//Force the result to lie inside full domain:
		for (int i = 0; i<3; i++){
			if (resultVec[i] < extents[i]) resultVec[i] = extents[i];
			if (resultVec[i] > extents[i+3]) resultVec[i] = extents[i+3];
		}
		myReader->MapUserToVox(timestep, resultVec, intResult, numRefs);
		// then make sure the container includes it:
		for(int i = 0; i< 3; i++){
			if(intResult[i]<boxMin[i]) boxMin[i] = intResult[i];
			if(intResult[i]>boxMax[i]) boxMax[i] = intResult[i];
		}
	}
	return;
}
//Find the smallest box containing the twoD, in block coords, at current refinement level
//and current time step.  Restrict it to the available data.
//
bool TwoDParams::
getAvailableBoundingBox(int timeStep, size_t boxMinBlk[3], size_t boxMaxBlk[3], 
		size_t boxMin[3], size_t boxMax[3], int numRefs){
	
	//Start with the bounding box for this refinement level:
	getBoundingBox(timeStep, boxMin, boxMax, numRefs);
	
	const size_t* bs = DataStatus::getInstance()->getCurrentMetadata()->GetBlockSize();
	size_t temp_min[3],temp_max[3];
	bool retVal = true;
	int i;
	//Now intersect with available bounds based on variables:
	for (int varIndex = 0; varIndex < DataStatus::getInstance()->getNumSessionVariables2D(); varIndex++){
		if (!variableSelected[varIndex]) continue;
		if (DataStatus::getInstance()->maxXFormPresent2D(varIndex,timeStep) < numRefs){
			retVal = false;
			continue;
		} else {
			const string varName = DataStatus::getInstance()->getVariableName2D(varIndex);
			int rc = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetValidRegion(timeStep, varName.c_str(),numRefs, temp_min, temp_max);
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
		size_t dataSize = DataStatus::getInstance()->getFullSizeAtLevel(numRefs,i);
		if(boxMax[i] > dataSize-1) boxMax[i] = dataSize - 1;
		if (boxMin[i] > boxMax[i]) boxMax[i] = boxMin[i];
		//And make the block coords:
		boxMinBlk[i] = boxMin[i]/ (*bs);
		boxMaxBlk[i] = boxMax[i]/ (*bs);
	}
	
	return retVal;
}

//Find the smallest stretched extents containing the twoD, 
//Similar to above, but using stretched extents
void TwoDParams::calcContainingStretchedBoxExtentsInCube(float* bigBoxExtents){
	if(!DataStatus::getInstance()) return;
	//Determine the smallest axis-aligned cube that contains the twoD.  This is
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
void TwoDParams::mapCursor(){
	//Get the transform matrix:
	float transformMatrix[12];
	float twoDCoord[3];
	buildCoordTransform(transformMatrix, 0.f);
	//The cursor sits in the z=0 plane of the twoD box coord system.
	//x is reversed because we are looking from the opposite direction (?)
	twoDCoord[0] = -cursorCoords[0];
	twoDCoord[1] = cursorCoords[1];
	twoDCoord[2] = 0.f;
	
	vtransform(twoDCoord, transformMatrix, selectPoint);
}
//Clear out the cache
void TwoDParams::setTwoDDirty(){
	if (twoDDataTextures){
		for (int i = 0; i<=maxTimestep; i++){
			if (twoDDataTextures[i]) {
				delete twoDDataTextures[i];
				twoDDataTextures[i] = 0;
			}
		}
	}
	
	textureSize[0]= textureSize[1] = 0;

}


//Calculate the twoD texture (if it needs refreshing).
//It's kept (cached) in the twoD params
//If nonzero texture dimensions are provided, then the cached image
//is not affected 
unsigned char* TwoDParams::
calcTwoDDataTexture(int ts, int texWidth, int texHeight){
	size_t blkMin[3], blkMax[3];
	size_t coordMin[3], coordMax[3];
	if (!isEnabled()) return 0;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return 0;

	bool doCache = (texWidth == 0 && texHeight == 0);

	//Make a list of the session variable nums we want:
	int* sesVarNums = new int[ds->getNumSessionVariables2D()];
	int actualRefLevel; 
	int numVars = 0;
	for (int varnum = 0; varnum < ds->getNumSessionVariables2D(); varnum++){
		if (!variableIsSelected(varnum)) continue;
		sesVarNums[numVars++] = varnum;
	}
	
	const size_t *bSize =  ds->getCurrentMetadata()->GetBlockSize();
	
	float** volData = getTwoDVariables(ts,  numVars, sesVarNums,
				  blkMin, blkMax, coordMin, coordMax, &actualRefLevel);

	if(!volData){
		delete sesVarNums;
		return 0;
	}

	float transformMatrix[12];
	//Set up to transform from twoD into volume:
	buildCoordTransform(transformMatrix, 0.f);

	//Get the data dimensions (at this resolution):
	int dataSize[3];
	//Start by initializing extents
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(actualRefLevel,i);
	}
	//Now calculate the texture.
	//
	//For each pixel, map it into the volume.
	//The blkMin values tell you the offset to use.
	//The blkMax values tell how to stride through the data
	
	//We first map the coords in the twoD to the volume.  
	//Then we map the volume into the region provided by dataMgr
	//This is done for each of the variables,
	//The RMS of the result is then mapped using the transfer function.
	float clut[256*4];
	TransferFunction* transFunc = getTransFunc();
	assert(transFunc);
	transFunc->makeLut(clut);
	
	float twoDCoord[3];
	double dataCoord[3];
	size_t arrayCoord[3];
	const float* extents = ds->getExtents();
	float extExtents[6]; //Extend extents 1/2 voxel on each side so no bdry issues.
	for (int i = 0; i<3; i++){
		float mid = (extents[i]+extents[i+3])*0.5;
		float halfExtendedSize = (extents[i+3]-extents[i])*0.5*(1.f+dataSize[i])/(float)(dataSize[i]);
		extExtents[i] = mid - halfExtendedSize;
		extExtents[i+3] = mid + halfExtendedSize;
	}
	//Can ignore depth, just mapping center plane
	twoDCoord[2] = 0.f;
	for (int cornum = 0; cornum < 4; cornum++){
		// coords relative to (-1,1)
		twoDCoord[1] = -1.f + 2.f*(float)(cornum/2);
		twoDCoord[0] = -1.f + 2.f*(float)(cornum%2);
		//Then transform to values in data 
		vtransform(twoDCoord, transformMatrix, dataCoord);
		
	}
	
	if (doCache) {
		int txsize[2];
		adjustTextureSize(txsize);
		texWidth = txsize[0];
		texHeight = txsize[1];
	}
	
	
	unsigned char* twoDTexture = new unsigned char[texWidth*texHeight*4];

	//Use the region reader to calculate coordinates in volume
	const VDFIOBase* myReader = ds->getRegionReader();

	if (ds->dataIsLayered()){
		RegionParams::setFullGridHeight(RegionParams::getFullGridHeight());
	}


	//Loop over pixels in texture.  Pixel centers map to edges of twoD
	
	for (int iy = 0; iy < texHeight; iy++){
		//Map iy to a value between -1 and 1
		twoDCoord[1] = -1.f + 2.f*(float)iy/(float)(texHeight-1);
		for (int ix = 0; ix < texWidth; ix++){
			
			
			twoDCoord[0] = -1.f + 2.f*(float)ix/(float)(texWidth-1);
			vtransform(twoDCoord, transformMatrix, dataCoord);
			//find the coords that the texture maps to
			//twoDCoord is the coord in the twoD, dataCoord is in data volume 
			myReader->MapUserToVox(ts, dataCoord, arrayCoord, actualRefLevel);
			bool dataOK = true;
			for (int i = 0; i< 3; i++){
				if (dataCoord[i] < extExtents[i] || dataCoord[i] > extExtents[i+3]) dataOK = false;
			}
			
			if(dataOK) { //find the coordinate in the data array
				
				int xyzCoord = (arrayCoord[0] - blkMin[0]*bSize[0]) +
					(arrayCoord[1] - blkMin[1]*bSize[1])*(bSize[0]*(blkMax[0]-blkMin[0]+1)) +
					(arrayCoord[2] - blkMin[2]*bSize[2])*(bSize[1]*(blkMax[1]-blkMin[1]+1))*(bSize[0]*(blkMax[0]-blkMin[0]+1));
				float varVal;
				assert(xyzCoord >= 0);

				//use the intDataCoords to index into the loaded data
				if (numVars == 1) varVal = volData[0][xyzCoord]; 
				else { //Add up the squares of the variables
					varVal = 0.f;
					for (int k = 0; k<numVars; k++){
						varVal += volData[k][xyzCoord]*volData[k][xyzCoord];
					}
					varVal = sqrt(varVal);
				}
				
				//Use the transfer function to map the data:
				int lutIndex = transFunc->mapFloatToIndex(varVal);
				
				twoDTexture[4*(ix+texWidth*iy)] = (unsigned char)(0.5+ clut[4*lutIndex]*255.f);
				twoDTexture[4*(ix+texWidth*iy)+1] = (unsigned char)(0.5+ clut[4*lutIndex+1]*255.f);
				twoDTexture[4*(ix+texWidth*iy)+2] = (unsigned char)(0.5+ clut[4*lutIndex+2]*255.f);
				twoDTexture[4*(ix+texWidth*iy)+3] = (unsigned char)(0.5+ clut[4*lutIndex+3]*255.f);
				
			}
			else {//point out of region
				twoDTexture[4*(ix+texWidth*iy)] = 0;
				twoDTexture[4*(ix+texWidth*iy)+1] = 0;
				twoDTexture[4*(ix+texWidth*iy)+2] = 0;
				twoDTexture[4*(ix+texWidth*iy)+3] = 0;
			}

		}//End loop over ix
	}//End loop over iy
	
	if (doCache) setTwoDTexture(twoDTexture,ts);
	delete volData;
	delete sesVarNums;
	return twoDTexture;
}
void TwoDParams::adjustTextureSize(int sz[2]){
	//Need to determine appropriate texture dimensions
	
	//First, map the corners of texture, to determine appropriate
	//texture sizes and image aspect ratio:
	//Get the data dimensions (at this resolution):
	int dataSize[3];
	//Start by initializing extents
	DataStatus* ds = DataStatus::getInstance();
	int refLevel = getNumRefinements();
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(refLevel,i);
	}
	int icor[4][3];
	float twoDCoord[3];
	float dataCoord[3];
	
	
	const float* extents = ds->getExtents();
	//Can ignore depth, just mapping center plane
	twoDCoord[2] = 0.f;

	float transformMatrix[12];
	
	//Set up to transform from twoD into volume:
	buildCoordTransform(transformMatrix, 0.f);
	
	for (int cornum = 0; cornum < 4; cornum++){
		// coords relative to (-1,1)
		twoDCoord[1] = -1.f + 2.f*(float)(cornum/2);
		twoDCoord[0] = -1.f + 2.f*(float)(cornum%2);
		//Then transform to values in data 
		vtransform(twoDCoord, transformMatrix, dataCoord);
		//Then get array coords:
		for (int i = 0; i<3; i++){
			icor[cornum][i] = (size_t) (0.5f+((float)dataSize[i])*(dataCoord[i] - extents[i])/(extents[i+3]-extents[i]));
		}
	}
	//To get texture width, take distance in array coords, get first power of 2
	//that exceeds integer dist, but at least 256.
	int distsq = (icor[0][0]-icor[1][0])*(icor[0][0]-icor[1][0]) + 
		(icor[0][1]-icor[1][1])*(icor[0][1]-icor[1][1])+
		(icor[0][2]-icor[1][2])*(icor[0][2]-icor[1][2]);
	int xdist = (int)sqrt((float)distsq);
	distsq = (icor[0][0]-icor[2][0])*(icor[0][0]-icor[2][0]) + 
		(icor[0][1]-icor[2][1])*(icor[0][1]-icor[2][1]) +
		(icor[0][2]-icor[2][2])*(icor[0][2]-icor[2][2]);
	int ydist = (int)sqrt((float)distsq);
	textureSize[0] = 1<<(VetsUtil::ILog2(xdist));
	textureSize[1] = 1<<(VetsUtil::ILog2(ydist));
	if (textureSize[0] < 256) textureSize[0] = 256;
	if (textureSize[1] < 256) textureSize[1] = 256;
	sz[0] = textureSize[0];
	sz[1] = textureSize[1];
	
	
}
//Determine the voxel extents of twoD mapped into data.
//Similar code is in calcTwoDTexture()
void TwoDParams::
getTwoDVoxelExtents(float voxdims[2]){
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) {
		voxdims[0] = voxdims[1] = 1.f;
		return;
	}
	const float* extents = DataStatus::getInstance()->getExtents();
	float twoDCoord[3];
	//Can ignore depth, just mapping center plane
	twoDCoord[2] = 0.f;
	float transformMatrix[12];
	
	//Set up to transform from twoD into volume:
	buildCoordTransform(transformMatrix, 0.f);
	
	//Get the data dimensions (at this resolution):
	int dataSize[3];
	//Start by initializing integer extents
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)DataStatus::getInstance()->getFullSizeAtLevel(numRefinements,i);
	}
	
	float cor[4][3];
		
	for (int cornum = 0; cornum < 4; cornum++){
		float dataCoord[3];
		// coords relative to (-1,1)
		twoDCoord[1] = -1.f + 2.f*(float)(cornum/2);
		twoDCoord[0] = -1.f + 2.f*(float)(cornum%2);
		//Then transform to values in data 
		vtransform(twoDCoord, transformMatrix, dataCoord);
		//Then get array coords:
		for (int i = 0; i<3; i++){
			cor[cornum][i] = ((float)dataSize[i])*(dataCoord[i] - extents[i])/(extents[i+3]-extents[i]);
		}
	}
	float vecWid[3], vecHt[3];
	
	vsub(cor[1],cor[0], vecWid);
	vsub(cor[3],cor[1], vecHt);
	voxdims[0] = vlength(vecWid);
	voxdims[1] = vlength(vecHt);
	return;
}
// Determine the length of the box sides, when rotated and stretched and put into
// unit cube
void TwoDParams::getRotatedBoxDims(float boxdims[3]){
	float corners[8][3];
	//Find all 8 corners but we will only use 4:
	//cor(1)-cor[0] is x side
	//cor[2] - cor[0] is y side
	//cor[4] - cor[0] is z side
	calcBoxCorners(corners, 0.f);
	
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	const float* fullExtents = DataStatus::getInstance()->getStretchedExtents();
	
	float maxSize = Max(Max(fullExtents[3]-fullExtents[0],fullExtents[4]-fullExtents[1]),fullExtents[5]-fullExtents[2]);
	//Obtain the 4 corners needed (0,1,2,4) in the unit cube
	float xformCors[4][3];
	for (int cornum = 0; cornum<4; cornum++){
		int corIndx = cornum;
		if (corIndx == 3) corIndx = 4;
		for (int crd = 0; crd<3; crd++){
			xformCors[cornum][crd] = (corners[corIndx][crd]*stretch[crd] - fullExtents[crd])/maxSize;
		}
	}
	//Now find the actual length (dist between corner 1,2,4 and 0):
	float distvec[3];
	for (int dim = 0; dim <3; dim++){
		for (int crd = 0; crd < 3; crd++){
			distvec[crd] = xformCors[dim+1][crd] - xformCors[0][crd];
		}
		boxdims[dim] = vlength(distvec);
	}
}


//General routine that obtains 1 or more variables from the cache in the smallest volume that
//contains the twoD.  sesVarNums is a list of session variable numbers to be obtained.
//If the variable num is negative, just returns a null data array.
//Note that this is a generalization of the first part of calcTwoDDataTexture
//Provides several variables to be used in adressing into the data

float** TwoDParams::
getTwoDVariables(int ts,  int numVars, int* sesVarNums,
				  size_t blkMin[3], size_t blkMax[3], size_t coordMin[3], size_t coordMax[3],
				  int* actualRefLevel){
	if (!isEnabled()) return 0;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return 0;
	
	int refLevel = getNumRefinements();
	//reduce reflevel if not all variables are available:
	if (ds->useLowerRefinementLevel()){
		for (int varnum = 0; varnum < numVars; varnum++){
			int sesVarNum = sesVarNums[varnum];
			if (sesVarNum >= 0){
				refLevel = Min(ds->maxXFormPresent2D(sesVarNum, ts), refLevel);
			}
		}
	}
	if (refLevel < 0) return 0;
	*actualRefLevel = refLevel;
	
	//Determine the integer extents of the containing cube, truncate to
	//valid integer coords:

	getAvailableBoundingBox(ts, blkMin, blkMax, coordMin, coordMax, refLevel);
	
	float boxExts[6];
	RegionParams::convertToStretchedBoxExtentsInCube(refLevel,coordMin, coordMax,boxExts); 
	int numMBs = RegionParams::getMBStorageNeeded(boxExts, boxExts+3, refLevel);
	
	int cacheSize = DataStatus::getInstance()->getCacheMB();
	if (numMBs*numVars > (int)(cacheSize*0.75)){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small for current twoD and resolution.\n%s \n%s",
			"Lower the refinement level, reduce the twoD size, or increase the cache size.",
			"Rendering has been disabled.");
		setEnabled(false);
		return 0;
	}
	
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable 
	float** volData = new float*[numVars];
	//obtain all of the volumes needed for this twoD:
	
	for (int varnum = 0; varnum < numVars; varnum++){
		int varindex = sesVarNums[varnum];
		if (varindex < 0) {volData[varnum] = 0; continue;}   //handle the zero field as a 0 pointer
		volData[varnum] = getContainingVolume(blkMin, blkMax, refLevel, varindex, ts);
		if (!volData[varnum]) {
			delete volData;
			return 0;
		}
	}

	return volData;
}




