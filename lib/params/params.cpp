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
#include "glutil.h"	// Must be included first!!!
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
#include <vapor/ParamNode.h>
#include <vapor/DataMgr.h>
#include <vapor/RegularGrid.h>
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
const string Params::_FidelityLevelTag = "FidelityLevel";
const string Params::_IgnoreFidelityTag = "IgnoreFidelity";
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
	for (int i = 0; i<6; i++) {
		savedBoxVoxExts[i] =0;
		savedBoxLocalExts[i] =0;
	}
	savedRefLevel = -1;
	
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
	getLocalBox(boxMin, boxMax, timestep);
	float maxSize = 1.f;
	if (!DataStatus::getInstance()){
		for (int i = 0; i<3; i++){
			extents[i] = (boxMin[i])/maxSize;
			extents[i+3] = (boxMax[i])/maxSize;
		}
		return;
	}
	const float* fullSizes = DataStatus::getInstance()->getFullStretchedSizes();
	const float* stretchFactors = DataStatus::getInstance()->getStretchFactors();
	maxSize = Max(Max(fullSizes[0],fullSizes[1]),fullSizes[2]);
	for (int i = 0; i<3; i++){
		extents[i] = (boxMin[i]*stretchFactors[i])/maxSize;
		extents[i+3] = (boxMax[i]*stretchFactors[i])/maxSize;
	}
}

//For params subclasses that have a box, do the same in (unstretched) world coords
void Params::calcBoxExtents(float* extents, int timestep){
	
	float boxMin[3], boxMax[3];
	getLocalBox(boxMin, boxMax, timestep);
	
	for (int i = 0; i<3; i++){
		extents[i] = boxMin[i];
		extents[i+3] = boxMax[i];
	}
}
//For params subclasses that have a box, do the same in stretched world coords
void Params::calcStretchedBoxExtents(float* extents, int timestep){
	
	float boxMin[3], boxMax[3];
	getLocalBox(boxMin, boxMax, timestep);
	const float* stretchFactors = DataStatus::getInstance()->getStretchFactors();
	
	for (int i = 0; i<3; i++){
		extents[i] = boxMin[i]*stretchFactors[i];
		extents[i+3] = boxMax[i]*stretchFactors[i];
	}
}
//Following calculates box corners in user space.  Does not use
//stretching.
void Params::
calcLocalBoxCorners(float corners[8][3], float extraThickness, int timestep, float rotation, int axis){
	float transformMatrix[12];
	buildLocalCoordTransform(transformMatrix, extraThickness, timestep, rotation, axis);
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
buildLocalCoordTransform(float transformMatrix[12], float extraThickness, int timestep, float rotation, int axis){
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
	getLocalBox(boxMin, boxMax, timestep);

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
void Params::
buildLocalCoordTransform(double transformMatrix[12], double extraThickness, int timestep, float rotation, int axis){
	
	float theta, phi, psi;
	if (rotation != 0.f) {
		convertThetaPhiPsi(&theta,&phi,&psi, axis, rotation);
	} else {
		theta = getTheta();
		phi = getPhi();
		psi = getPsi();
	}
	
	double boxSize[3];
	double boxMin[3], boxMax[3];
	getLocalBox(boxMin, boxMax, timestep);

	for (int i = 0; i< 3; i++) {
		boxMin[i] -= extraThickness;
		boxMax[i] += extraThickness;
		boxSize[i] = (boxMax[i] - boxMin[i]);
	}
	
	//Get the 3x3 rotation matrix:
	double rotMatrix[9];
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
	//Construct transform of form (x,y)-> a[0]x+b[0],a[1]y+b[1],
	//Mapping [-1,1]X[-1,1] into 3D volume, in local coordinates.
    //Also determine the first and second coords that are used in 
    //the transform and the constant value
    //mappedDims[0] and mappedDims[1] are the dimensions that are
    //varying in the 3D volume.  mappedDims[2] is constant.
    //constVal are the constant values that are used, for top and bottom of
    //box (only different if terrain mapped)
	//dataOrientation is 0,1,or 2 for plane normal to x, y, or z
void Params::buildLocal2DTransform(int dataOrientation, float a[2],float b[2],float constVal[2], int mappedDims[3]){
	
	mappedDims[2] = dataOrientation;
	mappedDims[0] = (dataOrientation == 0) ? 1 : 0;  // x or y
	mappedDims[1] = (dataOrientation < 2) ? 2 : 1; // z or y
	const vector<double>& exts = GetBox()->GetLocalExtents();
	constVal[0] = exts[dataOrientation];
	constVal[1] = exts[dataOrientation+3];
	//constant terms go to middle
	b[0] = 0.5*(exts[mappedDims[0]]+exts[3+mappedDims[0]]);
	b[1] = 0.5*(exts[mappedDims[1]]+exts[3+mappedDims[1]]);
	//linear terms send -1,1 to box min,max
	a[0] = b[0] - exts[mappedDims[0]];
	a[1] = b[1] - exts[mappedDims[1]];

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
//static method to measure how far point is from cube in each dimension
//Needed for histogram testing
float Params::
distancesToCube(const float point[3],const float normals[6][3], const float corners[6][3], float maxOut[3]){
	float testVec[3];
	float maxDist = -1.30f;
	for (int j = 0; j<3; j++) maxOut[j] = -1.e30f;
	for (int i = 0; i< 6; i++){
		vsub(point, corners[i], testVec);
		//dist is the projection of the testVec on the outward pointing normal
		float dist = vdot(testVec,normals[i]);
		if (dist > 0.0){
			for (int j = 0; j<3; j++){
				//find the magnitude of the outward distance in each direction:
				maxOut[j] = Max(maxOut[j], abs(dist*normals[i][j]));
			}
		}
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
//Map corners of box to voxels.  Save results to avoid expensive dataMgr call
//Save latest values of voxel extents, refinement level, and local user extents
//If refLevel and local user extents have not changed, then return saved voxel extents
void Params::mapBoxToVox(DataMgr* dataMgr, int refLevel, int lod, int timestep, size_t voxExts[6]){
	double userExts[6];
	double locUserExts[6];
	if (!dataMgr) return;
	Box* box = GetBox();
	if (refLevel == savedRefLevel){
		//Compare local extents to see if they have changed
		box->GetLocalExtents(locUserExts, timestep);
		bool match = true;
		for (int i = 0; i<6; i++) if (locUserExts[i] != savedBoxLocalExts[i]) match = false;
		if (match) {
			box->GetUserExtents(locUserExts, timestep);
			for (int i = 0; i<6; i++) voxExts[i] = savedBoxVoxExts[i];
			return;
		}
	}
	
	box->GetUserExtents(userExts,(size_t)timestep);
	
	//calculate new values for voxExts (this can be expensive with layered data)
	dataMgr->MapUserToVox((size_t)timestep,userExts,voxExts, refLevel,lod);
	dataMgr->MapUserToVox((size_t)timestep,userExts+3,voxExts+3, refLevel,lod);
	//save stuff to compare next time:
	savedRefLevel = refLevel;
	box->GetLocalExtents(savedBoxLocalExts, (size_t)timestep);
	for (int i = 0; i<6; i++) savedBoxVoxExts[i] = voxExts[i];
	return;

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
	getLocalBox(boxmin,boxmax, timestep);
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
	setLocalBox(newBoxmin, newBoxmax, timestep);
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

//Default camera distance just finds distance to region box in stretched local coordinates.
float RenderParams::getCameraDistance(ViewpointParams* vpp, RegionParams* rpp, int timestep){
	double exts[6];
	rpp->GetBox()->GetLocalExtents(exts, timestep);
	return getCameraDistance(vpp, exts);

}
	
//Static method to find distance to a box, in stretched local coordinates.
float RenderParams::getCameraDistance(ViewpointParams* vpp, const double exts[6]){

	//Intersect box with camera ray.  If no intersection, just shoot ray from
	//camera to region center.
	const float* camPos = vpp->getCameraPosLocal();
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
		for (int inst = 0; inst < GetNumParamsInstances(i,winnum); inst++){
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
void Params::clearDummyParamsInstances(){
	for(int i = 0; i< dummyParamsInstances.size(); i++){
		delete dummyParamsInstances[i];
	}
	dummyParamsInstances.clear();
}
//Obtain grids for a set of variables in requested extents. Pointer to requested LOD and refLevel, may change if not available 
//Extents are reduced if data not available at requested extents.
//Vector of varnames can include "0" for zero variable.
//Variables can be 2D or 3D depending on value of "varsAre2D"
//Returns 0 on failure
//
int Params::getGrids(size_t ts, const vector<string>& varnames, double extents[6], int* refLevel, int* lod, RegularGrid** grids){
	
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return 0;
	
	//reduce reflevel if variable is available:
	int tempRefLevel = *refLevel;
	int tempLOD = *lod;
	for (int i = 0; i< varnames.size(); i++){
		if (varnames[i] != "0"){
			tempRefLevel = Min(ds->maxXFormPresent(varnames[i], ts), tempRefLevel);
			tempLOD = Min(ds->maxLODPresent(varnames[i], ts), tempLOD);
		}
	}
	if (ds->useLowerAccuracy()){
		*lod = tempLOD;
		*refLevel = tempRefLevel;
	} else {
		if (tempRefLevel< *refLevel || tempLOD < *lod){
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, "Variable not present at required refinement and LOD\n");
			return 0;
		}
	}
						  
	if (*refLevel < 0 || *lod < 0) return 0;
	
	//Determine the integer extents of valid cube, truncate to
	//valid integer coords:
	//Find a containing voxel box:
	size_t boxMin[3], boxMax[3];
	dataMgr->GetEnclosingRegion(ts, extents, extents+3, boxMin, boxMax, *refLevel, *lod);
	
	//Determine what region is available for all variables
	size_t temp_min[3], temp_max[3];
	for (int i = 0; i<varnames.size(); i++){
		if (varnames[i] != "0"){
			int rc = dataMgr->GetValidRegion(ts, varnames[i].c_str(), *refLevel, temp_min, temp_max);
			if (rc != 0) {
				MyBase::SetErrCode(0);
				return 0;
			}
			for (int j=0; j<3; j++){
				if (temp_min[j] > boxMin[j]) boxMin[j] = temp_min[j];
				if (temp_max[j] < boxMax[j]) boxMax[j] = temp_max[j];
				//If there is no valid intersection, the min will be greater than the max
				if (boxMax[j] < boxMin[j]) return 0;
			}
		}
	}
			

	
	//see if we have enough space:
	int numMBs = 0;
	const vector<string> varnames3D = dataMgr->GetVariables3D();
	for (int i = 0; i<varnames.size(); i++){
		if (varnames[i] == "0") continue;
		bool varIs3D = false;
		for (int j = 0; j<varnames3D.size();j++){
			if (varnames[i] == varnames3D[j]){varIs3D = true; break;}
		}
		
		if (varIs3D)
			numMBs += 4*(boxMax[0]-boxMin[0]+1)*(boxMax[1]-boxMin[1]+1)*(boxMax[2]-boxMin[2]+1)/1000000;
		else
			numMBs += 4*(boxMax[0]-boxMin[0]+1)*(boxMax[1]-boxMin[1]+1)/1000000;
	} 	
	int cacheSize = DataStatus::getInstance()->getCacheMB();
	if (numMBs > (int)(cacheSize*0.75)){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small\nfor current probe and resolution.\n%s",
						  "Lower the refinement level, reduce the size, or increase the cache size.");
		return 0;
	}
	
	
	for (int i = 0; i< varnames.size(); i++){
		if(varnames[i] == "0") grids[i] = 0;
		else {
			RegularGrid* rg = dataMgr->GetGrid(ts, varnames[i], *refLevel, *lod, boxMin, boxMax, 1);
			if (!rg) {
				for (int j = 0; j<i; j++){
					if (grids[j]){ dataMgr->UnlockGrid(grids[j]); delete grids[j];}
				}
				return 0;
			}
			grids[i] = rg;
		}
	}
	//obtained all of the grids needed
	
	return 1;	
	
}

int RenderParams::GetCompressionLevel(){
	const vector<long> defaultLevel(1,2);
	vector<long> valvec = GetRootNode()->GetElementLong(_CompressionLevelTag,defaultLevel);
	return (int)valvec[0];
 }
void RenderParams::SetCompressionLevel(int level){
	 vector<long> valvec(1,(long)level);
	 GetRootNode()->SetElementLong(_CompressionLevelTag,valvec);
	 setAllBypass(false);
 }
int RenderParams::GetFidelityLevel(){
	vector<long> valvec = GetRootNode()->GetElementLong(_FidelityLevelTag);
	return (int)valvec[0];
 }
void RenderParams::SetFidelityLevel(int level){
	 vector<long> valvec(1,(long)level);
	 GetRootNode()->SetElementLong(_FidelityLevelTag,valvec);
 }
bool RenderParams::GetIgnoreFidelity(){
	vector<long> valvec = GetRootNode()->GetElementLong(_IgnoreFidelityTag);
	return (bool)valvec[0];
 }
void RenderParams::SetIgnoreFidelity(bool val){
	 vector<long> valvec(1,(long)val);
	 GetRootNode()->SetElementLong(_IgnoreFidelityTag,valvec);
 }
 void Params::SetVisualizerNum(int viznum){
	vector<long> valvec(1,(long)viznum);
	GetRootNode()->SetElementLong(_VisualizerNumTag,valvec);
	vizNum = viznum;
 }
 int Params::GetVisualizerNum(){
	const vector<long> visnum(1,0);
	vector<long> valvec = GetRootNode()->GetElementLong(_VisualizerNumTag,visnum);
	return (int)valvec[0];
 }
 //Clip the probe to the specified box extents
//Return false (and make no change) if the probe rectangle does not have a positive (rectangular)
//intersection in the box.

bool RenderParams::cropToBox(const double bxExts[6]){
	//0.  Initially need a startPoint that is in the box and on the probe center plane.
	//1.  Check if probe center works.  If not call intersectRotatedBox() to get another startPoint (on middle plane) inside box.
	//2.  Using the new startPoint, construct x-direction line.  Find its first two intersections (+ and -) with box.  Reset the start point to be the
	//		middle of the resulting line segment.
	//3.  Construct the y-direction line from the new startPoint.  Again find first + and - intersection points.
	//4.  Take 4 diagonals of x- and y- direction lines, find first box intersection (or corner if box intersection is after corner.
	//5.  find largest rectangle inside four diagonal points.  Use this as the new probe.

	//Transform the four probe corners to local region 
	double transformMatrix[12];
	double prCenter[3]; // original probe center in world coords
	double startPoint[3];
	double pmid[3] = {0.,0.,0.};
	double pxp[3] = {1.,0.,0.};
	double pyp[3] = {0.,1.,0.};
	double psize[2];
	double prbexts[4]; //probe extents relative to startPoint, in the 2 probe axis directions.
	double probeCoords[2] = {0.,0.};
	double pendx[3], pendy[3];
	
	double exts[6];
	GetBox()->GetLocalExtents(exts);
	
	double boxExts[6];
	//shrink box slightly, otherwise errors occur with highly stretched domains.
	for (int i = 0; i<3; i++){
		boxExts[i] = bxExts[i]+(bxExts[i+3]-bxExts[i])*1.e-4;
		boxExts[i+3] = bxExts[i+3]-(bxExts[i+3]-bxExts[i])*1.e-4;
	}
	buildLocalCoordTransform(transformMatrix, 0.f, -1);
	//initially set startPoint to probe center:
	vtransform(pmid,transformMatrix,startPoint);
	vcopy(startPoint,prCenter);
	//Determine probe size in world coords.
	vtransform(pxp,transformMatrix,pendx);
	vtransform(pyp,transformMatrix,pendy);
	vsub(pendx,startPoint,pendx);
	vsub(pendy,startPoint,pendy);
	psize[0] = vlength(pendx);
	psize[1] = vlength(pendy);
	prbexts[2] = vlength(pendx);
	prbexts[3] = vlength(pendy);
	prbexts[0] = -prbexts[2];
	prbexts[1] = -prbexts[3];
	
	//Get direction vectors for rotated probe
	double rotMatrix[9];
	const vector<double>& angles = GetBox()->GetAngles();
	getRotationMatrix((float)(angles[0]*M_PI/180.), (float)(angles[1]*M_PI/180.), (float)(angles[2]*M_PI/180.), rotMatrix);
	//determine the probe x- and y- direction vectors
	double vecx[3] = { 1., 0.,0.};
	double vecy[3] = { 0., 1.,0.};
	//Direction vectors:
	double dir[4][3];
	//Intersection parameters
	double result[4][2];
	double edgeDist[4]; //distances from start point to probe edges

	//Construct 2 rays in x-axis directions
	vtransform3(vecx, rotMatrix, dir[0]);
	vtransform3(vecy, rotMatrix, dir[1]);
	vnormal(dir[0]);
	vnormal(dir[1]);
	
	//also negate:
	vmult(dir[0], -1.f, dir[2]);
	vmult(dir[1], -1.f, dir[3]);


	
	//Test:  is startPoint inside box?
	bool pointInBox = true;
	for (int i = 0; i<3; i++){
		if (startPoint[i] < boxExts[i]) {pointInBox = false; break;}
		if (startPoint[i] > boxExts[i+3]) {pointInBox = false; break;}
	}
	if (!pointInBox){
		pointInBox = intersectRotatedBox(boxExts,startPoint,probeCoords);
		if (!pointInBox) return false;
		//Modify prbexts to have probe exts relative to new value of startPoint
		//probeCoords values are along the dir[0] and dir[1] directions, with a value of +1 indicating the probe x-width
		//Thus the startPoint in world coords is
		// prCenter + psize[0]*probeCoords[0]*dir[0] + psize[1]*probeCoords[1]*dir[1]
		// and the probe extents are similarly translated:
		prbexts[0] = prbexts[0] - probeCoords[0]*psize[0];
		prbexts[2] = prbexts[2] - probeCoords[0]*psize[0];
		prbexts[1] = prbexts[1] - probeCoords[1]*psize[1];
		prbexts[3] = prbexts[3] - probeCoords[1]*psize[1];
	}
	
	//Shoot rays in axis directions from startPoint.
	
	//Intersect each line with the box, get the nearest intersections
	int numpts;
	for (int i = 0; i<4; i+=2){
		numpts = rayBoxIntersect(startPoint,dir[i], boxExts,result[i]);
		//Each ray should have two intersection points with the box
		if (numpts < 2 || result[i][1] < 0.0) return false; 
	
		//find the distance from the start point to the second intersection point
		double interpt[3];
		//calculate the intersection point
		for (int j = 0; j<3; j++){
			interpt[j] = result[i][1]*dir[i][j] + startPoint[j];
		}
		//find the distances from the intersection points to starting point
		for (int j = 0; j<3; j++){
			interpt[j] = interpt[j] - startPoint[j];
		}
		edgeDist[i] = vlength(interpt);
		//shorten the distance if it exceeds the probe extent in that direction
		if (i==0 && edgeDist[i] > prbexts[2]) edgeDist[i] = prbexts[2];
		if (i==2 && edgeDist[i] > -prbexts[0]) edgeDist[i] = -prbexts[0];
		
	}
	//Find the midpoint of the line connecting the x intersections
	double midDist = edgeDist[0]-edgeDist[2];
	//Move startPoint  to the center
	for (int i = 0; i<3; i++){
		startPoint[i] = startPoint[i] +dir[0][i]*midDist*0.5;
	}
	//Modify edgeDist so that the new startPoint is the center. 
	edgeDist[0] = edgeDist[2] = 0.5*(edgeDist[0]+edgeDist[2]);

	//Now shoot rays in the y directions
	for (int i = 1; i<4; i+=2){
		numpts = rayBoxIntersect(startPoint,dir[i], boxExts,result[i]);
		//Each ray should have two intersection points with the box
		if (numpts < 2 || result[i][1] < 0.0) return false; 
	
		//find the distance from the start point to the second intersection point
		double interpt[3];
		//calculate the intersection point
		for (int j = 0; j<3; j++){
			interpt[j] = result[i][1]*dir[i][j] + startPoint[j];
		}
		//find the distances from the intersection points to starting point
		for (int j = 0; j<3; j++){
			interpt[j] = interpt[j] - startPoint[j];
		}
		edgeDist[i] = vlength(interpt);
		//Shorten the distance if it exceeds the probe
		if (i==1 && edgeDist[i] > prbexts[3]) edgeDist[i] = prbexts[3];
		if (i==3 && edgeDist[i] > -prbexts[1]) edgeDist[i] = -prbexts[1];
	}

	//Now shoot rays in the diagonal directions from startPoint.
	//First determine the diagonal directions and the distances to the diagonal corners
	double diagDirs[4][3];
	double diagDist[4];
	for (int j = 0; j<4; j++){
		//Determine vector from startPoint to corner:
		for (int i = 0; i<3; i++){
			diagDirs[j][i] = edgeDist[j]*dir[j][i]+edgeDist[(j+1)%4]*dir[(j+1)%4][i];
		}
		diagDist[j] = vlength(diagDirs[j]);
		vnormal(diagDirs[j]);
	}
	//Now shoot rays in the diagonal directions
	double diagInterDist[4];
	double diagInterPt[4][3];
	double component[4][2]; //components of the resulting diagonals along probe x and y axes
	for (int i = 0; i<4; i++){
		numpts = rayBoxIntersect(startPoint,diagDirs[i], bxExts,result[i]);
		//Each ray should have two intersection points with the box
		if (numpts < 2 || result[i][1] < 0.0) return false; 
		//and result[i][1] is the distance along diagonal i to intersection 1
	
		//find the distance from the start point to the second intersection point
		//calculate the intersection point
		for (int j = 0; j<3; j++){
			diagInterPt[i][j] = result[i][1]*diagDirs[i][j] + startPoint[j];
		}
		
		//find the distances from the intersection points to starting point
		double interVec[3];
		for (int j = 0; j<3; j++){
			interVec[j] = diagInterPt[i][j] - startPoint[j];
		}
		diagInterDist[i] = vlength(interVec);
		//Make sure the diagonal distance does not exceed the distance to the corner
		double shrinkFactor = 1.;
		if (diagInterDist[i]>diagDist[i]) {

			shrinkFactor = diagDist[i]/diagInterDist[i];
		}
		//project in probe directions to get components:
		component[i][0] = vdot(dir[0],diagDirs[i])*result[i][1]*shrinkFactor;
		component[i][1] = vdot(dir[1],diagDirs[i])*result[i][1]*shrinkFactor;
		
	}
	
	
	//Find the x,y extents (relative to startPoint):
	double pExts[4];
	//maxx must be the smaller of the two x displacements:
	pExts[2] = Min(component[3][0],component[0][0]);//maxx
	pExts[0] = Max(component[1][0],component[2][0]);//minx
	pExts[1] = Max(component[2][1],component[3][1]);//miny
	pExts[3] = Min(component[0][1],component[1][1]);//maxy

	double wid = pExts[2]-pExts[0];
	double ht = pExts[3]-pExts[1];

	//New probe center is translated from startPoint by average of extents:
	//add dir[0]*(pexts[2]+pexts[0])*.5 to move probe x coordinate, similarly for y:
	//Use dir[] to hold the resulting displacements.
	double probeCenter[3];
	vcopy(startPoint,probeCenter);
	vmult(dir[0],0.5*(pExts[0]+pExts[2]),dir[0]);
	vmult(dir[1],0.5*(pExts[1]+pExts[3]),dir[1]);
	vadd(startPoint,dir[0],probeCenter);
	vadd(probeCenter,dir[1],probeCenter);

	double depth = exts[5]-exts[2];
	//apply these as offsets to startPoint, to get probe local extents.
	//Don't change the z extents.

	exts[0] = probeCenter[0]-wid*0.5;
	exts[1] =  probeCenter[1]-ht*0.5;
	exts[3] =  probeCenter[0]+wid*0.5;
	exts[4] = probeCenter[1]+ht*0.5;
	exts[2] = probeCenter[2]-depth*0.5;
	exts[5] = probeCenter[2]+depth*0.5;

	GetBox()->SetLocalExtents(exts);
	
	return true;
}

//Find a point that lies in the probe plane and in a box, if the probe intersects a face of the box.
//Return false if there is no such intersection 
bool RenderParams::intersectRotatedBox(double boxExts[6],double intersectPoint[3],double probeCoords[2]){
	//Transform the four probe corners to local region 
	double transformMatrix[12];
	double cor[4][3]; //probe corners in local user coords
	double pcorn[3] = {0.,0.,0.}; //local probe corner coordinates
	
	
	buildLocalCoordTransform(transformMatrix, 0.f, -1);
	
	
	bool cornerInFace[4][6];
	for (int i = 0; i<4; i++){
		//make pcorn rotate counter-clockwise around probe 
		pcorn[0] = -1.;
		pcorn[1] = -1.;
		if (i>1) pcorn[1] = 1.;
		if (i==1 || i==2) pcorn[0] = 1.;
		vtransform(pcorn, transformMatrix, cor[i]);
		for (int k = 0; k<3; k++){
			//Classify each corner as to whether it is inside or outside the half-space defined by each face
			//cornerInFace[i][j] is true if the cor[i][j] is inside the half-space
			if (cor[i][k] <= boxExts[k]) cornerInFace[i][k] = false; else cornerInFace[i][k] = true;
			if (cor[i][k] >= boxExts[k+3]) cornerInFace[i][k+3] = false; else cornerInFace[i][k+3] = true;
		}

	}

	//initialize probe min & max:
	double minx = -1., miny = -1.;
	double maxx = 1., maxy = 1.;
	//2. For each box face:
	for (int face = 0; face < 6; face++){
		int faceDim = face%3; //(x, y, or z-oriented face)
		int faceDir = face/3; //either low or high face
		//Intersect this face with four sides of probe.
		//A side of probe is determined by line (1-t)*cor[k] + t*cor[(k+1)%4], going from cor[k] to cor[k+1]
		//for each pair of corners, equation is
		// (1-t)*cor[k][faceDim] + t*cor[k+1][faceDim] = boxExts[faceDim+faceDir*3]
		// t*(cor[(k+1)%4][faceDim] - cor[k][faceDim]) = boxExts[faceDim+faceDir*3] - cor[k][faceDim]
		// i.e.: t = (boxExts[faceDim+faceDir*3] - cor[k][faceDim])/(cor[(k+1)%4][faceDim] - cor[k][faceDim]);
		int interNum = 0;
		double interPoint[2][3];
		double interT[2];
		int interSide[2];
		//a. determine either 2 or 0 intersection points between probe boundary and box face, by intersecting all sides of probe with face
		for (int k=0;k<4; k++){
			double denom = (cor[(k+1)%4][faceDim] - cor[k][faceDim]);
			if (denom == 0.) continue;
			double t = (boxExts[faceDim+faceDir*3] - cor[k][faceDim])/denom;
			if (t< 0. || t>1.) continue;
			for (int j = 0; j<3; j++){
				interPoint[interNum][j] = (1.-t)*cor[k][j] + t*cor[(k+1)%4][j];
			}
			//Replace t with T, going from -1 to +1, increasing with x and y
			//This simplifies the logic later.
			if (k > 1) t = 1.-t; //make t increase with x and y
			interT[interNum] = 2.*t -1.; //make T go from -1 to 1 instead of 0 to 1
			interSide[interNum] = k;
			interNum++;	
		}
		assert(interNum == 0 || interNum == 2);
		//Are there two intersections?
		if (interNum==2){
			//are they on opposite sides?
			if (interSide[1]-interSide[0] == 2){
				//Does it intersect the two horizonal edges?
				if (interSide[0] == 0){
					//is vertex 0 in this half-space? If so use min t-coordinate to cut the probe max x-extents
					if (cornerInFace[interSide[0]][face]){
						double mint = Min(interT[0],interT[1]);
						if (maxx > mint) maxx = mint;
					} else { //must be vertex 0 is outside half-space, so use maxt to trim probe min x-extents
						double maxt = Max(interT[0],interT[1]);
						if (minx < maxt) minx = maxt;
					}
				} else { //It must intersect the two vertical edges, check if vertex 0 is inside half-space
					assert(interSide[0] == 1);
					if (cornerInFace[interSide[0]][face]){
						double mint = Min(interT[0],interT[1]);
						if (maxy > mint) maxy = mint;
					} else { //must be vertex 0 is outside half-space, so use max to trim
						double maxt = Max(interT[0],interT[1]);
						if (miny < maxt) miny = maxt;
					}
				}
			} else { //The two intersections must cut off a corner of the probe
				//The possible cases for interSide's are: 0,1 (cuts of vertex 1); 0,3 (cuts off vertex 0); 
				//1,2 (cuts off vertex 2); 2,3(cuts off vertex 3);  each case can exclude or include the corner vertex
				if (interSide[0] == 0 && interSide[1] == 1) {
					//new corner is midpoint of line between the two intersection points.
					double newcorx = 0.5*(interT[0]+1.);
					double newcory = 0.5*(interT[1]-1.);
					if (cornerInFace[interSide[1]][face]){
						//if vertex 1 is inside then minx must be at least as large as newcorx, maxy as small as newcory
						minx = Max(minx,newcorx);
						maxy = Min(maxy,newcory);
					} else { //maxx must be as small as newcorx, miny must be as large as newcory
						maxx = Min(maxx,newcorx);
						miny = Max(miny,newcory);
					}
				} else if (interSide[0] == 0 && interSide[1] == 3){
					//new corner is midpoint of line between the two intersection points.
					double newcorx = 0.5*(interT[0]-1.);
					double newcory = 0.5*(interT[1]-1.);
					if (cornerInFace[interSide[0]][face]){
						//if vertex 0 is inside then maxx must be at least as small as newcorx, maxy as small as newcory
						maxx = Min(maxx,newcorx);
						maxy = Min(maxy,newcory);
					} else { //minx must be as large as newcorx, miny must be as large as newcory
						minx = Max(minx,newcorx);
						miny = Max(miny,newcory);
					}
				} else if (interSide[0] == 1 && interSide[1] == 2){
					//new corner is midpoint of line between the two intersection points.
					double newcory = 0.5*(interT[0]+1.);
					double newcorx = 0.5*(interT[1]+1.);
					if (cornerInFace[interSide[1]][face]){
						//if vertex 2 is inside then minx must be at least as large as newcorx, miny as large as newcory
						minx = Max(minx,newcorx);
						miny = Max(miny,newcory);
					} else { //maxx must be as small as newcorx, maxy must be as small as newcory
						maxx = Min(maxx,newcorx);
						maxy = Min(maxy,newcory);
					}
				} else if (interSide[0] == 2 && interSide[1] == 3){
					//new corner is midpoint of line between the two intersection points.
					double newcorx = 0.5*(interT[0]-1.);
					double newcory = 0.5*(interT[1]+1.);
					if (cornerInFace[interSide[1]][face]){
						//if vertex 3 is inside then maxx must be at least as small as newcorx, miny as large as newcory
						maxx = Min(maxx,newcorx);
						minx = Max(miny,newcory);
					} else { //minx must be as large as newcorx, maxy must be as small as newcory
						minx = Max(minx,newcorx);
						maxy = Min(maxy,newcory);
					}
				} else assert(0);
			} //end of cutting off corner
		} else {
			//If no intersections, check if the probe is completely outside slab determined by the face
			//If any corner is outside, entire probe is outside, so check first corner
			
			if (faceDir == 0 && cor[0][faceDim] < boxExts[face]) return false;
			if (faceDir == 1 && cor[0][faceDim] > boxExts[face]) return false;
			
			//OK, entire probe is inside this face
		}
	} //finished with all 6 faces.
	//Use minx, miny, maxx, maxy to find a rectangle in probe and box
	
	//3.  rectangle defined as intersection of all limits found
	// probeCoords uses middle:
	probeCoords[0] = 0.5*(minx+maxx);
	probeCoords[1] = 0.5*(miny+maxy);
	// convert minx, miny, maxx, maxy to interpolate between 0 and 1 (instead of -1 and +1)
	maxx = 0.5*(maxx+1.);
	maxy = 0.5*(maxy+1.);
	minx = 0.5*(minx+1.);
	miny = 0.5*(miny+1.);
	//Determine center of intersection (in world coords) by bilinearly interpolating from corners using minx, maxx, miny,maxy,
	//then taking average.
	
	double newCor[4][3];
	for (int k = 0; k<3; k++){
		newCor[0][k] = ((1.-minx)*cor[0][k] + minx*cor[1][k])*(1.-miny) +
			miny*((1.-minx)*cor[3][k] + minx*cor[2][k]);
		newCor[1][k] = ((1.-maxx)*cor[0][k] + maxx*cor[1][k])*(1.-miny) +
			miny*((1.-maxx)*cor[3][k] + maxx*cor[2][k]);
		newCor[2][k] = ((1.-maxx)*cor[0][k] + maxx*cor[1][k])*(1.-maxy) +
			maxy*((1.-maxx)*cor[3][k] + maxx*cor[2][k]);
		newCor[3][k] = ((1.-minx)*cor[0][k] + minx*cor[1][k])*(1.-maxy) +
			maxy*((1.-minx)*cor[3][k] + minx*cor[2][k]);
		intersectPoint[k] = 0.25*(newCor[0][k]+newCor[1][k]+newCor[2][k]+newCor[3][k]);
	}
	return true;
}

//Find probe extents that are maximal and fit in box
bool RenderParams::fitToBox(const double boxExts[6]){

	//Increase the box if it is flat:
	double modBoxExts[6];
	for (int i = 0; i<3; i++){
		modBoxExts[i] = boxExts[i];
		modBoxExts[i+3] = boxExts[i+3];
		if (boxExts[i] >= boxExts[i+3]){
			if (boxExts[i] > 0.f){
				modBoxExts[i] = boxExts[i]*0.99999f;
				modBoxExts[i+3] = boxExts[i]*1.00001f;
			}
			else if (boxExts[i] < 0.f) {
				modBoxExts[i] = boxExts[i]*1.00001f;
				modBoxExts[i+3] = boxExts[i]*0.99999f;
			}
			else {
				modBoxExts[i] = -1.e-20f;
				modBoxExts[i+3] = 1.e-20f;
			}
		}
	}
		
			
	//find a point in the probe that lies in the box.   Do this by finding the intersections
	//of the box with the probe plane and averaging the resulting points:
	double startPoint[3];
	double interceptPoints[6][3];
	int numintercept = interceptBox(modBoxExts, interceptPoints);
	if (numintercept < 3) return false;
	vzero(startPoint);
	for (int i = 0; i<numintercept; i++){
		for (int j = 0; j< 3; j++){
			startPoint[j] += interceptPoints[i][j]*(1.f/(double)numintercept);
		}
	}

	
	//Expand the probe so that it will exceed the box extents in all dimensions
	//Begin with the startPoint and intersect horizontal rays with the far edges of the box.

	double probeCenter[3];
	double exts[6];
	GetBox()->GetLocalExtents(exts);
	for (int i = 0; i<3; i++)
		probeCenter[i] = 0.5*(exts[i]+exts[i+3]);

	double rotMatrix[9];
	const vector<double>& angles = GetBox()->GetAngles();
	getRotationMatrix((float)(angles[0]*M_PI/180.), (float)(angles[1]*M_PI/180.), (float)(angles[2]*M_PI/180.), rotMatrix);

	//determine the probe x- and y- direction vectors
	double vecx[3] = { 1., 0.,0.};
	double vecy[3] = { 0., 1.,0.};
	
	//Direction vectors:
	double dir[4][3];
	//Intersection parameters
	double result[4][2];
	double edgeDist[4]; //distances from center to probe edges

	//Construct 4 rays in axis directions
	
	vtransform3(vecx, rotMatrix, dir[0]);
	vtransform3(vecy, rotMatrix, dir[1]);

	vnormal(dir[0]);
	vnormal(dir[1]);
	//also negate:
	vmult(dir[0], -1.f, dir[2]);
	vmult(dir[1], -1.f, dir[3]);

	//Intersect with each line
	int numpts;
	for (int i = 0; i<4; i++){
		numpts = rayBoxIntersect(startPoint,dir[i], modBoxExts,result[i]);
		//Each ray should have two intersection points with the box
		if (numpts < 2 || result[i][1] < 0.f) return false; 
	}
	//Use the distance from the probe center to the second intersection point as the new probe size
	for (int i = 0; i<4; i++){
		double interpt[3];
		//calculate the intersection point
		for (int j = 0; j<3; j++){
			interpt[j] = result[i][1]*dir[i][j] + startPoint[j];
		}
		//find the distance from the intersection point to the probe center
		for (int j = 0; j<3; j++){
			interpt[j] = interpt[j] - probeCenter[j];
		}
		edgeDist[i] = vlength(interpt);
	}
	//Stretch a bit to ensure adequate coverage
	double wid = 2.1*Max(edgeDist[0],edgeDist[2]);
	double ht = 2.1*Max(edgeDist[1],edgeDist[3]);

	
	double depth = abs(exts[5]-exts[2]);
	exts[0] = probeCenter[0] - 0.5f*wid;
	exts[3] = probeCenter[0] + 0.5f*wid;
	exts[1] = probeCenter[1] - 0.5f*ht;
	exts[4] = probeCenter[1] + 0.5f*ht;
	exts[2] = probeCenter[2] - 0.5f*depth;
	exts[5] = probeCenter[2] + 0.5f*depth;
	
	GetBox()->SetLocalExtents(exts);
	bool success = cropToBox(boxExts);
	//bool success = true;
	return success;
}

//Calculate up to six intersections of box edges with probe plane, return the number found.
//Up to 6 intersection points are placed in intercept array
int RenderParams::interceptBox(const double boxExts[6], double intercept[6][3]){
	int numfound = 0;
	//Get the equation of the probe plane
	//First, find normal to plane:
	double rotMatrix[9];
	const double vecz[3] = {0.,0.,1.};
	const double vec0[3] = {0.,0.,0.};
	double probeNormal[3], probeCenter[3];
	const vector<double>& angles = GetBox()->GetAngles();
	getRotationMatrix((float)(angles[0]*M_PI/180.), (float)(angles[1]*M_PI/180.), (float)(angles[2]*M_PI/180.), rotMatrix);
	
	vtransform3(vecz, rotMatrix, probeNormal);
	double transformMatrix[12];
	
	buildLocalCoordTransform(transformMatrix, 0.01, -1);
	vtransform(vec0, transformMatrix, probeCenter);
	vnormal(probeNormal);
	//The equation of the probe plane is dot(V, probeNormal) = dst:
	double dst = vdot(probeNormal, probeCenter);
	//Now intersect the plane with all 6 edges of box.
	//each edge is defined by two equations of the form
	// x = boxExts[0] or boxExts[3]; y = boxExts[1] or boxExts[4]; z = boxExts[2] or boxExts[5]
	for (int edge = 0; edge < 12; edge++){
		//edge%3 is the coordinate that varies;
		//equation holds (edge+1)%3 to low or high value, based on (edge/3)%2
		//holds (edge+2) to low or high value based on (edge/6) 
		//Thus equations associated with edge are:
		//vcoord = edge%3, coord1 is (edge+1)%3, coord2 is (edge+2)%3;
		// boxExts[vcoord] <= pt[vcoord] <= boxExts[vcoord+3]
		// pt[coord1] = boxExts[coord1+3*((edge/3)%2)]
		// pt[coord2] = boxExts[coord2+3*((edge/6))]
		int vcoord = edge%3;
		int coord1 = (edge+1)%3;
		int coord2 = (edge+2)%3;
		double rhs = dst -boxExts[coord1 + 3*((edge/3)%2)]*probeNormal[coord1] - boxExts[coord2+3*(edge/6)]*probeNormal[coord2];
		// and the equation is V*probeNormal[vcoord] = rhs
		//Question is whether the other (vcoord) coordinate of the intersection point lies between
		//boxExts[vcoord] and boxExts[vcoord+3]
		
		if (probeNormal[vcoord] == 0.f) continue;
		if (rhs/probeNormal[vcoord] < boxExts[vcoord]) continue;
		if (rhs/probeNormal[vcoord] > boxExts[vcoord+3]) continue;
		//Intersection found!
		intercept[numfound][coord1] = boxExts[coord1 + 3*((edge/3)%2)];
		intercept[numfound][coord2] = boxExts[coord2 + 3*(edge/6)];
		intercept[numfound][vcoord] = rhs/probeNormal[vcoord];
		numfound++;
		if (numfound == 6) return numfound;
	}
	return numfound;
	
}

void RenderParams::rotateAndRenormalizeBox(int axis, float rotVal){
	
	//Now finalize the rotation
	float newTheta, newPhi, newPsi;
	convertThetaPhiPsi(&newTheta, &newPhi, &newPsi, axis, rotVal);
	double angles[3];
	angles[0] = newTheta;
	angles[1] = newPhi;
	angles[2] = newPsi;
	GetBox()->SetAngles(angles);
	return;

}
void RenderParams::getLocalContainingRegion(float regMin[3], float regMax[3]){
	//Determine the smallest axis-aligned cube that contains the rotated box local coordinates.  This is
	//obtained by mapping all 8 corners into the space.
	
	float transformMatrix[12];
	//Set up to transform from probe (coords [-1,1]) into volume:
	buildLocalCoordTransform(transformMatrix, 0.f, -1);
	const float* sizes = DataStatus::getInstance()->getFullSizes();

	//Calculate the normal vector to the probe plane:
	float zdir[3] = {0.f,0.f,1.f};
	float normEnd[3];  //This will be the unit normal
	float normBeg[3];
	float zeroVec[3] = {0.f,0.f,0.f};
	vtransform(zdir, transformMatrix, normEnd);
	vtransform(zeroVec,transformMatrix,normBeg);
	vsub(normEnd,normBeg,normEnd);
	vnormal(normEnd);

	//Start by initializing extents, and variables that will be min,max
	for (int i = 0; i< 3; i++){
		regMin[i] = 1.e30f;
		regMax[i] = -1.e30f;
	}
	
	for (int corner = 0; corner< 8; corner++){
		int intCoord[3];
		float startVec[3], resultVec[3];
		intCoord[0] = corner%2;
		intCoord[1] = (corner/2)%2;
		intCoord[2] = (corner/4)%2;
		for (int i = 0; i<3; i++)
			startVec[i] = -1.f + (float)(2.f*intCoord[i]);
		// calculate the mapping of this corner,
		vtransform(startVec, transformMatrix, resultVec);
		// force mapped corner to lie in the local extents
		//and then force box to contain the corner:
		for (int i = 0; i<3; i++) {
			//force to lie in domain
			if (resultVec[i] < 0.) resultVec[i] = 0.;
			if (resultVec[i] > sizes[i]) resultVec[i] = sizes[i];
			
			if (resultVec[i] < regMin[i]) regMin[i] = resultVec[i];
			if (resultVec[i] > regMax[i]) regMax[i] = resultVec[i];
		}
	}
	return;
}

//Find the smallest stretched extents containing the rotated box
//Similar to above, but using stretched extents
void Params::calcRotatedStretchedBoxExtentsInCube(float* bigBoxExtents){
	if(!DataStatus::getInstance()) return;
	//Determine the smallest axis-aligned cube that contains the probe.  This is
	//obtained by mapping all 8 corners into the space.
	//It will not necessarily fit inside the unit cube.
	float corners[8][3];
	calcLocalBoxCorners(corners, 0.f, -1);
	
	float boxMin[3],boxMax[3];
	int crd, cor;
	
	//initialize extents, and variables that will be min,max
	for (crd = 0; crd< 3; crd++){
		boxMin[crd] = 1.e30f;
		boxMax[crd] = -1.e30f;
	}
	
	
	for (cor = 0; cor< 8; cor++){
		
		
		//make sure the container includes it:
		for(crd = 0; crd< 3; crd++){
			if (corners[cor][crd]<boxMin[crd]) boxMin[crd] = corners[cor][crd];
			if (corners[cor][crd]>boxMax[crd]) boxMax[crd] = corners[cor][crd];
		}
	}
	//Now convert the min,max back into extents in unit cube:
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	const float* fullSizes = DataStatus::getInstance()->getFullStretchedSizes();
	
	float maxSize = Max(Max(fullSizes[0],fullSizes[1]),fullSizes[2]);
	for (crd = 0; crd<3; crd++){
		bigBoxExtents[crd] = (boxMin[crd]*stretch[crd])/maxSize;
		bigBoxExtents[crd+3] = (boxMax[crd]*stretch[crd])/maxSize;
	}
	return;
}
//Determine the voxel extents of rotated plane mapped into data.
//Similar code is in calcProbeTexture()
void RenderParams::
getRotatedVoxelExtents(float voxdims[2]){
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) {
		voxdims[0] = voxdims[1] = 1.f;
		return;
	}
	const float* fullSizes = DataStatus::getInstance()->getFullSizes();
	float sliceCoord[3];
	//Can ignore depth, just mapping center plane
	sliceCoord[2] = 0.f;
	float transformMatrix[12];
	
	//Set up to transform from probe into volume:
	buildLocalCoordTransform(transformMatrix, 0.f, -1);
	
	//Get the data dimensions (at this resolution):
	int dataSize[3];
	int numRefinements = GetRefinementLevel();
	//Start by initializing integer extents
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)DataStatus::getInstance()->getFullSizeAtLevel(numRefinements,i);
	}
	
	float cor[4][3];
		
	for (int cornum = 0; cornum < 4; cornum++){
		float dataCoord[3];
		// coords relative to (-1,1)
		sliceCoord[1] = -1.f + 2.f*(float)(cornum/2);
		sliceCoord[0] = -1.f + 2.f*(float)(cornum%2);
		//Then transform to values in data 
		vtransform(sliceCoord, transformMatrix, dataCoord);
		//Then get array coords:
		for (int i = 0; i<3; i++){
			cor[cornum][i] = ((float)dataSize[i])*(dataCoord[i])/(fullSizes[i]);
		}
	}
	float vecWid[3], vecHt[3];
	
	vsub(cor[1],cor[0], vecWid);
	vsub(cor[3],cor[1], vecHt);
	voxdims[0] = vlength(vecWid);
	voxdims[1] = vlength(vecHt);
	return;
}