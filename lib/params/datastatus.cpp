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
#include <vapor/DataMgrV3_0.h>
#include <vapor/Version.h>
#include <vapor/Proj4API.h>
#include <vapor/errorcodes.h>
#include "math.h"
using namespace VAPoR;
using namespace VetsUtil;
#include <vapor/common.h>

//This is a singleton class
//Following are static, must persist even when there is no instance:
DataStatus* DataStatus::theDataStatus = 0;
const std::string DataStatus::_emptyString = "";
const vector<string> DataStatus::emptyVec;
double DataStatus::stretchFactors[3];
double DataStatus::fullStretchedSizes[3];
bool DataStatus::doUseLowerAccuracy = false;
double DataStatus::extents[6];
double DataStatus::stretchedExtents[6];
double DataStatus::fullSizes[3];
size_t DataStatus::minTimeStep;
size_t DataStatus::maxTimeStep;
int DataStatus::numTimesteps;
DataMgrV3_0* DataStatus::dataMgr;

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
	
	defaultRefFidelity2D = 4.f;
	defaultRefFidelity3D = 4.f;
	defaultLODFidelity2D = 2.f;
	defaultLODFidelity3D = 2.f;
	
	for (int i = 0; i< 3; i++){
		extents[i] = 0.f;
		stretchedExtents[i] = 0.f;
		extents[i+3] = 1.f;
		stretchedExtents[i+3] = 1.f;
		stretchFactors[i] = 1.f;
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
// Set the extents based on the domainVariables in the Region 
bool DataStatus::
reset(DataMgrV3_0* dm, size_t cachesize){
	cacheMB = cachesize;
	
	dataMgr = dm;
	if (!dm) return false;
	vector<double>timecoords;
	dataMgr->GetTimeCoordinates(timecoords);
	size_t numTS = timecoords.size();
	if (numTS == 0) return false;
	numTimesteps = numTS;
	
	//Find the first and last time for which there is data:
	vector<string> varnames = dataMgr->GetDataVarNames();
	int mints = -1;
	int maxts = -1;
	for (size_t i = 0; i<numTS; i++){
		for (int j = 0; j<varnames.size(); j++){
			if (!dataMgr->VariableExists(i, varnames[j])) continue;
			mints = i;
			break;
		}
		if (mints >= 0) break;
	}
	for (size_t i = numTS-1; i>=0; i--){
		for (int j = 0; j<varnames.size(); j++){
			if (!dataMgr->VariableExists(i, varnames[j])) continue;
			maxts = i;
			break;
		}
		if (maxts >= 0) break;
	}
	if (mints < 0 || maxts < 0) return false;
	minTimeStep = mints;
	maxTimeStep = maxts;
	//Determine the domain extents.  Obtain them from RegionParams, but
	//if they are not available, find the first variable in the VDC
	vector<string> domainVars = RegionParams::GetDomainVariables();
	//Determine the range of time steps for which there is data.

	size_t firstTimeStep = getMinTimestep();
	
	
	bool getDomainVars = true;
	if (domainVars.size() > 0) {
		//see if a variable exists at the first time.  If so use the domainVars.  Otherwise find another
		getDomainVars = false;
		for (int j = 0; j<domainVars.size(); j++){
			if (dataMgr->VariableExists(firstTimeStep, domainVars[j])){
				getDomainVars = true;
				break;
			}
		}
	}
	if (getDomainVars){
		//Use the first variable that has data at the first time step.
		domainVars.clear();
		vector<string>varnames = dataMgr->GetDataVarNames(3,true);
		for (int j = 0; j< varnames.size(); j++){
			string varname = varnames[j];
			if (dataMgr->VariableExists(firstTimeStep, varname)){
				domainVars.push_back(varname);
				RegionParams::SetDomainVariables(domainVars);
				getDomainVars = false;
				break;
			}
		}
	}
	//Now use the domain vars to calculate extents. They will be the union of all the extents of the domain-defining variables:
	vector<double>minVarExts, maxVarExts;
	if (0!= GetExtents(firstTimeStep, minVarExts, maxVarExts)) return false;
		
	for (int i = 0; i< 3; i++) {
		extents[i+3] = (maxVarExts[i]-minVarExts[i]);
		extents[i] = 0.;
		fullSizes[i] =(maxVarExts[i]-minVarExts[i]);
		fullStretchedSizes[i] = fullSizes[i]*stretchFactors[i];
		stretchedExtents[i] = extents[i]*stretchFactors[i];
		stretchedExtents[i+3] = extents[i+3]*stretchFactors[i];
	}

	return true;
}



DataStatus::
~DataStatus(){
	
	theDataStatus = 0;
}


float
DataStatus::getDefaultDataMax(string vname){
	float range[2];
	float dmax = 1.f;
	if (dataMgr->VariableExists(minTimeStep, vname)){
		RegularGrid* rGrid = dataMgr->GetVariable(minTimeStep, vname ,0,0);
		rGrid->GetRange(range);
		dmax = range[1];
	}
	return dmax;
}
float
DataStatus::getDefaultDataMin(string vname){
	float range[2];
	float dmax = 1.f;
	if (dataMgr->VariableExists(minTimeStep, vname)){
		RegularGrid* rGrid = dataMgr->GetVariable(minTimeStep,vname, 0,0);
		rGrid->GetRange(range);
		dmax = range[1];
	}
	return dmax;
}


//Map corners of box to voxels.  
void DataStatus::mapBoxToVox(Box* box, int refLevel, int lod, int timestep, size_t voxExts[6]){
	double userExts[6];
	box->GetUserExtents(userExts,(size_t)timestep);
	
	//calculate new values for voxExts (Note: this can be expensive with layered data)
//??	dataMgr->MapUserToVox((size_t)timestep,userExts,voxExts, refLevel,lod);
//??	dataMgr->MapUserToVox((size_t)timestep,userExts+3,voxExts+3, refLevel,lod);
	return;

}
void DataStatus::
localToStretchedCube(const double fromCoords[3], double toCoords[3]){
	const double* stretch = getStretchFactors();
	for (int i = 0; i<3; i++){
		toCoords[i] = (fromCoords[i]*stretch[i])/getMaxStretchedSize();
	}
	return;
}
int DataStatus::maxXFormPresent(string varname, size_t timestep){
	int maxx = dataMgr->GetNumRefLevels(varname);
	int i;
	for (i = 0 ; i<maxx; i++){
		if (!dataMgr->VariableExists(timestep, varname, i)) break;
	}
	
	return i-1;
}
int DataStatus::maxLODPresent(string varname, size_t timestep){
	vector<size_t>ratios;
	int rc = dataMgr->GetCRatios(varname,ratios);
	if (rc != 0) return -1;
	int i;
	for (i =0 ; i<ratios.size(); i++){
		if (!dataMgr->VariableExists(timestep, varname,0,i)) break;
	}
	return i-1;
}

//static methods to convert coordinates to and from latlon
//coordinates are in the order longitude,latitude
//result is in user coordinates.
bool DataStatus::convertFromLonLat(double coords[2], int npoints){
	//Set up proj.4 to convert from LatLon to VDC coords
	string projString = getInstance()->getDataMgr()->GetMapProjection();
	if (projString.size() == 0) return false;

	Proj4API proj4API;
	int rc = proj4API.Initialize("", projString);
	if (rc<0) return (false);

	rc = proj4API.Transform(coords, coords+1, npoints, 2);
	if (rc<0) return(false);
	
	return true;
	
}
bool DataStatus::convertLocalFromLonLat(int timestep, double coords[2], int npoints){
	DataMgrV3_0* dataMgr = getInstance()->getDataMgr();
	if (!dataMgr) return false;
	if(!convertFromLonLat(coords, npoints)) return false;
	vector<double> minExts, maxExts;
	int rc = GetExtents((size_t)timestep,minExts, maxExts);
	if (rc) return false;
	for (int i = 0; i<npoints; i++){
		coords[2*i] -= minExts[0];
		coords[2*i+1] -= minExts[1];
	}
	return true;
}
bool DataStatus::convertLocalToLonLat(int timestep, double coords[2], int npoints){
	DataMgrV3_0* dataMgr = getInstance()->getDataMgr();
	if (!dataMgr) return false;
	vector<double> minExts, maxExts;
	int rc = GetExtents((size_t)timestep,minExts, maxExts);
	if (rc) return false;
	
	//Convert local to user coordinates:
	for (int i = 0; i<npoints; i++){
		coords[2*i] += minExts[0];
		coords[2*i+1] += minExts[1];
	}
	if(!convertToLonLat(coords, npoints)) return false;
	return true;
}
//coordinates are always in user coordinates.
bool DataStatus::convertToLonLat(double coords[2], int npoints){

	//Set up proj.4 to convert to latlon
	string pstring = getInstance()->getDataMgr()->GetMapProjection();
	if (pstring.size() == 0) return false;

	Proj4API proj4API;
	int rc = proj4API.Initialize(pstring, "");
	if (rc<0) return (false);

	rc = proj4API.Transform(coords, coords+1, npoints, 2);
	if (rc<0) return(false);

	return true;
	
}
int DataStatus::GetExtents(size_t timestep, vector<double>& minExts, vector<double>& maxExts){
	
	vector<double> tempMin,tempMax;
	vector<string> domainVars = RegionParams::GetDomainVariables();
	bool varfound = false;
	DataMgrV3_0* dataMgr = getDataMgr();
	for (int j = 0; j<domainVars.size(); j++){
		int rc = dataMgr->GetVariableExtents(minTimeStep, domainVars[j],maxXFormPresent(domainVars[j],minTimeStep), tempMin, tempMax);
		if (rc != 0) continue;
		if (!varfound) {
			minExts = tempMin;
			maxExts = tempMax;
			varfound = true;
		} else {
			for (int k = 0; k<3; k++){
				if (minExts[k] > tempMin[k]) minExts[k]=tempMin[k];
				if (maxExts[k] < tempMax[k]) maxExts[k] = tempMax[k];
			}
		}
	}
	return !varfound;
}
bool DataStatus::dataIsPresent(int timestep){
	DataMgrV3_0* dataMgr = getDataMgr();
	vector<string> vars = dataMgr->GetDataVarNames();
	for (int i = 0; i<vars.size(); i++){
		if (dataMgr->VariableExists(timestep,vars[i])) return true;
	}
	return false;
}
int DataStatus::mapVoxToUser(size_t timestep, string varname, const size_t vcoords[3], double uCoords[3], int reflevel){
	RegularGrid* rGrid = getDataMgr()->GetVariable(timestep, varname, reflevel, 0);
	int rc = rGrid->GetUserCoordinates(vcoords[0],vcoords[1],vcoords[2],uCoords, uCoords+1, uCoords+2);
	return rc;

}
void DataStatus::mapUserToVox(size_t timestep, string varname, const double uCoords[3], size_t vCoords[3], int reflevel){
	RegularGrid* rGrid = getDataMgr()->GetVariable(timestep, varname, reflevel, 0);
	rGrid->GetIJKIndex(uCoords[0],uCoords[1],uCoords[2],vCoords, vCoords+1, vCoords+2);
}
int DataStatus::getActiveVarNum(int dim, string varname){
	vector<string>varnames = dataMgr->GetDataVarNames(dim,true);
	for (int i = 0; i<varnames.size(); i++){
		if (varnames[i] == varname) return i;
	}
	return -1;
}
double DataStatus::getVoxelSize(size_t ts, string varname, int refLevel, int dir){
	RegularGrid* rGrid = dataMgr->GetVariable(ts, varname, refLevel, 0);
	size_t dims[3];
	rGrid->GetDimensions(dims);
	double extents[6];
	rGrid->GetUserExtents(extents);
	int numdims = rGrid->GetRank();
	if (numdims < dir) return 0.;
	double vsize =  ((extents[dir+3]-extents[dir])/dims[dir]);
	return vsize;
}


