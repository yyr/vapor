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

#include <vapor/ImpExp.h>
#include <vapor/MyBase.h>
#include <vapor/DataMgr.h>
#include <vapor/Version.h>

#include <vapor/errorcodes.h>
#include "math.h"
using namespace VAPoR;
using namespace VetsUtil;
#include <vapor/common.h>

//This is a singleton class, but it's created by the Session.
//Following are static, must persist even when there is no instance:
DataStatus* DataStatus::theDataStatus = 0;
const std::string DataStatus::_emptyString = "";
const vector<string> DataStatus::emptyVec;

size_t DataStatus::cacheMB = 0;


//Default constructor
//Whether or not it exists on disk, what's its max and min
//What resolutions are available.
//
DataStatus::
DataStatus()
{
	
	dataMgr = 0;

	minTimeStep = 0;
	maxTimeStep = 0;
	numTimesteps = 0;
	numTransforms = 0;
	numLODs = 0;
	
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
	
}

// After a metadata::merge, call resetDataStatus to 
// add additional and/or modify previous variables
// return true if there was anything to set up.
//
// If there are python scripts use their output variables.
// If the python scripts inputs are not in the DataMgr, remove the input variables.  
// 
bool DataStatus::
reset(DataMgr* dm, size_t cachesize){
	cacheMB = cachesize;
	
	dataMgr = dm;
	unsigned int numTS = (unsigned int)dataMgr->GetNumTimeSteps();
	if (numTS == 0) return false;
	MetadataVDC* md = dynamic_cast<MetadataVDC*>(dataMgr);
	
	assert (numTS >= getNumTimesteps());  //We should always be increasing this
	numTimesteps = numTS;
	if (!dm) return false;
	std::vector<double> mdExtents = dataMgr->GetExtents(0);
	for (int i = 0; i< 3; i++) {
		extents[i+3] = (float)(mdExtents[i+3]-mdExtents[i]);
		extents[i] = 0.;
		fullSizes[i] = (float)(mdExtents[i+3] - mdExtents[i]);
	}

	
	
	//clean out the various status arrays:

	
	int num3dVars = dataMgr->GetVariables3D().size();
	
	


	numTransforms = dataMgr->GetNumTransforms();
	numLODs = dataMgr->GetCRatios().size();
	
	dataMgr->GetDim(fullDataSize, -1);
		

	//As we go through the variables and timesteps, keep Track of min and max times
	unsigned int mints = 1000000000;
	int maxts = -1;
	
	
	
	
	bool someDataOverall = false;
	
	
    if (mints == 1000000000) mints = 0;
	
	

	minTimeStep = (size_t)mints;
	maxTimeStep = (size_t)maxts;
	
	return true;
}



DataStatus::
~DataStatus(){
	
	theDataStatus = 0;
}


