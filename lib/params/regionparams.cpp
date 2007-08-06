//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		regionparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Implements the RegionParams class.
//		This class supports parameters associted with the
//		region panel
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include "regionparams.h"


#include "datastatus.h"

#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "vapor/DataMgr.h"
#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "vapor/VDFIOBase.h"
#include "vapor/LayeredIO.h"
//#include "glutil.h"


using namespace VAPoR;

const string RegionParams::_regionCenterTag = "RegionCenter";
const string RegionParams::_regionSizeTag = "RegionSize";
const string RegionParams::_maxSizeAttr = "MaxSizeSlider";
const string RegionParams::_numTransAttr = "NumTrans";
const string RegionParams::_regionMinTag = "RegionMin";
const string RegionParams::_regionMaxTag = "RegionMax";
const string RegionParams::_fullHeightAttr = "FullGridHeight";

RegionParams::RegionParams(int winnum): Params(winnum){
	thisParamType = RegionParamsType;
	
	restart();
}
Params* RegionParams::
deepCopy(){
	//Just make a shallow copy, since there's nothing (yet) extra
	//
	RegionParams* p = new RegionParams(*this);
	
	return (Params*)(p);
}

RegionParams::~RegionParams(){
	
}






//See if the number of trans is ok.  If not, return an OK value
int RegionParams::
validateNumTrans(int n){
	//if we have a dataMgr, see if we can really handle this new numtrans:
	if (!DataStatus::getInstance()) return n;
	
	
	size_t min_dim[3], max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	getRegionVoxelCoords(n,min_dim,max_dim,min_bdim,max_bdim);
	
	//Size needed for data assumes blocksize = 2**5, 6 bytes per voxel, times 2.
	size_t newFullMB = (max_bdim[0]-min_bdim[0]+1)*(max_bdim[1]-min_bdim[1]+1)*(max_bdim[2]-min_bdim[2]+1);
	//right shift by 20 for megavoxels
	newFullMB >>= 20;
	//Multiply by 6 for 6 bytes per voxel
	newFullMB *= 6;
	while (newFullMB >= DataStatus::getInstance()->getCacheMB()){
		//find  and return a legitimate value.  Each time we increase n by 1,
		//we decrease the size needed by 8
		n--;
		newFullMB >>= 3;
	}	
	return n;
}
	//If we passed that test, then go ahead and change the numTrans.



//Reset region settings to initial state
void RegionParams::
restart(){
	infoNumRefinements = 0; 
	infoVarNum = 0;
	infoTimeStep = 0;
	fullHeight = 0;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds || !ds->getDataMgr()) ds = 0;
	const float* fullDataExtents = 0;
	if (ds) fullDataExtents = DataStatus::getInstance()->getExtents();
	
	
	for (int i = 0; i< 3; i++){
		if (ds){
			regionMin[i] = fullDataExtents[i];
			regionMax[i] = fullDataExtents[i+3];
		} else {
			regionMin[i] = 0.f;
			regionMax[i] = 1.f;
		}

	}
	
}
//Reinitialize region settings, session has changed
//Need to force the regionMin, regionMax to be OK.
bool RegionParams::
reinit(bool doOverride){
	int i;
	
	const float* extents = DataStatus::getInstance()->getExtents();
	bool isLayered = DataStatus::getInstance()->dataIsLayered();
	if (doOverride) {
		for (i = 0; i< 3; i++) {
			regionMin[i] = extents[i];
			regionMax[i] = extents[i+3];
		}
		if (isLayered) fullHeight = DataStatus::getInstance()->getFullDataSize(2);
		else fullHeight = 0;
	} else {
		//Just force them to fit in current volume 
		for (i = 0; i< 3; i++) {
			if (regionMin[i] > regionMax[i]) 
				regionMax[i] = regionMin[i];
			if (regionMin[i] > extents[i+3])
				regionMin[i] = extents[i+3];
			if (regionMin[i] < extents[i])
				regionMin[i] = extents[i];
			if (regionMax[i] > extents[i+3])
				regionMax[i] = extents[i+3];
			if (regionMax[i] < extents[i])
				regionMax[i] = extents[i];
		}
		if (isLayered && fullHeight == 0) fullHeight = DataStatus::getInstance()->getFullDataSize(2);
	}
	
	return true;	
}

void RegionParams::setRegionMin(int coord, float minval, bool checkMax){
	DataStatus* ds = DataStatus::getInstance();
	const float* fullDataExtents;
	if (ds && ds->getDataMgr()){
		fullDataExtents = ds->getExtents();
		if (minval < fullDataExtents[coord]) minval = fullDataExtents[coord];
		if (minval > fullDataExtents[coord+3]) minval = fullDataExtents[coord+3];
	}
	if (checkMax) {if (minval > regionMax[coord]) minval = regionMax[coord];}
	regionMin[coord] = minval;
}
void RegionParams::setRegionMax(int coord, float maxval, bool checkMin){
	DataStatus* ds = DataStatus::getInstance();
	const float* fullDataExtents;
	if (ds && ds->getDataMgr()){
		fullDataExtents = DataStatus::getInstance()->getExtents();
		if (maxval < fullDataExtents[coord]) maxval = fullDataExtents[coord];
		if (maxval > fullDataExtents[coord+3]) maxval = fullDataExtents[coord+3];
	}
	if (checkMin){if (maxval < regionMin[coord]) maxval = regionMin[coord];}
	regionMax[coord] = maxval;
}


//static method to do conversion to box coords (probably based on available
//coords, that may be smaller than region coords)
//Then puts into unit cube, for use by volume rendering
//
void RegionParams::
convertToStretchedBoxExtentsInCube(int refLevel, const size_t min_dim[3], const size_t max_dim[3], float extents[6],size_t fullHeight){
	double fullExtents[6];
	double subExtents[6];
	DataStatus* ds = DataStatus::getInstance();
	const size_t fullMin[3] = {0,0,0};
	size_t fullMax[3];
	
	for (int i = 0; i<3; i++) fullMax[i] = (int)DataStatus::getInstance()->getFullSizeAtLevel(refLevel,i,fullHeight) - 1;

	ds->mapVoxelToUserCoords(refLevel, fullMin, fullExtents);
	ds->mapVoxelToUserCoords(refLevel, fullMax, fullExtents+3);
	ds->mapVoxelToUserCoords(refLevel, min_dim, subExtents);
	ds->mapVoxelToUserCoords(refLevel, max_dim, subExtents+3);

	// Now apply stretch factors
	const float* stretchFactor = ds->getStretchFactors();
	for (int i = 0; i<6; i++){
		fullExtents[i] *= stretchFactor[i%3];
		subExtents[i] *= stretchFactor[i%3];
	}

	
	double maxSize = Max(Max(fullExtents[3]-fullExtents[0],fullExtents[4]-fullExtents[1]),fullExtents[5]-fullExtents[2]);
	for (int i = 0; i<3; i++){
		extents[i] = (float)((subExtents[i] - fullExtents[i])/maxSize);
		extents[i+3] = (float)((subExtents[i+3] - fullExtents[i])/maxSize);
	}
	
}
//static method to do conversion to box coords (probably based on available
//coords, that may be smaller than region coords)
//
void RegionParams::
convertToBoxExtents(int refLevel, const size_t min_dim[3], const size_t max_dim[3], float extents[6]){
	double dbExtents[6];
	DataStatus* ds = DataStatus::getInstance();
	
	ds->mapVoxelToUserCoords(refLevel, min_dim, dbExtents);
	ds->mapVoxelToUserCoords(refLevel, max_dim, dbExtents+3);
	for (int i = 0; i< 6; i++) extents[i] = (float)dbExtents[i];
}	
bool RegionParams::
getAvailableVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3], size_t timestep, const int* varNums, int numVars,
		double regMin[3], double regMax[3]){
	//First determine the bounds specified in this RegionParams
	int i;
	bool retVal = true;
	const size_t *bs ;
	
	DataStatus* ds = DataStatus::getInstance();
	//Special case before there is any data...
	if (!ds->getCurrentMetadata()){
		for (i = 0; i<3; i++) {
			min_dim[i] = 0;
			max_dim[i] = (1024>>numxforms) -1;
			min_bdim[i] = 0;
			max_bdim[i] = (32 >>numxforms) -1;
		}
		return false;
	}
	//Check that the data exists for this timestep and refinement:
	for (i = 0; i<numVars; i++){
		if(DataStatus::getInstance()->maxXFormPresent(varNums[i],(int)timestep) < numxforms)
			return false;
	}
	double userMinCoords[3];
	double userMaxCoords[3];
	for (i = 0; i<3; i++){
		userMinCoords[i] = (double)regionMin[i];
		userMaxCoords[i] = (double)regionMax[i];
	}
	const VDFIOBase* myReader = ds->getRegionReader();

	if (ds->dataIsLayered()){
		((LayeredIO*)myReader)->SetGridHeight(fullHeight);
	}
	//Do mapping to voxel coords
	myReader->MapUserToVox(timestep, userMinCoords, min_dim, numxforms);
	myReader->MapUserToVox(timestep, userMaxCoords, max_dim, numxforms);

	for(i = 0; i< 3; i++){
		//Make sure slab has nonzero thickness (this can only
		//be a problem while the mouse is pressed):
		//
		if (min_dim[i] >= max_dim[i]){
			if (max_dim[i] < 1){
				max_dim[i] = 1;
				min_dim[i] = 0;
			}
			else min_dim[i] = max_dim[i] - 1;
		}
	}
	//Now intersect with available bounds based on variables:
	size_t temp_min[3], temp_max[3];
	for (int varIndex = 0; varIndex < numVars; varIndex++){
		const string varName = ds->getVariableName(varNums[varIndex]);
		int rc = ((DataMgr*)ds->getDataMgr())->GetValidRegion(timestep, varName.c_str(),numxforms, temp_min, temp_max, fullHeight);
		if (rc < 0) retVal = false;
		else for (i = 0; i< 3; i++){
			if (min_dim[i] < temp_min[i]) min_dim[i] = temp_min[i];
			if (max_dim[i] > temp_max[i]) max_dim[i] = temp_max[i];
			//Again check for validity:
			if (min_dim[i] > max_dim[i]) retVal = false;
		}
	}
	//calc block dims
	bs = ds->getCurrentMetadata()->GetBlockSize();
	for (i = 0; i<3; i++){	
		min_bdim[i] = min_dim[i] / bs[i];
		max_bdim[i] = max_dim[i] / bs[i];
	}
	//If bounds are needed, calculate them:
	if (regMax && regMin){
		//Do mapping to voxel coords
		myReader->MapVoxToUser(timestep, min_dim, regMin, numxforms);
		myReader->MapVoxToUser(timestep, max_dim, regMax, numxforms);
	}
	
	return retVal;
}

void RegionParams::
getRegionVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3]){
	int i;
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getCurrentMetadata()){
		for (i = 0; i<3; i++) {
			min_dim[i] = 0;
			max_dim[i] = (1024>>numxforms) -1;
			min_bdim[i] = 0;
			max_bdim[i] = (32 >>numxforms) -1;
		}
		return;
	}
	double userMinCoords[3];
	double userMaxCoords[3];
	for (i = 0; i<3; i++){
		userMinCoords[i] = (double)regionMin[i];
		userMaxCoords[i] = (double)regionMax[i];
	}
	const VDFIOBase* myReader = ds->getRegionReader();
	size_t timestep = DataStatus::getInstance()->getMinTimestep();
	myReader->MapUserToBlk(timestep, userMinCoords, min_bdim, numxforms);
	myReader->MapUserToVox(timestep, userMinCoords, min_dim, numxforms);
	myReader->MapUserToBlk(timestep, userMaxCoords, max_bdim, numxforms);
	myReader->MapUserToVox(timestep, userMaxCoords, max_dim, numxforms);
	
	return;
}


bool RegionParams::
elementStartHandler(ExpatParseMgr* pm, int /* depth*/ , std::string& tagString, const char **attrs){
	if (StrCmpNoCase(tagString, _regionParamsTag) == 0) {
		//If it's a region tag, save 4 attributes (2 are from Params class)
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
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				if (value == "true") setLocal(true); else setLocal(false);
			}
			/*  No longer support these attributes:
			else if (StrCmpNoCase(attribName, _maxSizeAttr) == 0) {
				ist >> maxSize;
			}
			else if (StrCmpNoCase(attribName, _numTransAttr) == 0) {
				ist >> numTrans;
			}
			*/
			else if (StrCmpNoCase(attribName, _fullHeightAttr) == 0) {
				ist >> fullHeight;
			}
			else return false;
		}
		return true;
	}
	//Prepare for region min/max
	else if ((StrCmpNoCase(tagString, _regionMinTag) == 0) ||
		(StrCmpNoCase(tagString, _regionMaxTag) == 0) ) {
		//Should have a double type attribute
		string attribName = *attrs;
		attrs++;
		string value = *attrs;

		ExpatStackElement *state = pm->getStateStackTop();
		
		state->has_data = 1;
		if (StrCmpNoCase(attribName, _typeAttr) != 0) {
			pm->parseError("Invalid attribute : %s", attribName.c_str());
			return false;
		}
		if (StrCmpNoCase(value, _doubleType) != 0) {
			pm->parseError("Invalid type : %s", value.c_str());
			return false;
		}
		state->data_type = value;
		return true;  
	}
	else return false;
}
bool RegionParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	if (StrCmpNoCase(tag, _regionParamsTag) == 0) {
		//If this is a regionparams, need to
		//pop the parse stack.  
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} 
	else if (StrCmpNoCase(tag, _regionMinTag) == 0){
		vector<double> bound = pm->getDoubleData();
		regionMin[0] = bound[0];
		regionMin[1] = bound[1];
		regionMin[2] = bound[2];
		return true;
	} else if (StrCmpNoCase(tag, _regionMaxTag) == 0){
		vector<double> bound = pm->getDoubleData();
		regionMax[0] = bound[0];
		regionMax[1] = bound[1];
		regionMax[2] = bound[2];
		return true;
	} 
	// no longer support these...
	else if (StrCmpNoCase(tag, _regionCenterTag) == 0){
		//vector<long> posn = pm->getLongData();
		//centerPosition[0] = posn[0];
		//centerPosition[1] = posn[1];
		//centerPosition[2] = posn[2];
		return true;
	} else if (StrCmpNoCase(tag, _regionSizeTag) == 0){
		//vector<long> sz = pm->getLongData();
		//regionSize[0] = sz[0];
		//regionSize[1] = sz[1];
		//regionSize[2] = sz[2];
		return true;
	}
	
	else {
		pm->parseError("Unrecognized end tag in RegionParams %s",tag.c_str());
		return false;  
	}
}
XmlNode* RegionParams::
buildNode(){
	//Construct the region node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	oss << (long)fullHeight;
	attrs[_fullHeightAttr] = oss.str();

	oss.str(empty);
	if (local)
		oss << "true";
	else 
		oss << "false";
	attrs[_localAttr] = oss.str();

	XmlNode* regionNode = new XmlNode(_regionParamsTag, attrs, 2);

	//Now add children:  
	
	vector<double> bounds;
	int i;
	bounds.clear();
	for (i = 0; i< 3; i++){
		bounds.push_back((double) regionMin[i]);
	}
	regionNode->SetElementDouble(_regionMinTag,bounds);
	bounds.clear();
	for (i = 0; i< 3; i++){
		bounds.push_back((double) regionMax[i]);
	}
	regionNode->SetElementDouble(_regionMaxTag,bounds);
		
	return regionNode;
}
//Static method to find how many megabytes are needed for a dataset.
int RegionParams::getMBStorageNeeded(const float* boxMin, const float* boxMax, int refLevel){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getCurrentMetadata()) return 0;
	
	int bs = *(ds->getCurrentMetadata()->GetBlockSize());
	
	size_t min_bdim[3], max_bdim[3];
	double userMinCoords[3];
	double userMaxCoords[3];
	for (int i = 0; i<3; i++){
		userMinCoords[i] = (double)boxMin[i];
		userMaxCoords[i] = (double)boxMax[i];
	}
	const VDFIOBase* myReader = ds->getRegionReader();
	if (!myReader) return 0;
	size_t timestep = DataStatus::getInstance()->getMinTimestep();
	myReader->MapUserToBlk(timestep, userMinCoords, min_bdim, refLevel);
	myReader->MapUserToBlk(timestep, userMaxCoords, max_bdim, refLevel);
	
	
	float numVoxels = (float)(bs*bs*bs*(max_bdim[0]-min_bdim[0]+1)*(max_bdim[1]-min_bdim[1]+1)*(max_bdim[2]-min_bdim[2]+1));
	
	//divide by 1 million for megabytes, mult by 4 for 4 bytes per voxel:
	float fullMB = numVoxels/262144.f;
	return (int)fullMB;
}
 
