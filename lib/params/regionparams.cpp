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
#include <vapor/errorcodes.h>

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
#include <vapor/DataMgr.h>
#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>


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
	
	myBox = 0;
	restart();
}
Params* RegionParams::
deepCopy(ParamNode* ){
	//Just make a shallow copy, but duplicate the Box
	//
	RegionParams* p = new RegionParams(*this);
	ParamNode* pNode = new ParamNode(*(myBox->GetRootNode()));
	p->myBox = (Box*)myBox->deepCopy(pNode);
	
	return (Params*)(p);
}

RegionParams::~RegionParams(){
	clearRegionsMap();
	if (myBox) delete myBox;
}






//See if the number of trans is ok.  If not, return an OK value
int RegionParams::
validateNumTrans(int n, int timestep){
	//if we have a dataMgr, see if we can really handle this new numtrans:
	DataStatus* ds = DataStatus::getInstance();
	bool canUseLower = ds->useLowerAccuracy();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return n;
	int testRefLevel = n;
	if (dataMgr->GetGridType()== "layered"){
		int elevVarNum = ds->getSessionVariableNum3D("ELEVATION");
		if (elevVarNum < 0) return n;
		int availRefLevel = ds->maxXFormPresent3D(elevVarNum, timestep);
		if (availRefLevel < 0) return n;
		if (availRefLevel < testRefLevel && canUseLower) testRefLevel = availRefLevel;
	}

	size_t min_dim[3], max_dim[3];
	getRegionVoxelCoords(testRefLevel,0,min_dim,max_dim, timestep);
	//calc num voxels
	size_t newFullMB = (max_dim[0]-min_dim[0]+1)*(max_dim[1]-min_dim[1]+1)*(max_dim[2]-min_dim[2]+1);
	//right shift by 20 for megavoxels
	newFullMB >>= 20;
	//Multiply by 6 for 6 bytes per voxel
	newFullMB *= 6;
	bool reduced = false;
	while (newFullMB >= DataStatus::getInstance()->getCacheMB()){
		//find  and return a legitimate value.  Each time we increase n by 1,
		//we decrease the size needed by 8
		testRefLevel--;
		newFullMB >>= 3;
		reduced = true;
	}	
	if(reduced) return testRefLevel; 
	else return n;
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
	const float* fullSizes = 0;
	if (ds) fullSizes = DataStatus::getInstance()->getFullSizes();
	
	if (!myBox){
		myBox = new Box();
	}
	float defaultRegionExtents[6];
	for (int i = 0; i< 3; i++){
		if (ds){
			defaultRegionExtents[i] = 0.;
			defaultRegionExtents[i+3] = fullSizes[i];
		} else {
			defaultRegionExtents[i] = 0.f;
			defaultRegionExtents[i+3] = 1.f;
		}

	}
	vector<double> regexts;
	for (int i = 0; i<6; i++) regexts.push_back(defaultRegionExtents[i]);
	myBox->SetLocalExtents(regexts);
	
}
//Reinitialize region settings, session has changed
//Need to force the regionMin, regionMax to be OK.
bool RegionParams::
reinit(bool doOverride){
	int i;
	
	const float* extents = DataStatus::getInstance()->getLocalExtents();
	
	double regionExtents[6];
	vector<double> exts(3,0.0);
	if (doOverride) {
		for (i = 0; i< 3; i++) {
			exts.push_back( extents[i+3]-extents[i]);
		}
		clearRegionsMap();
		myBox->SetLocalExtents(exts);
		myBox->Trim();
	} else {
		//ensure all the local time-extents to be valid
		const vector<long>& times = myBox->GetTimes();
		for (int timenum = 0; timenum< times.size(); timenum++){
			int currTime = times[timenum];
			myBox->GetLocalExtents(regionExtents,currTime);
			if (DataStatus::pre22Session()){
				float * offset = DataStatus::getPre22Offset();
				//In old session files, the coordinate of box extents were not 0-based
				for (int i = 0; i<3; i++) {
					regionExtents[i] -= offset[i];
					regionExtents[i+3] -= offset[i];
				}
			}
			//force them to fit in current volume 
			for (i = 0; i< 3; i++) {
				
				if (regionExtents[i] > extents[i+3]-extents[i])
					regionExtents[i] = extents[i+3]-extents[i];
				if (regionExtents[i] < 0.)
					regionExtents[i] = 0.;
				if (regionExtents[i+3] > extents[i+3]-extents[i])
					regionExtents[i+3] = extents[i+3]-extents[i];
				if (regionExtents[i+3] < 0.)
					regionExtents[i+3] = 0.;
				if (regionExtents[i] > regionExtents[i+3]) 
					regionExtents[i+3] = regionExtents[i];
			}
			exts.clear();
			for (int j = 0; j< 6; j++) exts.push_back(regionExtents[j]);
			myBox->SetLocalExtents(exts,currTime);
		}	
	}
	
	return true;	
}

void RegionParams::setLocalRegionMin(int coord, float minval, int timestep, bool checkMax){
	DataStatus* ds = DataStatus::getInstance();
	const float* fullSizes;
	if (ds && ds->getDataMgr()){
		fullSizes = ds->getFullSizes();
		if (minval < 0.) minval = 0.;
		if (minval > fullSizes[coord]) minval = fullSizes[coord];
	}
	double exts[6];
	myBox->GetLocalExtents(exts, timestep);
	if (checkMax) {if (minval > exts[coord+3]) minval = exts[coord+3];}
	exts[coord] = minval;
	myBox->SetLocalExtents(exts,timestep);
}
void RegionParams::setLocalRegionMax(int coord, float maxval, int timestep, bool checkMin){
	DataStatus* ds = DataStatus::getInstance();
	const float* fullSizes;
	if (ds && ds->getDataMgr()){
		fullSizes = ds->getFullSizes();
		if (maxval < 0.) maxval = 0.;
		if (maxval > fullSizes[coord]) maxval = fullSizes[coord];
	}
	double exts[6];
	myBox->GetLocalExtents(exts, timestep);
	
	if (checkMin){if (maxval < exts[coord]) maxval = exts[coord];}
	exts[coord+3] = maxval;
	myBox->SetLocalExtents(exts, timestep);
}


int RegionParams::
getAvailableVoxelCoords(int numxforms, int lod, size_t min_dim[3], size_t max_dim[3], 
		size_t timestep, const int* varNums, int numVars,
		double regMin[3], double regMax[3]){
	//First determine the bounds specified in this RegionParams
	int i;
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return -1;
	//Special case before there is any data...
	if (!ds->getDataMgr()){
		for (i = 0; i<3; i++) {
			min_dim[i] = 0;
			max_dim[i] = (1024>>numxforms) -1;
		}
		return -1;
	}
	
	int minRefLevel = numxforms;
	int minLOD = lod;
	//check that the data exists for this timestep and refinement:
	
	for (i = 0; i<numVars; i++){
		if (varNums[i] < 0) continue; //in case of zero field component
		minRefLevel = Min(ds->maxXFormPresent3D(varNums[i],(int)timestep), minRefLevel);
		minLOD = Min(ds->maxLODPresent3D(varNums[i],(int)timestep), minLOD);
		//Test if it's acceptable, exit if not:
		if (minRefLevel < 0 || (minRefLevel < numxforms && !ds->useLowerAccuracy())){
			return -1;
		}
		if (minLOD < 0 || (minLOD < lod && !ds->useLowerAccuracy())) return -1;
	}
	
	double userMinCoords[3];
	double userMaxCoords[3];
	double regExts[6];
	GetBox()->GetLocalExtents(regExts, timestep);
	DataMgr* dataMgr = ds->getDataMgr();
	const vector<double>& usrExts = dataMgr->GetExtents(timestep);
	for (i = 0; i<3; i++){
		userMinCoords[i] = regExts[i]+usrExts[i];
		userMaxCoords[i] = regExts[i+3]+usrExts[i];
	}
	

	
	//Determine containing voxel coord box:
	dataMgr->GetEnclosingRegion(timestep, userMinCoords, userMaxCoords, min_dim, max_dim,minRefLevel,minLOD);

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
	for (int j = 0; j<3; j++){
		temp_min[j] = min_dim[j];
		temp_max[j] = max_dim[j];
	}

	for (int varIndex = 0; varIndex < numVars; varIndex++){
		if (varNums[varIndex] < 0) continue; //in case of zero field component
		const string varName = ds->getVariableName3D(varNums[varIndex]);
		int rc = getValidRegion(timestep, varName.c_str(),minRefLevel, temp_min, temp_max);
		if (rc < 0) {
			//Tell the datastatus that the data isn't really there at minRefLevel, at any LOD
			ds->setDataMissing3D(timestep, minRefLevel, 0, varNums[varIndex]);
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
	
	
	//If bounds are needed, calculate them; note that these are just corner coordinates,
	//may not be the actual region bounds with layered data.
	if (regMax && regMin){
		//Do mapping to voxel coords
		dataMgr->MapVoxToUser(timestep, min_dim, regMin, minRefLevel,lod);
		dataMgr->MapVoxToUser(timestep, max_dim, regMax, minRefLevel,lod);
	}
	
	return minRefLevel;
}
int RegionParams::PrepareCoordsForRetrieval(int numxforms, int lod, size_t timestep, const vector<string>& varnames,
		double* regMin, double* regMax, 
		size_t min_dim[3], size_t max_dim[3]) 
{
		
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return -1;
		
	//First determine the refinement level we will be using (for all the variables).
	int minRefLevel = numxforms;
	int minLOD = lod;
	for (int i = 0; i<varnames.size(); i++){
		if (varnames[i] == "" || varnames[i] == "0") continue;
		minRefLevel = Min(ds->maxXFormPresent(varnames[i],(int)timestep), minRefLevel);
		minLOD = Min(ds->maxLODPresent(varnames[i],(int)timestep), minLOD);
		//Test if it's acceptable, exit if not:
		if (minRefLevel < 0 || (minRefLevel < numxforms && !ds->useLowerAccuracy())){
			if (ds->warnIfDataMissing()){
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Data inaccessible\nfor variable %s at timestep %d.",
					varnames[i].c_str(), timestep);
			}
			return -1;
		}
		if (minLOD < 0 || (minLOD < lod && !ds->useLowerAccuracy())){
			if (ds->warnIfDataMissing()){
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Data inaccessible\nfor variable %s at timestep %d.",
					varnames[i].c_str(), timestep);
			}
			return -1;
		}
	}

	dataMgr->GetEnclosingRegion(
		timestep, regMin, regMax, min_dim, max_dim, minRefLevel, minLOD
	);
	
	//intersect with available bounds based on variables:
	size_t temp_min[3], temp_max[3];
	for (int j = 0; j<3; j++){
		temp_min[j] = min_dim[j];
		temp_max[j] = max_dim[j];
	}
	for (int i = 0; i < varnames.size(); i++){
		if (varnames[i] == "" || varnames[i] == "0") continue;
		int rc = getValidRegion(timestep, varnames[i].c_str(), minRefLevel, temp_min, temp_max);
		if (rc < 0) {
			//Tell the datastatus that the data isn't really there at minRefLevel at any LOD:
			int varnum = ds->getSessionVariableNum3D(varnames[i]);
			if (varnum >= 0)
				ds->setDataMissing3D(timestep, minRefLevel, 0, varnum);
			else {//must be 2D
				varnum = ds->getSessionVariableNum2D(varnames[i]);
				ds->setDataMissing2D(timestep, minRefLevel, 0, varnum);
			}
			if (ds->warnIfDataMissing()){
				SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"Data inaccessable for variable %s\nat current timestep.\n %s",
					varnames[i].c_str(),
					"This message can be silenced \nin the User Preferences Panel.");
			}
			return -1;
		}
		else for (int j = 0; j< 3; j++){
			if (min_dim[j] < temp_min[j]) min_dim[j] = temp_min[j];
			if (max_dim[j] > temp_max[j]) max_dim[j] = temp_max[j];
			//Again check for validity:
			if (min_dim[j] > max_dim[j]) minRefLevel = -1;
		}
	}
	
	//Calculate new bounds:
	
	dataMgr->MapVoxToUser(timestep, min_dim, regMin, minRefLevel,lod);
	dataMgr->MapVoxToUser(timestep, max_dim, regMax, minRefLevel,lod);
	
	return minRefLevel;
}

void RegionParams::
getRegionVoxelCoords(int numxforms, int lod, size_t min_dim[3], size_t max_dim[3], int timestep)
{
	int i;
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()){
		for (i = 0; i<3; i++) {
			min_dim[i] = 0;
			max_dim[i] = (1024>>numxforms) -1;
		}
		return;
	}
	double userMinCoords[3];
	double userMaxCoords[3];
	double regExts[6];
	GetBox()->GetUserExtents(regExts, timestep);
	for (i = 0; i<3; i++){
		userMinCoords[i] = regExts[i];
		userMaxCoords[i] = regExts[i+3];
	}
	DataMgr* dataMgr = ds->getDataMgr();

	dataMgr->GetEnclosingRegion((size_t)timestep, userMinCoords, userMaxCoords, min_dim, max_dim, numxforms,lod);

	return;
}


bool RegionParams::
elementStartHandler(ExpatParseMgr* pm, int depth, std::string& tagString, const char **attrs){
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
		double exts[6];
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
		insertTime(tstep);
		myBox->SetLocalExtents(exts, tstep);
		return true;
	}
	pm->skipElement(tagString, depth);
	return true;
}
bool RegionParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	double defaultRegionExtents[6];
	if (StrCmpNoCase(tag, _regionParamsTag) == 0) {
		//If this is a regionparams, need to
		//pop the parse stack.  
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} 
	else if (StrCmpNoCase(tag, _regionMinTag) == 0){
		vector<double> bound = pm->getDoubleData();
		myBox->GetLocalExtents(defaultRegionExtents);
		
		defaultRegionExtents[0] = bound[0];
		defaultRegionExtents[1] = bound[1];
		defaultRegionExtents[2] = bound[2];
		myBox->SetLocalExtents(defaultRegionExtents);
		return true;
	} else if (StrCmpNoCase(tag, _regionMaxTag) == 0){
		vector<double> bound = pm->getDoubleData();
		myBox->GetLocalExtents(defaultRegionExtents);
		defaultRegionExtents[3] = bound[0];
		defaultRegionExtents[4] = bound[1];
		defaultRegionExtents[5] = bound[2];
		myBox->SetLocalExtents(defaultRegionExtents);
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
	
	int numchildren = 2 + myBox->GetTimes().size();
	ParamNode* regionNode = new ParamNode(_regionParamsTag, attrs, numchildren);

	//Now add children:  
	double defaultRegionExtents[6];
	myBox->GetLocalExtents(defaultRegionExtents);
	
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

	const vector<double>& exts = myBox->GetRootNode()->GetElementDouble(Box::_extentsTag);
	const vector<long>& times = myBox->GetTimes();
	for (int i = 1; i<times.size(); i++){
		

		//Add a node for each region in the list, having the extents as an attribute:
		//Iterate over the Tims
		attrs.clear();
		oss.str(empty);
		oss << (long) times[i];
		attrs[_timestepAttr] = oss.str();
		oss.str(empty);
		oss << (double) exts[6*i+0]<< " " <<
			(double) exts[6*i+1]<< " " <<
			(double) exts[6*i+2]<< " " <<
			(double) exts[6*i+3]<< " " <<
			(double) exts[6*i+4]<< " " <<
			(double) exts[6*i+5];
		attrs[_extentsAttr] = oss.str();
		
		ParamNode* regionTimeNode = new ParamNode(_regionAtTimeTag, attrs, 0);
		regionNode->AddChild(regionTimeNode);
		
	}

	return regionNode;
}

//Static method to find how many megabytes are needed for a dataset.
int RegionParams::getMBStorageNeeded(const double* extents, int refLevel){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return 0;
	
	
	size_t min_dim[3], max_dim[3];
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return 0;
	size_t timestep = DataStatus::getInstance()->getMinTimestep();
	dataMgr->GetEnclosingRegion(timestep, extents, extents+3, min_dim, max_dim, refLevel, 0);
	
	
	float numVoxels = (float)(max_dim[0]-min_dim[0]+1)*(max_dim[1]-min_dim[1]+1)*(max_dim[2]-min_dim[2]+1);
	
	//divide by 1 million for megabytes, mult by 4 for 4 bytes per voxel:
	float fullMB = numVoxels/262144.f;
	return (int)fullMB;
}
 
//calculate the 3D variable, at a specific point.
//Returns the OUT_OF_BOUNDS flag if point is not in current region
//


float RegionParams::
calcCurrentValue(const string& varname, const double point[3], int numRefinements, int lod, size_t timeStep){
	
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr;
	if (!ds || !(dataMgr = ds->getDataMgr())) return OUT_OF_BOUNDS;
	
	vector<string>varnames;
	varnames.push_back(varname);
	RegularGrid* varGrid;
	double exts[6];
	for (int i = 0; i<3; i++){exts[i] = point[i]; exts[i+3] = point[i];}
	int rc = getGrids(timeStep, varnames, exts, &numRefinements, &lod, &varGrid);
	if (!rc) return OUT_OF_BOUNDS;
	float val = varGrid->GetValue(point[0],point[1],point[2]);
	delete varGrid;
	return val;
	
}

int RegionParams::getValidRegion(size_t timestep, const char* varname, int minRefLevel, size_t min_coord[3], size_t max_coord[3]){
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dm = ds->getDataMgr();
	if (!dm) return -1;
	int rc;
	if (!dm->VariableExists(timestep,varname)) return -1;
	rc = dm->GetValidRegion(timestep, varname, minRefLevel, min_coord, max_coord);
	if(rc<0) {
		SetErrCode(0);
	}
	return rc;
}

void RegionParams::clearRegionsMap(){
	myBox->Trim();
}
bool RegionParams::insertTime(int timestep){
	if (timestep<0) return false;
	const vector<long>& times = GetTimes();
	int index = 0;
	for (int i = 1; i<times.size(); i++){
		if (times[i] == timestep){
			index = i;
			break;
		}
	}
	if (index != 0) return false;
	const vector<double>& extents = GetAllExtents();
	vector<long> copyTimes = vector<long>(times);
	vector<double>copyExts = vector<double>(extents);
	copyTimes.push_back(timestep);
	//Set the new extents to default extents:
	for (int i = 0; i<6; i++) copyExts.push_back(extents[i]);
	myBox->GetRootNode()->SetElementLong(Box::_timesTag, copyTimes);
	myBox->GetRootNode()->SetElementDouble(Box::_extentsTag, copyExts);
	return true;
}
bool RegionParams::removeTime(int timestep){
	if (timestep<0) return false;
	const vector<long>& times = GetTimes();
	int index = 0;
	for (int i = 1; i<times.size(); i++){
		if (times[i] == timestep){
			index = i;
			break;
		}
	}
	if (index == 0) return false;
	const vector<double>& extents = GetAllExtents();
	vector<long> copyTimes = vector<long>(times);
	vector<double>copyExts = vector<double>(extents);
	vector<long>::iterator itlong = copyTimes.begin()+index;
	vector<double>::iterator itdbl = copyExts.begin()+6*index;
	copyTimes.erase(itlong);
	copyExts.erase(itdbl,itdbl+5);
	return true;
}
