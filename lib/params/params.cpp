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
#include "vapor/XmlNode.h"
#include <cstring>
#include <qwidget.h>
#include <qapplication.h>
#include <string>
#include "vapor/MyBase.h"
#include "datastatus.h"
#include "assert.h"
#include "params.h"
#include "mapperfunction.h"
#include "glutil.h"
#include "ParamNode.h"
#include "vapor/DataMgr.h"
#include "vapor/errorcodes.h"
#include "viewpointparams.h"
#include "regionparams.h"


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


QString& Params::paramName(Params::ParamType type){
	switch (type){
		
		case(ViewpointParamsType):
			return *(new QString("View"));
		case(RegionParamsType):
			return *(new QString("Region"));
		
		case(FlowParamsType):
			return *(new QString("Flow"));
		case(DvrParamsType):
			return *(new QString("DVR"));
		case(IsoParamsType):
			return *(new QString("Iso"));
		case(ProbeParamsType):
			return *(new QString("Probe"));
		case(TwoDImageParamsType):
			return *(new QString("Image"));
		case(TwoDDataParamsType):
			return *(new QString("2D"));
		case(AnimationParamsType):
			return *(new QString("Animation"));
		case (UnknownParamsType):
		default:
			return *(new QString("Unknown"));
	}
	
}
float RenderParams::getMinColorMapBound(){
	return (getMapperFunc()? getMapperFunc()->getMinColorMapValue(): 0.f);
}
float RenderParams::getMaxColorMapBound(){
	return (getMapperFunc()? getMapperFunc()->getMaxColorMapValue(): 1.f);
}
float RenderParams::getMinOpacMapBound(){
	return (getMapperFunc()? getMapperFunc()->getMinOpacMapValue(): 0.f);
}
float RenderParams::getMaxOpacMapBound(){
	return (getMapperFunc()? getMapperFunc()->getMaxOpacMapValue(): 1.f);
}

//For params subclasses that have a box:
void Params::calcStretchedBoxExtentsInCube(float* extents, int timestep){
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
	SetErrMsg(VAPOR_FATAL,"Fatal error: %s",errorMessage.ascii());
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

Params::Params(
	XmlNode *parent, const string &name, int winNum
	): ParamsBase(parent,name) {
	vizNum = winNum;
	if(winNum < 0) local = false; else local = true;
	thisParamType = UnknownParamsType;
}


Params::~Params() {
	
}

RenderParams::RenderParams(XmlNode *parent, const string &name, int winnum):Params(parent, name, winnum){
	minColorEditBounds = 0;
	maxColorEditBounds = 0;
	minOpacEditBounds = 0;
	maxOpacEditBounds = 0;
}
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
float* RenderParams::
getContainingVolume(size_t blkMin[3], size_t blkMax[3], int refLevel, int sessionVarNum, int timeStep, bool twoDim){
	//Get the region associated with the specified variable in the 
	//specified block extents
	DataStatus* ds = DataStatus::getInstance();
	char* vname;
	int maxRes;
	if (twoDim){
		vname = (char*) ds->getVariableName2D(sessionVarNum).c_str();
		maxRes = ds->maxXFormPresent2D(sessionVarNum,timeStep);
	}
	else {
		vname = (char*)ds->getVariableName(sessionVarNum).c_str();
		maxRes = ds->maxXFormPresent(sessionVarNum,timeStep);
	}

	//Check if maxRes is not OK:
	if (maxRes < 0 || (maxRes < refLevel && !ds->useLowerRefinementLevel())){
		setBypass(timeStep);
		if (ds->warnIfDataMissing()){
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,
				"Data unavailable for refinement level %d \nof variable %s, at current timestep.\n%s",
				refLevel, vname,
				"This message can be silenced\nin user preferences panel.");
		}
		return 0;
	}
	//Modify refinement level if allowed:
	if (refLevel > maxRes) refLevel = maxRes;
	
	float* reg = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetRegion((size_t)timeStep,
		vname, refLevel, blkMin, blkMax,  0);
	if (!reg){
		if (ds->warnIfDataMissing()){
			setBypass(timeStep);
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,
				"Data unavailable for refinement level %d\nof variable %s, at current timestep.\n %s",
				refLevel, vname,
				"This message can be silenced\nin user preferences panel.");
		}
		ds->setDataMissing(timeStep,refLevel, sessionVarNum);
	}
	return reg;
}
	
//Default camera distance just finds distance to region box in stretched coordinates.
float RenderParams::getCameraDistance(ViewpointParams* vpp, RegionParams* rpp, int timestep){

	//Intersect region with camera ray.  If no intersection, just shoot ray from
	//camera to region center.
	const float* camPos = vpp->getCameraPos();
	const float* camDir = vpp->getViewDir();
	const float* regExts = rpp->getRegionExtents(timestep);

	//Stretch everything:
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	float cPos[3], rExts[6];
	for (int i = 0; i<3; i++){
		cPos[i] = camPos[i]*stretch[i];
		rExts[i] = regExts[i]*stretch[i];
		rExts[i+3] = regExts[i+3]*stretch[i];
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

	/* Following strategy doesn't work very well
	//Find distance of camera from current region
	
	
	const float *camPos = vpp->getCameraPos();
	const float* regExts = rpp->getRegionExtents(timestep);
	//First see which side it's on:
	//There are 6 tests, 3 for each dimension
	bool inReg[6];
	bool inDim[3];
	int numIn = 0;
	float testPoint[4][3];  
	for (int i = 0; i<3; i++){
		inReg[i] = (camPos[i] >= regExts[i]);
		inReg[i+3] = (camPos[i] <= regExts[i+3]);
		inDim[i] = (inReg[i]&&inReg[i+3]);
		if (inDim[i]) numIn++;
	}
	//Test if camera is in box:
	if (numIn == 3) return 0.f;

	//Check the faces (numIn = 2)
	if (numIn == 2) {
		//Check for camera on face (out in only one dimension)
		for (int k = 0; k< 3; k++){
			if (!inDim[k]){ //camera is on k-face of box.  find 2 test points
				//use other 2 (k1,k2) coords of camera, k- coord of box
				int k1 = (k+1)%3;
				int k2 = (k+2)%3;
				testPoint[0][k1] = camPos[k1];
				testPoint[0][k2] = camPos[k2];
				testPoint[1][k1] = camPos[k1];
				testPoint[1][k2] = camPos[k2];
				testPoint[0][k] = regExts[k];
				testPoint[1][k] = regExts[k+3];
				float dist1 = vdist(camPos, testPoint[0]);
				float dist2 = vdist(camPos, testPoint[1]);
				return Min(dist1,dist2);
			}
		}
	}
	//If numIn = 1, the camera is in the slab between two side planes of the box
	//Try the three slabs 
	if (numIn == 1){
		//Check each face (the dimension that's not in)
		for (int k = 0; k<3; k++){
			if (inDim[k]){//identify the unique in-dimension:
				//get other 2 dimensions:
				int k1 = (k+1)%3;
				int k2 = (k+2)%3;
				//Find 4 points that have same k-coord as camera, but
				//have other 2 coordinates on box faces:
				float minDist = 1.e30;
				for(int i = 0; i<4; i++) {
					// oddCrd goes 0,1,0,1 (*3)
					// secondCrd goes 0,0,1,1 (*3)
					int oddCrd = (i%2)*3;
					int secondCrd = (i>>1)*3;
					testPoint[i][k] = camPos[k];
					testPoint[i][k1] = regExts[k1+oddCrd];
					testPoint[i][k2] = regExts[k2+secondCrd];
					float dist = vdist(camPos, testPoint[i]);
					if (dist < minDist) minDist = dist;
				}
				return minDist;
			}
		}
	}
	//Final case:  corner point
	assert(numIn == 0);
	float minDist = 1.e30;
	float pnt[3];
	for (int i = 0; i<8; i++){
		int i1 = (i%2)*3;
		int i2 = ((i>>1)%2)*3;
		int i3 = (i>>2)*3;
		pnt[0] = regExts[i1];
		pnt[1] = regExts[1+i2];
		pnt[2] = regExts[2+i3];
		float dist = vdist(camPos,pnt);
		if (minDist > dist) minDist = dist;
	}
	return minDist;
	*/

}