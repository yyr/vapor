//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		dvrparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implementation of the dvrparams class
//		This contains all the parameters required to support the
//		dvr renderer.  Embeds a transfer function and a
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
#include "dvrparams.h"
#include "params.h"
#include "transferfunction.h"
 
#include <math.h>
#include <vapor/Metadata.h>
#include <vapor/Version.h>

#include "common.h"

using namespace VAPoR;

const string DvrParams::_activeVariableNameAttr = "ActiveVariableName";
const string DvrParams::_editModeAttr = "TFEditMode";
const string DvrParams::_histoStretchAttr = "HistoStretchFactor";
const string DvrParams::_dvrLightingAttr = "DVRLighting";
const string DvrParams::_dvrPreIntegrationAttr = "DVRPreIntegration";
const string DvrParams::_numBitsAttr = "BitsPerVoxel";
int DvrParams::defaultBitsPerVoxel = 8;
bool DvrParams::defaultPreIntegrationEnabled = false;
bool DvrParams::defaultLightingEnabled = false;

DvrParams::DvrParams(int winnum) : RenderParams(winnum)
{
	thisParamType = DvrParamsType;
	numBits = defaultBitsPerVoxel;
	numVariables = 0;
	type = DVR_INVALID_TYPE;
	restart();
	
}
DvrParams::~DvrParams(){
	if (transFunc){
		for (int i = 0; i< numVariables; i++){
			delete transFunc[i];  //will delete editor
		}
		delete transFunc;
	}
	
}

//Deepcopy requires cloning tf 
RenderParams* DvrParams::
deepRCopy(){
	DvrParams* newParams = new DvrParams(*this);
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
	if(numVariables>0)
		newParams->transFunc = new TransferFunction*[numVariables];
	else newParams->transFunc = 0;

	for (int i = 0; i<numVariables; i++){
		newParams->transFunc[i] = new TransferFunction(*transFunc[i]);
	}
	
	return newParams;
}
//Method called when undo/redo changes params:


void DvrParams::
refreshCtab() {
	((TransferFunction*)getMapperFunc())->makeLut((float*)ctab);
}
	



//Change variable, plus other side-effects, updating tf.
//Should only be called by gui.  The value of varnum is NOT the same as
//the index in the variableName combo.
//
void DvrParams::
setVarNum(int val) 
{
	varNum = val;
}


void DvrParams::
setClut(const float newTable[256][4]){
	for (int i = 0; i< 256; i++) {
		for (int j = 0; j< 4; j++){
			ctab[i][j] = newTable[i][j];
		}
	}
}

float DvrParams::getOpacityScale() 
{
  if (numVariables)
  {
    return transFunc[varNum]->getOpacityScaleFactor();
  }

  return 1.0;
}

void DvrParams::setOpacityScale(float val) 
{
  if (numVariables)
  {
    return transFunc[varNum]->setOpacityScaleFactor(val);
  }
}




//Initialize for new metadata.  Keep old transfer functions
//
bool DvrParams::
reinit(bool doOverride){
	int i;
	DataStatus* ds = DataStatus::getInstance();
	
	int totNumVariables = ds->getNumSessionVariables();
	//See if current varNum is valid.  It needs to correspond to data
	//if not, reset to first variable that is present:
	if (varNum >= totNumVariables || 
		!ds->variableIsPresent(varNum)){
		varNum = -1;
		for (i = 0; i<totNumVariables; i++) {
			if (ds->variableIsPresent(i)){
				setVarNum(i);
				break;
			}
		}
	}
	if (varNum == -1){
		
		for (i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete transFunc;
		numVariables = 0;
		return false;
	}
	
	//Set up the numRefinements. 
	int maxNumRefinements = ds->getNumTransforms();
	
	if (doOverride){
		numRefinements = 0;
		numBits = defaultBitsPerVoxel;
		lightingOn = defaultLightingEnabled;
		preIntegrationOn = defaultPreIntegrationEnabled;
	} else {//Try to use existing value
		if (numRefinements > maxNumRefinements) numRefinements = maxNumRefinements;
	}
	//Create new arrays to hold bounds and transfer functions:
	assert(totNumVariables > 0);
	TransferFunction** newTransFunc = new TransferFunction*[totNumVariables];
	float* newMinEdit = new float[totNumVariables];
	float* newMaxEdit = new float[totNumVariables];
	//If we are overriding previous values, delete the transfer functions, create new ones.
	//Set the map bounds to the actual bounds in the data
	if (doOverride){
		for (i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		//Create new transfer functions, their editors, hook them up:
		
		for (i = 0; i<totNumVariables; i++){
			newTransFunc[i] = new TransferFunction(this, numBits);

			newTransFunc[i]->setMinMapValue(DataStatus::getInstance()->getDefaultDataMin(i));
			newTransFunc[i]->setMaxMapValue(DataStatus::getInstance()->getDefaultDataMax(i));
			newMinEdit[i] = DataStatus::getInstance()->getDefaultDataMin(i);
			newMaxEdit[i] = DataStatus::getInstance()->getDefaultDataMax(i);

            newTransFunc[i]->setVarNum(i);
		}
	} else { 
		//attempt to make use of existing transfer functions, edit ranges.
		//delete any that are no longer referenced
		for (i = 0; i<totNumVariables; i++){
			if(i<numVariables){
				newTransFunc[i] = transFunc[i];
				newMinEdit[i] = minColorEditBounds[i];
				newMaxEdit[i] = maxColorEditBounds[i];
			} else { 
				//attempt to make use of existing transfer functions, edit ranges.
				//delete any that are no longer referenced
				for (i = 0; i<totNumVariables; i++){
					if(i<numVariables){
						newTransFunc[i] = transFunc[i];
						newMinEdit[i] = minColorEditBounds[i];
						newMaxEdit[i] = maxColorEditBounds[i];
					} else { //create new tf
						newTransFunc[i] = new TransferFunction(this, numBits);

						newTransFunc[i]->setMinMapValue(DataStatus::getInstance()->getDefaultDataMin(i));
						newTransFunc[i]->setMaxMapValue(DataStatus::getInstance()->getDefaultDataMax(i));
						newMinEdit[i] = DataStatus::getInstance()->getDefaultDataMin(i);
						newMaxEdit[i] = DataStatus::getInstance()->getDefaultDataMax(i);
						newTransFunc[i]->setVarNum(i);
					}
				}
			}
			
		}
			//Delete trans funcs that are not in the session
		for (i = totNumVariables; i<numVariables; i++){
			delete transFunc[i];
		}
	} //end if(doOverride)
	//Make sure edit bounds are valid
	for(i = 0; i<totNumVariables; i++){
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
	minOpacEditBounds = new float[totNumVariables];
	maxOpacEditBounds = new float[totNumVariables];
	for (i = 0; i<totNumVariables; i++){
		minOpacEditBounds[i] = minColorEditBounds[i];
		maxOpacEditBounds[i] = maxColorEditBounds[i];
	}

	transFunc = newTransFunc;
	
	numVariables = totNumVariables;
	//bool wasEnabled = enabled;
	//setEnabled(false);
	
	return true;
}
//Initialize to default state
//
void DvrParams::
restart(){
	histoStretchFactor = 1.f;
	varNum = 0;
	lightingOn = defaultLightingEnabled;
    preIntegrationOn = defaultPreIntegrationEnabled;
	numBits = defaultBitsPerVoxel;
	
	if(numVariables > 0){
		for (int i = 0; i<numVariables; i++){
			delete transFunc[i];
		}
		delete transFunc;
	}
	numVariables = 0;
	numRefinements = 0;
	
	transFunc = 0;
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
	
	
}


//Hook up the new transfer function in specified slot,
//Delete the old one.  This is called whenever a new tf is loaded.
//
void DvrParams::
hookupTF(TransferFunction* tf, int index){

	//Create a new TFEditor
	if (transFunc[index]) delete transFunc[index];
	transFunc[index] = tf;

	minColorEditBounds[index] = tf->getMinMapValue();
	maxColorEditBounds[index] = tf->getMaxMapValue();
	tf->setParams(this);
	tf->setColorVarNum(varNum);
	tf->setOpacVarNum(varNum);
}

//Handlers for Expat parsing.
//
bool DvrParams::
elementStartHandler(ExpatParseMgr* pm, int depth , std::string& tagString, const char **attrs){
	
	int i;
	static string varName;  //name in variable node
	string activeVarName;   //name of current variable
	if (StrCmpNoCase(tagString, _dvrParamsTag) == 0) {
		int newNumVariables = 0;
		//If it's a Dvr tag, save 5 attributes (2 are from Params class)
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
			else if (StrCmpNoCase(attribName, _activeVariableNameAttr) == 0) {
				ist >> activeVarName;
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				//Ignore
			}
			else if (StrCmpNoCase(attribName, _histoStretchAttr) == 0){
				float histStretch;
				ist >> histStretch;
				setHistoStretch(histStretch);
			}
			else if (StrCmpNoCase(attribName, _editModeAttr) == 0){
				if (value == "true") setEditMode(true); 
				else setEditMode(false);
			}
			else if (StrCmpNoCase(attribName, _dvrLightingAttr) == 0) {
				if (value == "true") setLighting(true); else setLighting(false);
			}
			else if (StrCmpNoCase(attribName, _dvrPreIntegrationAttr) == 0) {
              if (value == "true") setPreIntegration(true); else setPreIntegration(false);
			}
			else if (StrCmpNoCase(attribName, _numBitsAttr) == 0){
				ist >> numBits;
			}
            else return false;
		}
		// Now set the values obtained from attribute parsing.
		//Need to match up the varName with the varNum!!
		setVarNum(DataStatus::getInstance()->mergeVariableName(activeVarName));
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
		transFunc = 0;
		numVariables = newNumVariables;
		if (numVariables > 0)
			transFunc = new TransferFunction*[numVariables];
		//Create default transfer functions and editors
		for (int j = 0; j<numVariables; j++){
			transFunc[j] = new TransferFunction(this, numBits);
            transFunc[j]->setVarNum(j);
		}
		
		return true;
	}
	//Parse a Variable:
	else if (StrCmpNoCase(tagString, _variableTag) == 0) {
		
		float leftEdit = 0.f;
		float rightEdit = 1.f;
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
				ist >> varName;
			}
			else if (StrCmpNoCase(attribName, _opacityScaleAttr) == 0){
				ist >> opacFac;
				//Prior to VAPOR 1.2.3, the square root of the actual opacity scale was saved
				DataStatus* ds = DataStatus::getInstance();
				const string& sesver = ds->getSessionVersion();
				if (Version::Compare(sesver, "1.2.2") <= 0) 
					opacFac = opacFac*opacFac;
			}
			else return false;
		}
		// Now set the values obtained from attribute parsing.
		//Need to match up the varName with the varNum!!
		int vnum = DataStatus::getInstance()->mergeVariableName(varName);
		
		minColorEditBounds[vnum] = leftEdit;
		maxColorEditBounds[vnum] = rightEdit;
		transFunc[vnum]->setOpacityScaleFactor(opacFac);
        transFunc[vnum]->setVarNum(vnum);
		return true;
	}
	//Parse a transferFunction
	//Note we are (no longer) relying on parsedvarnum obtained by previous start handler:
	else if (StrCmpNoCase(tagString, TransferFunction::_transferFunctionTag) == 0) {
		//Need to "push" to transfer function parser.
		//That parser will "pop" back to dvrparams when done.
		int vnum = DataStatus::getInstance()->mergeVariableName(varName);
		pm->pushClassStack(transFunc[vnum]);

		transFunc[vnum]->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	}
	else return false;
}
//The end handler needs to pop the parse stack, not much else
bool DvrParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	
	if (StrCmpNoCase(tag, _dvrParamsTag) == 0) {
		//Align the editor
		setMinEditBound(getMinColorMapBound());
		setMaxEditBound(getMaxColorMapBound());
		//If we are current, update the tab panel
		//if (isCurrent()) updateDialog();
		//If this is a dvrparams, need to
		//pop the parse stack.  The caller will need to save the resulting
		//transfer function (i.e. this)
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0) {
		return true;
	} else if (StrCmpNoCase(tag, _variableTag) == 0){
		return true;
	} else {
		pm->parseError("Unrecognized end tag in DVRParams %s",tag.c_str());
		return false;  //Could there be other end tags that we ignore??
	}
}

//Method to construct Xml for state saving
XmlNode* DvrParams::
buildNode() {
	//Construct the dvr node
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

	//convert the active variable num to a name:
	string varName = DataStatus::getInstance()->getVariableName(varNum);
	oss.str(empty);
	oss << varName;
	attrs[_activeVariableNameAttr] = oss.str();

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
	if (lightingOn)
		oss << "true";
	else 
		oss << "false";
	attrs[_dvrLightingAttr] = oss.str();

	oss.str(empty);
	oss << (long)numBits;
	attrs[_numBitsAttr] = oss.str();

	oss.str(empty);
	if (preIntegrationOn)
		oss << "true";
	else 
		oss << "false";
	attrs[_dvrPreIntegrationAttr] = oss.str();

	XmlNode* dvrNode = new XmlNode(_dvrParamsTag, attrs, 3);

	//Now add children:  
	//Create the Variables nodes
	for (int i = 0; i<numVariables; i++){
		attrs.clear();

		oss.str(empty);
		oss << DataStatus::getInstance()->getVariableName(i);
		attrs[_variableNameAttr] = oss.str();

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
		dvrNode->AddChild(varNode);
	}
	return dvrNode;
}

MapperFunction* DvrParams::getMapperFunc() {
	return (numVariables > 0 ? transFunc[varNum] : 0);
}

void DvrParams::setMinColorMapBound(float val){
	getMapperFunc()->setMinColorMapValue(val);
}
void DvrParams::setMaxColorMapBound(float val){
	getMapperFunc()->setMaxColorMapValue(val);
}


void DvrParams::setMinOpacMapBound(float val){
	getMapperFunc()->setMinOpacMapValue(val);
}
void DvrParams::setMaxOpacMapBound(float val){
	getMapperFunc()->setMaxOpacMapValue(val);
}


