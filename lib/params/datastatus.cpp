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

#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

#include "vapor/Metadata.h"
#include "vapor/ImpExp.h"
#include "vapor/MyBase.h"
#include "vapor/DataMgr.h"
#include "vapor/Version.h"
#include <qstring.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qcolor.h>
#include "vapor/errorcodes.h"
#include "vapor/LayeredIO.h"

using namespace VAPoR;
using namespace VetsUtil;
#include "vaporinternal/common.h"

//This is a singleton class, but it's created by the Session.
//Following are static, must persist even when there is no instance:
DataStatus* DataStatus::theDataStatus = 0;
std::vector<std::string> DataStatus::variableNames;
std::vector<float> DataStatus::aboveValues;
std::vector<float> DataStatus::belowValues;
int DataStatus::numMetadataVariables = 0;
int* DataStatus::mapMetadataVars = 0;
std::vector<std::string> DataStatus::variableNames2D;

int DataStatus::numMetadataVariables2D = 0;
int* DataStatus::mapMetadataVars2D = 0;
bool DataStatus::doWarnIfDataMissing = true;
bool DataStatus::doUseLowerRefinementLevel = false;
QColor DataStatus::backgroundColor =  Qt::black;
QColor DataStatus::regionFrameColor = Qt::white;
QColor DataStatus::subregionFrameColor = Qt::red;
bool DataStatus::regionFrameEnabled = true;
bool DataStatus::subregionFrameEnabled = false;
bool DataStatus::textureSizeSpecified = false;
int DataStatus::textureSize = 0;
size_t DataStatus::cacheMB = 0;

const string DataStatus::_backgroundColorAttr = "BackgroundColor";
const string DataStatus::_regionFrameColorAttr = "DomainFrameColor";
const string DataStatus::_subregionFrameColorAttr = "SubregionFrameColor";
const string DataStatus::_regionFrameEnabledAttr = "DomainFrameEnabled";
const string DataStatus::_subregionFrameEnabledAttr = "SubregionFrameEnabled";
const string DataStatus::_useLowerRefinementAttr = "UseLowerRefinementLevel";
const string DataStatus::_missingDataWarningAttr = "WarnDataMissing";
//Default constructor
//Whether or not it exists on disk, what's its max and min
//What resolutions are available.
//
DataStatus::
DataStatus()
{
	
	dataMgr = 0;
    currentMetadata = 0;
	renderOK = false;
	myReader = 0;
	
	minTimeStep = 0;
	maxTimeStep = 0;
	numTimesteps = 0;
	numTransforms = 0;
	
	for (int i = 0; i< 3; i++){
		extents[i] = 0.f;
		stretchedExtents[i] = 0.f;
		extents[i+3] = 1.f;
		stretchedExtents[i+3] = 1.f;
		stretchFactors[i] = 1.f;
		fullDataSize[i] = 64;
	}
	
	theDataStatus = this;
	sessionVersion = Version::GetVersionString();
	
}

// After a metadata::merge, call resetDataStatus to 
// add additional and/or modify previous variables
// return true if there was anything to set up.
// 
bool DataStatus::
reset(DataMgr* dm, size_t cachesize, QApplication* app){
	cacheMB = cachesize;
	
	currentMetadata = (Metadata*)dm->GetMetadata();
	dataMgr = dm;
	myReader = (VDFIOBase*)dm->GetRegionReader();
	unsigned int numTS = (unsigned int)currentMetadata->GetNumTimeSteps();
	if (numTS == 0) return false;
	assert (numTS >= getNumTimesteps());  //We should always be increasing this
	numTimesteps = numTS;
	
	std::vector<double> mdExtents = currentMetadata->GetExtents();
	for (int i = 0; i< 6; i++)
		extents[i] = (float)mdExtents[i];
	
	//clean out the various status arrays:

	//Add all new variable names to the variable name list, while building the 
	//mapping of metadata var nums into session var nums:
	removeMetadataVars();
	
	
	int numVars = currentMetadata->GetVariables3D().size();
	if (numVars == 0) return false;
	numMetadataVariables = numVars;
	mapMetadataVars = new int[numMetadataVariables];

	int numMetaVars2DXY = currentMetadata->GetVariables2DXY().size();
	int numMetaVars2DYZ = currentMetadata->GetVariables2DYZ().size();
	int numMetaVars2DXZ = currentMetadata->GetVariables2DXZ().size();

	int numMetaVars2D = numMetaVars2DXY+numMetaVars2DYZ+numMetaVars2DXZ;
	numMetadataVariables2D = numMetaVars2D;
	mapMetadataVars2D = new int[numMetadataVariables2D];
	
	for (int i = 0; i<numVars; i++){
		bool match = false;
		for (int j = 0; j< getNumSessionVariables(); j++){
			if (getVariableName(j) == currentMetadata->GetVariableNames()[i]){
				mapMetadataVars[i] = j;
				match = true;
				break;
			}
		}
		if (match) continue;
		//Note that we are modifying the very array that we are looping over.
		//
		addVarName(currentMetadata->GetVariableNames()[i]);
		mapMetadataVars[i] = variableNames.size()-1;
	}
	//Make sure all the metadata vars are in session vars
	for (int i = 0; i<numMetaVars2D; i++){
		bool match = false;
		for (int j = 0; j< getNumSessionVariables2D(); j++){
			
			if (i < numMetaVars2DXY){
				if (getVariableName2D(j) == currentMetadata->GetVariables2DXY()[i]){
					mapMetadataVars2D[i] = j;
					match = true;
					break;
				}
			} else if ( i < numMetaVars2DXY+numMetaVars2DYZ){
				if (getVariableName2D(j) == currentMetadata->GetVariables2DYZ()[i-numMetaVars2DXY]){
					mapMetadataVars2D[i] = j;
					match = true;
					break;
				}
			} else if ( i < numMetaVars2DXY+numMetaVars2DYZ+numMetaVars2DXZ){
				if (getVariableName2D(j) == currentMetadata->GetVariables2DXZ()[i-numMetaVars2DXY-numMetaVars2DXZ]){
					mapMetadataVars2D[i] = j;
					match = true;
					break;
				}
			}
				
		}
		if (match) continue;
		//Note that we are modifying the very array that we are looping over.
		//
		if (i < numMetaVars2DXY)
			addVarName2D(currentMetadata->GetVariables2DXY()[i]);
		else if (i < numMetaVars2DYZ)
			addVarName2D(currentMetadata->GetVariables2DYZ()[i-numMetaVars2DXY]);
		else addVarName2D(currentMetadata->GetVariables2DXZ()[i-numMetaVars2DXY-numMetaVars2DXZ]);
		mapMetadataVars2D[i] = variableNames2D.size()-1;
	}

	int numSesVariables = getNumSessionVariables();
	int numSesVariables2D = getNumSessionVariables2D();
	
	variableExists.resize(numSesVariables);
	variableExists2D.resize(numSesVariables2D);
	
	maxNumTransforms.resize(numSesVariables);
	maxNumTransforms2D.resize(numSesVariables2D);
	dataMin.resize(numSesVariables);
	dataMax.resize(numSesVariables);
	dataMin2D.resize(numSesVariables2D);
	dataMax2D.resize(numSesVariables2D);
	for (int i = 0; i<numSesVariables; i++){
		variableExists[i] = false;
		maxNumTransforms[i] = new int[numTimesteps];
		dataMin[i] = new float[numTimesteps];
		dataMax[i] = new float[numTimesteps];
		//Initialize these to flagged values.  They will be recalculated
		//as needed.  Do this lazily because it's expensive and unnecessary to go through
		//all the timesteps to obtain the data bounds
		for (int k = 0; k<numTimesteps; k++){
			maxNumTransforms[i][k] = -1;
			dataMin[i][k] = 1.e30f;
			dataMax[i][k] = -1.e30f;
		}
	}
	for (int i = 0; i<numSesVariables2D; i++){
		variableExists2D[i] = false;
		maxNumTransforms2D[i] = new int[numTimesteps];
		dataMin2D[i] = new float[numTimesteps];
		dataMax2D[i] = new float[numTimesteps];
		//Initialize these to flagged values.  They will be recalculated
		//as needed.  Do this lazily because it's expensive and unnecessary to go through
		//all the timesteps to obtain the data bounds
		for (int k = 0; k<numTimesteps; k++){
			maxNumTransforms2D[i][k] = -1;
			dataMin2D[i][k] = 1.e30f;
			dataMax2D[i][k] = -1.e30f;
		}
	}


	numTransforms = currentMetadata->GetNumTransforms();
	for (int k = 0; k<dataAtLevel.size(); k++) delete dataAtLevel[k];
	dataAtLevel.clear();
	for (int k = 0; k<= numTransforms; k++){
		dataAtLevel.push_back(new size_t[3]);
	}
	
	myReader->GetDim(fullDataSize, -1);
		

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
	//WaveletBlock3DRegionReader* myReader = (WaveletBlock3DRegionReader*)dm->GetRegionReader();
	const VDFIOBase* myReader = dm->GetRegionReader();
	for (int lev = 0; lev <= numTransforms; lev++){
		myReader->GetDim(dataAtLevel[lev],lev);
	}
	bool someDataOverall = false;
	for (int var = 0; var< numSesVariables; var++){
		bool dataExists = false;
		//string s = getVariableName(var);
		//Check first if this variable is in the metadata:
		bool inMetadata = false;
		for (int i = 0; i< (int)currentMetadata->GetVariables3D().size(); i++){
			if (currentMetadata->GetVariables3D()[i] == getVariableName(var)){
				inMetadata = true;
				break;
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
			int maxXLevel = -1;
			for (int xf = numTransforms; xf>= 0; xf--){
				if (myReader->VariableExists(ts, 
					getVariableName(var).c_str(),
					xf)) {
						maxXLevel = xf;
						break;
					}
			}
			
			maxNumTransforms[var][ts] = maxXLevel;
			if (maxXLevel >= 0){
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
		//string s = getVariableName(var);
		//Check first if this variable is in the metadata:
		bool inMetadata = false;
		for (int i = 0; i< (int)currentMetadata->GetVariables2DXY().size(); i++){
			if (currentMetadata->GetVariables2DXY()[i] == getVariableName2D(var)){
				inMetadata = true;
				break;
			}
		}
		if (!inMetadata) {
			for (int i = 0; i< (int)currentMetadata->GetVariables2DYZ().size(); i++){
				if (currentMetadata->GetVariables2DYZ()[i] == getVariableName2D(var)){
					inMetadata = true;
					break;
				}
			}
		}
		if (!inMetadata) {
			for (int i = 0; i< (int)currentMetadata->GetVariables2DXZ().size(); i++){
				if (currentMetadata->GetVariables2DXZ()[i] == getVariableName2D(var)){
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
			int maxXLevel = -1;
			for (int xf = numTransforms; xf>= 0; xf--){
				if (myReader->VariableExists(ts, 
					getVariableName2D(var).c_str(),
					xf)) {
						maxXLevel = xf;
						break;
					}
			}
			
			maxNumTransforms2D[var][ts] = maxXLevel;
			if (maxXLevel >= 0){
				dataExists = true;
				someDataOverall = true;
				if (ts > (int)maxts) maxts = ts;
				if (ts < (int)mints) mints = ts;
				
			}
		}
		variableExists2D[var] = dataExists; 
	}

	QApplication::restoreOverrideCursor();
	minTimeStep = (size_t)mints;
	maxTimeStep = (size_t)maxts;
	if (dataIsLayered()){	
		LayeredIO* layeredReader = (LayeredIO*)myReader;
		layeredReader->SetLowHighVals(variableNames, belowValues, aboveValues);
	}
	//For now there are no low high values for 2D variables...
	return someDataOverall;
}



DataStatus::
~DataStatus(){
	int numVariables = maxNumTransforms.size();
	for (int i = 0; i< numVariables; i++){
		delete maxNumTransforms[i];
		delete dataMin[i];
		delete dataMax[i];
	}
	for (int i = 0; i< dataAtLevel.size(); i++){
		delete dataAtLevel[i];
	}
	theDataStatus = 0;
}
void DataStatus::setDefaultPrefs(){
	doWarnIfDataMissing = true;
	doUseLowerRefinementLevel = false;
	backgroundColor =  Qt::black;
	regionFrameColor = Qt::white;
	subregionFrameColor = Qt::red;
	regionFrameEnabled = true;
	subregionFrameEnabled = false;
}
int DataStatus::
getFirstTimestep(int sesvarnum){
	for (int i = 0; i< numTimesteps; i++){
		if(dataIsPresent(sesvarnum,i)) return i;
	}
	return -1;
}
	
// calculate the datarange for a specific session variable and timestep:
// Performed on demand.  Error if not in metadata.
// 
void DataStatus::calcDataRange(int varnum, int ts){
	vector<double>minMax;
	
	if (maxNumTransforms[varnum][ts] >= 0){
		
		//Trap errors:
		//
		ErrMsgCB_T errorCallback = GetErrMsgCB();
		SetErrMsgCB(0);
		float mnmx[2];
		int rc = ((DataMgr*)getDataMgr())->GetDataRange(ts, 
			getVariableName(varnum).c_str(), mnmx);
		//Turn it back on:
		SetErrMsgCB(errorCallback);
					
		if(rc<0){
			//Post an error:
			SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"Missing DataRange in variable %s, at timestep %d \n Interval [0,1] assumed",
				getMetadata()->GetVariableNames()[varnum].c_str(), ts);
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
	
	if (maxNumTransforms2D[varnum][ts] >= 0){
		
		//Trap errors:
		//
		ErrMsgCB_T errorCallback = GetErrMsgCB();
		SetErrMsgCB(0);
		float mnmx[2];
		int rc = ((DataMgr*)getDataMgr())->GetDataRange(ts, 
			getVariableName2D(varnum).c_str(), mnmx);
		//Turn it back on:
		SetErrMsgCB(errorCallback);
					
		if(rc<0){
			//Post an error:
			SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"Missing DataRange in 2D variable %s, at timestep %d \n Interval [0,1] assumed",
				getMetadata()->GetVariableNames()[varnum].c_str(), ts);
		}
		else{
			dataMax2D[varnum][ts] = mnmx[1];
			dataMin2D[varnum][ts] = mnmx[0];
		}
	}
}

//Find which index is associated with a name, or -1 if not metadata:
int DataStatus::getSessionVariableNum(const string& str){
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
int DataStatus::mapSessionToMetadataVarNum2D(int sesVarNum){
	for (int i = 0; i< numMetadataVariables2D; i++){
		if (mapMetadataVars2D[i] == sesVarNum) return i;
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
void DataStatus::getExtentsAtLevel(int level, float exts[6]){
	size_t minm[3], maxm[3];
	double usermin[3], usermax[3];
	int minframe = (int)minTimeStep;
	myReader->GetValidRegion(minm, maxm, level);
	myReader->MapVoxToUser(minframe, minm, usermin, level);
	myReader->MapVoxToUser(minframe, maxm, usermax, level);
	for (int i = 0; i<3; i++){
		exts[i] = (float)usermin[i];
		exts[i+3] = (float)usermax[i];
	}
}

bool DataStatus::sphericalTransform()
{
  if (currentMetadata)
  {
    return (currentMetadata->GetCoordSystemType() == "spherical");
  }

  return false;
}

bool DataStatus::dataIsLayered(){
	if (!currentMetadata) return false;
	if(!(StrCmpNoCase(currentMetadata->GetGridType(),"layered") == 0)) return false;
	//Check if there is an ELEVATION variable:
	for (int i = 0; i<variableNames.size(); i++){
		if (StrCmpNoCase(variableNames[i],"ELEVATION") == 0) return true;
	}
	return false;
}

int DataStatus::
getMetadataVarNum(std::string varname){
	if (currentMetadata == 0) return -1;

	const vector<string>& names = currentMetadata->GetVariableNames();
	for (int i = 0; i<names.size(); i++){
		if (names[i] == varname) return i;
	}
	return -1;
}
bool DataStatus::fieldDataOK(int refLevel, int tstep, int varx, int vary, int varz){
	int testRefLevel = refLevel;
	if (doUseLowerRefinementLevel) testRefLevel = 0;

	if (varx >= 0 && (maxXFormPresent(varx, tstep) < testRefLevel)) return false;
	if (vary >= 0 && (maxXFormPresent(vary, tstep) < testRefLevel)) return false;
	if (varz >= 0 && (maxXFormPresent(varz, tstep) < testRefLevel)) return false;
	return true;
}
