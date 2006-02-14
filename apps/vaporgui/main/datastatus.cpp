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
#include "session.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include "histo.h"
#include "vapor/Metadata.h"
#include "vapor/ImpExp.h"
#include "animationcontroller.h"
#include "transferfunction.h"
#include "messagereporter.h"
#include <qstring.h>
#include <qapplication.h>
#include <qcursor.h>

using namespace VAPoR;

//Default constructor
//Whether or not it exists on disk, what's its max and min
//What resolutions are available.
//
DataStatus::
DataStatus()
{
	
	minTimeStep = 0;
	maxTimeStep = 0;
	numTimesteps = 0;
	numTransforms = 0;
	
}

// After a metadata::merge, call resetDataStatus to 
// add additional and/or modify previous variables
// return true if there was anything to set up.
// 
bool DataStatus::
reset(DataMgr* dm){
	Session* ses = Session::getInstance();
	const Metadata* currentMetadata = dm->GetMetadata();
	
	unsigned int numTS = (unsigned int)currentMetadata->GetNumTimeSteps();
	if (numTS == 0) return false;
	assert (numTS >= getNumTimesteps());  //We should always be increasing this
	numTimesteps = numTS;
	
	//clean out the various status arrays:

	//Add all new variable names to the session's variable name list.
	int numVars = currentMetadata->GetVariableNames().size();
	if (numVars == 0) return false;
	
	for (int i = 0; i<numVars; i++){
		bool match = false;
		for (int j = 0; j< ses->getNumVariables(); j++){
			if (ses->getVariableName(j) == currentMetadata->GetVariableNames()[i]){
				match = true;
				break;
			}
		}
		if (match) continue;
		//Note that we are modifying the very array that we are looping over.
		//
		ses->addVarName(currentMetadata->GetVariableNames()[i]);
	}

	int numVariables = ses->getNumVariables();
	
	variableExists.resize(numVariables);
	
	maxNumTransforms.resize(numVariables);
	dataMin.resize(numVariables);
	dataMax.resize(numVariables);
	for (int i = 0; i<numVariables; i++){
		variableExists[i] = false;
		maxNumTransforms[i] = new int[numTimesteps];
		dataMin[i] = new float[numTimesteps];
		dataMax[i] = new float[numTimesteps];
		for (int k = 0; k<numTimesteps; k++){
			maxNumTransforms[i][k] = -1;
			dataMin[i][k] = -1.f;
			dataMax[i][k] = 1.f;
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
		int maxXLevel = -1;
		//Check first if this variable is in the metadata:
		bool inMetadata = false;
		for (int i = 0; i< currentMetadata->GetVariableNames().size(); i++){
			if (currentMetadata->GetVariableNames()[i] == ses->getVariableName(var)){
				inMetadata = true;
				break;
			}
		}
		if (! inMetadata) {
			//variableExists[var] = false;
			continue;
		}
		//OK, it's in the metadata, check the timesteps...
		for (unsigned int ts = 0; ts< numTimesteps; ts++){
			//Find the maximum number of transforms available on disk
			//i.e., the highest transform level  (highest resolution) that exists
			
			for (int xf = numTransforms; xf>= 0; xf--){
				if (myReader->VariableExists(ts, 
					ses->getVariableName(var).c_str(),
					xf)) {
						maxXLevel = xf;
						break;
					}
			}
			
			maxNumTransforms[var][ts] = maxXLevel;
			if (maxXLevel >= 0){
				dataExists = true;
				someDataOverall = true;
				if (ts > maxts) maxts = ts;
				if (ts < mints) mints = ts;
				//Get the min, max datarange at the max refinement level
				//This sets the values
				calcDataRange(var,ts);
			}
		}
		variableExists[var] = dataExists; 
	}
	QApplication::restoreOverrideCursor();
	minTimeStep = (size_t)mints;
	maxTimeStep = (size_t)maxts;
	return someDataOverall;
}

// calculate the datarange for a specific variable and timestep:
// 
void DataStatus::calcDataRange(int varnum, int ts){
	vector<double>minMax;
	Session* ses = Session::getInstance();
	if (maxNumTransforms[varnum][ts] >= 0){
		//Turn off error callback, we can handle missing datarange:
		ses->pauseErrorCallback();
		const float* mnmx = ses->getDataMgr()->GetDataRange(ts, 
			ses->getDataMgr()->GetMetadata()->GetVariableNames()[varnum].c_str());
		//Turn it back on:
		ses->resumeErrorCallback();
					
		if(!mnmx){
			MessageReporter::warningMsg("Missing DataRange in variable %s, at timestep %d \n Interval [0,1] assumed",
				ses->getDataMgr()->GetMetadata()->GetVariableNames()[varnum].c_str(), ts);
			MyBase::SetErrCode(0);
		}
		else{
			dataMax[varnum][ts] = mnmx[1];
			dataMin[varnum][ts] = mnmx[0];
		}
	}
}

DataStatus::
~DataStatus(){
	int numVariables = maxNumTransforms.size();
	for (int i = 0; i< numVariables; i++){
		delete maxNumTransforms[i];
		delete dataMin[i];
		delete dataMax[i];
	}
}
int DataStatus::
getFirstTimestep(int varnum){
	for (int i = 0; i< numTimesteps; i++){
		if(dataIsPresent(varnum,i)) return i;
	}
	return -1;
}
		

