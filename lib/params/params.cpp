//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		params.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the Params and RenderParams classes.
//		These are  abstract classes for all the tabbed panel params classes.
//		Supports functionality common to all the tabbed panel params.
//
#include <vapor/XmlNode.h>
#include <cstring>
#include <qwidget.h>
#include <qapplication.h>
#include <string>
#include <vapor/MyBase.h>
#include "datastatus.h"
#include "assert.h"
#include "params.h"
#include "mapperfunction.h"
#include "glutil.h"
#include <vapor/ParamNode.h>
#include <vapor/DataMgr.h>
#include <vapor/errorcodes.h>
#include "viewpointparams.h"
#include "regionparams.h"
#include "dvrparams.h"
#include "flowparams.h"
#include "twoDdataparams.h"
#include "twoDimageparams.h"
#include "probeparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "animationparams.h"
#include "Box.h"


using namespace VAPoR;

const string Params::_dvrParamsTag = "DvrPanelParameters";
const string Params::_isoParamsTag = "IsoPanelParameters";
const string Params::_probeParamsTag = "ProbePanelParameters";
const string Params::_twoDImageParamsTag = "TwoDImagePanelParameters";
const string Params::_twoDDataParamsTag = "TwoDDataPanelParameters";
const string Params::_twoDParamsTag = "TwoDPanelParameters";//kept for backwards compat.
const string Params::_regionParamsTag = "RegionPanelParameters";
const string Params::_animationParamsTag = "AnimationPanelParameters";
const string Params::_viewpointParamsTag = "ViewpointPanelParameters";
const string Params::_flowParamsTag = "FlowPanelParameters";
const string Params::_CompressionLevelTag = "CompressionLevel";
const string Params::_RefinementLevelTag = "RefinementLevel";
const string Params::_VisualizerNumTag = "VisualizerNum";
const string Params::_VariableNamesTag = "VariableNames";
const string Params::_VariableNameTag = "VariableName";
const string Params::_VariablesTag = "Variables";

const string Params::_localAttr = "Local";
const string Params::_vizNumAttr = "VisualizerNum";
const string Params::_numVariablesAttr = "NumVariables";
const string Params::_numTransformsAttr = "NumTransforms";
const string Params::_variableTag = "Variable";

const string Params::_leftEditBoundAttr = "LeftEditBound";
const string Params::_rightEditBoundAttr = "RightEditBound";
const string Params::_variableNameAttr = "VariableName";
const string Params::_opacityScaleAttr = "OpacityScale";
const string Params::_useTimestepSampleListAttr = "UseTimestepSampleList";
const string Params::_timestepSampleListAttr = "TimestepSampleList";

std::map<pair<int,int>,vector<Params*> > Params::paramsInstances;
std::map<pair<int,int>, int> Params::currentParamsInstance;
std::map<int, Params*> Params::defaultParamsInstance;
std::vector<Params*> Params::dummyParamsInstances;



Params::Params(
	XmlNode *parent, const string &name, int winNum
	): ParamsBase(parent,name) {
	vizNum = winNum;
	if(winNum < 0) local = false; else local = true;
	
}


Params::~Params() {
	
}

RenderParams::RenderParams(XmlNode *parent, const string &name, int winnum):Params(parent, name, winnum){
	minColorEditBounds = 0;
	maxColorEditBounds = 0;
	minOpacEditBounds = 0;
	maxOpacEditBounds = 0;
	local = true;
	enabled = false;
}
const std::string& Params::paramName(Params::ParamsBaseType type){
	Params* p = GetDefaultParams(type);
	const std::string& name = p->getShortName();
	return GetDefaultParams(type)->getShortName();
}
float RenderParams::getMinColorMapBound(){
	return (GetMapperFunc()? GetMapperFunc()->getMinColorMapValue(): 0.f);
}
float RenderParams::getMaxColorMapBound(){
	return (GetMapperFunc()? GetMapperFunc()->getMaxColorMapValue(): 1.f);
}
float RenderParams::getMinOpacMapBound(){
	return (GetMapperFunc()? GetMapperFunc()->getMinOpacMapValue(): 0.f);
}
float RenderParams::getMaxOpacMapBound(){
	return (GetMapperFunc()? GetMapperFunc()->getMaxOpacMapValue(): 1.f);
}

//For params subclasses that have a box:
void Params::calcStretchedBoxExtentsInCube(float extents[6], int timestep){
	float boxMin[3], boxMax[3];
	getBox(boxMin, boxMax, timestep);
	float maxSize = 1.f;
	if (!DataStatus::getInstance()){
		for (int i = 0; i<3; i++){
			extents[i] = (boxMin[i])/maxSize;
			extents[i+3] = (boxMax[i])/maxSize;
		}
		return;
	}
	const float* fullExtents = DataStatus::getInstance()->getStretchedExtents();
	const float* stretchFactors = DataStatus::getInstance()->getStretchFactors();
	maxSize = Max(Max(fullExtents[3]-fullExtents[0],fullExtents[4]-fullExtents[1]),fullExtents[5]-fullExtents[2]);
	for (int i = 0; i<3; i++){
		extents[i] = (boxMin[i]*stretchFactors[i] - fullExtents[i])/maxSize;
		extents[i+3] = (boxMax[i]*stretchFactors[i] - fullExtents[i])/maxSize;
	}
}
//For params subclasses that have a box, do the same in (unstretched) world coords
void Params::calcBoxExtents(float* extents, int timestep){
	
	float boxMin[3], boxMax[3];
	getBox(boxMin, boxMax, timestep);
	
	for (int i = 0; i<3; i++){
		extents[i] = boxMin[i];
		extents[i+3] = boxMax[i];
	}
}
//For params subclasses that have a box, do the same in stretched world coords
void Params::calcStretchedBoxExtents(float* extents, int timestep){
	
	float boxMin[3], boxMax[3];
	getBox(boxMin, boxMax, timestep);
	const float* stretchFactors = DataStatus::getInstance()->getStretchFactors();
	
	for (int i = 0; i<3; i++){
		extents[i] = boxMin[i]*stretchFactors[i];
		extents[i+3] = boxMax[i]*stretchFactors[i];
	}
}
//Following calculates box corners in world space.  Does not use
//stretching.
void Params::
calcBoxCorners(float corners[8][3], float extraThickness, int timestep, float rotation, int axis){
	float transformMatrix[12];
	buildCoordTransform(transformMatrix, extraThickness, timestep, rotation, axis);
	float boxCoord[3];
	//Return the corners of the box (in world space)
	//Go counter-clockwise around the back, then around the front
	//X increases fastest, then y then z; 

	//Fatten box slightly, in case it is degenerate.  This will
	//prevent us from getting invalid face normals.

	boxCoord[0] = -1.f;
	boxCoord[1] = -1.f;
	boxCoord[2] = -1.f;
	vtransform(boxCoord, transformMatrix, corners[0]);
	boxCoord[0] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[1]);
	boxCoord[1] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[3]);
	boxCoord[0] = -1.f;
	vtransform(boxCoord, transformMatrix, corners[2]);
	boxCoord[1] = -1.f;
	boxCoord[2] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[4]);
	boxCoord[0] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[5]);
	boxCoord[1] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[7]);
	boxCoord[0] = -1.f;
	vtransform(boxCoord, transformMatrix, corners[6]);
	
}
//extraThickness parameter results in mapping to fatter box,
//Useful to prevent degenerate transform when box is flattened.
//Optional rotation and axis parameters modify theta and phi
//by rotation about axis.  Rotation is in degrees!
void Params::
buildCoordTransform(float transformMatrix[12], float extraThickness, int timestep, float rotation, int axis){
	//Note:  transformMatrix is a 3x4 matrix that converts Box coords
	// in the range [-1,1] to float coords in the volume.
	//The last column of the matrix is the translation
	//float transformMatrix[12];
	//calculate rotation matrix from phi and theta.
	//This is the matrix that takes box coords 
	// (with center at origin) to coords in the volume,
	// taking the origin to the box center.
	//  the z-axis is mapped to N, by rotating by theta and phi
	//  the x axis is mapped to U by rotating by theta,
	//  and the y axis is mapped to N x U. 
	
	//Rotation matrix is found by first rotating an angle (180-phi) in the Z-X plane,
	//then by angle theta in the X-Y plane.
	//The rows of the rotation matrix are:
	// 1st:  Cth Cph,  -Sth, Cth Sph
	// 2nd:  Sth Cph, Cth, SthSph
	// 3rd:  -Sph, 0, Cph

	//Hmmm.  Consider reversing order: first do the theta rotation, then the phi.
	//Also negate phi so phi rotation is in the +x direction
	// 1st:  CthCph  -CphSth  -Sph
	// 2nd:  Sth      Cth     0
	// 3rd:  CthSph  -SthSph  Cph
	//Note we are reversing phi, since it is more intuitive to have the user
	//looking down the negative z-axis.
	float theta, phi, psi;
	if (rotation != 0.f) {
		convertThetaPhiPsi(&theta,&phi,&psi, axis, rotation);
	} else {
		theta = getTheta();
		phi = getPhi();
		psi = getPsi();
	}
	
	float boxSize[3];
	float boxMin[3], boxMax[3];
	getBox(boxMin, boxMax, timestep);

	for (int i = 0; i< 3; i++) {
		boxMin[i] -= extraThickness;
		boxMax[i] += extraThickness;
		boxSize[i] = (boxMax[i] - boxMin[i]);
	}
	
	//Get the 3x3 rotation matrix:
	float rotMatrix[9];
	getRotationMatrix(theta*M_PI/180., phi*M_PI/180., psi*M_PI/180., rotMatrix);

	//then scale according to box:
	transformMatrix[0] = 0.5*boxSize[0]*rotMatrix[0];
	transformMatrix[1] = 0.5*boxSize[1]*rotMatrix[1];
	transformMatrix[2] = 0.5*boxSize[2]*rotMatrix[2];
	//2nd row:
	transformMatrix[4] = 0.5*boxSize[0]*rotMatrix[3];
	transformMatrix[5] = 0.5*boxSize[1]*rotMatrix[4];
	transformMatrix[6] = 0.5*boxSize[2]*rotMatrix[5];
	//3rd row:
	transformMatrix[8] = 0.5*boxSize[0]*rotMatrix[6];
	transformMatrix[9] = 0.5*boxSize[1]*rotMatrix[7];
	transformMatrix[10] = 0.5*boxSize[2]*rotMatrix[8];
	//last column, i.e. translation:
	transformMatrix[3] = .5f*(boxMax[0]+boxMin[0]);
	transformMatrix[7] = .5f*(boxMax[1]+boxMin[1]);
	transformMatrix[11] = .5f*(boxMax[2]+boxMin[2]);
	
}

//static method to measure how far point is from cube.
//Needed for histogram testing
//negative value means it's inside.
float Params::
distanceToCube(const float point[3],const float normals[6][3], const float corners[6][3]){
	float testVec[3];
	float maxDist = -1.30f;
	for (int i = 0; i< 6; i++){
		vsub(point, corners[i], testVec);
		float dist = vdot(testVec,normals[i]);
		if (dist > maxDist) maxDist = dist;
	}
	return maxDist;
}

//----------------------------------------------------------------------------
// Returns true if the flag is in the command-line arguments
//----------------------------------------------------------------------------
bool Params::searchCmdLine(const char *flag)
{
 if (!flag) return false;
 
  int argc    = qApp->argc();
  char **argv = qApp->argv();

  int arg = 0;

  while (arg < argc)
  {
    if (strcmp(argv[arg], flag) == 0)
    {
      return true;
    }
 
    arg++;
  }

  return false;
}

//----------------------------------------------------------------------------
// Returns the argument immediately following the flag; NULL otherwise
//----------------------------------------------------------------------------
const char* Params::parseCmdLine(const char *flag)
{
 if (!flag) return NULL;
 
  int argc    = qApp->argc();
  char **argv = qApp->argv();

  int arg = 0;

  while (arg < argc)
  {
    if (strcmp(argv[arg], flag) == 0)
    {
      if (arg+1 < argc)
      {
        return argv[arg+1];
      }
      else
      {
        return NULL;
      }
    }
 
    arg++;
  }

  return NULL;
}
void Params::BailOut(const char *errstr, const char *fname, int lineno)
{
    /* Terminate program after printing an error message.
     * Use via the macros Verify and MemCheck.
     */
    //Error("Error: %s, at %s:%d\n", errstr, fname, lineno);
    //if (coreDumpOnError)
	//abort();
	QString errorMessage(errstr);
	errorMessage += "\n in file: ";
	errorMessage += fname;
	errorMessage += " at line ";
	errorMessage += QString::number(lineno);
	SetErrMsg(VAPOR_FATAL,"Fatal error: %s",(const char*)errorMessage.toAscii());
	//MessageReporter::fatalMsg(errorMessage);
    exit(-1);
}
//Determine a new value of theta phi and psi when the probe is rotated around either the
//x-, y-, or z- axis.  axis is 0,1,or 2 1. rotation is in degrees.
//newTheta and newPhi are in degrees, with theta between -180 and 180, phi between 0 and 180
//and newPsi between -180 and 180
void Params::convertThetaPhiPsi(float *newTheta, float* newPhi, float* newPsi, int axis, float rotation){

	//First, get original rotation matrix R0(theta, phi, psi)
	float origMatrix[9], axisRotate[9], newMatrix[9];
	getRotationMatrix(getTheta()*M_PI/180., getPhi()*M_PI/180., getPsi()*M_PI/180., origMatrix);
	//Second, get rotation matrix R1(axis,rotation)
	getAxisRotation(axis, rotation*M_PI/180., axisRotate);
	//New rotation matrix is R1*R0
	mmult33(axisRotate, origMatrix, newMatrix);
	//Calculate newTheta, newPhi, newPsi from R1*R0 
	getRotAngles(newTheta, newPhi, newPsi, newMatrix);
	//Convert back to degrees:
	(*newTheta) *= (180./M_PI);
	(*newPhi) *= (180./M_PI);
	(*newPsi) *= (180./M_PI);
	return;
}
	
void Params::getStretchedBox(float boxmin[3], float boxmax[3], int timestep){
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	getBox(boxmin,boxmax, timestep);
	for (int i = 0; i< 3; i++){
		boxmin[i] *= stretch[i];
		boxmax[i] *= stretch[i];
	}
}
void Params::setStretchedBox(const float boxmin[3], const float boxmax[3], int timestep){
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	float newBoxmin[3], newBoxmax[3];
	for (int i = 0; i< 3; i++){
		newBoxmin[i] = boxmin[i]/stretch[i];
		newBoxmax[i] = boxmax[i]/stretch[i];
	}
	setBox(newBoxmin, newBoxmax, timestep);
}

//Following methods adapted from ParamsBase.cpp


void RenderParams::initializeBypassFlags(){
	bypassFlags.clear();
	int numTimesteps = DataStatus::getInstance()->getNumTimesteps();
	bypassFlags.resize(numTimesteps, 0);
}
void RenderParams::setAllBypass(bool val){ 
	//set all bypass flags to either 0 or 2
	//when set to 0, indicates "never bypass"
	//when set to 2, indicates "always bypass"
	int ival = val ? 2 : 0;
	for (int i = 0; i<bypassFlags.size(); i++)
		bypassFlags[i] = ival;
}
// used by Probe, TwoD, and Region currently.  Can retrieve either slice or volume
// reduce the refinement level, or reduce the compression level (if allowed)
float* RenderParams::
getContainingVolume(size_t blkMin[3], size_t blkMax[3], int refLevel, int sessionVarNum, int timeStep, bool twoDim){
	//Get the region associated with the specified variable in the 
	//specified block extents
	DataStatus* ds = DataStatus::getInstance();
	char* vname;
	int maxLOD, maxRefLevel;
	int lod = GetCompressionLevel();
	if (twoDim){
		vname = (char*) ds->getVariableName2D(sessionVarNum).c_str();
		maxLOD = ds->maxLODPresent2D(sessionVarNum,timeStep);
		maxRefLevel = ds->maxXFormPresent2D(sessionVarNum, timeStep);
	}
	else {
		vname = (char*)ds->getVariableName3D(sessionVarNum).c_str();
		maxLOD = ds->maxLODPresent3D(sessionVarNum,timeStep);
		maxRefLevel = ds->maxXFormPresent3D(sessionVarNum, timeStep);
	}

	
	if (maxLOD < 0 || (maxLOD < lod && !ds->useLowerAccuracy())){
		setBypass(timeStep);
		if (ds->warnIfDataMissing()){
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,
				"Data unavailable for LOD %d \nof variable %s, at current timestep.\n%s",
				lod, vname,
				"This message can be silenced\nin user preferences panel.");
		}
		return 0;
	}
	if (maxRefLevel < 0 || (maxRefLevel < refLevel && !ds->useLowerAccuracy())){
		setBypass(timeStep);
		if (ds->warnIfDataMissing()){
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,
				"Data unavailable for Refinement level %d \nof variable %s, at current timestep.\n%s",
				refLevel, vname,
				"This message can be silenced\nin user preferences panel.");
		}
		return 0;
	}
	//Modify lod level if allowed:
	if (lod > maxLOD) lod = maxLOD;
	if (refLevel > maxRefLevel) refLevel = maxRefLevel;
	
	float* reg = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetRegion((size_t)timeStep,
		vname, refLevel, lod, blkMin, blkMax,  0);
	if (!reg){
		ds->getDataMgr()->SetErrCode(0);
		if (ds->warnIfDataMissing()){
			setBypass(timeStep);
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,
				"Data unavailable for refinement level %d, compression %d\nof variable %s, at current timestep.\n %s",
				refLevel, lod, vname,
				"This message can be silenced\nin user preferences panel.");
		}
		if(twoDim) ds->setDataMissing2D(timeStep,refLevel, lod, sessionVarNum);
		else ds->setDataMissing3D(timeStep,refLevel, lod, sessionVarNum);
	}
	return reg;
}
	
//Default camera distance just finds distance to region box in stretched coordinates.
float RenderParams::getCameraDistance(ViewpointParams* vpp, RegionParams* rpp, int timestep){
	double exts[6];
	rpp->GetBox()->GetExtents(exts, timestep);
	return getCameraDistance(vpp, exts);

}
	
//Static method to find distance to a box, in stretched coordinates.
float RenderParams::getCameraDistance(ViewpointParams* vpp, const double exts[6]){

	//Intersect box with camera ray.  If no intersection, just shoot ray from
	//camera to region center.
	const float* camPos = vpp->getCameraPos();
	const float* camDir = vpp->getViewDir();

	//Stretch everything:
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	float cPos[3], rExts[6];
	for (int i = 0; i<3; i++){
		cPos[i] = camPos[i]*stretch[i];
		rExts[i] = exts[i]*stretch[i];
		rExts[i+3] = exts[i+3]*stretch[i];
	}
	float hitPoint[3];
	
	//Solve for intersections with 6 planar sides of region, find smallest.
	float minDist = 1.e30f;
	
	for (int i = 0; i< 6; i++){
		if (camDir[i%3] == 0.f) continue;
		//Find parameter T (position along ray) so that camPos+T*camDir intersects plane
		float T = (rExts[i] - cPos[i%3])/camDir[i%3];
		if (T < 0.f) continue;
		for (int k = 0; k<3; k++)
			hitPoint[k] = cPos[k]+T*camDir[k];
		// Check if hitpoint is inside face:
		int i1 = (i+1)%3;
		int i2 = (i+2)%3;
		if (hitPoint[i1] >= rExts[i1] && hitPoint[i2] >= rExts[i2] &&
			hitPoint[i1] <= rExts[i1+3] && hitPoint[i2] <= rExts[i2+3])
		{
			float dist = vdist(hitPoint, cPos);
			minDist = Min(dist, minDist);
		}
	}
	//Was there an intersection?

	if (minDist < 1.e30f) return minDist;

	//Otherwise, find ray that points to center, and intersect it with region
	float rayDir[3];
	for (int i = 0; i<3; i++){
		rayDir[i] = 0.5f*(rExts[i]+rExts[i+3]) - cPos[i];
	}
	for (int i = 0; i< 6; i++){
		if (rayDir[i%3] == 0.f) continue;
		//Find parameter T (position along ray) so that camPos+T*rayDir intersects plane
		float T = (rExts[i] - cPos[i%3])/rayDir[i%3];
		if (T < 0.f) continue;
		for (int k = 0; k<3; k++)
			hitPoint[k] = cPos[i]+T*rayDir[i];
		// Check if hitpoint is inside face:
		int i1 = (i+1)%3;
		int i2 = (i+2)%3;
		if (hitPoint[i1] >= rExts[i1] && hitPoint[i2] >= rExts[i2] &&
			hitPoint[i1] <= rExts[i1+3] && hitPoint[i2] <= rExts[i2+3])
		{
			float dist = vdist(hitPoint, cPos);
			minDist = Min(dist, minDist);
		}
	}
	return minDist;

}

Params* Params::GetParamsInstance(int pType, int winnum, int instance){
	if (winnum < 0) return defaultParamsInstance[pType];
	if (instance < 0) instance = currentParamsInstance[make_pair(pType,winnum)];
	if (instance >= paramsInstances[make_pair(pType, winnum)].size()) {
		Params* p = GetDefaultParams(pType)->deepCopy();
		p->setVizNum(winnum);
		AppendParamsInstance(pType,winnum,p);
		return p;
	}
	return paramsInstances[make_pair(pType, winnum)][instance];
}

void Params::RemoveParamsInstance(int pType, int winnum, int instance){
	vector<Params*>& instVec = paramsInstances[make_pair(pType,winnum)];
	Params* p = instVec.at(instance);
	int currInst = currentParamsInstance[make_pair(pType,winnum)];
	if (currInst > instance) currentParamsInstance[make_pair(pType,winnum)] = currInst - 1;
	instVec.erase(instVec.begin()+instance);
	if (currInst >= (int) instVec.size()) currentParamsInstance[make_pair(pType,winnum)]--;
	delete p;
}
map <int, vector<Params*> >* Params::cloneAllParamsInstances(int winnum){
	map<int, vector<Params*> >* winParamsMap = new map<int, vector<Params*> >;
	for (int i = 1; i<= GetNumParamsClasses(); i++){
		vector<Params*> *paramsVec = new vector<Params*>;
		for (int j = 0; j<GetNumParamsInstances(i,winnum); j++){
			Params* p = GetParamsInstance(i,winnum,j);
			paramsVec->push_back(p->deepCopy(0));
		}
		(*winParamsMap)[i] = *paramsVec;
	}
	return winParamsMap;

			
}
vector <Params*>* Params::cloneAllDefaultParams(){
	vector <Params*>* defaultParams = new vector<Params*>;
	defaultParams->push_back(0); //don't use position 0
	for (int i = 1; i<= GetNumParamsClasses(); i++){
		Params* p = GetDefaultParams(i);
		defaultParams->push_back(p->deepCopy(0));
	}
	return (defaultParams);
}

bool Params::IsRenderingEnabled(int winnum){
	for (int i = 1; i<= GetNumParamsClasses(); i++){
		for (int inst = 0; inst < GetNumParamsInstances(i,winnum); i++){
			Params* p = GetParamsInstance(i,winnum, inst);
			if (!(p->isRenderParams())) break;
			if (((RenderParams*)p)->isEnabled()) return true;
		}
	}
	return false;
}
//Default copy constructor.  Don't copy the paramnode
Params::Params(const Params& p) :
	ParamsBase(p)
{
	local = p.local;
	vizNum = p.vizNum;
}
Params* Params::deepCopy(ParamNode* ){
	//Start with default copy  
	Params* newParams = CreateDefaultParams(GetParamsBaseTypeId());
	newParams->local = isLocal();
	newParams->setVizNum(getVizNum());
	// Need to clone the xmlnode; 
	ParamNode* rootNode = GetRootNode();
	if (rootNode) {
		newParams->SetRootParamNode(rootNode->deepCopy());
		newParams->GetRootNode()->SetParamsBase(newParams);
	}
	
	newParams->setCurrentParamNode(newParams->GetRootNode());
	return newParams;
}
Params* RenderParams::deepCopy(ParamNode* nd){
	//Start with default copy  
	Params* newParams = Params::deepCopy(nd);
	
	RenderParams* renParams = dynamic_cast<RenderParams*>(newParams);
	
	renParams->enabled = enabled;
	renParams->stopFlag = stopFlag;
	
	renParams->minColorEditBounds = 0;
	renParams->maxColorEditBounds = 0;
	renParams->minOpacEditBounds = 0;
	renParams->maxOpacEditBounds = 0;

	renParams->bypassFlags = bypassFlags;
	return renParams;
}
Params* Params::CreateDummyParams(const std::string tag){
	return ((Params*)(new DummyParams(0,tag,0)));
}
DummyParams::DummyParams(XmlNode* parent, const string tag, int winnum) : Params(parent, tag, winnum){
	myTag = tag;
}