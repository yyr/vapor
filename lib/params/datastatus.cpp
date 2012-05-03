//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		datastatus.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		February 2006
//
//	Description:	Implements the DataStatus class
//
#include <cstdlib>
#include <cstdio>
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include "datastatus.h"
#include "pythonpipeline.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

#include <vapor/ImpExp.h>
#include <vapor/MyBase.h>
#include <vapor/DataMgr.h>
#include <vapor/Version.h>
#include <qstring.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qcolor.h>
#include <vapor/errorcodes.h>
#include "proj_api.h"
#include "math.h"
using namespace VAPoR;
using namespace VetsUtil;
#include <vapor/common.h>

//This is a singleton class, but it's created by the Session.
//Following are static, must persist even when there is no instance:
DataStatus* DataStatus::theDataStatus = 0;
const std::string DataStatus::_emptyString = "";
const vector<string> DataStatus::emptyVec;
std::vector<std::string> DataStatus::variableNames;
bool DataStatus::sessionBefore22 = false;
float DataStatus::pre22Offset[3] = {0.f,0.f,0.f};
int DataStatus::numMetadataVariables = 0;
int* DataStatus::mapMetadataVars = 0;
std::vector<std::string> DataStatus::variableNames2D;

std::string DataStatus::projString;
int DataStatus::numMetadataVariables2D = 0;
int DataStatus::numOriented2DVars[3] = {0,0,0};
int* DataStatus::mapMetadataVars2D = 0;
std::vector<int> DataStatus::activeVariableNums3D;
std::vector<int> DataStatus::activeVariableNums2D;
bool DataStatus::doWarnIfDataMissing = true;
bool DataStatus::trackMouseInTfe = true;
bool DataStatus::doUseLowerAccuracy = false;
QColor DataStatus::backgroundColor =  Qt::black;
QColor DataStatus::regionFrameColor = Qt::white;
QColor DataStatus::subregionFrameColor = Qt::red;
bool DataStatus::regionFrameEnabled = true;
bool DataStatus::subregionFrameEnabled = false;
bool DataStatus::textureSizeSpecified = false;
int DataStatus::textureSize = 0;
size_t DataStatus::cacheMB = 0;
int DataStatus::interactiveRefLevel = 0;


	//Python script mappings:
	//mapping from index to Python function
map<int,string> DataStatus::derivedMethodMap;
	//mapping from index to input 2D variables
map<int,vector<string> > DataStatus::derived2DInputMap;
	//mapping from index to input 3D variables
map<int,vector<string> > DataStatus::derived3DInputMap;
	//mapping to 2d outputs
map<int,vector<string> > DataStatus::derived2DOutputMap;
	//mapping from index to output 3D variables
map<int,vector<string> > DataStatus::derived3DOutputMap;
const string DataStatus::_backgroundColorAttr = "BackgroundColor";
const string DataStatus::_regionFrameColorAttr = "DomainFrameColor";
const string DataStatus::_subregionFrameColorAttr = "SubregionFrameColor";
const string DataStatus::_regionFrameEnabledAttr = "DomainFrameEnabled";
const string DataStatus::_subregionFrameEnabledAttr = "SubregionFrameEnabled";
const string DataStatus::_useLowerRefinementAttr = "UseLowerRefinementLevel";
const string DataStatus::_missingDataWarningAttr = "WarnDataMissing";
const string DataStatus::_trackMouseAttr = "TrackMouseInTFE";

//Default constructor
//Whether or not it exists on disk, what's its max and min
//What resolutions are available.
//
DataStatus::
DataStatus()
{
	
	dataMgr = 0;
	renderOK = false;
	
	minTimeStep = 0;
	maxTimeStep = 0;
	numTimesteps = 0;
	numTransforms = 0;
	numLODs = 0;
	clearActiveVars();
	
	for (int i = 0; i< 3; i++){
		extents[i] = 0.f;
		stretchedExtents[i] = 0.f;
		extents[i+3] = 1.f;
		stretchedExtents[i+3] = 1.f;
		stretchFactors[i] = 1.f;
		fullDataSize[i] = 64;
		fullStretchedSizes[i] = 1.f;
	}
	
	theDataStatus = this;
	sessionVersion = Version::GetVersionString();
	theApp = NULL;
	
}

// After a metadata::merge, call resetDataStatus to 
// add additional and/or modify previous variables
// return true if there was anything to set up.
//
// If there are python scripts use their output variables.
// If the python scripts inputs are not in the DataMgr, remove the input variables.  
// 
bool DataStatus::
reset(DataMgr* dm, size_t cachesize, QApplication* app){
	cacheMB = cachesize;
	
	dataMgr = dm;
	unsigned int numTS = (unsigned int)dataMgr->GetNumTimeSteps();
	if (numTS == 0) return false;
	MetadataVDC* md = dynamic_cast<MetadataVDC*>(dataMgr);
	if (md) VDCType = md->GetVDCType();
	else VDCType = 0;
	assert (numTS >= getNumTimesteps());  //We should always be increasing this
	numTimesteps = numTS;
	if (!dm) return false;
	std::vector<double> mdExtents = dataMgr->GetExtents(0);
	for (int i = 0; i< 3; i++) {
		extents[i+3] = (float)(mdExtents[i+3]-mdExtents[i]);
		extents[i] = 0.;
		fullSizes[i] = (float)(mdExtents[i+3] - mdExtents[i]);
	}

#ifdef	DEAD
	if (! sphericalTransform()) {
		for (int i = 0; i< 6; i++) extents[i] = (float)mdExtents[i];
	}
	else {
		//
		// Convert spherical extents to Cartesian coordinates, which is
		// given by the outer radius of the sphere
		//
		vector <long> perm = dataMgr->GetGridPermutation();
		extents[0] = extents[1] = extents[2] = -1.0 * mdExtents[perm[2]+3];
		extents[3] = extents[4] = extents[5] = mdExtents[perm[2]+3];
	}
#endif

	projString = dataMgr->GetMapProjection();
	
	
	
	
	//clean out the various status arrays:

	//Add all new variable names to the variable name list, while building the 
	//mapping of metadata var nums into session var nums:
	removeMetadataVars();
	clearActiveVars();
	
	int num3dVars = dataMgr->GetVariables3D().size();
	
	numMetadataVariables = num3dVars;
	mapMetadataVars = new int[numMetadataVariables];

	numOriented2DVars[0] = dataMgr->GetVariables2DXY().size();
	numOriented2DVars[1] = dataMgr->GetVariables2DYZ().size();
	numOriented2DVars[2] = dataMgr->GetVariables2DXZ().size();

	int numVars = num3dVars + numOriented2DVars[0];
	if (numVars <=0) return false;
	int numMetaVars2D = numOriented2DVars[0]+numOriented2DVars[1]+numOriented2DVars[2];
	numMetadataVariables2D = numMetaVars2D;
	mapMetadataVars2D = new int[numMetadataVariables2D];
	
	for (int i = 0; i<num3dVars; i++){
		bool match = false;
		for (int j = 0; j< getNumSessionVariables(); j++){
			if (getVariableName3D(j) == dataMgr->GetVariables3D()[i]){
				mapMetadataVars[i] = j;
				match = true;
				break;
			}
		}
		if (match) continue;
		//Note that we are modifying the very array that we are looping over.
		//
		string str = dataMgr->GetVariables3D()[i];
		addVarName(dataMgr->GetVariables3D()[i]);
		mapMetadataVars[i] = variableNames.size()-1;
	}
	//Make sure all the metadata vars are in session vars
	for (int i = 0; i<numMetaVars2D; i++){
		bool match = false;
		
		for (int j = 0; j< getNumSessionVariables2D(); j++){
			
			if (i < numOriented2DVars[0]){
				
				if (getVariableName2D(j) == dataMgr->GetVariables2DXY()[i]){
					mapMetadataVars2D[i] = j;
					match = true;
					break;
				}
			} else if ( i < numOriented2DVars[0]+numOriented2DVars[1]){
				if (getVariableName2D(j) == dataMgr->GetVariables2DYZ()[i-numOriented2DVars[0]]){
					mapMetadataVars2D[i] = j;
					match = true;
					break;
				}
			} else if ( i < numOriented2DVars[0]+numOriented2DVars[1]+numOriented2DVars[2]){
				if (getVariableName2D(j) == dataMgr->GetVariables2DXZ()[i-numOriented2DVars[0]-numOriented2DVars[1]]){
					mapMetadataVars2D[i] = j;
					match = true;
					break;
				}
			}
				
		}
		if (match) continue;
		//Note that we are modifying the very array that we are looping over.
		//
		if (i < numOriented2DVars[0])
			addVarName2D(dataMgr->GetVariables2DXY()[i]);
		else if (i < numOriented2DVars[0]+numOriented2DVars[1])
			addVarName2D(dataMgr->GetVariables2DYZ()[i-numOriented2DVars[0]]);
		else addVarName2D(dataMgr->GetVariables2DXZ()[i-numOriented2DVars[0]-numOriented2DVars[1]]);
		mapMetadataVars2D[i] = variableNames2D.size()-1;
	}

	int numSesVariables = getNumSessionVariables();
	int numSesVariables2D = getNumSessionVariables2D();
	
	variableExists.resize(numSesVariables);
	variableExists2D.resize(numSesVariables2D);
	
	maxLevel3D.resize(numSesVariables);
	maxLevel2D.resize(numSesVariables2D);
	dataMin.resize(numSesVariables);
	dataMax.resize(numSesVariables);
	dataMin2D.resize(numSesVariables2D);
	dataMax2D.resize(numSesVariables2D);
	for (int i = 0; i<numSesVariables; i++){
		variableExists[i] = false;
		maxLevel3D[i] = new int[numTimesteps];
		dataMin[i] = new float[numTimesteps];
		dataMax[i] = new float[numTimesteps];
		//Initialize these to flagged values.  They will be recalculated
		//as needed.  Do this lazily because it's expensive and unnecessary to go through
		//all the timesteps to obtain the data bounds
		for (int k = 0; k<numTimesteps; k++){
			maxLevel3D[i][k] = -1;
			dataMin[i][k] = 1.e30f;
			dataMax[i][k] = -1.e30f;
		}
	}
	for (int i = 0; i<numSesVariables2D; i++){
		variableExists2D[i] = false;
		maxLevel2D[i] = new int[numTimesteps];
		dataMin2D[i] = new float[numTimesteps];
		dataMax2D[i] = new float[numTimesteps];
		//Initialize these to flagged values.  They will be recalculated
		//as needed.  Do this lazily because it's expensive and unnecessary to go through
		//all the timesteps to obtain the data bounds
		for (int k = 0; k<numTimesteps; k++){
			maxLevel2D[i][k] = -1;
			dataMin2D[i][k] = 1.e30f;
			dataMax2D[i][k] = -1.e30f;
		}
	}


	numTransforms = dataMgr->GetNumTransforms();
	numLODs = dataMgr->GetCRatios().size();
	for (int k = 0; k<dataAtLevel.size(); k++) delete [] dataAtLevel[k];
	dataAtLevel.clear();
	for (int k = 0; k<= numTransforms; k++){
		dataAtLevel.push_back(new size_t[3]);
	}
	
	dataMgr->GetDim(fullDataSize, -1);
		

	//As we go through the variables and timesteps, keep Track of min and max times
	unsigned int mints = 1000000000;
	unsigned int maxts = 0;
	
	//Note:  It takes a long time for all the calls to VariableExists
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	
	//Now check each variable and each timestep in the metadata.
	//If no data is there, delete the entry.
	//The ones that have data become the "active" variables in the session.
	//Extend the list of session variables to include all that are in dataStatus.
	//Construct a mapping from variable nums to variable names, first use the
	//nums and names that are active, then the remainder.
	for (int lev = 0; lev <= numTransforms; lev++){
		dataMgr->GetDim(dataAtLevel[lev],lev);
	}
	bool someDataOverall = false;
	for (int var = 0; var< numSesVariables; var++){
		bool dataExists = false;
		//string s = getVariableName3D(var);
		//Check first if this variable is in the metadata:
		bool inMetadata = false;
		string s = getVariableName3D(var);
		if (!dataMgr->IsVariableDerived(s)){
			for (int i = 0; i< (int)dataMgr->GetVariables3D().size(); i++){
				if (dataMgr->GetVariables3D()[i] == s){
					inMetadata = true;
					break;
				}
			}
		}
		if (! inMetadata) {
			//variableExists[var] = false;
			continue;
		}
		//OK, it's in the metadata, check the timesteps...
		for (int ts = 0; ts< numTimesteps; ts++){
			//Find the maximum number of transforms available on disk
			//i.e., the highest transform level  (highest resolution) that exists
			int maxLevel = -1;
			int topLevel;
			if (getVDCType() == 2) topLevel = numLODs; else topLevel = numTransforms;
			for (int lvl = topLevel; lvl>= 0; lvl--){
				int xf=0, lod=0;
				if (getVDCType()==2) lod = lvl; else xf = lvl;
				if (dataMgr->VariableExists(ts, 
					getVariableName3D(var).c_str(),
					xf,lod)) {
						maxLevel = lvl;
						break;
					}
			}
			
			maxLevel3D[var][ts] = maxLevel;
			if (maxLevel >= 0){
				dataExists = true;
				someDataOverall = true;
				if (ts > (int)maxts) maxts = ts;
				if (ts < (int)mints) mints = ts;
				
			}
		}
		variableExists[var] = dataExists; 
	}

	for (int var = 0; var< numSesVariables2D; var++){
		bool dataExists = false;
		//string s = getVariableName3D(var);
		//Check first if this variable is in the metadata:
		bool inMetadata = false;
		for (int i = 0; i< (int)dataMgr->GetVariables2DXY().size(); i++){
			if (dataMgr->GetVariables2DXY()[i] == getVariableName2D(var)){
				inMetadata = true;
				break;
			}
		}
		if (!inMetadata) {
			for (int i = 0; i< (int)dataMgr->GetVariables2DYZ().size(); i++){
				if (dataMgr->GetVariables2DYZ()[i] == getVariableName2D(var)){
					inMetadata = true;
					break;
				}
			}
		}
		if (!inMetadata) {
			for (int i = 0; i< (int)dataMgr->GetVariables2DXZ().size(); i++){
				if (dataMgr->GetVariables2DXZ()[i] == getVariableName2D(var)){
					inMetadata = true;
					break;
				}
			}
		}
		if (! inMetadata) {
			//variableExists[var] = false;
			continue;
		}
		//OK, it's in the metadata, check the timesteps...
		for (int ts = 0; ts< numTimesteps; ts++){
			//Find the maximum number of transforms available on disk
			//i.e., the highest transform level  (highest resolution) that exists
			int maxLevel = -1;
			int topLevel;
			if (getVDCType() == 2) topLevel = numLODs; else topLevel = numTransforms;
			for (int lvl = topLevel; lvl>= 0; lvl--){
				int xf=0, lod=0;
				if (getVDCType()==2) lod = lvl; else xf = lvl;
				if (dataMgr->VariableExists(ts, 
					getVariableName2D(var).c_str(),
					xf,lod)) {
						maxLevel = lvl;
						break;
					}
			}
			
			maxLevel2D[var][ts] = maxLevel;
			if (maxLevel >= 0){
				dataExists = true;
				someDataOverall = true;
				if (ts > (int)maxts) maxts = ts;
				if (ts < (int)mints) mints = ts;
				
			}
		}
		variableExists2D[var] = dataExists; 
	}
    if (mints == 1000000000) mints = 0;
	QApplication::restoreOverrideCursor();
	//Now process the python variables.
	//They must be added to the session variables.
	//Set up the active variable mapping initially to coincide with the metadata variable mapping,
	//then add entries for python variables.
	//A pipeline must be created for each python variable

	for (int i = 0; i< numMetadataVariables; i++)
		getInstance()->activeVariableNums3D.push_back(mapMetadataVars[i]);
	for (int i = 0; i< numMetadataVariables2D; i++)
		getInstance()->activeVariableNums2D.push_back(mapMetadataVars2D[i]);
	//Add all the variables that show up in any 2D or 3D output.
	std::map<int, vector<string> > :: const_iterator outIter = derived2DOutputMap.begin();
	while (outIter != derived2DOutputMap.end()){
		vector<string> vars = outIter->second;
		for (int i = 0; i<vars.size(); i++){
			setDerivedVariable2D(vars[i]);
		}
		outIter++;
	}
	outIter = derived3DOutputMap.begin();
	while (outIter != derived3DOutputMap.end()){
		vector<string> vars = outIter->second;
		for (int i = 0; i<vars.size(); i++){
			setDerivedVariable3D(vars[i]);
		}
		outIter++;
	}
	std::map<int, string > :: const_iterator methodIter = derivedMethodMap.begin();
	while (methodIter != derivedMethodMap.end()){
		int scriptId = methodIter->first;
		string method = methodIter->second;
		//Create string vectors for input and output variables
		vector<string> inputs;
		vector<string> in2dVars = getDerived2DInputVars(scriptId);
		vector<string> in3dVars = getDerived3DInputVars(scriptId);
		vector<string> out2dVars = getDerived2DOutputVars(scriptId);
		vector<string> out3dVars = getDerived3DOutputVars(scriptId);
		string pname;
		if (out3dVars.size() > 0) pname = out3dVars[0]; else pname = out2dVars[0];
		
		//Check to make sure all input variables are present:
		bool validInputs = true;
		for (int i = 0; i<in2dVars.size(); i++) {
			inputs.push_back(in2dVars[i]);
			if (getMetadataVarNum2D(in2dVars[i]) < 0){
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, 
								  "Script defining %s is missing input variable %s",pname.c_str(),
								  in2dVars[i].c_str());
				validInputs = false;
			}
		}
		
		for (int i = 0; i<in3dVars.size(); i++){
			inputs.push_back(in3dVars[i]);
			if (getMetadataVarNum(in3dVars[i]) < 0){
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, 
								  "Script defining %s is missing input variable %s",pname.c_str(),
								  in3dVars[i].c_str());
				validInputs = false;
			}
		}
		
		vector<pair<string, DataMgr::VarType_T> > outpairs;
		for (int i = 0; i < out3dVars.size(); i++){
			outpairs.push_back( make_pair(out3dVars[i],DataMgr::VAR3D));
		}
		for (int i = 0; i < out2dVars.size(); i++){
			outpairs.push_back( make_pair(out2dVars[i],DataMgr::VAR2D_XY));
		}
		if (validInputs){
			PythonPipeLine* pipe = new PythonPipeLine(pname, inputs, outpairs, dataMgr);
			int rc = dataMgr->NewPipeline(pipe);
			if (rc<0)MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Invalid Python Script");
		}		
		methodIter++;
	}

	minTimeStep = (size_t)mints;
	maxTimeStep = (size_t)maxts;
	
	
	if (!someDataOverall){
		MyBase::SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE, "No data found in VDC");
	}
	return true;
}



DataStatus::
~DataStatus(){
	int numVariables = maxLevel3D.size();
	for (int i = 0; i< numVariables; i++){
		delete [] maxLevel3D[i];
		delete [] dataMin[i];
		delete [] dataMax[i];
	}
	for (int i = 0; i< dataAtLevel.size(); i++){
		delete [] dataAtLevel[i];
	}
	theDataStatus = 0;
}
void DataStatus::setDefaultPrefs(){
	doWarnIfDataMissing = true;
	trackMouseInTfe = true;
	doUseLowerAccuracy = false;
	backgroundColor =  Qt::black;
	regionFrameColor = Qt::white;
	subregionFrameColor = Qt::red;
	regionFrameEnabled = true;
	subregionFrameEnabled = false;
}
int DataStatus::
getFirstTimestep(int sesvarnum){
	for (int i = 0; i< numTimesteps; i++){
		if(dataIsPresent3D(sesvarnum,i)) return i;
	}
	return -1;
}
	
// calculate the datarange for a specific session variable and timestep:
// Performed on demand.  Error if not in metadata.
// 
void DataStatus::calcDataRange(int varnum, int ts){
	vector<double>minMax;
	
	if (maxLevel3D[varnum][ts] >= 0){
		
		float mnmx[2];
		//Turn off messages from datamgr. 
		//Not sure if this is necessary, but it does at least prevent
		//redundant messages
		//ErrMsgCB_T errorCallback = GetErrMsgCB();
		//SetErrMsgCB(0);
		int rc = ((DataMgr*)getDataMgr())->GetDataRange(ts, 
			getVariableName3D(varnum).c_str(), mnmx);
		//Turn messages back on
		//SetErrMsgCB(errorCallback);
		if(rc<0){
			//Post an error:
			MyBase::SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"Error accessing variable %s, at timestep %d; Data range [0,1] assumed",
				getVariableName3D(varnum).c_str(), ts);
			//Assign default values:
			dataMax[varnum][ts] = 1.f;
			dataMin[varnum][ts] = 0.f;
		}
		else{
			dataMax[varnum][ts] = mnmx[1];
			dataMin[varnum][ts] = mnmx[0];
		}
	}
}
// calculate the datarange for a specific 2D session variable and timestep:
// Performed on demand.
// 
void DataStatus::calcDataRange2D(int varnum, int ts){
	vector<double>minMax;
	
	if (maxLevel2D[varnum][ts] >= 0){
		
		float mnmx[2];
		
		int rc = ((DataMgr*)getDataMgr())->GetDataRange(ts, 
			getVariableName2D(varnum).c_str(), mnmx);
					
		if(rc<0){
			//Post an error:
			MyBase::SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"Missing DataRange in 2D variable %s, at timestep %d \n Interval [0,1] assumed",
				getVariableName2D(varnum).c_str(), ts);
			//Assign default values:
			dataMax2D[varnum][ts] = 1.f;
			dataMin2D[varnum][ts] = 0.f;
		}
		else{
			dataMax2D[varnum][ts] = mnmx[1];
			dataMin2D[varnum][ts] = mnmx[0];
		}
	}
}

//Find which index is associated with a name, or -1 if not metadata:
int DataStatus::getSessionVariableNum3D(const string& str){
	for (int i = 0; i<variableNames.size(); i++){
		if(variableNames[i] == str) return i;
	}
	return -1;
}
//Find which index is associated with a name, or -1 if not metadata:
int DataStatus::getSessionVariableNum2D(const string& str){
	for (int i = 0; i<variableNames2D.size(); i++){
		if(variableNames2D[i] == str) return i;
	}
	return -1;
}
//Make sure this name is in list; if not, insert it, return session var num index.
//Useful in parsing
int DataStatus::mergeVariableName(const string& str){
	for (int i = 0; i<variableNames.size(); i++){
		if(variableNames[i] == str) return i;
	}
	//Not found, put it in:
	addVarName(str);
	return (variableNames.size()-1);
}
int DataStatus::mergeVariableName2D(const string& str){
	for (int i = 0; i<variableNames2D.size(); i++){
		if(variableNames2D[i] == str) return i;
	}
	//Not found, put it in:
	addVarName2D(str);
	return (variableNames2D.size()-1);
}
int DataStatus::mapSessionToMetadataVarNum(int sesVarNum){
	for (int i = 0; i< numMetadataVariables; i++){
		if (mapMetadataVars[i] == sesVarNum) return i;
	}
	return -1;
}



//Convert the max stretched extents into cube coords
void DataStatus::getMaxStretchedExtentsInCube(float maxExtents[3]){
	float maxSize = Max(stretchedExtents[3]-stretchedExtents[0],Max(stretchedExtents[4]-stretchedExtents[1],stretchedExtents[5]-stretchedExtents[2]));
	maxExtents[0] = (stretchedExtents[3]-stretchedExtents[0])/maxSize;
	maxExtents[1] = (stretchedExtents[4]-stretchedExtents[1])/maxSize;
	maxExtents[2] = (stretchedExtents[5]-stretchedExtents[2])/maxSize;
}
//Determine the min and max extents at a given level:
void DataStatus::getExtentsAtLevel(size_t ts, int level, float exts[6]){
	size_t dim[3], maxm[3];
	size_t minm[3] = {0,0,0};
	double usermin[3], usermax[3];

	dataMgr->GetDim(dim, level);
	for (int i=0; i<3; i++) {
		maxm[i] = dim[i]-1;
	}

	dataMgr->MapVoxToUser(ts, minm, usermin, level);
	dataMgr->MapVoxToUser(ts, maxm, usermax, level);
	for (int i = 0; i<3; i++){
		exts[i] = (float)usermin[i];
		exts[i+3] = (float)usermax[i];
	}
}

bool DataStatus::sphericalTransform()
{
	if (! dataMgr) return (false);
	return (dataMgr->GetCoordSystemType() == "spherical");
}


int DataStatus::
getMetadataVarNum(std::string varname){
	if (dataMgr == 0) return -1;

	const vector<string>& names = dataMgr->GetVariables3D();
	for (int i = 0; i<names.size(); i++){
		if (names[i] == varname) return i;
	}
	return -1;
}
int DataStatus::
getMetadataVarNum2D(std::string varname){
	if (dataMgr == 0) return -1;

	const vector<string>& namesxy = dataMgr->GetVariables2DXY();
	for (int i = 0; i<namesxy.size(); i++){
		if (namesxy[i] == varname) return i;
	}
	const vector<string>& namesyz = dataMgr->GetVariables2DYZ();
	for (int i = 0; i<namesyz.size(); i++){
		if (namesyz[i] == varname) return (i+namesxy.size());
	}
	const vector<string>& namesxz = dataMgr->GetVariables2DXZ();
	for (int i = 0; i<namesxz.size(); i++){
		if (namesxz[i] == varname) return (i+namesxy.size()+namesyz.size());
	}
	return -1;
}
bool DataStatus::fieldDataOK(int refLevel, int lod, int tstep, int varx, int vary, int varz){
	MetadataVDC* md = dynamic_cast<MetadataVDC*>(dataMgr);
	if (md) VDCType = md->GetVDCType();
	else VDCType = 0;
	int testLevel = refLevel;
	if (VDCType == 2) testLevel = lod;
	if (tstep < 0 || tstep > numTimesteps - 1) return false;
	if (doUseLowerAccuracy) testLevel = 0;
	if (varx >= 0 && (maxLevel3D[varx][tstep] < testLevel)) return false;
	if (vary >= 0 && (maxLevel3D[vary][tstep] < testLevel)) return false;
	if (varz >= 0 && (maxLevel3D[varz][tstep] < testLevel)) return false;
	return true;
}
//Orientation is 2 for XY, 0 for YZ, 1 for XZ 
int DataStatus::get2DOrientation(int mdvar){
	if (getNumMetadataVariables2D() <= mdvar) return 2;
	if (mdvar < numOriented2DVars[0]) return 2;
	if (mdvar < numOriented2DVars[0]+numOriented2DVars[1]) return 0;
	assert(mdvar < numOriented2DVars[0]+numOriented2DVars[1]+numOriented2DVars[2]);
	return 1;
}

void DataStatus::getLocalExtentsCartesian(float myExtents[6]) {
	const float *extptr;
	
	extptr = getLocalExtents();
	
	if (sphericalTransform() && dataMgr) {
		//
		// Convert spherical extents to Cartesian coordinates, which is
		// given by the outer radius of the sphere
		//
		vector <long> perm = dataMgr->GetGridPermutation();
		myExtents[0] = myExtents[1] = myExtents[2] = -1.0 * extptr[perm[2]+3];
		myExtents[3] = myExtents[4] = myExtents[5] = extptr[perm[2]+3];
	}
	else {
		for (int i=0; i<6; i++) myExtents[i] = extptr[i];
	}
}


//static methods to convert coordinates to and from latlon
//coordinates are in the order longitude,latitude
//result is in user coordinates.
bool DataStatus::convertFromLonLat(double coords[2], int npoints){
	//Set up proj.4 to convert from LatLon to VDC coords
	if (getProjectionString().size() == 0) return false;
	projPJ vapor_proj = pj_init_plus(getProjectionString().c_str());
	if (!vapor_proj) return false;
	projPJ latlon_proj = pj_latlong_from_proj( vapor_proj); 
	if (!latlon_proj) return false;
	
	if (!pj_is_latlong(vapor_proj)){ //if data is already latlong, bypass following:
		
		static const double DEG2RAD = 3.1415926545/180.;
		//source point is in degrees, convert to radians:
		for (int i = 0; i<npoints*2; i++) coords[i] *= DEG2RAD;

		int rc = pj_transform(latlon_proj,vapor_proj,npoints,2, coords,coords+1, 0);

		if (rc){
			MyBase::SetErrMsg(VAPOR_WARNING, "Error in coordinate projection: \n%s",
				pj_strerrno(rc));
			pj_free(vapor_proj);
			pj_free(latlon_proj);
			return false;
		}
		//convert radians to degrees if the vapor projection is rotated lat-lon
		if (!string::npos == getProjectionString().find("ob_tran")){
			static const double RAD2DEG = 180./3.1415926545;
			//dest point is in radians, convert to degrees:
			for (int i = 0; i<npoints*2; i++) coords[i] *= RAD2DEG;
		}
	}
	
	pj_free(vapor_proj);
	pj_free(latlon_proj);
	return true;
	
}
bool DataStatus::convertLocalFromLonLat(int timestep, double coords[2], int npoints){
	DataMgr* dataMgr = getInstance()->getDataMgr();
	if (!dataMgr) return false;
	if(!convertFromLonLat(coords, npoints)) return false;
	const vector<double>& tvExts = dataMgr->GetExtents((size_t)timestep);
	for (int i = 0; i<npoints; i++){
		coords[2*i] -= tvExts[0];
		coords[2*i+1] -= tvExts[1];
	}
	return true;
}
bool DataStatus::convertLocalToLonLat(int timestep, double coords[2], int npoints){
	DataMgr* dataMgr = getInstance()->getDataMgr();
	if (!dataMgr) return false;
	const vector<double>& tvExts = dataMgr->GetExtents((size_t)timestep);
	//Convert local to user coordinates:
	for (int i = 0; i<npoints; i++){
		coords[2*i] += tvExts[0];
		coords[2*i+1] += tvExts[1];
	}
	if(!convertToLonLat(coords, npoints)) return false;
	return true;
}
//coordinates are always in user coordinates.
bool DataStatus::convertToLonLat(double coords[2], int npoints){

	//Set up proj.4 to convert to latlon
	if (getProjectionString().size() == 0) return false;
	projPJ vapor_proj = pj_init_plus(getProjectionString().c_str());
	projPJ latlon_proj = 0;
	if (vapor_proj) latlon_proj = pj_latlong_from_proj( vapor_proj); 
	if (!latlon_proj) {
		MyBase::SetErrMsg(VAPOR_ERROR_GEOREFERENCE, "Georeferencing error.  Georeferencing will be disabled; \n Projection string = \n%s",
				getProjectionString().c_str());
		projString.clear();
		return false;
	}
	bool vaporRad = pj_is_latlong(vapor_proj)||(string::npos != getProjectionString().find("ob_tran"));

	static const double RAD2DEG = 180./3.1415926545;
	static const double DEG2RAD = 3.1415926545/180.;
	if (vaporRad) //If vapor coord system is lat lon or rotated latlon then must convert meters to degrees to radians
		for (int i = 0; i<2*npoints; i++) coords[i]*=(DEG2RAD/111177.);
	
	int rc = pj_transform(vapor_proj,latlon_proj,npoints,2, coords,coords+1, 0);

	if (rc){
		MyBase::SetErrMsg(VAPOR_WARNING, "Error in coordinate projection: \n%s",
			pj_strerrno(rc));
		return false;
	}
	 //results are in radians, convert to degrees
	for (int i = 0; i<npoints*2; i++) coords[i] *= RAD2DEG;


	return true;
	
}
//Derived var support
//Obtain the id for a given output variable, return -1 if it does not exist
//More or less the same as the method on DataMgr, but valid even if there is not DataMgr
int DataStatus::getDerivedScriptId(const string& outvar) {
	map <int, vector<string> > :: const_iterator outIter = derived2DOutputMap.begin();
	while (outIter != derived2DOutputMap.end()){
		vector<string> vars = outIter->second;
		for (int i = 0; i<vars.size(); i++){
			if (vars[i] == outvar) return outIter->first;
		}
		outIter++;
	}
	outIter = derived3DOutputMap.begin();
	while (outIter != derived3DOutputMap.end()){
		vector<string> vars = outIter->second;
		for (int i = 0; i<vars.size(); i++){
			if (vars[i] == outvar) return outIter->first;
		}
		outIter++;
	}
	return -1;
}

//Purge all the derived output variables from the datamgr cache
void DataStatus::purgeAllCachedDerivedVariables() {
	DataMgr* dataMgr = getInstance()->getDataMgr();
	if (!dataMgr) return;
	map <int, vector<string> > :: const_iterator outIter = derived2DOutputMap.begin();
	while (outIter != derived2DOutputMap.end()){
		vector<string> vars = outIter->second;
		for (int i = 0; i<vars.size(); i++){
			dataMgr->PurgeVariable(vars[i]);
		}
		outIter++;
	}
	outIter = derived3DOutputMap.begin();
	while (outIter != derived3DOutputMap.end()){
		vector<string> vars = outIter->second;
		for (int i = 0; i<vars.size(); i++){
			dataMgr->PurgeVariable(vars[i]);
		}
		outIter++;
	}
	return;
}
const string& DataStatus::getDerivedScriptName(int id){
	const vector<string>& vars3 = getDerived3DOutputVars(id);
	if(vars3.size()>0) return vars3[0];
	const vector<string>& vars2 = getDerived2DOutputVars(id);
	if(vars2.size()>0) return vars2[0];
	return (_emptyString);
}

const string& DataStatus::getDerivedScript(int id) {
	map<int,string> :: const_iterator iter = derivedMethodMap.find(id);
	if (iter == derivedMethodMap.end()) return *(new string(""));
	else return iter->second;
}
const vector<string>& DataStatus::getDerived2DInputVars(int id) {
	map<int,vector<string> > :: const_iterator iter = derived2DInputMap.find(id);
	if (iter == derived2DInputMap.end()) return emptyVec;
	else return iter->second;
}
const vector<string>& DataStatus::getDerived3DInputVars(int id) {
	map<int,vector<string> > :: const_iterator iter = derived3DInputMap.find(id);
	if (iter == derived3DInputMap.end()) return emptyVec;
	else return iter->second;
}
const vector<string>& DataStatus::getDerived2DOutputVars(int id) {
	map<int,vector<string> > :: const_iterator iter = derived2DOutputMap.find(id);
	if (iter == derived2DOutputMap.end()) return emptyVec;
	else return iter->second;
}
const vector<string>& DataStatus::getDerived3DOutputVars(int id) {
	map<int,vector<string> > :: const_iterator iter = derived3DOutputMap.find(id);
	if (iter == derived3DOutputMap.end()) return emptyVec;
	else return iter->second;
}
//Remove a python script and associated variable lists. 
//Delete derived variables from active variable lists
//Return false if it's already gone
 bool DataStatus::removeDerivedScript(int id){
	map<int,string> :: iterator iter = derivedMethodMap.find(id);
	if (iter == derivedMethodMap.end()) return false;
	 
	 string name;
	 if (getDerived3DOutputVars(id).size()>0)
		 name = getDerived3DOutputVars(id)[0];
	 else 
		 name = getDerived2DOutputVars(id)[0];
	 
	derivedMethodMap.erase(iter);

	map<int,vector<string> > :: iterator iter1 = derived2DOutputMap.find(id);
	if (iter1 != derived2DOutputMap.end()) {
		vector<string> outvars = iter1->second;
		for (int i = 0; i<outvars.size(); i++){
			removeDerivedVariable2D(outvars[i]);
		}
		derived2DOutputMap.erase(iter1);
	}
	iter1 = derived3DOutputMap.find(id);
	if (iter1 != derived3DOutputMap.end()) {
		vector<string> outvars = iter1->second;
		for (int i = 0; i<outvars.size(); i++){
			removeDerivedVariable3D(outvars[i]);
		}
		derived3DOutputMap.erase(iter1);
	}
	iter1 = derived2DInputMap.find(id);
	if (iter1 != derived2DInputMap.end()) derived2DInputMap.erase(iter1);
	iter1 = derived3DInputMap.find(id);
	if (iter1 != derived3DInputMap.end()) derived3DInputMap.erase(iter1);
	 
	if (dataMgr) dataMgr->RemovePipeline(name);
	return true;
 }
 int DataStatus::addDerivedScript(const vector<string>& in2DVars, const vector<string>& out2DVars, 
							 const vector<string>& in3DVars, const vector<string>& out3DVars, const string& script,
							 bool useMetadata){
	//First test to make sure that none of the outvars are in other scripts:
	for (int i = 0; i<out2DVars.size(); i++){
		if (getDerivedScriptId(out2DVars[i]) >= 0) return -1;
	}
	for (int i = 0; i<out3DVars.size(); i++){
		if (getDerivedScriptId(out3DVars[i]) >= 0) return -1;
	}
	//Find an unused index:
	int newIndex = -1;
	for (int indx = 1;; indx++){
		map<int, string>::const_iterator iter = derivedMethodMap.find(indx);
		if (iter == derivedMethodMap.end()) {
			newIndex = indx;
			break;
		}
	}
	
	derivedMethodMap[newIndex] = script;
	derived2DInputMap[newIndex] = in2DVars;
	derived3DInputMap[newIndex] = in3DVars;
	derived2DOutputMap[newIndex] = out2DVars;
	derived3DOutputMap[newIndex] = out3DVars;

	if (useMetadata  && dataMgr){
		//Add the new derived variables to existing variables
		//in the datastatus
		for (int i = 0; i<out2DVars.size(); i++){
			int sesid = setDerivedVariable2D(out2DVars[i]);
			if (sesid < 0) return -1;
		}
		for (int i = 0; i<out3DVars.size(); i++){
			int sesid = setDerivedVariable3D(out3DVars[i]);
			if (sesid < 0) return -1;
		}
		//Create a new pipeline:
		string pname;
		if (out3DVars.size() > 0) pname = out3DVars[0]; else pname = out2DVars[0];
		
		vector<string>inputs;
		for (int i = 0; i< in2DVars.size(); i++) inputs.push_back(in2DVars[i]);
		for (int i = 0; i< in3DVars.size(); i++) inputs.push_back(in3DVars[i]);
		vector<pair<string, DataMgr::VarType_T> > outpairs;
		for (int i = 0; i < out3DVars.size(); i++){
			outpairs.push_back( make_pair(out3DVars[i],DataMgr::VAR3D));
		}
		for (int i = 0; i < out2DVars.size(); i++){
			outpairs.push_back( make_pair(out2DVars[i],DataMgr::VAR2D_XY));
		}
		
		PythonPipeLine* pipe = new PythonPipeLine(pname, inputs, outpairs, dataMgr);
		int rc = dataMgr->NewPipeline(pipe);
		if (rc<0)MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Invalid Python Script");
	}
	
	return newIndex;
}
 //Find the largest index that is used for a scriptID, or 0 if there are none.
 int DataStatus::getMaxDerivedScriptId(){
	 int lastIndex = 0;
	 map<int,string> :: const_iterator iter = derivedMethodMap.begin();
	 while (iter != derivedMethodMap.end()) {
		 int index = iter->first;
		 if (index > lastIndex) lastIndex = index;
		 iter++;
	 }
	 return lastIndex;
 }
	
//Replace an existing derived script with a new one.  
//
int DataStatus::replaceDerivedScript(int id, const vector<string>& in2DVars, const vector<string>& out2DVars,
								 const vector<string>& in3DVars, const vector<string>& out3DVars, const string& script){

	if (dataMgr){
		//Must purge cache of the previous output variables of the script
		vector<string> oldOut2dvars = getDerived2DOutputVars(id);
		for (int i = 0; i<oldOut2dvars.size(); i++){
			dataMgr->PurgeVariable(oldOut2dvars[i]);
		}	
		vector<string> oldOut3dvars = getDerived3DOutputVars(id);
		for (int i = 0; i<oldOut3dvars.size(); i++){
			dataMgr->PurgeVariable(oldOut3dvars[i]);
		}
	}
	removeDerivedScript(id);
	return (addDerivedScript(in2DVars, out2DVars, in3DVars, out3DVars, script));
}
int DataStatus::setDerivedVariable3D(const string& derivedVarName){
	int scriptid = getDerivedScriptId(derivedVarName);
	if (scriptid < 0) return -1;
	if (!getDataMgr()) return -1;
	//Iterate over inputs, make sure they are all in metadata...
	map<int,vector<string> > :: const_iterator iter1 = derived2DInputMap.find(scriptid);
	if (iter1 != derived2DInputMap.end()) {
		vector<string> vars = iter1->second;
		for (int i = vars.size()-1; i>=0; i--){
			if (getMetadataVarNum2D(vars[i]) < 0) {
				vars.erase(vars.begin()+ i);
			}
		}
	}
	iter1 = derived3DInputMap.find(scriptid);
	if (iter1 != derived3DInputMap.end()) {
		vector<string> vars = iter1->second;
		for (int i = vars.size()-1; i>=0; i--){
			if (getMetadataVarNum(vars[i]) < 0) {
				vars.erase(vars.begin()+ i);
			}
		}
	}
	
	//Is it already in session?
	int sesvarnum = getSessionVariableNum3D(derivedVarName);
	if (sesvarnum < 0){
		
		sesvarnum = mergeVariableName(derivedVarName);
		int numSesVariables = getNumSessionVariables();
		assert(numSesVariables == sesvarnum+1);  //should have added at end
		variableExists.resize(numSesVariables);
		maxLevel3D.resize(numSesVariables);
		dataMin.resize(numSesVariables);
		dataMax.resize(numSesVariables);
		variableExists[sesvarnum] = false;
		maxLevel3D[sesvarnum] = new int[numTimesteps];
		dataMin[sesvarnum] = new float[numTimesteps];
		dataMax[sesvarnum] = new float[numTimesteps];
	}
	//Initialize to default values.	
	int maxLevel = numTransforms;
	if (getVDCType()==2) maxLevel = getNumLODs()-1;
	for (int k = 0; k<numTimesteps; k++){
		maxLevel3D[sesvarnum][k] = maxLevel;
		dataMin[sesvarnum][k] = 1.e30f;
		dataMax[sesvarnum][k] = -1.e30f;
	}
	bool varexists = true;
	//Then check all the input 2D variables for max num transforms
	iter1 = derived2DInputMap.find(scriptid);
	if (iter1 != derived2DInputMap.end()) {
		vector<string> vars = iter1->second;
		for (int i = 0; i<vars.size(); i++){
			int sesinvar = getSessionVariableNum2D(vars[i]);
			if(sesinvar >= 0) {
				for (int k = 0; k<numTimesteps; k++){
					if(maxLevel3D[sesvarnum][k]>maxLevel2D[sesinvar][k])
						maxLevel3D[sesvarnum][k]=maxLevel2D[sesinvar][k];
				}
				if (!variableExists2D[sesinvar]) varexists = false;
			} else {
				varexists = false;
			}
		
		}
	}
	
	//Then check all the 3D input variables for max num transforms
	iter1 = derived3DInputMap.find(scriptid);
	if (iter1 != derived3DInputMap.end()) {
		vector<string> vars = iter1->second;
		for (int i = 0; i<vars.size(); i++){
			int sesinvar = getSessionVariableNum3D(vars[i]);
			if(sesinvar >= 0) {
				for (int k = 0; k<numTimesteps; k++){
					if(maxLevel3D[sesvarnum][k]>maxLevel3D[sesinvar][k])
						maxLevel3D[sesvarnum][k]=maxLevel3D[sesinvar][k];
				}
				if (!variableExists[sesinvar]) varexists = false;
			} else {
				varexists = false;
			}
		}
	}
	
	//Is it already mapped?
	if (mapSessionToActiveVarNum3D(sesvarnum) < 0){
		activeVariableNums3D.push_back(sesvarnum);
	}
	variableExists[sesvarnum] = varexists;
	return sesvarnum;
}

bool DataStatus::removeDerivedVariable2D(const string& derivedVarName){
	int sesnum = getSessionVariableNum2D(derivedVarName);
	if (sesnum < 0 ) return false;
	int activeNum = mapSessionToActiveVarNum2D(sesnum);
	if (activeNum < 0) assert(0);  //Not active

	else {
		//need to erase it from active list
		//Don't leave it in the session...
		getInstance()->variableExists2D[sesnum] = false;
		activeVariableNums2D.erase(activeVariableNums2D.begin()+activeNum);
	}
	return true;
}
bool DataStatus::removeDerivedVariable3D(const string& derivedVarName){
	int sesnum = getSessionVariableNum3D(derivedVarName);
	if (sesnum < 0 ) return false;
	int activeNum = mapSessionToActiveVarNum3D(sesnum);
	if (activeNum < 0) return false;  //Not active
	else {
		getInstance()->variableExists[sesnum] = false;
		activeVariableNums3D.erase(activeVariableNums3D.begin()+activeNum);
	}
	return true;
}
int DataStatus::setDerivedVariable2D(const string& varName){
	int scriptid = getDerivedScriptId(varName);
	if (scriptid < 0) return -1;
	if (!getDataMgr()) return -1;
	//Iterate over inputs, make sure they are all in metadata
	map<int,vector<string> > :: const_iterator iter1 = derived2DInputMap.find(scriptid);
	if (iter1 != derived2DInputMap.end()) {
		vector<string> vars = iter1->second;
		for (int i = vars.size()-1; i>=0; i--){
			if (getMetadataVarNum2D(vars[i]) < 0) {
				vars.erase(vars.begin()+i);
			}
		}
	}
	iter1 = derived3DInputMap.find(scriptid);
	if (iter1 != derived3DInputMap.end()) {
		vector<string> vars = iter1->second;
		for (int i = vars.size()-1; i>=0; i--){
			if (getMetadataVarNum(vars[i]) < 0) {
				vars.erase(vars.begin()+i);
			}
		}
	}
	//Is it already in session?
	int sesvarnum = getSessionVariableNum2D(varName);
	if (sesvarnum < 0){
		sesvarnum = mergeVariableName2D(varName);
		int numSesVariables2D = getNumSessionVariables2D();
		assert(numSesVariables2D == sesvarnum+1);  //should have added at end
		variableExists2D.resize(numSesVariables2D);
		maxLevel2D.resize(numSesVariables2D);
		dataMin2D.resize(numSesVariables2D);
		dataMax2D.resize(numSesVariables2D);
		variableExists2D[sesvarnum] = false;
		maxLevel2D[sesvarnum] = new int[numTimesteps];
		dataMin2D[sesvarnum] = new float[numTimesteps];
		dataMax2D[sesvarnum] = new float[numTimesteps];
	}
	//Initialize to default values.
	int numLevels = numTransforms;
	if (getVDCType()==2) numLevels = numLODs;
	for (int k = 0; k<numTimesteps; k++){
		maxLevel2D[sesvarnum][k] = numLevels;
		dataMin2D[sesvarnum][k] = 1.e30f;
		dataMax2D[sesvarnum][k] = -1.e30f;
	}
	bool varexists = true;
	//Then check all the input 2d variables for max num transforms
	iter1 = derived2DInputMap.find(scriptid);
	if (iter1 != derived2DInputMap.end()) {
		vector<string> vars = iter1->second;
		for (int i = 0; i<vars.size(); i++){
			int sesinvar = getSessionVariableNum2D(vars[i]);
			if(sesinvar >= 0) {
				for (int k = 0; k<numTimesteps; k++){
					if(maxLevel2D[sesvarnum][k]>maxLevel2D[sesinvar][k])
						maxLevel2D[sesvarnum][k]=maxLevel2D[sesinvar][k];
				}
				if (!variableExists2D[sesinvar]) varexists = false;
			} else {
				varexists = false;
			}
		}
	}
	
	//Then check all the input 3d variables for max num transforms
	iter1 = derived3DInputMap.find(scriptid);
	if (iter1 != derived3DInputMap.end()) {
		vector<string> vars = iter1->second;
		for (int i = 0; i<vars.size(); i++){
			int sesinvar = getSessionVariableNum3D(vars[i]);
			if(sesinvar >= 0) {
				for (int k = 0; k<numTimesteps; k++){
					if(maxLevel2D[sesvarnum][k]>maxLevel3D[sesinvar][k])
						maxLevel2D[sesvarnum][k]=maxLevel3D[sesinvar][k];
				}
				if (!variableExists[sesinvar]) varexists = false;
			} else {
				varexists = false;
			}
		}
	}
	
	//Is it already mapped?
	if (mapSessionToActiveVarNum2D(sesvarnum) < 0){
		activeVariableNums2D.push_back(sesvarnum);
	}
	variableExists2D[sesvarnum] = varexists;
	return sesvarnum;
}
int DataStatus::mapSessionToActiveVarNum2D(int sesvarnum){
	for (int i = 0; i<activeVariableNums2D.size(); i++){
		if (activeVariableNums2D[i] == sesvarnum) return i;
	}
	return -1;
}
int DataStatus::mapSessionToActiveVarNum3D(int sesvarnum){
	for (int i = 0; i<activeVariableNums3D.size(); i++){
		if (activeVariableNums3D[i] == sesvarnum) return i;
	}
	return -1;
}
int DataStatus::getActiveVarNum3D(const string varname) const{
	//Find it in the session names, then see if it is mapped:
	int i;
	for (i = 0; i<variableNames.size(); i++){
		if (variableNames[i] == varname) break;
	}
	if (i >= variableNames.size()) return -1;
	for (int j = 0; j<activeVariableNums3D.size(); j++){
		if (activeVariableNums3D[j] == i) return j;
	}
	return -1;
}
int DataStatus::getActiveVarNum2D(const string varname) const{
	//Find it in the session names, then see if it is mapped:
	int i;
	for (i = 0; i<variableNames2D.size(); i++){
		if (variableNames2D[i] == varname) break;
	}
	if (i >= variableNames2D.size()) return -1;
	for (int j = 0; j<activeVariableNums2D.size(); j++){
		if (activeVariableNums2D[j] == i) return j;
	}
	return -1;
}
double DataStatus::getDefaultDataMax3D(int varnum){
	int activeNum = mapSessionToActiveVarNum3D(varnum);
	if (activeNum < 0) return 1.f;
	string varname = getActiveVarName3D(activeNum);
	//If it's derived, we don't get the actual max until it's retrieved for other purposes
	return getDataMax3D(varnum, (int)minTimeStep, !isDerivedVariable(varname));
}
double DataStatus::getDefaultDataMin3D(int varnum){
	int activeNum = mapSessionToActiveVarNum3D(varnum);
	if (activeNum < 0) return -1.f;
	string varname = getActiveVarName3D(activeNum);
	//If it's derived, we don't get the actual min until it's retrieved for other purposes
	return getDataMin3D(varnum, (int)minTimeStep, !isDerivedVariable(varname));
}
double DataStatus::getDefaultDataMax2D(int varnum){
	int activeNum = mapSessionToActiveVarNum2D(varnum);
	if (activeNum < 0) return 1.f;
	string varname = getActiveVarName2D(activeNum);
	//If it's derived, we don't get the actual max until it's retrieved for other purposes
	return getDataMax2D(varnum, (int)minTimeStep, !isDerivedVariable(varname));
}
double DataStatus::getDefaultDataMin2D(int varnum){
	int activeNum = mapSessionToActiveVarNum2D(varnum);
	if (activeNum < 0) return -1.f;
	string varname = getActiveVarName2D(activeNum);
	//If it's derived, we don't get the actual max until it's retrieved for other purposes
	return getDataMax2D(varnum, (int)minTimeStep, !isDerivedVariable(varname));
}
double DataStatus::getDataMin3D(int sesvarNum, int timestep, bool mustGet){
	if (!dataIsPresent3D(sesvarNum, timestep))return -1.0;
	if (dataMin[sesvarNum][timestep] == 1.e30f){
		if (!mustGet) return -1.0;
		calcDataRange(sesvarNum,timestep);
	}
	return dataMin[sesvarNum][timestep];
}
double DataStatus::getDataMin2D(int sesvarNum, int timestep, bool mustGet){
	if (!dataIsPresent2D(sesvarNum, timestep))return -1.0;
	if (dataMin2D[sesvarNum][timestep] == 1.e30f){
		if (!mustGet) return -1.0;
		calcDataRange2D(sesvarNum,timestep);
	}
	return dataMin2D[sesvarNum][timestep];
}
double DataStatus::getDataMax3D(int sesvarNum, int timestep, bool mustGet){
	if (!dataIsPresent3D(sesvarNum, timestep))return 1.0;
	if (dataMax[sesvarNum][timestep] == -1.e30f){
		if (!mustGet) return 1.0;
		calcDataRange(sesvarNum,timestep);
	}
	return dataMax[sesvarNum][timestep];
}
double DataStatus::getDataMax2D(int sesvarNum, int timestep, bool mustGet){
	if (!dataIsPresent2D(sesvarNum, timestep))return 1.0;
	if (dataMax2D[sesvarNum][timestep] == -1.e30f){
		if (!mustGet) return 1.;
		calcDataRange2D(sesvarNum,timestep);
	}
	return dataMax2D[sesvarNum][timestep];
}
int DataStatus::maxLODPresent3D(int sesvarnum, int timestep){
		if (timestep < 0 || timestep >= numTimesteps) return -1;
		if (sesvarnum<0) return -1;
		if (!variableExists[sesvarnum]) return -1;
		if (getVDCType()!=2) return 0;
		return (maxLevel3D[sesvarnum][timestep]);
}
int DataStatus::maxLODPresent2D(int sesvarnum, int timestep){
		if (timestep < 0 || timestep >= numTimesteps) return -1;
		if (sesvarnum<0) return -1;
		if (!variableExists2D[sesvarnum]) return -1;
		if (getVDCType()!=2) return 0;
		return (maxLevel2D[sesvarnum][timestep]);
}
int DataStatus::maxXFormPresent3D(int sesvarnum, int timestep){
		if (timestep < 0 || timestep >= numTimesteps) return -1;
		if (sesvarnum<0) return -1;
		if (!variableExists[sesvarnum]) return -1;
		if (getVDCType()==2) return getNumTransforms();
		return (maxLevel3D[sesvarnum][timestep]);
}
int DataStatus::maxXFormPresent2D(int sesvarnum, int timestep){
		if (timestep < 0 || timestep >= numTimesteps) return -1;
		if (sesvarnum<0) return -1;
		if (!variableExists2D[sesvarnum]) return -1;
		if (getVDCType()==2) return getNumTransforms();
		return (maxLevel2D[sesvarnum][timestep]);
}
