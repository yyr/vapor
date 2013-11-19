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
#include "twoDdataparams.h"
#include "regionparams.h"
#include "params.h"
#include "transferfunction.h"

#include "histo.h"
#include "animationparams.h"
#include "viewpointparams.h"

#include <vapor/DataMgr.h>
#include <vapor/errorcodes.h>


using namespace VAPoR;
const string TwoDDataParams::_shortName = "2D";
const string TwoDDataParams::_editModeAttr = "TFEditMode";
const string TwoDDataParams::_histoStretchAttr = "HistoStretchFactor";
const string TwoDDataParams::_variableSelectedAttr = "VariableSelected";
const string TwoDDataParams::_linearInterpAttr = "LinearInterp";

TwoDDataParams::TwoDDataParams(int winnum) : TwoDParams(winnum, Params::_twoDDataParamsTag){
	
	_timestep = -1;
	_varname.clear();
	_reflevel = -1;
	_lod = -1;
	_usrExts.clear();
	_textureSizes[0] = _textureSizes[1] = 0;
	_texBuf = NULL;

	numVariables = 0;
	maxTimestep = 1;
	restart();
	
}
TwoDDataParams::~TwoDDataParams(){
	
	if (transFunc){
		for (int i = 0; i< numVariables; i++){
			delete transFunc[i];  //will delete editor
		}
		delete [] transFunc;
	}
	if (_texBuf) delete [] _texBuf;
	
}


//Deepcopy requires cloning tf 
Params* TwoDDataParams::
deepCopy(ParamNode*){
	TwoDDataParams* newParams = new TwoDDataParams(*this);
	
	ParamNode* pNode = new ParamNode(*(myBox->GetRootNode()));
	newParams->myBox = (Box*)myBox->deepCopy(pNode);
	
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
	_timestep = -1;
	_texBuf = NULL;

	//Clone the Transfer Functions
	newParams->transFunc = new TransferFunction*[numVariables];
	for (int i = 0; i<numVariables; i++){
		newParams->transFunc[i] = new TransferFunction(*transFunc[i]);
	}
	//never keep the SavedCommand:
	
	return newParams;
}

bool TwoDDataParams::twoDIsDirty(int timestep) {
	DataStatus* ds = DataStatus::getInstance();

	if (timestep != _timestep) return(true);
	if (! _varname.size()) return(true);
	if (_varname[0].compare(ds->getVariableName2D(firstVarNum)) != 0) return(true);
	if (_reflevel != GetRefinementLevel()) return(true);
	if (_lod != GetCompressionLevel()) return(true);
	if (_usrExts != ds->getDataMgr()->GetExtents((size_t)timestep)) return(true);

	return(false);
}



void TwoDDataParams::
refreshCtab() {
	((TransferFunction*)GetMapperFunc())->makeLut((float*)ctab);
}
	
bool TwoDDataParams::
usingVariable(const string& varname){
	if ((varname == GetHeightVariableName()) && isMappedToTerrain()) return true;
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
	
	const float* localExtents = ds->getLocalExtents();
	setMaxNumRefinements(ds->getNumTransforms());
	//Set up the numRefinements combo
	
	
	//Either set the twoD bounds to default (full) size in the center of the domain, or 
	//try to use the previous bounds:
	double twoDExts[6];  //These will be local extents
	if (doOverride){
		for (int i = 0; i<3; i++){
			float twoDRadius = 0.5f*(localExtents[i+3] - localExtents[i]);
			float twoDMid = 0.5f*(localExtents[i+3] - localExtents[i]);
			if (i<2) {
				twoDExts[i] = twoDMid - twoDRadius;
				twoDExts[i+3] = twoDMid + twoDRadius;
			} else {
				twoDExts[i] = twoDExts[i+3] = twoDMid;
			}
		}
		
		cursorCoords[0] = 0.;
		cursorCoords[1] = 0.;
		numRefinements = 0;
	} else {
		//Force the twoD horizontal size to be no larger than the domain size, and 
		//force the twoD horizontal center to be inside the domain.  Note that
		//because of rotation, the twoD max/min may not correspond
		//to the same extents.
		
		GetBox()->GetLocalExtents(twoDExts);
		if (DataStatus::pre22Session()){
			float * offset = DataStatus::getPre22Offset();
			//In old session files, the coordinate of box extents were not 0-based
			for (int i = 0; i<3; i++) {
				twoDExts[i] -= offset[i];
				twoDExts[i+3] -= offset[i];
			}
		}
		float maxExtents = Max(Max(localExtents[3],localExtents[4]),localExtents[5]);
		for (int i = 0; i<2; i++){
			if (twoDExts[i+3] - twoDExts[i] > maxExtents)
				twoDExts[i+3] = twoDExts[i] + maxExtents;
			float center = 0.5f*(twoDExts[i]+twoDExts[i+3]);
			if (center < 0.) {
				twoDExts[i] -= center;
				twoDExts[i+3] -= center;
			}
			if (center > (localExtents[i+3])) {
				twoDExts[i] += (localExtents[i+3]-center);
				twoDExts[i+3] += (localExtents[i+3]-center);
			}
			if(twoDExts[i+3] < twoDExts[i]) 
				twoDExts[i+3] = twoDExts[i];
		}
		if(twoDExts[5] < twoDExts[2]) 
				twoDExts[5] = twoDExts[2];
		if (numRefinements > maxNumRefinements) numRefinements = maxNumRefinements;
	}
	GetBox()->SetLocalExtents(twoDExts);
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
	//Use HGT as the height variable name, if it's there. If not just use the first 2d variable.
	minTerrainHeight = twoDExts[2];
	maxTerrainHeight = twoDExts[2];
	int hgtvarindex;
	
	if (doOverride){
		string varname = "HGT";
		hgtvarindex = ds->getActiveVarNum2D(varname);
		if (hgtvarindex < 0 && ds->getNumActiveVariables2D()>0) {
			varname = ds->getVariableName2D(0);
			hgtvarindex=0;
		}
		SetHeightVariableName(varname);
	} else {
		string varname = GetHeightVariableName();
		hgtvarindex = ds->getActiveVarNum2D(varname);
		if (hgtvarindex < 0 && ds->getNumActiveVariables2D()>0) {
			varname = ds->getVariableName2D(0);
			SetHeightVariableName(varname);
			hgtvarindex = 0;
		}
	}
	maxTerrainHeight = ds->getDefaultDataMax2D(hgtvarindex);
	if (ds->getNumActiveVariables2D() <= 0) setMappedToTerrain(false);
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
	
	maxTimestep = DataStatus::getInstance()->getNumTimesteps()-1;
	
	initializeBypassFlags();
	return true;
}
//Initialize to default state
//
void TwoDDataParams::
restart(){
	
	if (!myBox){
		myBox = new Box();
	}
	mapToTerrain = false;
	minTerrainHeight = 0.f;
	maxTerrainHeight = 0.f;
	compressionLevel = 0;
	histoStretchFactor = 1.f;
	firstVarNum = 0;
	orientation = 2;
	setTwoDDirty();

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
	double twoDexts[6];
	for (int i = 0; i<3; i++){
		if (i < 2) twoDexts[i] = 0.0f;
		else twoDexts[i] = 0.5f;
		if(i<2) twoDexts[i+3] = 1.0f;
		else twoDexts[i+3] = 0.5f;
		selectPoint[i] = 0.5f;
	}
	GetBox()->SetLocalExtents(twoDexts);
	
	heightVariableName = "HGT";
	linearInterp = true;
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
		setLinearInterp(false);
		orientation = 2; //X-Y aligned
		int newNumVariables = 0;
		heightVariableName = "HGT";

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
			else if (StrCmpNoCase(attribName, _heightVariableAttr) == 0) {
				heightVariableName = value;
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
			else if (StrCmpNoCase(attribName, _linearInterpAttr) == 0){
				if (value == "true") setLinearInterp(true); 
				else setLinearInterp(false);
			}
			else if (StrCmpNoCase(attribName, _verticalDisplacementAttr) == 0){
				//obsolete
			}
			
			else if (StrCmpNoCase(attribName, _orientationAttr) == 0) {
				ist >> orientation;
			}
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
		float exts[6];
		GetBox()->GetLocalExtents(exts);
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _twoDMinAttr) == 0) {
				ist >> exts[0];ist >> exts[1];ist >> exts[2];
				GetBox()->SetLocalExtents(exts);
			}
			else if (StrCmpNoCase(attribName, _twoDMaxAttr) == 0) {
				ist >> exts[3];ist >> exts[4];ist >> exts[5];
				GetBox()->SetLocalExtents(exts);
			}
			else if (StrCmpNoCase(attribName, _cursorCoordsAttr) == 0) {
				ist >> cursorCoords[0];ist >> cursorCoords[1];
			}
		}
		return true;
	}
	pm->skipElement(tagString, depth);
	return true;
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
	if (linearInterp)
		oss << "true";
	else 
		oss << "false";
	attrs[_linearInterpAttr] = oss.str();

	oss.str(empty);
	oss << (double)GetHistoStretch();
	attrs[_histoStretchAttr] = oss.str();


	oss.str(empty);
	if (isMappedToTerrain())
		oss << "true";
	else 
		oss << "false";
	attrs[_terrainMapAttr] = oss.str();

	attrs[_heightVariableAttr] = heightVariableName;

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
	const vector<double>& exts = GetBox()->GetLocalExtents();
	attrs.clear();
	oss.str(empty);
	oss << (double)exts[0]<<" "<<(double)exts[1]<<" "<<(double)exts[2];
	attrs[_twoDMinAttr] = oss.str();
	oss.str(empty);
	oss << (double)exts[3]<<" "<<(double)exts[4]<<" "<<(double)exts[5];
	attrs[_twoDMaxAttr] = oss.str();
	oss.str(empty);
	oss << (double)cursorCoords[0]<<" "<<(double)cursorCoords[1];
	attrs[_cursorCoordsAttr] = oss.str();
	twoDDataNode->NewChild(_geometryTag, attrs, 0);
	return twoDDataNode;
}

MapperFunction* TwoDDataParams::GetMapperFunc() {
	return (numVariables > 0 ? transFunc[firstVarNum] : 0);
}

void TwoDDataParams::setMinColorMapBound(float val){
	GetMapperFunc()->setMinColorMapValue(val);
}
void TwoDDataParams::setMaxColorMapBound(float val){
	GetMapperFunc()->setMaxColorMapValue(val);
}


void TwoDDataParams::setMinOpacMapBound(float val){
	GetMapperFunc()->setMinOpacMapValue(val);
}
void TwoDDataParams::setMaxOpacMapBound(float val){
	GetMapperFunc()->setMaxOpacMapValue(val);
}

//Clear out the cache.  
//just delete the elevation grid
void TwoDDataParams::setTwoDDirty(){
	
	_timestep = -1;
	_varname.clear();
	_reflevel = -1;
	_lod = -1;
	_usrExts.clear();

	setElevGridDirty(true);
	setAllBypass(false);
}

//Calculate the twoD texture (if it needs refreshing).
//It's kept (cached) in the twoD params
//If nonzero texture dimensions are provided, then the cached image
//is not affected 
const unsigned char* TwoDDataParams::
calcTwoDDataTexture(int ts, int &texWidth, int &texHeight){
	
	if (!isEnabled()) return 0;
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();

	if (!dataMgr) return 0;
	if (doBypass(ts)) return 0;

    if (! twoDIsDirty(ts)) {
		texWidth = _textureSizes[0];
		texHeight = _textureSizes[1];
		return(_texBuf); 
	}

	
	//Get the first variable name
	_varname.clear();
	_varname.push_back(ds->getVariableName2D(firstVarNum));
	_timestep = ts;
	
	RegularGrid* twoDGrid;
	_reflevel = GetRefinementLevel();
	_lod = GetCompressionLevel();
	double exts[6];
	_usrExts = dataMgr->GetExtents((size_t)ts);
	for (int i = 0; i<3; i++){
		exts[i] = getLocalTwoDMin(i)+_usrExts[i];
		exts[i+3] = getLocalTwoDMax(i)+_usrExts[i];
	}
	int rc = getGrids(ts, _varname, exts, &_reflevel, &_lod,  &twoDGrid);
	if(!rc){
		setBypass(ts);
		return 0;
	}
	if (linearInterpTex())twoDGrid->SetInterpolationOrder(1);
	else twoDGrid->SetInterpolationOrder(0);
	
	float a[2],b[2];  //transform of (x,y) is to (a[0]x+b[0],a[1]y+b[1])
	//Set up to transform from twoD into volume:
	float constValue[2];
	int mapDims[3];
	buildLocal2DTransform(a,b,constValue,mapDims);

	//Get the data dimensions (at this resolution):
	int dataSize[3];
	//Start by initializing extents
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(_reflevel,i);
	}
	//Now calculate the texture.
	//
	//For each pixel, map it into the volume.
	//The blkMin values tell you the offset to use.
	//The blkMax values tell how to stride through the data
	
	//We first map the coords in the twoD plane to the volume.  
	//Then we map the volume into the region provided by dataMgr
	//This is done for each of the variables,
	//The result is then mapped using the transfer function.
	float clut[256*4];
	TransferFunction* transFunc = GetTransFunc();
	assert(transFunc);
	transFunc->makeLut(clut);
	
	float twoDCoord[2];
	double dataCoord[3];
	const float* localExtents = ds->getLocalExtents();
	float extExtents[6]; //Extend extents 1/2 voxel on each side so no bdry issues.
	for (int i = 0; i<3; i++){
		float mid = (localExtents[i]+localExtents[i+3])*0.5;
		float halfExtendedSize = (localExtents[i+3]-localExtents[i])*0.5*(1.f+dataSize[i])/(float)(dataSize[i]);
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

	adjustTextureSize(_textureSizes);
	texWidth = _textureSizes[0];
	texHeight = _textureSizes[1];

	_texBuf = new unsigned char[texWidth*texHeight*4];

	//Loop over pixels in texture.  Pixel centers start at 1/2 pixel from edge
	int dataOrientation = ds->get2DOrientation(firstVarNum);
	float halfPixHt = 1./(float)texHeight;
	float halfPixWid = 1./(float)texWidth;

	for (int iy = 0; iy < texHeight; iy++){
		//Map iy to a value between -1 and 1
		// .5*h - 1 and 1 -.5*h, where h is the height of a pixel relative to [-1,1]
		
		twoDCoord[1] = halfPixHt-1.f + 2.f*(1.-halfPixHt)*(float)iy/(float)(texHeight-1);
		for (int ix = 0; ix < texWidth; ix++){
			bool dataOK = true;
			twoDCoord[0] = halfPixWid-1.f + 2.f*(1.-halfPixWid)*(float)ix/(float)(texWidth-1);
			//find the coords that the texture maps to
			//twoDCoord is the coord in the twoD slice, dataCoord is in data volume 
			dataCoord[mapDims[2]] = constValue[0];
			dataCoord[mapDims[0]] = twoDCoord[0]*a[0]+b[0];
			dataCoord[mapDims[1]] = twoDCoord[1]*a[1]+b[1];
			
			for (int i = 0; i< 3; i++){
				if (i == dataOrientation) continue;
				if (dataCoord[i] < extExtents[i] || dataCoord[i] > extExtents[i+3]) dataOK = false;
			}
			
			if(dataOK) { //find the coordinate in the data array
				//Convert to user coordinates
				
				for (int k=0; k<3; k++) dataCoord[k] += _usrExts[k];
				float varVal = twoDGrid->GetValue(dataCoord[0],dataCoord[1],dataCoord[2]);

				if (varVal != twoDGrid->GetMissingValue()){ 
	
					//Use the transfer function to map the data:
					int lutIndex = transFunc->mapFloatToIndex(varVal);
				
					_texBuf[4*(ix+texWidth*iy)] = (unsigned char)(0.5+ clut[4*lutIndex]*255.f);
					_texBuf[4*(ix+texWidth*iy)+1] = (unsigned char)(0.5+ clut[4*lutIndex+1]*255.f);
					_texBuf[4*(ix+texWidth*iy)+2] = (unsigned char)(0.5+ clut[4*lutIndex+2]*255.f);
					_texBuf[4*(ix+texWidth*iy)+3] = (unsigned char)(0.5+ clut[4*lutIndex+3]*255.f);
				} else {dataOK = false;}
				
			}
			if(!dataOK) {//point out of region or missing value:
				_texBuf[4*(ix+texWidth*iy)] = 0;
				_texBuf[4*(ix+texWidth*iy)+1] = 0;
				_texBuf[4*(ix+texWidth*iy)+2] = 0;
				_texBuf[4*(ix+texWidth*iy)+3] = 0;
			}

		}//End loop over ix
	}//End loop over iy
	
	dataMgr->UnlockGrid(twoDGrid);
	delete twoDGrid;
	return _texBuf;
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
	int refLevel = GetRefinementLevel();
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(refLevel,i);
	}
	int dataOrientation = ds->get2DOrientation(firstVarNum);
	int xcrd = 0, ycrd = 1;
	if (dataOrientation < 2) ycrd++;
	if (dataOrientation < 1) xcrd++;
	const float* localExtents = ds->getLocalExtents();
	const vector<double>& box = GetBox()->GetLocalExtents();
	float relWid = (box[3+xcrd]-box[xcrd])/(localExtents[xcrd+3]);
	float relHt = (box[3+ycrd]-box[ycrd])/(localExtents[ycrd+3]);
	int xdist = (int)(relWid*dataSize[xcrd]);
	int ydist = (int)(relHt*dataSize[ycrd]);
	texSize[0] = 1<<(VetsUtil::ILog2(xdist));
	texSize[1] = 1<<(VetsUtil::ILog2(ydist));
	if (texSize[0] < 256) texSize[0] = 256;
	if (texSize[1] < 256) texSize[1] = 256;
	
	sz[0] = texSize[0];
	sz[1] = texSize[1];
	
}

bool TwoDDataParams::IsOpaque(){
	if(GetTransFunc()->isOpaque() && getOpacityScale() > 0.99f) return true;
	return false;
}


///
  
