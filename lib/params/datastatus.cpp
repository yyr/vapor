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
#include <qstring.h>
#include <qapplication.h>
#include <qcursor.h>

using namespace VAPoR;
using namespace VetsUtil;
#include "vaporinternal/common.h"

//This is a singleton class, but it's created by the Session.
//Following are static, must persist even when there is no instance:
DataStatus* DataStatus::theDataStatus = 0;
std::vector<std::string> DataStatus::variableNames;
int DataStatus::numMetadataVariables = 0;
int* DataStatus::mapMetadataVars = 0;
//Default constructor
//Whether or not it exists on disk, what's its max and min
//What resolutions are available.
//
DataStatus::
DataStatus()
{
	dataMgr = 0;
	renderOK = false;
	myReader = 0;
	cacheMB = 0;
	minTimeStep = 0;
	maxTimeStep = 0;
	numTimesteps = 0;
	numTransforms = 0;
	for (int i = 0; i< 3; i++){
		extents[i] = 0.f;
		extents[i+3] = 1.f;
	}
	theDataStatus = this;
}

// After a metadata::merge, call resetDataStatus to 
// add additional and/or modify previous variables
// return true if there was anything to set up.
// 
bool DataStatus::
reset(DataMgr* dm, size_t cachesize){
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
	
	
	int numVars = currentMetadata->GetVariableNames().size();
	if (numVars == 0) return false;
	numMetadataVariables = numVars;
	mapMetadataVars = new int[numMetadataVariables];
	
	for (int i = 0; i<numVars; i++){
		bool match = false;
		for (int j = 0; j< getNumVariables(); j++){
			if (getVariableName(j) == currentMetadata->GetVariableNames()[i]){
				const char* foo = getVariableName(j).c_str();
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

	int numVariables = getNumVariables();
	
	variableExists.resize(numVariables);
	
	maxNumTransforms.resize(numVariables);
	dataMin.resize(numVariables);
	dataMax.resize(numVariables);
	for (int i = 0; i<numVariables; i++){
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


	numTransforms = currentMetadata->GetNumTransforms();

	
	for (int k = 0; k< 3; k++){
		fullDataSize[k] = currentMetadata->GetDimension()[k]; 
	}
	

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
	WaveletBlock3DRegionReader* myReader = (WaveletBlock3DRegionReader*)dm->GetRegionReader();
	bool someDataOverall = false;
	for (int var = 0; var< numVariables; var++){
		bool dataExists = false;
		
		//Check first if this variable is in the metadata:
		bool inMetadata = false;
		for (int i = 0; i< (int)currentMetadata->GetVariableNames().size(); i++){
			if (currentMetadata->GetVariableNames()[i] == getVariableName(var)){
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
	QApplication::restoreOverrideCursor();
	minTimeStep = (size_t)mints;
	maxTimeStep = (size_t)maxts;
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
	theDataStatus = 0;
}
int DataStatus::
getFirstTimestep(int varnum){
	for (int i = 0; i< numTimesteps; i++){
		if(dataIsPresent(varnum,i)) return i;
	}
	return -1;
}
	
// calculate the datarange for a specific variable and timestep:
// Performed on demand.
// 
void DataStatus::calcDataRange(int varnum, int ts){
	vector<double>minMax;
	
	if (maxNumTransforms[varnum][ts] >= 0){
		
		//Trap errors:
		//
		ErrMsgCB_T errorCallback = GetErrMsgCB();
		SetErrMsgCB(0);
		const float* mnmx = ((DataMgr*)getDataMgr())->GetDataRange(ts, 
			getMetadata()->GetVariableNames()[varnum].c_str());
		//Turn it back on:
		SetErrMsgCB(errorCallback);
					
		if(!mnmx){
			//Post an error:
			SetErrMsg("Missing DataRange in variable %s, at timestep %d \n Interval [0,1] assumed",
				getMetadata()->GetVariableNames()[varnum].c_str(), ts);
		}
		else{
			dataMax[varnum][ts] = mnmx[1];
			dataMin[varnum][ts] = mnmx[0];
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
//Make sure this name is in list; if not, insert it, return index.
//Useful in parsing
int DataStatus::mergeVariableName(const string& str){
	for (int i = 0; i<variableNames.size(); i++){
		if(variableNames[i] == str) return i;
	}
	//Not found, put it in:
	variableNames.push_back(str);
	return (variableNames.size()-1);
}
int DataStatus::mapRealToMetadataVarNum(int realVarNum){
	for (int i = 0; i< numMetadataVariables; i++){
		if (mapMetadataVars[i] == realVarNum) return i;
	}
	return -1;
}
/*
void DataStatus::fillMetadataVars(){
	removeMetadataVars();
	for (int i = 0; i< getNumVariables(); i++){
		if (variableIsPresent(i)){
			numMetadataVariables++;
		}
	}
	mapMetadataVars = new int[numMetadataVariables];
	int posnCounter = 0;
	//fill in the values...
	for (int i = 0; i< getNumVariables(); i++){
		if (variableIsPresent(i)){
			mapMetadataVars[posnCounter++] = i;
		}
	}
}
*/

//Convert the max extents into cube coords
void DataStatus::getMaxExtentsInCube(float maxExtents[3]){
	float maxSize = Max(extents[3]-extents[0],Max(extents[4]-extents[1],extents[5]-extents[2]));
	maxExtents[0] = (extents[3]-extents[0])/maxSize;
	maxExtents[1] = (extents[4]-extents[1])/maxSize;
	maxExtents[2] = (extents[5]-extents[2])/maxSize;
}
