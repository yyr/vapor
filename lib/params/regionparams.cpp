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
#include "vapor/errorcodes.h"

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
#include "vapor/DataMgrLayered.h"
#include "vapor/XmlNode.h"
#include "ParamNode.h"
//#include "glutil.h"


using namespace VAPoR;

const string RegionParams::_regionCenterTag = "RegionCenter";
const string RegionParams::_shortName = "Region";
const string RegionParams::_regionSizeTag = "RegionSize";
const string RegionParams::_maxSizeAttr = "MaxSizeSlider";
const string RegionParams::_numTransAttr = "NumTrans";
const string RegionParams::_timestepAttr = "TimeStep";
const string RegionParams::_extentsAttr = "Extents";
const string RegionParams::_regionMinTag = "RegionMin";
const string RegionParams::_regionMaxTag = "RegionMax";
const string RegionParams::_regionAtTimeTag = "RegionAtTime";
const string RegionParams::_regionListTag = "RegionList";

const string RegionParams::_fullHeightAttr = "FullGridHeight";
size_t RegionParams::fullHeight = 0;

RegionParams::RegionParams(int winnum): Params(winnum, Params::_regionParamsTag){
	
	
	restart();
}
Params* RegionParams::
deepCopy(ParamNode* ){
	//Just make a shallow copy, but duplicate the extentsMap
	//
	RegionParams* p = new RegionParams(*this);
	p->extentsMap.clear();
	if (extentsMap.size() > 0){
		//Iterate over extents map, copy all the extents
		map <int, float*> :: iterator eIter;
		for ( eIter = extentsMap.begin( ) ; eIter != extentsMap.end() ; eIter++ ){
			float *exts = eIter->second;
			int ts = eIter->first;
			float *extsCopy = new float[6];
			for (int i = 0; i< 6; i++){
				extsCopy[i] = exts[i];
			}
			p->extentsMap[ts] = extsCopy;
		}
		
	}
	return (Params*)(p);
}

RegionParams::~RegionParams(){
	clearRegionsMap();
}






//See if the number of trans is ok.  If not, return an OK value
int RegionParams::
validateNumTrans(int n, int timestep){
	//if we have a dataMgr, see if we can really handle this new numtrans:
	if (!DataStatus::getInstance()) return n;
	
	
	size_t min_dim[3], max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	getRegionVoxelCoords(n,min_dim,max_dim,min_bdim,max_bdim, timestep);
	
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
			defaultRegionExtents[i] = fullDataExtents[i];
			defaultRegionExtents[i+3] = fullDataExtents[i+3];
		} else {
			defaultRegionExtents[i] = 0.f;
			defaultRegionExtents[i+3] = 1.f;
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
		for (i = 0; i< 6; i++) {
			defaultRegionExtents[i] = extents[i];
		}
		clearRegionsMap();
		if (isLayered) setFullGridHeight(4*DataStatus::getInstance()->getFullDataSize(2));
		else fullHeight = 0;
	} else {
		//Just force them to fit in current volume 
		for (i = 0; i< 3; i++) {
			if (defaultRegionExtents[i] > defaultRegionExtents[i+3]) 
				defaultRegionExtents[i+3] = defaultRegionExtents[i];
			if (defaultRegionExtents[i] > extents[i+3])
				defaultRegionExtents[i] = extents[i+3];
			if (defaultRegionExtents[i] < extents[i])
				defaultRegionExtents[i] = extents[i];
			if (defaultRegionExtents[i+3] > extents[i+3])
				defaultRegionExtents[i+3] = extents[i+3];
			if (defaultRegionExtents[i+3] < extents[i])
				defaultRegionExtents[i+3] = extents[i];
		}
		if (extentsMap.size() > 0){
			//Iterate over extents map, make every region fit:
			map <int, float*> :: iterator eIter;
			for ( eIter = extentsMap.begin( ) ; eIter != extentsMap.end() ; eIter++ ){
				float *exts = eIter->second;
				for (int i = 0; i< 3; i++){
					if (exts[i] < extents[i]) exts[i] = extents[i];
					if (exts[i+3] > extents[i+3]) exts[i+3] = extents[i+3];
				}
			}

		}
		//If layered data is being read into a session that was not
		//set for layered data, then the fullheight may need to be
		//set to default
		if (isLayered && fullHeight == 0)
			setFullGridHeight(4*DataStatus::getInstance()->getFullDataSize(2));
		else if(isLayered) setFullGridHeight(fullHeight);
	}
	
	return true;	
}

void RegionParams::setRegionMin(int coord, float minval, int timestep, bool checkMax){
	DataStatus* ds = DataStatus::getInstance();
	const float* fullDataExtents;
	if (ds && ds->getDataMgr()){
		fullDataExtents = ds->getExtents();
		if (minval < fullDataExtents[coord]) minval = fullDataExtents[coord];
		if (minval > fullDataExtents[coord+3]) minval = fullDataExtents[coord+3];
	}
	float * exts = getRegionExtents(timestep);
	if (checkMax) {if (minval > exts[coord+3]) minval = exts[coord+3];}
	exts[coord] = minval;
}
void RegionParams::setRegionMax(int coord, float maxval, int timestep, bool checkMin){
	DataStatus* ds = DataStatus::getInstance();
	const float* fullDataExtents;
	if (ds && ds->getDataMgr()){
		fullDataExtents = DataStatus::getInstance()->getExtents();
		if (maxval < fullDataExtents[coord]) maxval = fullDataExtents[coord];
		if (maxval > fullDataExtents[coord+3]) maxval = fullDataExtents[coord+3];
	}
	float* exts = getRegionExtents(timestep);
	if (checkMin){if (maxval < exts[coord]) maxval = exts[coord];}
	exts[coord+3] = maxval;
}


//static method to do conversion to box coords (probably based on available
//coords, that may be smaller than region coords)
//Then puts into unit cube, for use by volume rendering
//
void RegionParams::
convertToStretchedBoxExtentsInCube(int refLevel, const size_t min_dim[3], const size_t max_dim[3], float extents[6]){
	double fullExtents[6];
	double subExtents[6];
	DataStatus* ds = DataStatus::getInstance();
	const size_t fullMin[3] = {0,0,0};
	size_t fullMax[3];
	
	for (int i = 0; i<3; i++) fullMax[i] = (int)DataStatus::getInstance()->getFullSizeAtLevel(refLevel,i) - 1;

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
int RegionParams::
getAvailableVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3], size_t timestep, const int* varNums, int numVars,
		double regMin[3], double regMax[3]){
	//First determine the bounds specified in this RegionParams
	int i;
	
	const size_t *bs ;
	
	DataStatus* ds = DataStatus::getInstance();
	//Special case before there is any data...
	if (!ds->getDataMgr()){
		for (i = 0; i<3; i++) {
			min_dim[i] = 0;
			max_dim[i] = (1024>>numxforms) -1;
			min_bdim[i] = 0;
			max_bdim[i] = (32 >>numxforms) -1;
		}
		return -1;
	}
	//Check that the data exists for this timestep and refinement:
	int minRefLevel = numxforms;
	for (i = 0; i<numVars; i++){
		minRefLevel = Min(ds->maxXFormPresent(varNums[i],(int)timestep), minRefLevel);
		//Test if it's acceptable, exit if not:
		if (minRefLevel < 0 || (minRefLevel < numxforms && !ds->useLowerRefinementLevel())){
			return -1;
		}
	}

	double userMinCoords[3];
	double userMaxCoords[3];
	float* regExts = getRegionExtents(timestep);
	for (i = 0; i<3; i++){
		userMinCoords[i] = (double)regExts[i];;
		userMaxCoords[i] = (double)regExts[i+3];
	}
	const DataMgr* dataMgr = ds->getDataMgr();

	if (ds->dataIsLayered()){
		setFullGridHeight(fullHeight);
	}
	//Do mapping to voxel coords
	dataMgr->MapUserToVox((size_t)-1, userMinCoords, min_dim, minRefLevel);
	dataMgr->MapUserToVox((size_t)-1, userMaxCoords, max_dim, minRefLevel);

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
		int rc = getValidRegion(timestep, varName.c_str(),minRefLevel, temp_min, temp_max);
		if (rc < 0) {
			//Tell the datastatus that the data isn't really there at minRefLevel:
			ds->setDataMissing(timestep, minRefLevel, varNums[varIndex]);
			if (ds->warnIfDataMissing()){
				SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"VDC access error: \nvariable %s not accessible at timestep %d.",
					varName.c_str(), timestep);
			}
			return -1;
		}
		else for (i = 0; i< 3; i++){
			if (min_dim[i] < temp_min[i]) min_dim[i] = temp_min[i];
			if (max_dim[i] > temp_max[i]) max_dim[i] = temp_max[i];
			//Again check for validity:
			if (min_dim[i] > max_dim[i]) minRefLevel = -1;
		}
	}
	//calc block dims
	bs = ds->getDataMgr()->GetBlockSize();
	for (i = 0; i<3; i++){	
		min_bdim[i] = min_dim[i] / bs[i];
		max_bdim[i] = max_dim[i] / bs[i];
	}
	//If bounds are needed, calculate them:
	if (regMax && regMin){
		//Do mapping to voxel coords
		dataMgr->MapVoxToUser((size_t)-1, min_dim, regMin, minRefLevel);
		dataMgr->MapVoxToUser((size_t)-1, max_dim, regMax, minRefLevel);
	}
	
	return minRefLevel;
}
int RegionParams::
shrinkToAvailableVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3], size_t timestep, const int* varNums, int numVars,
		double regMin[3], double regMax[3], bool twoDim){
	
	int i;
	
	const size_t *bs ;
	
	DataStatus* ds = DataStatus::getInstance();
	//Special case before there is any data...
	if (!ds->getDataMgr()){
		for (i = 0; i<3; i++) {
			min_dim[i] = 0;
			max_dim[i] = (1024>>numxforms) -1;
			min_bdim[i] = 0;
			max_bdim[i] = (32 >>numxforms) -1;
		}
		return -1;
	}
	//Check that the data exists for this timestep and refinement:
	int minRefLevel = numxforms;
	for (i = 0; i<numVars; i++){
		if (twoDim)
			minRefLevel = Min(ds->maxXFormPresent2D(varNums[i],(int)timestep), minRefLevel);
		else
			minRefLevel = Min(ds->maxXFormPresent(varNums[i],(int)timestep), minRefLevel);
	
		//Test if it's acceptable, exit if not:
		if (minRefLevel < 0 || (minRefLevel < numxforms && !ds->useLowerRefinementLevel())){
			if (ds->warnIfDataMissing()){
				char* vname;
				if (twoDim)
					vname = (char*)ds->getVariableName2D(varNums[i]).c_str();
				else
					vname = (char*)ds->getVariableName(varNums[i]).c_str();
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Data inaccessible\nfor variable %s at timestep %d.",
					vname, timestep);
			}
			return -1;
		}
	}

	
	const DataMgr* dataMgr = ds->getDataMgr();

	if (ds->dataIsLayered()){
		setFullGridHeight(fullHeight);
	}
	//Do mapping to voxel coords
	dataMgr->MapUserToVox((size_t)-1, regMin, min_dim, minRefLevel);
	dataMgr->MapUserToVox((size_t)-1, regMax, max_dim, minRefLevel);
	if (!twoDim){
		for(i = 0; i< 3; i++){
			//Make sure 3D slab has nonzero thickness (this can only
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
	}
	//Now intersect with available bounds based on variables:
	size_t temp_min[3], temp_max[3];
	for (int varIndex = 0; varIndex < numVars; varIndex++){
		string varName;
		if (twoDim)
			varName = ds->getVariableName2D(varNums[varIndex]);
		else 
			varName = ds->getVariableName(varNums[varIndex]);
		int rc = getValidRegion(timestep, varName.c_str(),minRefLevel, temp_min, temp_max);
		if (rc < 0) {
			//Tell the datastatus that the data isn't really there at minRefLevel:
			if (twoDim)
				ds->setDataMissing2D(timestep, minRefLevel, varNums[varIndex]);
			else
				ds->setDataMissing(timestep, minRefLevel, varNums[varIndex]);
			if (ds->warnIfDataMissing()){
				SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"Data inaccessable for variable %s\nat current timestep.\n %s",
					varName.c_str(),
					"This message can be silenced \nin the User Preferences Panel.");
			}
			return -1;
		}
		else for (i = 0; i< 3; i++){
			if (min_dim[i] < temp_min[i]) min_dim[i] = temp_min[i];
			if (max_dim[i] > temp_max[i]) max_dim[i] = temp_max[i];
			//Again check for validity:
			if (min_dim[i] > max_dim[i]) minRefLevel = -1;
		}
	}
	//calc block dims
	bs = ds->getDataMgr()->GetBlockSize();
	for (i = 0; i<3; i++){	
		min_bdim[i] = min_dim[i] / bs[i];
		max_bdim[i] = max_dim[i] / bs[i];
	}
	//Calculate new bounds:
	
	dataMgr->MapVoxToUser((size_t)-1, min_dim, regMin, minRefLevel);
	dataMgr->MapVoxToUser((size_t)-1, max_dim, regMax, minRefLevel);
	
	return minRefLevel;
}
void RegionParams::
getRegionVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3], int timestep){
	int i;
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()){
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
	float* regExts = getRegionExtents(timestep);
	for (i = 0; i<3; i++){
		userMinCoords[i] = (double)regExts[i];
		userMaxCoords[i] = (double)regExts[i+3];
	}
	const DataMgr* dataMgr = ds->getDataMgr();

	dataMgr->MapUserToBlk((size_t)-1, userMinCoords, min_bdim, numxforms);
	dataMgr->MapUserToVox((size_t)-1, userMinCoords, min_dim, numxforms);
	dataMgr->MapUserToBlk((size_t)-1, userMaxCoords, max_bdim, numxforms);
	dataMgr->MapUserToVox((size_t)-1, userMaxCoords, max_dim, numxforms);

	
	return;
}


bool RegionParams::
elementStartHandler(ExpatParseMgr* pm, int /* depth*/ , std::string& tagString, const char **attrs){
	if (StrCmpNoCase(tagString, _regionParamsTag) == 0) {
		clearRegionsMap();
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
	else if (StrCmpNoCase(tagString, _regionAtTimeTag) == 0) {
		//Attributes are timestep and extents
		int tstep = -1;
		float* exts = new float[6];
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
		
			if (StrCmpNoCase(attribName, _timestepAttr) == 0) {
				ist >> tstep;
			}
			else if (StrCmpNoCase(attribName, _extentsAttr) == 0) {
				
				ist >> exts[0];ist >> exts[1];ist >> exts[2];ist >> exts[3];ist >> exts[4];ist >> exts[5];
			}
			
		}
		extentsMap[tstep] = exts;
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
		defaultRegionExtents[0] = bound[0];
		defaultRegionExtents[1] = bound[1];
		defaultRegionExtents[2] = bound[2];
		return true;
	} else if (StrCmpNoCase(tag, _regionMaxTag) == 0){
		vector<double> bound = pm->getDoubleData();
		defaultRegionExtents[3] = bound[0];
		defaultRegionExtents[4] = bound[1];
		defaultRegionExtents[5] = bound[2];
		return true;
	} else if (StrCmpNoCase(tag, _regionAtTimeTag) == 0){
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
ParamNode* RegionParams::
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
	int numchildren = 2 + extentsMap.size();
	ParamNode* regionNode = new ParamNode(_regionParamsTag, attrs, numchildren);

	//Now add children:  
	
	vector<double> bounds;
	int i;
	bounds.clear();
	for (i = 0; i< 3; i++){
		bounds.push_back((double) defaultRegionExtents[i]);
	}
	regionNode->SetElementDouble(_regionMinTag,bounds);
	bounds.clear();
	for (i = 0; i< 3; i++){
		bounds.push_back((double) defaultRegionExtents[i+3]);
	}
	regionNode->SetElementDouble(_regionMaxTag,bounds);

	if (extentsMap.size() > 0){
		

		//Add a node for each region in the list, having the extents as an attribute:
		//Iterate over the extents map:
		map <int, float*> :: iterator eIter;
		for ( eIter = extentsMap.begin( ) ; eIter != extentsMap.end() ; eIter++ ){
			
			attrs.clear();
			oss.str(empty);
			float *exts = eIter->second;
			int ts = eIter->first;
			oss << (long) ts;
			attrs[_timestepAttr] = oss.str();
			oss.str(empty);
			oss << (double) exts[0]<< " " <<
				(double) exts[1]<< " " <<
				(double) exts[2]<< " " <<
				(double) exts[3]<< " " <<
				(double) exts[4]<< " " <<
				(double) exts[5];
			attrs[_extentsAttr] = oss.str();
			
			ParamNode* regionTimeNode = new ParamNode(_regionAtTimeTag, attrs, 0);
			regionNode->AddChild(regionTimeNode);
		}
		//regionNode->AddChild(regionListNode);
	}

	return regionNode;
}
//Static method to find how many megabytes are needed for a dataset.
int RegionParams::getMBStorageNeeded(const float* boxMin, const float* boxMax, int refLevel){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return 0;
	
	int bs = *(ds->getDataMgr()->GetBlockSize());
	
	size_t min_bdim[3], max_bdim[3];
	double userMinCoords[3];
	double userMaxCoords[3];
	for (int i = 0; i<3; i++){
		userMinCoords[i] = (double)boxMin[i];
		userMaxCoords[i] = (double)boxMax[i];
	}
	const DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return 0;
	size_t timestep = DataStatus::getInstance()->getMinTimestep();
	dataMgr->MapUserToBlk(timestep, userMinCoords, min_bdim, refLevel);
	dataMgr->MapUserToBlk(timestep, userMaxCoords, max_bdim, refLevel);
	
	
	float numVoxels = (float)(bs*bs*bs*(max_bdim[0]-min_bdim[0]+1)*(max_bdim[1]-min_bdim[1]+1)*(max_bdim[2]-min_bdim[2]+1));
	
	//divide by 1 million for megabytes, mult by 4 for 4 bytes per voxel:
	float fullMB = numVoxels/262144.f;
	return (int)fullMB;
}
 
//calculate the 3D variable, at a specific point.
//Returns the OUT_OF_BOUNDS flag if point is not in current region
//


float RegionParams::
calcCurrentValue(RenderParams* renParams, int sessionVarNum, const float point[3], int numRefinements, int timeStep){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds || !ds->getDataMgr()) return OUT_OF_BOUNDS;

	//Get the data dimensions (at current resolution):
	int voxCoords[3];
	const size_t* bs = ds->getDataMgr()->GetBlockSize();
	const char* varname = ds->getVariableName(sessionVarNum).c_str();
	double regMin[3], regMax[3];
	size_t min_dim[3],max_dim[3], min_bdim[3], max_bdim[3],blkmin[3], blkmax[3];
	int availRefLevel = getAvailableVoxelCoords(numRefinements, min_dim, max_dim, min_bdim, max_bdim, timeStep, &sessionVarNum, 1, regMin, regMax);
	if (availRefLevel < 0) {
		renParams->setBypass(timeStep);
		if (ds->warnIfDataMissing()){
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, 
				"Data unavailable for variable %s\nat refinement level %d",varname, numRefinements);
		}
		return OUT_OF_BOUNDS;
	}
	//Get the closest voxel coords of the point, and the block coords that
	//contain it.
	for (int i = 0; i<3; i++) {
		if (point[i] < regMin[i]) return OUT_OF_BOUNDS;
		if (point[i] > regMax[i]) return OUT_OF_BOUNDS;
		voxCoords[i] = min_dim[i]+(int)((float)(max_dim[i] - min_dim[i])*(point[i] - regMin[i])/(regMax[i] - regMin[i]) + 0.5);
		
		blkmin[i] = voxCoords[i]/bs[i];
		blkmax[i] = blkmin[i];
		//Reset the voxel coords to index within the block:
		voxCoords[i] -= blkmin[i]*bs[i];
	}
	// Obtain the region:
	float *reg = renParams->getContainingVolume(blkmin,blkmax, availRefLevel, sessionVarNum, timeStep, false);
	if (!reg) return OUT_OF_BOUNDS;
	
	//find the coords within this block:
	int xyzcoord = voxCoords[0] + bs[0]*voxCoords[1] + bs[0]*bs[1]*voxCoords[2];

	float val = reg[xyzcoord];
	return val;
}
void RegionParams::
setFullGridHeight(size_t val){
	DataStatus* ds = DataStatus::getInstance();
	DataMgr *dataMgr = ds->getDataMgr();

	if (ds->dataIsLayered()){
		DataMgrLayered  *data_mgr_layered = dynamic_cast<DataMgrLayered*>(dataMgr);
		assert(data_mgr_layered != NULL);

		
		size_t currentHeight = data_mgr_layered->GetGridHeight();
		if (currentHeight != val){			
			data_mgr_layered->SetGridHeight(val);
			//purge cache:
			dataMgr->Clear();
		}
		fullHeight = val;
		return;
	}
	assert(val == 0);
	fullHeight = 0;

}
int RegionParams::getValidRegion(size_t timestep, const char* varname, int minRefLevel, size_t min_coord[3], size_t max_coord[3]){
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dm = ds->getDataMgr();
	if (!dm) return -1;
	int scriptid = ds->getDerivedScriptId(varname);
	int rc;
	if (scriptid >= 0){
		size_t temp_min[3], temp_max[3]; 
		size_t curr_min[3] = {1000000000,1000000000,1000000000}, curr_max[3] = {0,0,0};
		const vector<string> invars3d = ds->getDerived3DInputVars(scriptid);
		const vector<string> invars2d = ds->getDerived2DInputVars(scriptid);
		//If there are no 3d inputs, make the 3D min/max be the full data size
		if (invars3d.size() == 0){
			curr_min[2] = 0;
			curr_max[2] = ds->getFullSizeAtLevel(minRefLevel,2); 
		}
		//Intersect the valid regions of all the input variables:
		for (int i = 0; i<invars3d.size(); i++){
			rc = dm->GetValidRegion(timestep, invars3d[i].c_str(), minRefLevel, temp_min, temp_max);
			if (rc < 0) return rc;
			for (int j = 0; j<3; j++){
				if (temp_min[j] < curr_min[j]) curr_min[j] = temp_min[j];
				if (temp_max[j] > curr_max[j]) curr_max[j] = temp_max[j];
			}
		}
		for (int i = 0; i<invars2d.size(); i++){
			rc = dm->GetValidRegion(timestep, invars2d[i].c_str(), minRefLevel, temp_min, temp_max);
			if (rc < 0) return rc;
			for (int j = 0; j<2; j++){
				if (temp_min[j] < curr_min[j]) curr_min[j] = temp_min[j];
				if (temp_max[j] > curr_max[j]) curr_max[j] = temp_max[j];
			}
		}
		for (int j = 0; j<3; j++){
			min_coord[j] = curr_min[j];
			max_coord[j] = curr_max[j];
		}

	} else {
		rc = dm->GetValidRegion(timestep, varname, minRefLevel, min_coord, max_coord);
	}
	if (!ds->dataIsLayered()) return rc;
	if (rc < 0) return rc;
	int maxRefLevel = ds->getNumTransforms();
	min_coord[2] = 0;
	max_coord[2] = (fullHeight >> (maxRefLevel -minRefLevel)) -1;
	return rc;
}
float* RegionParams::getRegionExtents(int timestep){
	if (extentsMap.size() == 0) return defaultRegionExtents;
	map <int, float*> :: const_iterator extsIter;
	extsIter = extentsMap.find( timestep);
	if ( extsIter == extentsMap.end()) return defaultRegionExtents;
	return  extsIter->second;
}

void RegionParams::clearRegionsMap(){
	if (extentsMap.size() > 0){
		//Iterate over extents map, make every region fit:
		map <int, float*> :: iterator eIter;
		for ( eIter = extentsMap.begin( ) ; eIter != extentsMap.end() ; eIter++ ){
			float *exts = eIter->second;
			delete exts;
		}
		extentsMap.clear();
	}
}
