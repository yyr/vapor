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



#include "glutil.h"	// Must be included first!!!
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "probeparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "params.h"
#include "transferfunction.h"

#include "histo.h"
#include "animationparams.h"

#include <math.h>
#include <vapor/DataMgr.h>
#include <vapor/errorcodes.h>


using namespace VAPoR;
const string ProbeParams::_shortName = "Probe";
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
const string ProbeParams::_fieldVarsAttr = "FieldVariables";
const string ProbeParams::_mergeColorAttr = "MergeColors";
const string ProbeParams::_fieldScaleAttr = "FieldScale";
const string ProbeParams::_alphaAttr= "Alpha";
const string ProbeParams::_probeTypeAttr = "ProbeType";
const string ProbeParams::_linearInterpAttr = "LinearInterp";
float ProbeParams::defaultAlpha = 0.12f;
float ProbeParams::defaultScale = 1.0f;
float ProbeParams::defaultTheta = 0.0f;
float ProbeParams::defaultPhi = 0.0f;
float ProbeParams::defaultPsi = 0.0f;

ProbeParams::ProbeParams(int winnum) : RenderParams(winnum, Params::_probeParamsTag){
	myBox = 0;
	numVariables = 0;
	probeDataTextures = 0;
	probeIBFVTextures = 0;
	maxTimestep = 1;
	ibfvUField = 0;
	ibfvVField = 0;
	ibfvValid = 0;
	ibfvMag = -1.f;
	mergeColor = false;
	restart();
	
}
ProbeParams::~ProbeParams(){
	
	if (myBox) delete myBox;
	if (transFunc){
		for (int i = 0; i< numVariables; i++){
			delete transFunc[i];  //will delete editor
		}
		delete [] transFunc;
	}
	if (probeDataTextures) {
		for (int i = 0; i<= maxTimestep; i++){
			if (probeDataTextures[i]) delete probeDataTextures[i];
		}
		delete [] probeDataTextures;
	}
	if (probeIBFVTextures) {
		for (int i = 0; i<= maxTimestep; i++){
			if (probeIBFVTextures[i]) delete probeIBFVTextures[i];
		}
		delete [] probeIBFVTextures;
	}
	if (ibfvUField) {
		for (int i = 0; i<= maxTimestep; i++){
			if (ibfvUField[i]) delete ibfvUField[i];
			if (ibfvVField[i]) delete ibfvVField[i];
			if (ibfvValid[i]) delete ibfvValid[i];
		}
		delete [] ibfvUField;
		delete [] ibfvVField;
		delete [] ibfvValid;
	}
	ibfvMag = -1.f;
	
}


//Deepcopy requires cloning tf 
Params* ProbeParams::
deepCopy(ParamNode*){
	ProbeParams* newParams = new ProbeParams(*this);
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

	//Clone the Transfer Functions
	if (numVariables > 0){
		newParams->transFunc = new TransferFunction*[numVariables];
		for (int i = 0; i<numVariables; i++){
			newParams->transFunc[i] = new TransferFunction(*transFunc[i]);
		}
	} else newParams->transFunc = 0;
	//Probe texture must be recreated when needed
	newParams->probeDataTextures = 0;
	newParams->probeIBFVTextures = 0;
	newParams->ibfvUField = 0;
	newParams->ibfvVField = 0;
	newParams->ibfvValid = 0;
	newParams->ibfvMag = -1.f;
	
	//never keep the SavedCommand:
	
	return newParams;
}


void ProbeParams::
refreshCtab() {
	((TransferFunction*)GetMapperFunc())->makeLut((float*)ctab);
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
	DataStatus* ds = DataStatus::getInstance();
	const float* extents =ds->getLocalExtents();
	setMaxNumRefinements(ds->getNumTransforms());
	//Set up the numRefinements combo
	
	
	//Either set the probe bounds to a default size in the center of the domain, or 
	//try to use the previous bounds:
	double exts[6];
	if (doOverride){
		for (int i = 0; i<3; i++){
			float probeRadius = 0.1f*(extents[i+3] - extents[i]);
			float probeMid = 0.5f*(extents[i+3] - extents[i]);
			if (i<2) {
				exts[i] = probeMid - probeRadius;
				exts[i+3] = probeMid + probeRadius;
			} else {
				exts[i] = exts[i+3] = probeMid;
			}
		}
		//default values for phi, theta,  cursorPosition
		double angles[3];
		angles[1] = defaultPhi;
		angles[0] = defaultTheta;
		angles[2] = defaultPsi;
		GetBox()->SetAngles(angles);
		fieldScale = defaultScale;
		alpha = defaultAlpha;
		
		cursorCoords[0] = cursorCoords[1] = 0.0f;
		numRefinements = 0;
		SetFidelityLevel(0);
		SetIgnoreFidelity(false);
	} else {
		//Force the probe size to be no larger than the domain extents, Note that
		//because of rotation, the probe max/min may not correspond
		//to the same extents.
		GetBox()->GetLocalExtents(exts);
		if (DataStatus::pre22Session()){
			//In old session files, the coordinate of box extents were not 0-based
			float * offset = DataStatus::getPre22Offset();
			for (int i = 0; i<3; i++) {
				exts[i] -= offset[i];
				exts[i+3] -= offset[i];
			}
		}
		float maxExtents = Max(Max(extents[3]-extents[0],extents[4]-extents[1]),extents[5]-extents[2]);
		for (int i = 0; i<3; i++){
			if (exts[i+3] - exts[i] > maxExtents)
				exts[i+3] = exts[i] + maxExtents;
		
			if(exts[i+3] < exts[i]) 
				exts[i+3] = exts[i];
		}
		if (numRefinements > maxNumRefinements) numRefinements = maxNumRefinements;
	}
	GetBox()->SetLocalExtents(exts);

	//Make sure fidelity is valid:
	int fidelity = GetFidelityLevel();
	DataMgr* dataMgr = ds->getDataMgr();
	if (dataMgr && fidelity > maxNumRefinements+dataMgr->GetCRatios().size()-1)
		SetFidelityLevel(maxNumRefinements+dataMgr->GetCRatios().size()-1);
	//Get the variable names:

	int newNumVariables = ds->getNumSessionVariables();
	
	//See if current firstVarNum is valid
	//if not, reset to first variable that is present:
	if (!ds->variableIsPresent3D(firstVarNum)){
		firstVarNum = -1;
		for (int i = 0; i<newNumVariables; i++) {
			if (ds->variableIsPresent3D(i)){
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
				!ds->variableIsPresent3D(i))
				variableSelected[i] = false;
		}
	}
	variableSelected[firstVarNum] = true;
	numVariablesSelected = 0;
	for (int i = 0; i< newNumVariables; i++){
		if (variableSelected[i]) numVariablesSelected++;
	}

	// set up the ibfv variables
	int numComboVariables = ds->getNumActiveVariables3D()+1;
	if (doOverride){
		for (int j = 0; j<3; j++){
			ibfvComboVarNum[j] = Min(j+1,numComboVariables);
			ibfvSessionVarNum[j] = ds->mapActiveToSessionVarNum3D(ibfvComboVarNum[j]-1)+1;
		}
	} 
	for (int dim = 0; dim < 3; dim++){
		//See if current varNum is valid.  If not, 
		//reset to first variable that is present:
		if (ibfvSessionVarNum[dim] > 0) {
			if (!ds->variableIsPresent3D(ibfvSessionVarNum[dim]-1)){
				ibfvSessionVarNum[dim] = -1;
				for (int i = 0; i<newNumVariables; i++) {
					if (ds->variableIsPresent3D(i)){
						ibfvSessionVarNum[dim] = i+1;
						break;
					}
				}
			}
		}
	}
	if (ibfvSessionVarNum[0] == -1){
		return false;
	}
	//Determine the combo var nums from the varNum's
	for (int dim = 0; dim < 3; dim++){
		if(ibfvSessionVarNum[dim] == 0) ibfvComboVarNum[dim] = 0;
		else 
			ibfvComboVarNum[dim] = ds->mapSessionToActiveVarNum3D(ibfvSessionVarNum[dim]-1)+1;
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
			string varname = ds->getVariableName3D(i);
			newTransFunc[i] = new TransferFunction(this, 8);
			//Initialize to be fully opaque:
			newTransFunc[i]->setOpaque();

			newTransFunc[i]->setMinMapValue(ds->getDefaultDataMin3D(i,usingVariable(varname)));
			newTransFunc[i]->setMaxMapValue(ds->getDefaultDataMax3D(i,usingVariable(varname)));
			newMinEdit[i] = ds->getDefaultDataMin3D(i,usingVariable(varname));
			newMaxEdit[i] = ds->getDefaultDataMax3D(i,usingVariable(varname));

            newTransFunc[i]->setVarNum(i);
		}
	} else { 
		//attempt to make use of existing transfer functions, edit ranges.
		//delete any that are no longer referenced
		for (int i = 0; i<newNumVariables; i++){
			string varname = ds->getVariableName3D(i);
			if(i<numVariables){
				newTransFunc[i] = transFunc[i];
				newMinEdit[i] = minColorEditBounds[i];
				newMaxEdit[i] = maxColorEditBounds[i];
			} else { //create new tf
				newTransFunc[i] = new TransferFunction(this, 8);
				//Initialize to be fully opaque:
				newTransFunc[i]->setOpaque();

				newTransFunc[i]->setMinMapValue(ds->getDefaultDataMin3D(i,usingVariable(varname)));
				newTransFunc[i]->setMaxMapValue(ds->getDefaultDataMax3D(i,usingVariable(varname)));
				newMinEdit[i] = ds->getDefaultDataMin3D(i,usingVariable(varname));
				newMaxEdit[i] = ds->getDefaultDataMax3D(i,usingVariable(varname));
                newTransFunc[i]->setVarNum(i);
			}
		}
			//Delete extra trans funcs 
		for (int i = newNumVariables; i<numVariables; i++){
			delete transFunc[i];
		}
	} //end if(doOverride)
	//In any case the currentDatarange should be set to the bounds on firstVarNum
	if (firstVarNum>=0)
		setCurrentDatarange(newTransFunc[firstVarNum]->getMinMapValue(),newTransFunc[firstVarNum]->getMaxMapValue());
	//Make sure edit bounds are valid
	for(int i = 0; i<newNumVariables; i++){
		string varname = ds->getVariableName3D(i);
		if (newMinEdit[i] >= newMaxEdit[i]){
			newMinEdit[i] = ds->getDefaultDataMin3D(i,usingVariable(varname));
			newMaxEdit[i] = ds->getDefaultDataMax3D(i,usingVariable(varname));
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
	setProbeDirty();
	if (probeDataTextures) delete [] probeDataTextures;
	if (probeIBFVTextures) delete [] probeIBFVTextures;
	if (ibfvUField) delete [] ibfvUField;
	if (ibfvVField) delete [] ibfvVField;
	if (ibfvValid) delete [] ibfvValid;
	maxTimestep = ds->getNumTimesteps()-1;
	probeDataTextures = 0;
	probeIBFVTextures = 0;
	ibfvUField = 0;
	ibfvVField = 0;
	initializeBypassFlags();
	return true;
}
//Initialize to default state
//
void ProbeParams::
restart(){
	probeType = 0;
	planar = true;
	textureSize[0] = textureSize[1] = 0;
	histoStretchFactor = 1.f;
	firstVarNum = 0;
	compressionLevel = 0;
	if (!myBox){
		myBox = new Box();
	}
	SetIgnoreFidelity(false);
	setProbeDirty();
	if (probeDataTextures) delete [] probeDataTextures;
	probeDataTextures = 0;
	if (probeIBFVTextures) delete [] probeIBFVTextures;
	probeIBFVTextures = 0;
	if (ibfvUField) delete [] ibfvUField;
	if (ibfvVField) delete [] ibfvVField;
	if (ibfvValid) delete [] ibfvValid;
	ibfvUField = 0;
	ibfvVField = 0;
	ibfvMag = -1.f;
	mergeColor = false;
	ibfvSessionVarNum[0]= 1;
	ibfvSessionVarNum[1] = 2;
	ibfvSessionVarNum[2] = 3;
	ibfvComboVarNum[0]= 1;
	ibfvComboVarNum[1] = 2;
	ibfvComboVarNum[2] = 3;

	alpha = defaultAlpha;
	
	fieldScale = defaultScale;
	
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
	double angles[3];
	angles[0] = defaultTheta;
	angles[1] = defaultPhi;
	angles[2] = defaultPsi;
	GetBox()->SetAngles(angles);
	double exts[6];
	for (int i = 0; i<3; i++){
		if (i < 2) exts[i] = 0.4f;
		else exts[i] = 0.5f;
		if(i<2) exts[i+3] = 0.6f;
		else exts[i+3] = 0.5f;
		selectPoint[i] = 0.5f;
	}
	GetBox()->SetLocalExtents(exts);
	NPN = 0;
	NMESH = 0;
	linearInterp = true;
	
}
void ProbeParams::setDefaultPrefs(){
	defaultAlpha = 0.12f;
	defaultScale = 1.0f;
	defaultTheta = 0.0f;
	defaultPhi = 0.0f;
	defaultPsi = 0.0f;
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
	
	if (StrCmpNoCase(tagString, _probeParamsTag) == 0) {
		//default to linear interpolation off, for old session files
		setLinearInterp(false);
		int newNumVariables = 0;
		SetIgnoreFidelity(true);
		SetFidelityLevel(0);
		//If it's a Probe tag, obtain 10 attributes (2 are from Params class)
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
			else if (StrCmpNoCase(attribName, _CompressionLevelTag) == 0){
				ist >> compressionLevel;
			}
			else if (StrCmpNoCase(attribName, _FidelityLevelTag) == 0){
				int fid;
				ist >> fid;
				SetFidelityLevel(fid);
			}
			else if (StrCmpNoCase(attribName, _IgnoreFidelityTag) == 0){
				if (value == "true") SetIgnoreFidelity(true); 
				else SetIgnoreFidelity(false);
			}
			else if (StrCmpNoCase(attribName, _editModeAttr) == 0){
				if (value == "true") setEditMode(true); 
				else setEditMode(false);
			}
			else if (StrCmpNoCase(attribName, _linearInterpAttr) == 0){
				if (value == "true") setLinearInterp(true); 
				else setLinearInterp(false);
			}
			else if (StrCmpNoCase(attribName, _planarAttr) == 0){
				if (value == "true") setPlanar(true); 
				else setPlanar(false);
			}
			else if (StrCmpNoCase(attribName, _mergeColorAttr) == 0){
				if (value == "true") setIBFVColorMerged(true); 
				else setIBFVColorMerged(false);
			}
			else if (StrCmpNoCase(attribName, _alphaAttr) == 0){
				ist >> alpha;
			}
			else if (StrCmpNoCase(attribName, _fieldScaleAttr) == 0){
				ist >> fieldScale;
			}
			else if (StrCmpNoCase(attribName, _probeTypeAttr) == 0){
				ist >> probeType;
			}
			else if (StrCmpNoCase(attribName, _fieldVarsAttr) == 0) {
				string vName;
				for (int i = 0; i< 3; i++){
					ist >> vName;//peel off the steady name
					if (strcmp(vName.c_str(),"0") != 0){
						int varnum = DataStatus::getInstance()->mergeVariableName(vName);
						ibfvSessionVarNum[i] = varnum+1;
					} else {
						ibfvSessionVarNum[i] = 0;
					}
					//The combo setting will need to change when/if the variable is
					//read in the metadata
					ibfvComboVarNum[i] = -1;
				}
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
		for (int i = 0; i<newNumVariables; i++){
			variableSelected.push_back(false);
		}
		//Setup with default values, in case not specified:
		for (int i = 0; i< newNumVariables; i++){
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
		float exts[6], angles[3];
		GetBox()->GetLocalExtents(exts);
		GetBox()->GetAngles(angles);
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			
			if (StrCmpNoCase(attribName, _thetaAttr) == 0) {
				ist >> angles[0];
				GetBox()->SetAngles(angles);
			}
			else if (StrCmpNoCase(attribName, _phiAttr) == 0) {
				ist >> angles[1];
				GetBox()->SetAngles(angles);
			}
			else if (StrCmpNoCase(attribName, _psiAttr) == 0) {
				ist >> angles[2];
				GetBox()->SetAngles(angles);
			}
			else if (StrCmpNoCase(attribName, _probeMinAttr) == 0) {
				ist >> exts[0];ist >> exts[1];ist >> exts[2];
				GetBox()->SetLocalExtents(exts);
			}
			else if (StrCmpNoCase(attribName, _probeMaxAttr) == 0) {
				ist >> exts[3];ist >> exts[4];ist >> exts[5];
				GetBox()->SetLocalExtents(exts);
			}
			else if (StrCmpNoCase(attribName, _cursorCoordsAttr) == 0) {
				ist >> cursorCoords[0];ist >> cursorCoords[1];
			}
		}
		return true;
	}
	else {
		pm->skipElement(tagString, depth);
		return true;
	}
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
ParamNode* ProbeParams::
buildNode() {
	//Construct the probe node
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
	oss << (int)GetFidelityLevel();
	attrs[_FidelityLevelTag] = oss.str();
	oss.str(empty);
	if (GetIgnoreFidelity())
		oss << "true";
	else 
		oss << "false";
	attrs[_IgnoreFidelityTag] = oss.str();
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
	if (linearInterp)
		oss << "true";
	else 
		oss << "false";
	attrs[_linearInterpAttr] = oss.str();

	oss.str(empty);
	oss << (double)GetHistoStretch();
	attrs[_histoStretchAttr] = oss.str();
	
	oss.str(empty);
	for (int i = 0; i<3; i++){
		string varName = "0";
		if (ibfvSessionVarNum[i]>0) varName = DataStatus::getInstance()->getVariableName3D(ibfvSessionVarNum[i]-1);
		oss << varName << " ";
	}
	attrs[_fieldVarsAttr] = oss.str();

	oss.str(empty);
	oss << (double)getAlpha();
	attrs[_alphaAttr] = oss.str();

	oss.str(empty);
	oss << (double)getFieldScale();
	attrs[_fieldScaleAttr] = oss.str();

	oss.str(empty);
	oss << (long)getProbeType();
	attrs[_probeTypeAttr] = oss.str();

	oss.str(empty);
	if (ibfvColorMerged()) oss << "true"; else oss << "false";
	attrs[_mergeColorAttr] = oss.str();

	ParamNode* probeNode = new ParamNode(_probeParamsTag, attrs, 3);

	//Now add children:  
	//Create the Variables nodes
	for (int i = 0; i<numVariables; i++){
		attrs.clear();
	

		oss.str(empty);
		oss << DataStatus::getInstance()->getVariableName3D(i);
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
		probeNode->AddChild(varNode);
	}
	//Now do geometry node:
	const vector<double>& angles = GetBox()->GetAngles();
	attrs.clear();
	oss.str(empty);
	oss << (double)angles[1];
	attrs[_phiAttr] = oss.str();
	oss.str(empty);
	oss << (double)angles[2];
	attrs[_psiAttr] = oss.str();
	oss.str(empty);
	oss << (double) angles[0];
	attrs[_thetaAttr] = oss.str();

	const vector<double>& exts = GetBox()->GetLocalExtents();
	oss.str(empty);
	oss << (double)exts[0]<<" "<<(double)exts[1]<<" "<<(double)exts[2];
	attrs[_probeMinAttr] = oss.str();
	oss.str(empty);
	oss << (double)exts[3]<<" "<<(double)exts[4]<<" "<<(double)exts[5];
	attrs[_probeMaxAttr] = oss.str();
	oss.str(empty);
	oss << (double)cursorCoords[0]<<" "<<(double)cursorCoords[1];
	attrs[_cursorCoordsAttr] = oss.str();
	probeNode->NewChild(_geometryTag, attrs, 0);
	return probeNode;
}

MapperFunction* ProbeParams::GetMapperFunc() {
	return (numVariables > 0 ? transFunc[firstVarNum] : 0);
}

void ProbeParams::setMinColorMapBound(float val){
	GetMapperFunc()->setMinColorMapValue(val);
}
void ProbeParams::setMaxColorMapBound(float val){
	GetMapperFunc()->setMaxColorMapValue(val);
}


void ProbeParams::setMinOpacMapBound(float val){
	GetMapperFunc()->setMinOpacMapValue(val);
}
void ProbeParams::setMaxOpacMapBound(float val){
	GetMapperFunc()->setMaxOpacMapValue(val);
}


//Clear out the cache
void ProbeParams::setProbeDirty(){
	if (probeDataTextures){
		for (int i = 0; i<=maxTimestep; i++){
			if (probeDataTextures[i]) {
				delete [] probeDataTextures[i];
				probeDataTextures[i] = 0;
			}
		}
	}
	if (probeIBFVTextures){
		for (int i = 0; i<=maxTimestep; i++){
			if (probeIBFVTextures[i]) {
				delete [] probeIBFVTextures[i];
				probeIBFVTextures[i] = 0;
			}
		}
	}
	textureSize[0]= textureSize[1] = 0;
	if (ibfvUField) {
		for (int i = 0; i<= maxTimestep; i++){
			if (ibfvUField[i]) delete ibfvUField[i];
			ibfvUField[i] = 0;
			if (ibfvVField[i])delete ibfvVField[i];
			ibfvVField[i] = 0;
			if (ibfvValid[i]) delete ibfvValid[i];
			ibfvValid[i] = 0;
		}
	}
	ibfvMag = -1.f;
	setAllBypass(false);
}


//Calculate the probe texture (if it needs refreshing).
//It's kept (cached) in the probe params
//If nonzero texture dimensions are provided, then the cached image
//is not affected 
unsigned char* ProbeParams::
calcProbeDataTexture(int ts, int texWidth, int texHeight){
	
	
	if (!isEnabled()) return 0;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return 0;

	if (doBypass(ts)) return 0;
	bool doCache = (texWidth == 0 && texHeight == 0);

	int actualRefLevel = GetRefinementLevel();
	int lod = GetCompressionLevel();
		
	const vector<double>&userExts = ds->getDataMgr()->GetExtents((size_t)ts);
	RegularGrid* probeGrid;
	vector<string>varnames;
	varnames.push_back(ds->getVariableName3D(firstVarNum));
	double extents[6];
	float boxmin[3],boxmax[3];
	getLocalContainingRegion(boxmin, boxmax);
	for (int i = 0; i<3; i++){
		extents[i] = boxmin[i]+userExts[i];
		extents[i+3] = boxmax[i]+userExts[i];
	}
	
	int rc = getGrids( ts, varnames, extents, &actualRefLevel, &lod, &probeGrid);
	
	if(!rc){
		return 0;
	}
	
	if (linearInterpTex())probeGrid->SetInterpolationOrder(1);
	else probeGrid->SetInterpolationOrder(0);

	float transformMatrix[12];
	//Set up to transform from probe into volume:
	buildLocalCoordTransform(transformMatrix, 0.f, -1);

	//Get the data dimensions (at this resolution):
	int dataSize[3];
	//Start by initializing extents
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(actualRefLevel,i);
	}
	//Now calculate the texture.
	//
	//For each pixel, map it into the volume.
	
	
	//We first map the coords in the probe to the volume.  
	//Then we map the volume into the region provided by dataMgr
	
	float clut[256*4];
	TransferFunction* transFunc = GetTransFunc();
	assert(transFunc);
	transFunc->makeLut(clut);
	
	float probeCoord[3];
	double dataCoord[3];
	const float* sizes = ds->getFullSizes();
	float extExtents[6]; //Extend extents 1/2 voxel on each side so no bdry issues.
	for (int i = 0; i<3; i++){
		float mid = (sizes[i])*0.5;
		float halfExtendedSize = sizes[i]*0.5*(1.f+dataSize[i])/(float)(dataSize[i]);
		extExtents[i] = mid - halfExtendedSize;
		extExtents[i+3] = mid + halfExtendedSize;
	}
	//Can ignore depth, just mapping center plane
	probeCoord[2] = 0.f;
	
	if (doCache) {
		int txsize[2];
		adjustTextureSize(txsize);
		texWidth = txsize[0];
		texHeight = txsize[1];
	}
	
	unsigned char* probeTexture = new unsigned char[texWidth*texHeight*4];

	//Use the region reader to calculate coordinates in volume
	DataMgr* dataMgr = ds->getDataMgr();

	//Loop over pixels in texture.  Pixel centers map to edges of probe
	
	for (int iy = 0; iy < texHeight; iy++){
		//Map iy to a value between -1 and 1
		probeCoord[1] = -1.f + 2.f*(float)iy/(float)(texHeight-1);
		for (int ix = 0; ix < texWidth; ix++){
			
			
			probeCoord[0] = -1.f + 2.f*(float)ix/(float)(texWidth-1);
			vtransform(probeCoord, transformMatrix, dataCoord);
			//find the coords that the texture maps to
			//probeCoord is the coord in the probe, dataCoord is in data volume 
			bool dataOK = true;
			for (int i = 0; i< 3; i++){
				if (dataCoord[i] < extExtents[i] || dataCoord[i] > extExtents[i+3]) dataOK = false;
				dataCoord[i] += userExts[i]; //Convert to user coordinates.
			}
			float varVal;
			if(dataOK) { //find the coordinate in the data array
				
				varVal = probeGrid->GetValue(dataCoord[0],dataCoord[1],dataCoord[2]);
				if (varVal == probeGrid->GetMissingValue()) dataOK = false;
			}
			if (dataOK) {				
				//Use the transfer function to map the data:
				int lutIndex = transFunc->mapFloatToIndex(varVal);
				
				probeTexture[4*(ix+texWidth*iy)] = (unsigned char)(0.5+ clut[4*lutIndex]*255.f);
				probeTexture[4*(ix+texWidth*iy)+1] = (unsigned char)(0.5+ clut[4*lutIndex+1]*255.f);
				probeTexture[4*(ix+texWidth*iy)+2] = (unsigned char)(0.5+ clut[4*lutIndex+2]*255.f);
				probeTexture[4*(ix+texWidth*iy)+3] = (unsigned char)(0.5+ clut[4*lutIndex+3]*255.f);
			}
				
			else {//point out of region, or missing value
				probeTexture[4*(ix+texWidth*iy)] = 0;
				probeTexture[4*(ix+texWidth*iy)+1] = 0;
				probeTexture[4*(ix+texWidth*iy)+2] = 0;
				probeTexture[4*(ix+texWidth*iy)+3] = 0;
			}

		}//End loop over ix
	}//End loop over iy
	
	if (doCache) setProbeTexture(probeTexture,ts, 0);
	dataMgr->UnlockGrid(probeGrid);
	delete probeGrid;
	return probeTexture;
}

void ProbeParams::adjustTextureSize(int sz[2]){
	//Need to determine appropriate texture dimensions
	
	//First, map the corners of texture, to determine appropriate
	//texture sizes and image aspect ratio:
	//Get the data dimensions (at this resolution):
	int dataSize[3];
	//Start by initializing extents
	DataStatus* ds = DataStatus::getInstance();
	int refLevel = GetRefinementLevel();
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(refLevel,i);
	}
	int icor[4][3];
	float probeCoord[3];
	float dataCoord[3];
	
	
	const float* fullSizes = ds->getFullSizes();
	//Can ignore depth, just mapping center plane
	probeCoord[2] = 0.f;

	float transformMatrix[12];
	
	//Set up to transform from probe into volume:
	buildLocalCoordTransform(transformMatrix, 0.f, -1);
	
	for (int cornum = 0; cornum < 4; cornum++){
		// coords relative to (-1,1)
		probeCoord[1] = -1.f + 2.f*(float)(cornum/2);
		probeCoord[0] = -1.f + 2.f*(float)(cornum%2);
		//Then transform to values in data 
		vtransform(probeCoord, transformMatrix, dataCoord);
		//Then get array coords:
		for (int i = 0; i<3; i++){
			icor[cornum][i] = (size_t) (0.5f+(float)dataSize[i]*dataCoord[i]/fullSizes[i]);
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
	if (textureSize[0] > 2048) textureSize[0] = 2048;
	if (textureSize[1] > 2048) textureSize[1] = 2048;
	sz[0] = textureSize[0];
	sz[1] = textureSize[1];
	//For IBFV texture, the size is always 256, however
	//NPN and NMESH may be larger than their minima (64 and 100)
	if (probeType == 1) {
		int maxTex = Max(textureSize[0],textureSize[1]);
		maxTex = maxTex/256;
		NPN = maxTex*64;
		NMESH = 100*maxTex;
		sz[0] = textureSize[0] = 256;
		sz[1] = textureSize[1] = 256;
	}
	
}

//Advect the point (x,y) in the probe to the point (*px, *py)
//Requres that buildIBFVFields be called first!
//Input xa,ya are between 0 and 1, and *px, *py are in same coord space.
//Requires valid data
void ProbeParams::getIBFVValue(int ts, float xa, float ya, float* px, float* py){
	assert(ibfvUField && ibfvUField[ts]);
	assert(xa >= 0.f && ya >= 0.f && xa < 1.f && ya < 1.f);
	//convert xa and ya to grid coords
	float x = xa * (float)(textureSize[0]-1);
	float y = ya * (float)(textureSize[1]-1);
	
	float xfrac = x - floor(x);
	float yfrac = y - floor(y);
	float u00 = ibfvUField[ts][(int)x + textureSize[0]*(int)y];
	float u10 = ibfvUField[ts][1+(int)x + textureSize[0]*(int)y];
	float u11 = ibfvUField[ts][1+(int)x + textureSize[0]*(1+(int)y)];
	float u01 = ibfvUField[ts][(int)x + textureSize[0]*(1+(int)y)];
	
	float uval = (1.-xfrac)*((1.-yfrac)*u00+yfrac*u01)+
		xfrac*((1.-yfrac)*u10+yfrac*u11);

	float v00 = ibfvVField[ts][(int)x + textureSize[0]*(int)y];
	float v10 = ibfvVField[ts][1+(int)x + textureSize[0]*(int)y];
	float v11 = ibfvVField[ts][1+(int)x + textureSize[0]*(1+(int)y)];
	float v01 = ibfvVField[ts][(int)x + textureSize[0]*(1+(int)y)];
	float vval = (1.-xfrac)*((1.-yfrac)*v00+yfrac*v01)+
		xfrac*((1.-yfrac)*v10+yfrac*v11);

	//Resulting u, v has already been scaled to account for non-equal x,y dimensions,
	//also multiplied by fieldScale and 0.01
	float r = uval*uval+vval*vval;
	if (r > 16.f*fieldScale*fieldScale/(textureSize[0]*textureSize[1])) { 
      r  = sqrt(r); 
      uval *= 4.f*fieldScale/(textureSize[0]*r); 
      vval *= 4.f*fieldScale/(textureSize[1]*r);  
   }
	*px = xa + uval;
	*py = ya + vval;
	
}
bool ProbeParams::buildIBFVFields(int timestep){
	//Set up the cache if it's not ready:
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return false;
	if (!ibfvUField) {
		ibfvUField = new float*[maxTimestep + 1];
		ibfvVField = new float*[maxTimestep + 1];
		ibfvValid = new unsigned char*[maxTimestep + 1];
		ibfvMag = -1.f;
		for (int i = 0; i<= maxTimestep; i++) {ibfvUField[i] = 0; ibfvVField[i] = 0; ibfvValid[i] = 0;}
	}
	//If the required data is valid, just return true:
	if(ibfvUField[timestep] && ibfvVField[timestep]) return true;
	
	const vector<double>&userExts = ds->getDataMgr()->GetExtents((size_t)timestep);
	//Now obtain the field values for the current probe
	//Session variable nums are -1 if the variable is 0
	vector<string> varnames;
	for (int i = 0; i<3; i++){
		int svnum = ibfvSessionVarNum[i]-1;
		if (svnum < 0) varnames.push_back("0");
		else varnames.push_back(ds->getVariableName3D(svnum));
	}
	double extents[6];
	float boxmin[3],boxmax[3];
	getLocalContainingRegion(boxmin, boxmax);
	for (int i = 0; i<3; i++){
		extents[i] = boxmin[i]+userExts[i];
		extents[i+3] = boxmax[i]+userExts[i];
	}							
	int actualRefLevel = GetRefinementLevel();
	int lod = GetCompressionLevel();
	RegularGrid* grids[3];
	
	int rc = getGrids( timestep, varnames, extents, &actualRefLevel, &lod,  grids);
	if (!rc) return false;
	

	//Then go through the grids, projecting the field values to the 2D plane.
	//Each (x,y) probe coord is converted to the closest point in the probe grid
	//Each field value is also multiplied by scale factor, times a factor that relates the
	//user coord system to the probe grid coords.
	
	//Get the data dimensions (at this resolution):
	int dataSize[3];
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(actualRefLevel,i);
	}
	//Set up to transform from probe into volume:
	float transformMatrix[12];
	buildLocalCoordTransform(transformMatrix, 0.f, -1);

	//Get the 3x3 rotation matrix:
	float rotMatrix[9], invMatrix[9];
	const vector<double>& angles = GetBox()->GetAngles();
	getRotationMatrix((float)(angles[0]*M_PI/180.), (float)(angles[1]*M_PI/180.), (float)(angles[2]*M_PI/180.), rotMatrix);
	//Invert it by transposing:
	invMatrix[0] = rotMatrix[0];
	invMatrix[4] = rotMatrix[4];
	invMatrix[8] = rotMatrix[8];
	invMatrix[1] = rotMatrix[3];
	invMatrix[2] = rotMatrix[6];
	invMatrix[3] = rotMatrix[1];
	invMatrix[5] = rotMatrix[7];
	invMatrix[6] = rotMatrix[2];
	invMatrix[7] = rotMatrix[5];

	float probeCoord[3];
	double dataCoord[3];
	
	const float* fullSizes = ds->getFullSizes();
	float extExtents[6]; //Extend extents 1/2 voxel on each side so no bdry issues.
	for (int i = 0; i<3; i++){
		float mid = fullSizes[i]*0.5;
		float halfExtendedSize = fullSizes[i]*0.5*(1.f+dataSize[i])/(float)(dataSize[i]);
		extExtents[i] = mid - halfExtendedSize;
		extExtents[i+3] = mid + halfExtendedSize;
	}
	float localCorner[4][3];
	//Map corners of probe into volume 
	probeCoord[2] = 0.f;
	for (int cornum = 0; cornum < 4; cornum++){
		// coords relative to (-1,1)
		probeCoord[1] = -1.f + 2.f*(float)(cornum/2);
		probeCoord[0] = -1.f + 2.f*(float)(cornum%2);
		//Then transform to values in data 
		vtransform(probeCoord, transformMatrix, localCorner[cornum]);
	}
	float tempCoord[3];
	vsub(localCorner[1],localCorner[0],tempCoord);
	float xside = vlength(tempCoord);  
	vsub(localCorner[2],localCorner[0],tempCoord);
	float yside = vlength(tempCoord);
	int texHeight = textureSize[1];
	int texWidth = textureSize[0];
	assert(textureSize[0] != 0); //Should be a valid value

	ibfvXScale = (float)texWidth/xside;  //Cells per meter
	ibfvYScale = (float)texHeight/yside;

	if (ibfvUField[timestep]) delete ibfvUField[timestep];
	if (ibfvVField[timestep]) delete ibfvVField[timestep];
	if (ibfvValid[timestep]) delete ibfvValid[timestep];

	ibfvUField[timestep] = new float[textureSize[0]*textureSize[1]];
	ibfvVField[timestep] = new float[textureSize[0]*textureSize[1]];
	ibfvValid[timestep] = new unsigned char[textureSize[0]*textureSize[1]];
	float sumMag = 0.f;
	int numValidPoints = 0;
	//Use the RegularGrid to calculate coordinates in volume
	DataMgr* dataMgr = ds->getDataMgr();

	//Loop over pixels in texture
	for (int iy = 0; iy < texHeight; iy++){
		//Map iy to a value between -1 and 1
		probeCoord[1] = -1.f + 2.f*(float)iy/(float)(texHeight-1);
		for (int ix = 0; ix < texWidth; ix++){
			
			//find the coords that the texture maps to
			//probeCoord is the coord in the probe, dataCoord is in data volume 
			//probeCoord goes from -1 to 1, 
			//datacoord is a point in probe in world coords.
			probeCoord[0] = -1.f + 2.f*(float)ix/(float)(texWidth-1);
			vtransform(probeCoord, transformMatrix, dataCoord);
			
			bool dataOK = true;
			for (int i = 0; i< 3; i++){
				if (dataCoord[i] < extExtents[i] || dataCoord[i] > extExtents[i+3]) dataOK = false;
				//Convert to user coordinates:
				dataCoord[i] += userExts[i];
			}
			
			int texPos = ix + texWidth*iy;
			
			if(dataOK) { //find the coordinate in the data array
				float vecField[3];
				
				for (int k = 0; k<3; k++){
					if(grids[k]) {
						float fieldval = grids[k]->GetValue(dataCoord[0],dataCoord[1],dataCoord[2]);
						if (fieldval == grids[k]->GetMissingValue()) {
							dataOK = false;
							vecField[k]=0.;
						}
						else vecField[k] = fieldval;
					}
					else vecField[k] = 0.f;
				}
				
				if (dataOK){
					//Project this vector to the probe plane, saving the magnitude:
					numValidPoints++;
					float uVal, vVal;
					projToPlane(vecField, invMatrix, &uVal,&vVal);
					if(ibfvMag < 0.f) sumMag += (uVal*uVal + vVal*vVal);
					ibfvUField[timestep][texPos] = uVal;
					ibfvVField[timestep][texPos] = vVal;
					ibfvValid[timestep][texPos] = 1;
				} else {
					ibfvUField[timestep][texPos] = 0;
					ibfvVField[timestep][texPos] = 0;
					ibfvValid[timestep][texPos] = 0;
				}
			}
			else {//point is out of region
				ibfvUField[timestep][texPos] = 0;
				ibfvVField[timestep][texPos] = 0;
				ibfvValid[timestep][texPos] = 0;
			}

		}//End loop over ix
	}//End loop over iy
	if (ibfvMag < 0.f) {  //calculate new magnitude, or use previously calculated mag
		if (sumMag > 0.f) sumMag = sqrt(sumMag);
		ibfvMag = sumMag;
		if (ibfvMag == 0.f) ibfvMag = 1.f; //special case for constant 0 field
	}
	float scaleFac = 4.f*fieldScale/ibfvMag;
	//Now renormalize
	for (int iy = 0; iy < texHeight; iy++){
		for (int ix = 0; ix < texWidth; ix++){
			int texPos = ix + texWidth*iy;
			ibfvUField[timestep][texPos] *= scaleFac;
			ibfvVField[timestep][texPos] *= scaleFac;
		}
	}
	for (int i = 0; i<3; i++){
		if (grids[i]){
			dataMgr->UnlockGrid(grids[i]);
			delete grids[i];
		}
	}
			
	return true;
}

//Project a 3-vector to the probe plane.  Uses inverse rotation matrix
void ProbeParams::projToPlane(float vecField[3], float invRotMtrx[9], float* U, float* V){
	//Calculate component of vecField in direction of normVec:
	float projVec[3];
	vtransform3(vecField, invRotMtrx, projVec);
	*U = projVec[0]*ibfvXScale;
	*V = projVec[1]*ibfvYScale;
}


//Find camera distance to probe in stretched local coordinates
float ProbeParams::getCameraDistance(ViewpointParams* vpp, RegionParams* , int ){
	//Intersect probe with ray from camera
	//Rotate camPos and camDir to probe coordinate system,
	//Find ray intersection with probe (if it intersects)
	const float* camPos = vpp->getCameraPosLocal();
	const float* camDir = vpp->getViewDir();
	float relCamPos[3], localCamPos[3], localCamDir[3];
	float rotMatrix[9];
	const vector<double>& exts = GetBox()->GetLocalExtents();
	//translate so camPos is relative to middle of probe
	for (int i = 0; i<3; i++)
		relCamPos[i] = camPos[i] - 0.5f*(exts[i+3]+exts[i]);
	//Get the 3x3 rotation matrix and invert(transpose) it
	const vector<double>& angles = GetBox()->GetAngles();
	getRotationMatrix((float)(angles[0]*M_PI/180.), (float)(angles[1]*M_PI/180.), (float)(angles[2]*M_PI/180.), rotMatrix);
	
	//multiply camPos by transpose (inverse) of matrix, to
	//get camera position, direction in rotated system, relative to probe center
	vtransform3t(relCamPos,rotMatrix,localCamPos);
	vtransform3t(camDir,rotMatrix,localCamDir);
	//Now intersect ray with probe:
	if (localCamDir[2] == 0.f) return 1.e30f;
	//Apply stretch factors to camPos and probe dimensions
	//and stretch everything:
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	float cPos[3], pMin[3],pMax[3];
	for (int i = 0; i<3; i++){
		cPos[i] = localCamPos[i]*stretch[i];
		pMin[i] = exts[i]*stretch[i];
		pMax[i] = exts[i+3]*stretch[i];
	}
	
	//Find parameter T (position along ray) so that camPos+T*camDir intersects xy plane
	float T = -cPos[2]/localCamDir[2];
	if (T < 0.f) return 1.e30f;  //intersection is behind camera

	//Test if resulting point is inside translated probe extents:
	float hitPoint[3];
	for (int i = 0; i<3; i++) hitPoint[i] = cPos[i]+T*localCamDir[i];
	float probeHWidth = 0.5f*(pMax[0]-pMin[0]);
	float probeHHeight = 0.5f*(pMax[1]-pMin[1]);
	//Test if localCamPos is in (x,y) bounds of probe:
	if ( abs(hitPoint[0]) <= probeHWidth &&
		abs(hitPoint[1]) <= probeHHeight ){
		//We are in
		return vdist(cPos, hitPoint);
	}

	//Otherwise, find distance to center, back in original system, stretched:
	for (int i = 0; i<3; i++){
		hitPoint[i] = 0.5f*(pMin[i]+pMax[i]);
	}
	for (int i = 0; i<3; i++) cPos[i] = camPos[i]*stretch[i];
	return (vdist(cPos, hitPoint));

}
bool ProbeParams::
usingVariable(const string& varname){
	int varnum = DataStatus::getInstance()->getSessionVariableNum3D(varname);
	return (variableIsSelected(varnum));
}

bool ProbeParams::IsOpaque(){
	if(GetMapperFunc()->isOpaque() && getOpacityScale() > 0.99f) return true;
	return false;
}
