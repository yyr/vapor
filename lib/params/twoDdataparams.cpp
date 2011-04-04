//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDdataparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2009
//
//	Description:	Implementation of the twoDdataparams class
//		This contains all the parameters required to support the
//		twoD data renderer.  Embeds a transfer function and a
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
#include "twoDdataparams.h"
#include "regionparams.h"
#include "params.h"
#include "transferfunction.h"

#include "histo.h"
#include "animationparams.h"
#include "viewpointparams.h"

#include <vapor/DataMgr.h>
#include <vapor/errorcodes.h>
#include <vapor/WRF.h>


using namespace VAPoR;
const string TwoDDataParams::_shortName = "2D";
const string TwoDDataParams::_editModeAttr = "TFEditMode";
const string TwoDDataParams::_histoStretchAttr = "HistoStretchFactor";
const string TwoDDataParams::_variableSelectedAttr = "VariableSelected";

TwoDDataParams::TwoDDataParams(int winnum) : TwoDParams(winnum, Params::_twoDDataParamsTag){
	
	numVariables = 0;
	twoDDataTextures = 0;
	maxTimestep = 1;
	textureSizes = 0;
	restart();
	
}
TwoDDataParams::~TwoDDataParams(){
	
	if (transFunc){
		for (int i = 0; i< numVariables; i++){
			delete transFunc[i];  //will delete editor
		}
		delete [] transFunc;
	}
	if (twoDDataTextures) {
		for (int i = 0; i<= maxTimestep; i++){
			if (twoDDataTextures[i]) delete [] twoDDataTextures[i];
		}
		delete [] twoDDataTextures;
	}
	
}


//Deepcopy requires cloning tf 
Params* TwoDDataParams::
deepCopy(ParamNode*){
	TwoDDataParams* newParams = new TwoDDataParams(*this);
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


void TwoDDataParams::
refreshCtab() {
	((TransferFunction*)getMapperFunc())->makeLut((float*)ctab);
}
	
bool TwoDDataParams::
usingVariable(const string& varname){
	if (varname == "HGT" && isMappedToTerrain()) return true;
	int varnum = DataStatus::getInstance()->getSessionVariableNum2D(varname);
	return (variableIsSelected(varnum));
}


void TwoDDataParams::
setClut(const float newTable[256][4]){
	for (int i = 0; i< 256; i++) {
		for (int j = 0; j< 4; j++){
			ctab[i][j] = newTable[i][j];
		}
	}
}


float TwoDDataParams::getOpacityScale() 
{
  if (numVariables)
  {
    return transFunc[firstVarNum]->getOpacityScaleFactor();
  }

  return 1.0;
}

void TwoDDataParams::setOpacityScale(float val) 
{
  if (numVariables)
  {
    return transFunc[firstVarNum]->setOpacityScaleFactor(val);
  }
}


//Initialize for new metadata.  Keep old transfer functions
//
bool TwoDDataParams::
reinit(bool doOverride){
	
	DataStatus* ds = DataStatus::getInstance();
	const float* extents = ds->getExtents();
	setMaxNumRefinements(ds->getNumTransforms());
	//Set up the numRefinements combo
	
	
	//Either set the twoD bounds to default (full) size in the center of the domain, or 
	//try to use the previous bounds:
	if (doOverride){
		for (int i = 0; i<3; i++){
			float twoDRadius = 0.5f*(extents[i+3] - extents[i]);
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
		for (int i = 0; i<3; i++){
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
		for (int i = 0; i<newNumVariables; i++) {
			if (ds->variableIsPresent2D(i)){
				firstVarNum = i;
				break;
			}
		}
	}
	if (firstVarNum == -1){
		
		for (int i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete [] transFunc;
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
	numVariablesSelected = 0;
	for (int i = 0; i< newNumVariables; i++){
		if (variableSelected[i]) numVariablesSelected++;
	}
	//Create new arrays to hold bounds and transfer functions:
	TransferFunction** newTransFunc = new TransferFunction*[newNumVariables];
	float* newMinEdit = new float[newNumVariables];
	float* newMaxEdit = new float[newNumVariables];
	//If we are overriding previous values, delete the transfer functions, create new ones.
	//Set the map bounds to the actual bounds in the data
	if (doOverride){
		for (int i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		//Create new transfer functions, their editors, hook them up:
		
		for (int i = 0; i<newNumVariables; i++){
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
		for (int i = 0; i<newNumVariables; i++){
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
		for (int i = newNumVariables; i<numVariables; i++){
			delete transFunc[i];
		}
	} //end if(doOverride)
	//Make sure edit bounds are valid
	for(int i = 0; i<newNumVariables; i++){
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
	delete [] minColorEditBounds;
	delete [] maxColorEditBounds;
	delete [] transFunc;
	transFunc = 0;
	minColorEditBounds = newMinEdit;
	maxColorEditBounds = newMaxEdit;
	//And clone the color edit bounds to use as opac edit bounds:
	minOpacEditBounds = new float[newNumVariables];
	maxOpacEditBounds = new float[newNumVariables];
	for (int i = 0; i<newNumVariables; i++){
		minOpacEditBounds[i] = minColorEditBounds[i];
		maxOpacEditBounds[i] = maxColorEditBounds[i];
	}

	transFunc = newTransFunc;
	
	numVariables = newNumVariables;
	
	// set up the texture cache
	setTwoDDirty();
	if (twoDDataTextures) {
		delete [] twoDDataTextures;
	}
	
	maxTimestep = DataStatus::getInstance()->getNumTimesteps()-1;
	twoDDataTextures = 0;
	
	initializeBypassFlags();
	return true;
}
//Initialize to default state
//
void TwoDDataParams::
restart(){
	
	
	mapToTerrain = false;
	minTerrainHeight = 0.f;
	maxTerrainHeight = 0.f;
	compressionLevel = 0;
	histoStretchFactor = 1.f;
	firstVarNum = 0;
	orientation = 2;
	setTwoDDirty();
	if (twoDDataTextures) {
		delete [] twoDDataTextures;
	}
	twoDDataTextures = 0;

	if(numVariables > 0){
		for (int i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete [] transFunc;
	}
	numVariables = 0;
	transFunc = 0;
	cursorCoords[0] = cursorCoords[1] = 0.0f;
	//Initialize the mapping bounds to [0,1] until data is read
	
	if (minColorEditBounds) delete [] minColorEditBounds;
	if (maxColorEditBounds) delete [] maxColorEditBounds;
	if (minOpacEditBounds) delete [] minOpacEditBounds;
	if (maxOpacEditBounds) delete [] maxOpacEditBounds;
	
	
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
		if (i < 2) twoDMin[i] = 0.0f;
		else twoDMin[i] = 0.5f;
		if(i<2) twoDMax[i] = 1.0f;
		else twoDMax[i] = 0.5f;
		selectPoint[i] = 0.5f;
	}
	
	
}



//Hook up the new transfer function in specified slot,
//Delete the old one.  This is called whenever a new tf is loaded.
//
void TwoDDataParams::
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
bool TwoDDataParams::
elementStartHandler(ExpatParseMgr* pm, int depth , std::string& tagString, const char **attrs){
	
	static int parsedVarNum = -1;
	int i;
	if (StrCmpNoCase(tagString, _twoDDataParamsTag) == 0 ||
		StrCmpNoCase(tagString, _twoDParamsTag) == 0) {
		//Set defaults in case reading an old session:
		
		orientation = 2; //X-Y aligned
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
			else if (StrCmpNoCase(attribName, _CompressionLevelTag) == 0){
				ist >> compressionLevel;
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
				//obsolete
			}
			
			else if (StrCmpNoCase(attribName, _orientationAttr) == 0) {
				ist >> orientation;
			}
			else return false;
		}
		//Create space for the variables:
		int numVars = Max (newNumVariables, 1);
		if (minColorEditBounds) delete [] minColorEditBounds;
		minColorEditBounds = new float[numVars];
		if (maxColorEditBounds) delete [] maxColorEditBounds;
		maxColorEditBounds = new float[numVars];
		if (minOpacEditBounds) delete [] minOpacEditBounds;
		minOpacEditBounds = new float[numVars];
		if (maxOpacEditBounds) delete [] maxOpacEditBounds;
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
			delete [] transFunc;
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
bool TwoDDataParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	
	if (StrCmpNoCase(tag, _twoDDataParamsTag) == 0 ||
		StrCmpNoCase(tag, _twoDParamsTag) == 0){
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
		pm->parseError("Unrecognized end tag in TwoDDataParams %s",tag.c_str());
		return false; 
	}
}

//Method to construct Xml for state saving
ParamNode* TwoDDataParams::
buildNode() {
	//Construct the twoD node
	if (numVariables <= 0) return 0;
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
	oss << (long)compressionLevel;
	attrs[_CompressionLevelTag] = oss.str();

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
	if (isMappedToTerrain())
		oss << "true";
	else 
		oss << "false";
	attrs[_terrainMapAttr] = oss.str();

	oss.str(empty);
	oss << (long)orientation;
	attrs[_orientationAttr] = oss.str();
	
	ParamNode* twoDDataNode = new ParamNode(_twoDDataParamsTag, attrs, 3);

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

		ParamNode* varNode = new ParamNode(_variableTag,attrs,1);

		//Create a transfer function node, add it as child
		ParamNode* tfNode = transFunc[i]->buildNode(empty);
		varNode->AddChild(tfNode);
		twoDDataNode->AddChild(varNode);
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
	twoDDataNode->NewChild(_geometryTag, attrs, 0);
	return twoDDataNode;
}

MapperFunction* TwoDDataParams::getMapperFunc() {
	return (numVariables > 0 ? transFunc[firstVarNum] : 0);
}

void TwoDDataParams::setMinColorMapBound(float val){
	getMapperFunc()->setMinColorMapValue(val);
}
void TwoDDataParams::setMaxColorMapBound(float val){
	getMapperFunc()->setMaxColorMapValue(val);
}


void TwoDDataParams::setMinOpacMapBound(float val){
	getMapperFunc()->setMinOpacMapValue(val);
}
void TwoDDataParams::setMaxOpacMapBound(float val){
	getMapperFunc()->setMaxOpacMapValue(val);
}
	


//Find the smallest box containing the twoD slice, in 3d block coords, at current refinement level
//and current time step.  Restrict it to the available data.
//The constant dimension is also included:
//
bool TwoDDataParams::
getAvailableBoundingBox(int timeStep, size_t boxMinBlk[3], size_t boxMaxBlk[3], 
		size_t boxMin[3], size_t boxMax[3], int numRefs){
	
	//Start with the bounding box for this refinement level:
	getBoundingBox(timeStep, boxMin, boxMax, numRefs);
	size_t bs[3];
	DataStatus::getInstance()->getDataMgr()->GetBlockSize(bs, numRefs);
	size_t temp_min[3],temp_max[3];
	for (int j = 0; j<3; j++){
		temp_min[j] = boxMin[j];
		temp_max[j] = boxMax[j];
	}
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
	boxMax[constDim] = temp_max[constDim];
	//Now do the block dimensions:
	for (int i = 0; i< 3; i++){
		size_t dataSize = DataStatus::getInstance()->getFullSizeAtLevel(numRefs,i);
		if(boxMax[i] > dataSize-1) boxMax[i] = dataSize - 1;
		if (boxMin[i] > boxMax[i]) boxMax[i] = boxMin[i];
		//And make the block coords:
		boxMinBlk[i] = boxMin[i]/ bs[i];
		boxMaxBlk[i] = boxMax[i]/ bs[i];
	}
	
	return retVal;
}


//Clear out the cache.  
//just delete the elevation grid
void TwoDDataParams::setTwoDDirty(){
	
	if (twoDDataTextures){
		for (int i = 0; i<=maxTimestep; i++){
			if (twoDDataTextures[i]) {
				delete [] twoDDataTextures[i];
				twoDDataTextures[i] = 0;
			}
		}
		twoDDataTextures = 0;
	}
	
	setElevGridDirty(true);
	setAllBypass(false);
	lastTwoDTexture = 0;
}

//Calculate the twoD texture (if it needs refreshing).
//It's kept (cached) in the twoD params
//If nonzero texture dimensions are provided, then the cached image
//is not affected 
unsigned char* TwoDDataParams::
calcTwoDDataTexture(int ts, int texWidth, int texHeight){
	size_t blkMin[3], blkMax[3];
	size_t coordMin[3], coordMax[3];
	if (!isEnabled()) return 0;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return 0;
	if (doBypass(ts)) return 0;
	
	//if width and height are 0, then the image will
	//be of the size specified in the 2D params, and the result
	//will be placed in the cache. Otherwise we are just needing
	//an image of specified size for writing to file, not to be put in the cache.
	//
	bool doCache = (texWidth == 0 && texHeight == 0);


	//Make a list of the session variable nums we want:
	int* sesVarNums = new int[ds->getNumSessionVariables2D()];
	int actualRefLevel; 
	int numVars = 0;
	for (int varnum = 0; varnum < ds->getNumSessionVariables2D(); varnum++){
		if (!variableIsSelected(varnum)) continue;
		sesVarNums[numVars++] = varnum;
	}
	
	
	//get the slice(s) from the DataMgr
	//one of the 3 coords in each coordinate argument will be ignored,
	//depending on orientation of the variables
	float** planarData = getTwoDVariables(ts,  numVars, sesVarNums,
				  blkMin, blkMax, coordMin, coordMax, &actualRefLevel);

	if(!planarData){
		delete [] sesVarNums;
		setBypass(ts);
		return 0;
	}
	size_t bSize[3];
	ds->getDataMgr()->GetBlockSize(bSize, actualRefLevel);

	float a[2],b[2];  //transform of (x,y) is to (a[0]x+b[0],a[1]y+b[1])
	//Set up to transform from twoD into volume:
	float constValue[2];
	int mapDims[3];
	build2DTransform(a,b,constValue,mapDims);

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
	//map plane corners.  Always use constValue[0] = z, this is bottom of 2D box
	dataCoord[mapDims[2]] = constValue[0];

	for (int cornum = 0; cornum < 4; cornum++){
		// coords relative to (-1,1)
		twoDCoord[1] = -1.f + 2.f*(float)(cornum/2);
		twoDCoord[0] = -1.f + 2.f*(float)(cornum%2);
		//transform into data volume
		dataCoord[mapDims[0]] = a[0]*twoDCoord[0]+b[0];
		dataCoord[mapDims[1]] = a[1]*twoDCoord[1]+b[1];
		
	}
	int txsize[2];
	if (doCache) {
		adjustTextureSize(txsize);
		texWidth = txsize[0];
		texHeight = txsize[1];
	}
	
	
	unsigned char* twoDTexture = new unsigned char[texWidth*texHeight*4];

	//Use the region reader to calculate coordinates in volume
	const DataMgr* dataMgr = ds->getDataMgr();

	if (ds->dataIsLayered()){
		RegionParams::setFullGridHeight(RegionParams::getFullGridHeight());
	}


	//Loop over pixels in texture.  Pixel centers map to edges of twoD plane
	int dataOrientation = ds->get2DOrientation(firstVarNum);
	for (int iy = 0; iy < texHeight; iy++){
		//Map iy to a value between -1 and 1
		twoDCoord[1] = -1.f + 2.f*(float)iy/(float)(texHeight-1);
		for (int ix = 0; ix < texWidth; ix++){
			
			
			twoDCoord[0] = -1.f + 2.f*(float)ix/(float)(texWidth-1);
			//find the coords that the texture maps to
			//twoDCoord is the coord in the twoD slice, dataCoord is in data volume 
			dataCoord[mapDims[2]] = constValue[0];
			dataCoord[mapDims[0]] = twoDCoord[0]*a[0]+b[0];
			dataCoord[mapDims[1]] = twoDCoord[1]*a[1]+b[1];
			
			
			dataMgr->MapUserToVox((size_t)-1, dataCoord, arrayCoord, actualRefLevel);
			bool dataOK = true;
			for (int i = 0; i< 3; i++){
				if (i == dataOrientation) continue;
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
	
	if (doCache) setTwoDTexture(twoDTexture,ts, txsize);
	delete [] planarData;
	delete [] sesVarNums;
	return twoDTexture;
}
void TwoDDataParams::adjustTextureSize(int sz[2]){
	//Need to determine appropriate texture dimensions
	//It should be at least 256x256, or more if the data resolution
	//is higher than that.
	//Find the full size of the data at the current resolution in the
	//two dimensions being used.
	//Then see how large is the box, compared to the full extents
	//in those two dimensions

	//Get the data dimensions (at this resolution):
	int dataSize[3];
	int texSize[2];
	DataStatus* ds = DataStatus::getInstance();
	int refLevel = getNumRefinements();
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(refLevel,i);
	}
	int dataOrientation = ds->get2DOrientation(firstVarNum);
	int xcrd = 0, ycrd = 1;
	if (dataOrientation < 2) ycrd++;
	if (dataOrientation < 1) xcrd++;
	const float* extents = ds->getExtents();
	
	float relWid = (twoDMax[xcrd]-twoDMin[xcrd])/(extents[xcrd+3]-extents[xcrd]);
	float relHt = (twoDMax[ycrd]-twoDMin[ycrd])/(extents[ycrd+3]-extents[ycrd]);
	int xdist = (int)(relWid*dataSize[xcrd]);
	int ydist = (int)(relHt*dataSize[ycrd]);
	texSize[0] = 1<<(VetsUtil::ILog2(xdist));
	texSize[1] = 1<<(VetsUtil::ILog2(ydist));
	if (texSize[0] < 2) texSize[0] = 2;
	if (texSize[1] < 2) texSize[1] = 2;
	
	sz[0] = texSize[0];
	sz[1] = texSize[1];
	
}
//Determine the voxel extents of plane mapped into data.




//General routine that obtains 1 or more 2D variables from the cache in the smallest volume that
//contains the 2D slice.  sesVarNums is a list of session variable numbers to be obtained.
//If the variable num is negative, just returns a null data array.
//(Note that this is a generalization of the first part of calcTwoDDataTexture)
//Provides several variables to be used in addressing into the data

float** TwoDDataParams::
getTwoDVariables(int ts,  int numVars, int* sesVarNums,
				  size_t blkMin[3], size_t blkMax[3], size_t coordMin[3], size_t coordMax[3],
				  int* actualRefLevel){
	if (!isEnabled()) return 0;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return 0;
	
	int refLevel = getNumRefinements();
	//reduce reflevel if not all variables are available:
	if (ds->useLowerAccuracy()){
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
		setAllBypass(true);
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small\nfor current twoD and resolution.\n%s \n%s",
			"Lower the refinement level,\nreduce the twoD size,\nor increase the cache size.",
			"Rendering has been disabled.");
		setEnabled(false);
		return 0;
	}
	
	//Make the z -extents equal to the full z-extents of the global region.  This is in case the
	//2D variable is the output of a script, and there is a 3D input to the script. This way the 3D extents of
	//the 3D input variable are determined by the current global variable.
	RegionParams* rParams = (RegionParams*) GetDefaultParams(Params::_regionParamsTag);
	size_t regBlockExtents[6], regExts[6];
	rParams->getRegionVoxelCoords(refLevel, regExts, regExts+3, regBlockExtents,regBlockExtents+3, ts);
	blkMin[2] = regBlockExtents[2];
	blkMax[2] = regBlockExtents[5];
	

	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable 
	float** planarData = new float*[numVars];
	//obtain all of the planes needed for this twoD:
	
	for (int varnum = 0; varnum < numVars; varnum++){
		int varindex = sesVarNums[varnum];
		if (varindex < 0) {planarData[varnum] = 0; continue;}   //handle the zero field as a 0 pointer
		planarData[varnum] = getContainingVolume(blkMin, blkMax, refLevel, varindex, ts, true);
		if (!planarData[varnum]) {
			delete [] planarData;
			return 0;
		}
	}

	return planarData;
}

bool TwoDDataParams::isOpaque(){
	if(getTransFunc()->isOpaque() && getOpacityScale() > 0.99f) return true;
	return false;
}


///
  
