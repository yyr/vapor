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
	DataStatus* ds = DataStatus::getInstance();
	const float* extents = ds->getExtents();
	setMaxNumRefinements(ds->getNumTransforms());
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

	int newNumVariables = ds->getNumSessionVariables2D();
	
	//See if current firstVarNum is valid
	//if not, reset to first 2D variable that is present:
	if (!ds->variableIsPresent2D(firstVarNum)){
		firstVarNum = -1;
		for (i = 0; i<newNumVariables; i++) {
			if (ds->variableIsPresent2D(i)){
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
				!ds->variableIsPresent2D(i))
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

			newTransFunc[i]->setMinMapValue(ds->getDefaultDataMin2D(i));
			newTransFunc[i]->setMaxMapValue(ds->getDefaultDataMax2D(i));
			newMinEdit[i] = ds->getDefaultDataMin2D(i);
			newMaxEdit[i] = ds->getDefaultDataMax2D(i);

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

				newTransFunc[i]->setMinMapValue(ds->getDefaultDataMin2D(i));
				newTransFunc[i]->setMaxMapValue(ds->getDefaultDataMax2D(i));
				newMinEdit[i] = ds->getDefaultDataMin2D(i);
				newMaxEdit[i] = ds->getDefaultDataMax2D(i);
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
			newMinEdit[i] = ds->getDefaultDataMin2D(i);
			newMaxEdit[i] = ds->getDefaultDataMax2D(i);
		}
		//And check again...
		if (newMinEdit[i] >= newMaxEdit[i]){
			newMinEdit[i] = 0.f;
			newMaxEdit[i] = 1.f;
		}
	}
	minTerrainHeight = extents[2];
	maxTerrainHeight = extents[2];
	int hgtvar = ds->getSessionVariableNum2D("HGT");
	if (hgtvar >= 0)
		maxTerrainHeight = ds->getDefaultDataMax2D(hgtvar);
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
	minTerrainHeight = 0.f;
	maxTerrainHeight = 0.f;
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
		if (i == numVariables && numVariables > 0) variableSelected[0] = true;
		
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
	

//Find the smallest box containing the twoD slice, in block coords, at current refinement level.
//We also need the actual box coords (min and max) to check for valid coords in the block.
//Note that the box region may be strictly smaller than the block region
//
void TwoDParams::getBoundingBox(int timestep, size_t boxMin[3], size_t boxMax[3], int numRefs){
	//Determine the box that contains the twoD slice.
	DataStatus* ds = DataStatus::getInstance();
	const VDFIOBase* myReader = ds->getRegionReader();
	double dmin[3],dmax[3];
	for (int i = 0; i<3; i++) {
		dmin[i] = twoDMin[i];
		dmax[i] = twoDMax[i];
	}
	myReader->MapUserToVox(timestep,dmin, boxMin, numRefs);
	myReader->MapUserToVox(timestep,dmax, boxMax, numRefs);

}
//Find the smallest box containing the twoD slice, in 3d block coords, at current refinement level
//and current time step.  Restrict it to the available data.
//The constant dimension is also included:
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
			int rc = RegionParams::getValidRegion(timeStep, varName.c_str(),numRefs, temp_min, temp_max);
			if (rc < 0) {
				retVal = false;
			}
		}
		if(retVal) for (i = 0; i< 3; i++){
			if (boxMin[i] < temp_min[i]) boxMin[i] = temp_min[i];
			if (boxMax[i] > temp_max[i]) boxMax[i] = temp_max[i];
		}
	}
	int constDim = DataStatus::getInstance()->get2DOrientation(getFirstVarNum());
	
	boxMin[constDim] = temp_min[constDim];
	boxMax[constDim] = temp_min[constDim];
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
	setElevGridDirty(true);
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
	
	//get the slice(s) from the DataMgr
	//one of the 3 coords in each coordinate argument will be ignored,
	//depending on orientation of the variables
	float** planarData = getTwoDVariables(ts,  numVars, sesVarNums,
				  blkMin, blkMax, coordMin, coordMax, &actualRefLevel);

	if(!planarData){
		delete sesVarNums;
		return 0;
	}

	float a[2],b[2];  //transform of (x,y) is to (a[0]x+b[0],a[1]y+b[1])
	//Set up to transform from twoD into volume:
	float constValue;
	int mapDims[3];
	build2DTransform(a,b,&constValue,mapDims);

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
	
	//We first map the coords in the twoD plane to the volume.  
	//Then we map the volume into the region provided by dataMgr
	//This is done for each of the variables,
	//The RMS of the result is then mapped using the transfer function.
	float clut[256*4];
	TransferFunction* transFunc = getTransFunc();
	assert(transFunc);
	transFunc->makeLut(clut);
	
	float twoDCoord[2];
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
	//map plane corners
	dataCoord[mapDims[2]] = constValue;

	for (int cornum = 0; cornum < 4; cornum++){
		// coords relative to (-1,1)
		twoDCoord[1] = -1.f + 2.f*(float)(cornum/2);
		twoDCoord[0] = -1.f + 2.f*(float)(cornum%2);
		//transform into data volume
		dataCoord[mapDims[0]] = a[0]*twoDCoord[0]+b[0];
		dataCoord[mapDims[1]] = a[1]*twoDCoord[1]+b[1];
		
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


	//Loop over pixels in texture.  Pixel centers map to edges of twoD plane
	int orientation = ds->get2DOrientation(firstVarNum);
	for (int iy = 0; iy < texHeight; iy++){
		//Map iy to a value between -1 and 1
		twoDCoord[1] = -1.f + 2.f*(float)iy/(float)(texHeight-1);
		for (int ix = 0; ix < texWidth; ix++){
			
			
			twoDCoord[0] = -1.f + 2.f*(float)ix/(float)(texWidth-1);
			//find the coords that the texture maps to
			//twoDCoord is the coord in the twoD slice, dataCoord is in data volume 
			dataCoord[mapDims[2]] = constValue;
			dataCoord[mapDims[0]] = twoDCoord[0]*a[0]+b[0];
			dataCoord[mapDims[1]] = twoDCoord[1]*a[1]+b[1];
			
			
			myReader->MapUserToVox(ts, dataCoord, arrayCoord, actualRefLevel);
			bool dataOK = true;
			for (int i = 0; i< 3; i++){
				if (i == orientation) continue;
				if (dataCoord[i] < extExtents[i] || dataCoord[i] > extExtents[i+3]) dataOK = false;
				if (arrayCoord[i] < coordMin[i] || arrayCoord[i] > coordMax[i]) dataOK = false;
			}
			
			if(dataOK) { //find the coordinate in the data array
				
				int xyzCoord = (arrayCoord[mapDims[0]] - blkMin[mapDims[0]]*bSize[mapDims[0]]) +
					(arrayCoord[mapDims[1]] - blkMin[mapDims[1]]*bSize[mapDims[1]])*(bSize[mapDims[0]]*(blkMax[mapDims[0]]-blkMin[mapDims[0]]+1));
					
				float varVal;
				assert(xyzCoord >= 0);

				//use the intDataCoords to index into the loaded data
				if (numVars == 1) varVal = planarData[0][xyzCoord]; 
				else { //Add up the squares of the variables
					varVal = 0.f;
					for (int k = 0; k<numVars; k++){
						varVal += planarData[k][xyzCoord]*planarData[k][xyzCoord];
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
	delete planarData;
	delete sesVarNums;
	return twoDTexture;
}
void TwoDParams::adjustTextureSize(int sz[2]){
	//Need to determine appropriate texture dimensions
	//It should be at least 256x256, or more if the data resolution
	//is higher than that.
	//Find the full size of the data at the current resolution in the
	//two dimensions being used.
	//Then see how large is the box, compared to the full extents
	//in those two dimensions

	//Get the data dimensions (at this resolution):
	int dataSize[3];
	
	DataStatus* ds = DataStatus::getInstance();
	int refLevel = getNumRefinements();
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(refLevel,i);
	}
	int orientation = ds->get2DOrientation(firstVarNum);
	int xcrd = 0, ycrd = 1;
	if (orientation < 2) ycrd++;
	if (orientation < 1) xcrd++;
	const float* extents = ds->getExtents();
	
	float relWid = (twoDMax[xcrd]-twoDMin[xcrd])/(extents[xcrd+3]-extents[xcrd]);
	float relHt = (twoDMax[ycrd]-twoDMin[ycrd])/(extents[ycrd+3]-extents[ycrd]);
	int xdist = relWid*dataSize[xcrd];
	int ydist = relHt*dataSize[ycrd];
	textureSize[0] = 1<<(VetsUtil::ILog2(xdist));
	textureSize[1] = 1<<(VetsUtil::ILog2(ydist));
	if (textureSize[0] < 256) textureSize[0] = 256;
	if (textureSize[1] < 256) textureSize[1] = 256;
	sz[0] = textureSize[0];
	sz[1] = textureSize[1];
	
}
//Determine the voxel extents of plane mapped into data.

void TwoDParams::
getTwoDVoxelExtents(float voxdims[2]){
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) {
		voxdims[0] = voxdims[1] = 1.f;
		return;
	}
	int orientation = ds->get2DOrientation(firstVarNum);
	int xcrd = 0, ycrd = 1;
	if (orientation < 2) ycrd = 2;
	if (orientation < 1) xcrd = 1;
	voxdims[0] = twoDMax[xcrd] - twoDMin[xcrd];
	voxdims[1] = twoDMax[ycrd] - twoDMin[ycrd];
	return;
}



//General routine that obtains 1 or more 2D variables from the cache in the smallest volume that
//contains the 2D slice.  sesVarNums is a list of session variable numbers to be obtained.
//If the variable num is negative, just returns a null data array.
//(Note that this is a generalization of the first part of calcTwoDDataTexture)
//Provides several variables to be used in addressing into the data

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
	
	//Determine the integer extents of the containing square, truncate to
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
	float** planarData = new float*[numVars];
	//obtain all of the planes needed for this twoD:
	
	for (int varnum = 0; varnum < numVars; varnum++){
		int varindex = sesVarNums[varnum];
		if (varindex < 0) {planarData[varnum] = 0; continue;}   //handle the zero field as a 0 pointer
		planarData[varnum] = getContainingVolume(blkMin, blkMax, refLevel, varindex, ts, true);
		if (!planarData[varnum]) {
			delete planarData;
			return 0;
		}
	}

	return planarData;
}
	//Construct transform of form (x,y)-> a[0]x+b[0],a[1]y+b[1],
	//Mapping [-1,1]X[-1,1] into 3D volume.
    //Also determine the first and second coords that are used in 
    //the transform and the constant value
    //mappedDims[0] and mappedDims[1] are the dimensions that are
    //varying in the 3D volume.  mappedDims[2] is constant.
    //constVal is the constant value that is used.
void TwoDParams::build2DTransform(float a[2],float b[2],float constVal[2], int mappedDims[3]){
	//Find out orientation:
	int orientation = DataStatus::getInstance()->get2DOrientation(getFirstVarNum());
	mappedDims[2] = orientation;
	mappedDims[0] = (orientation == 0) ? 1 : 0;  // x or y
	mappedDims[1] = (orientation < 2) ? 2 : 1; // z or y
	constVal[0] = twoDMin[orientation];
	constVal[1] = twoDMax[orientation];
	//constant terms go to middle
	b[0] = 0.5*(twoDMin[mappedDims[0]]+twoDMax[mappedDims[0]]);
	b[1] = 0.5*(twoDMin[mappedDims[1]]+twoDMax[mappedDims[1]]);
	//linear terms send -1,1 to box min,max
	a[0] = b[0] - twoDMin[mappedDims[0]];
	a[1] = b[1] - twoDMin[mappedDims[1]];

}
//Following overrides version in Param for 2D
//Does not support rotation or thickness
void TwoDParams::
calcBoxCorners(float corners[8][3], float, float, int ){
	
	float a[2],b[2],constValue[2];
	int mapDims[3];
	build2DTransform(a,b,constValue,mapDims);
	float boxCoord[3];
	//Return the corners of the box (in world space)
	//Go counter-clockwise around the back, then around the front
	//X increases fastest, then y then z; 

	//Fatten box slightly, in case it is degenerate.  This will
	//prevent us from getting invalid face normals.

	boxCoord[0] = -1.f;
	boxCoord[1] = -1.f;
	boxCoord[2] = -1.f;
	// calculate the mapping of each corner,
	corners[0][mapDims[2]] = constValue[0];
	corners[0][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[0][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[0] = 1.f;
	corners[1][mapDims[2]] = constValue[0];
	corners[1][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[1][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[1] = 1.f;
	corners[3][mapDims[2]] = constValue[0];
	corners[3][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[3][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[0] = -1.f;
	corners[2][mapDims[2]] = constValue[0];
	corners[2][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[2][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[1] = -1.f;
	boxCoord[2] = 1.f;
	corners[4][mapDims[2]] = constValue[1];
	corners[4][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[4][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[0] = 1.f;
	corners[5][mapDims[2]] = constValue[1];
	corners[5][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[5][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[1] = 1.f;
	corners[7][mapDims[2]] = constValue[1];
	corners[7][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[7][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[0] = -1.f;
	corners[6][mapDims[2]] = constValue[1];
	corners[6][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[6][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
}