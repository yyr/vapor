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
#include "proj_api.h"
#include "math.h"
using namespace VAPoR;
using namespace VetsUtil;
#include "vaporinternal/common.h"

//This is a singleton class, but it's created by the Session.
//Following are static, must persist even when there is no instance:
DataStatus* DataStatus::theDataStatus = 0;
std::vector<std::string> DataStatus::variableNames;
std::vector<float> DataStatus::aboveValues;
std::vector<float> DataStatus::belowValues;
std::vector<bool> DataStatus::extendUp;
std::vector<bool> DataStatus::extendDown;
int DataStatus::numMetadataVariables = 0;
int* DataStatus::mapMetadataVars = 0;
std::vector<std::string> DataStatus::variableNames2D;
std::vector<float*> DataStatus::timeVaryingExtents;
std::string DataStatus::projString;

int DataStatus::numMetadataVariables2D = 0;
int DataStatus::numOriented2DVars[3] = {0,0,0};
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
int DataStatus::interactiveRefLevel = 0;

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

	projString = currentMetadata->GetMapProjection();
	
	timeVaryingExtents.clear();
	for (int i = 0; i<numTS; i++){
		
		const vector<double>& mdexts = currentMetadata->GetTSExtents(i);
		if (mdexts.size() < 6) {
			timeVaryingExtents.push_back(0);
			continue;
		}
		float* tsexts = new float[6];
		for (int j = 0; j< 6; j++){ tsexts[j] = mdexts[j];}
		timeVaryingExtents.push_back(tsexts);
	}
	
	//clean out the various status arrays:

	//Add all new variable names to the variable name list, while building the 
	//mapping of metadata var nums into session var nums:
	removeMetadataVars();
	
	int num3dVars = currentMetadata->GetVariables3D().size();
	
	numMetadataVariables = num3dVars;
	mapMetadataVars = new int[numMetadataVariables];

	numOriented2DVars[0] = currentMetadata->GetVariables2DXY().size();
	numOriented2DVars[1] = currentMetadata->GetVariables2DYZ().size();
	numOriented2DVars[2] = currentMetadata->GetVariables2DXZ().size();

	int numVars = num3dVars + numOriented2DVars[0];
	if (numVars <=0) return false;
	int numMetaVars2D = numOriented2DVars[0]+numOriented2DVars[1]+numOriented2DVars[2];
	numMetadataVariables2D = numMetaVars2D;
	mapMetadataVars2D = new int[numMetadataVariables2D];
	
	for (int i = 0; i<num3dVars; i++){
		bool match = false;
		for (int j = 0; j< getNumSessionVariables(); j++){
			if (getVariableName(j) == currentMetadata->GetVariables3D()[i]){
				mapMetadataVars[i] = j;
				match = true;
				break;
			}
		}
		if (match) continue;
		//Note that we are modifying the very array that we are looping over.
		//
		string str = currentMetadata->GetVariables3D()[i];
		addVarName(currentMetadata->GetVariables3D()[i]);
		mapMetadataVars[i] = variableNames.size()-1;
	}
	//Make sure all the metadata vars are in session vars
	for (int i = 0; i<numMetaVars2D; i++){
		bool match = false;
		
		for (int j = 0; j< getNumSessionVariables2D(); j++){
			
			if (i < numOriented2DVars[0]){
				
				if (getVariableName2D(j) == currentMetadata->GetVariables2DXY()[i]){
					mapMetadataVars2D[i] = j;
					match = true;
					break;
				}
			} else if ( i < numOriented2DVars[0]+numOriented2DVars[1]){
				if (getVariableName2D(j) == currentMetadata->GetVariables2DYZ()[i-numOriented2DVars[0]]){
					mapMetadataVars2D[i] = j;
					match = true;
					break;
				}
			} else if ( i < numOriented2DVars[0]+numOriented2DVars[1]+numOriented2DVars[2]){
				if (getVariableName2D(j) == currentMetadata->GetVariables2DXZ()[i-numOriented2DVars[0]-numOriented2DVars[1]]){
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
			addVarName2D(currentMetadata->GetVariables2DXY()[i]);
		else if (i < numOriented2DVars[0]+numOriented2DVars[1])
			addVarName2D(currentMetadata->GetVariables2DYZ()[i-numOriented2DVars[0]]);
		else addVarName2D(currentMetadata->GetVariables2DXZ()[i-numOriented2DVars[0]-numOriented2DVars[1]]);
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
		//construct a list of the non extended variables
		std::vector<string> vNames;
		std::vector<float> vals;
		for (int i = 0; i< getNumSessionVariables(); i++){
			if (!isExtendedDown(i)){
				vNames.push_back(getVariableName(i));
				vals.push_back(getBelowValue(i));
			}
		}
		layeredReader->SetLowVals(vNames, vals);
		vNames.clear();
		for (int i = 0; i< getNumSessionVariables(); i++){
			if (!isExtendedUp(i)){
				vNames.push_back(getVariableName(i));
				vals.push_back(getAboveValue(i));
			}
		}
		layeredReader->SetHighVals(vNames, vals);
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
			SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"Missing DataRange in variable %s,\nat timestep %d \n Interval [0,1] assumed",
				getVariableName(varnum).c_str(), ts);
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
				getVariableName2D(varnum).c_str(), ts);
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
	//int minframe = (int)minTimeStep;
	myReader->GetValidRegion(minm, maxm, level);
	myReader->MapVoxToUser((size_t)-1, minm, usermin, level);
	myReader->MapVoxToUser((size_t)-1, maxm, usermax, level);
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

	const vector<string>& names = currentMetadata->GetVariables3D();
	for (int i = 0; i<names.size(); i++){
		if (names[i] == varname) return i;
	}
	return -1;
}
int DataStatus::
getMetadataVarNum2D(std::string varname){
	if (currentMetadata == 0) return -1;

	const vector<string>& namesxy = currentMetadata->GetVariables2DXY();
	for (int i = 0; i<namesxy.size(); i++){
		if (namesxy[i] == varname) return i;
	}
	const vector<string>& namesyz = currentMetadata->GetVariables2DYZ();
	for (int i = 0; i<namesyz.size(); i++){
		if (namesyz[i] == varname) return (i+namesxy.size());
	}
	const vector<string>& namesxz = currentMetadata->GetVariables2DXZ();
	for (int i = 0; i<namesxz.size(); i++){
		if (namesxz[i] == varname) return (i+namesxy.size()+namesyz.size());
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
//Orientation is 2 for XY, 0 for YZ, 1 for XZ 
int DataStatus::get2DOrientation(int mdvar){
	if (getNumMetadataVariables2D() <= mdvar) return 2;
	if (mdvar < numOriented2DVars[0]) return 2;
	if (mdvar < numOriented2DVars[0]+numOriented2DVars[1]) return 0;
	assert(mdvar < numOriented2DVars[0]+numOriented2DVars[1]+numOriented2DVars[2]);
	return 1;
}


//static methods to convert coordinates to and from latlon
//coordinates are in the order longitude,latitude
bool DataStatus::convertFromLatLon(int timestep, double coords[2], int npoints){
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
			return false;
		}
	}
	//Subtract offset to convert projection coords to vapor coords
	if(timestep >= 0) {
		const float * globExts = DataStatus::getInstance()->getExtents();
		const float* exts = getExtents(timestep);
		for (int i = 0; i<2*npoints; i++) coords[i] -= (exts[i%2]-globExts[i%2]);
	}
	return true;
	
}
//if timestep is -1, the coordinates are assumed to be in the invariant
//extents.  Otherwise they are taken to be in the time-varying extents
bool DataStatus::convertToLatLon(int timestep, double coords[2], int npoints){

	//Set up proj.4 to convert to latlon
	if (getProjectionString().size() == 0) return false;
	projPJ vapor_proj = pj_init_plus(getProjectionString().c_str());
	if (!vapor_proj) return false;
	projPJ latlon_proj = pj_latlong_from_proj( vapor_proj); 
	if (!latlon_proj) return false;
	if (timestep >= 0){
		const float * globExts = DataStatus::getInstance()->getExtents();
		//Apply projection offset to convert vapor local coords to projection space:
		const float* exts = getExtents(timestep);
		for (int i = 0; i<2*npoints; i++) coords[i] += (exts[i%2]-globExts[i%2]);
	}
	
	//If it's already latlon, we're done:
	if (pj_is_latlong(vapor_proj)) return true;

	static const double RAD2DEG = 180./3.1415926545;
	
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