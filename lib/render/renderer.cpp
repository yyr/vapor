//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		renderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Implements the Renderer class.
//		A pure virtual class that is implemented for each renderer.
//		Methods are called by the Visualizer class as needed.
//
#include "glutil.h"	// Must be included first!!!

#include "renderer.h"
#include <vapor/DataMgr.h>
#include "regionparams.h"
#include "viewpointparams.h"
#include "animationparams.h"
#include "vapor/ControlExecutive.h"
#include "vapor/errorcodes.h"
#ifdef Darwin
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

using namespace VAPoR;

Renderer::Renderer( Visualizer* glw, RenderParams* rp, string name)
{
	//Establish the data sources for the rendering:
	//
	
	_myName = name;
    myVisualizer = glw;
	currentRenderParams = rp;
	initialized = false;
	rp->initializeBypassFlags();
}

// Destructor
Renderer::~Renderer()
{
   
}


int Renderer::initializeGL() {
	int rc = _initializeGL();
	if (rc<0) {
		return(rc);
	}

	vector <int> status;
	bool ok = oglStatusOK(status);
	if (! ok) {
		SetErrMsg("OpenGL error : %s", oglGetErrMsg(status).c_str());
		return(-1);
	}
	return(0);
}

int Renderer::paintGL(DataMgr* dataMgr) {
	int rc = _paintGL(dataMgr);
	if (rc<0) {
		return(-1);
	}

	vector <int> status;
	bool ok = oglStatusOK(status);
	if (! ok) {
		SetErrMsg("OpenGL error : %s", oglGetErrMsg(status).c_str());
		return(-1);
	}
	return(0);
}


void Renderer::enableClippingPlanes(const double extents[6]){

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

	glLoadMatrixd(myVisualizer->getModelViewMatrix());

    //cerr << "transforming everything to unit box coords :-(\n";
   // myVisualizer->TransformToUnitBox();

    const double* scales = DataStatus::getInstance()->getStretchFactors();
    glScalef(scales[0], scales[1], scales[2]);

	GLdouble x0Plane[] = {1., 0., 0., 0.};
	GLdouble x1Plane[] = {-1., 0., 0., 1.0};
	GLdouble y0Plane[] = {0., 1., 0., 0.};
	GLdouble y1Plane[] = {0., -1., 0., 1.};
	GLdouble z0Plane[] = {0., 0., 1., 0.};
	GLdouble z1Plane[] = {0., 0., -1., 1.};//z largest


	x0Plane[3] = -extents[0];
	x1Plane[3] = extents[3];
	y0Plane[3] = -extents[1];
	y1Plane[3] = extents[4];
	z0Plane[3] = -extents[2];
	z1Plane[3] = extents[5];

	glClipPlane(GL_CLIP_PLANE0, x0Plane);
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE1, x1Plane);
	glEnable(GL_CLIP_PLANE1);
	glClipPlane(GL_CLIP_PLANE2, y0Plane);
	glEnable(GL_CLIP_PLANE2);
	glClipPlane(GL_CLIP_PLANE3, y1Plane);
	glEnable(GL_CLIP_PLANE3);
	glClipPlane(GL_CLIP_PLANE4, z0Plane);
	glEnable(GL_CLIP_PLANE4);
	glClipPlane(GL_CLIP_PLANE5, z1Plane);
	glEnable(GL_CLIP_PLANE5);

	glPopMatrix();
}

void Renderer::enableFullClippingPlanes() {

	AnimationParams *myAnimationParams = myVisualizer->getActiveAnimationParams();
    size_t timeStep = myAnimationParams->getCurrentTimestep();
	DataMgr *dataMgr = DataStatus::getInstance()->getDataMgr();

	const vector<double>& extvec = dataMgr->GetExtents(timeStep);
	double extents[6];
	for (int i=0; i<3; i++) {
		extents[i] = extvec[i] - ((extvec[3+i]-extvec[i])*0.001);
		extents[i+3] = extvec[i+3] + ((extvec[3+i]-extvec[i])*0.001);
	}

	enableClippingPlanes(extents);
}
	
void Renderer::disableClippingPlanes(){
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);
}

 void Renderer::enable2DClippingPlanes(){
	GLdouble topPlane[] = {0., -1., 0., 1.}; //y = 1
	GLdouble rightPlane[] = {-1., 0., 0., 1.0};// x = 1
	GLdouble leftPlane[] = {1., 0., 0., 0.001};//x = -.001
	GLdouble botPlane[] = {0., 1., 0., 0.001};//y = -.001
	
	const double* sizes = DataStatus::getInstance()->getFullStretchedSizes();
	topPlane[3] = sizes[1]*1.001;
	rightPlane[3] = sizes[0]*1.001;
	
	glClipPlane(GL_CLIP_PLANE0, topPlane);
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE1, rightPlane);
	glEnable(GL_CLIP_PLANE1);
	glClipPlane(GL_CLIP_PLANE2, botPlane);
	glEnable(GL_CLIP_PLANE2);
	glClipPlane(GL_CLIP_PLANE3, leftPlane);
	glEnable(GL_CLIP_PLANE3);
}

void Renderer::enableRegionClippingPlanes() {

	AnimationParams *myAnimationParams = myVisualizer->getActiveAnimationParams();
    size_t timeStep = myAnimationParams->getCurrentTimestep();
	RegionParams* myRegionParams = myVisualizer->getActiveRegionParams();

	double regExts[6];
	myRegionParams->GetBox()->GetUserExtents(regExts,timeStep);

	//
	// add padding for floating point roundoff
	//
	for (int i=0; i<3; i++) {
		regExts[i] = regExts[i] - ((regExts[3+i]-regExts[i])*0.001);
		regExts[i+3] = regExts[i+3] + ((regExts[3+i]-regExts[i])*0.001);
	}

	enableClippingPlanes(regExts);
}


void Renderer::UndoRedo(bool isUndo, int instance, Params* beforeP, Params* afterP){
	//This only handles RenderParams
	assert (beforeP->isRenderParams());
	assert (afterP->isRenderParams());
	RenderParams* rbefore = (RenderParams*)beforeP;
	RenderParams* rafter = (RenderParams*)afterP;
	//and only a change of enablement
	if (rbefore->IsEnabled() == rafter->IsEnabled()) return;
	//Undo an enable, or redo a disable
	bool doEnable = ((rafter->IsEnabled() && !isUndo) || (rbefore->IsEnabled() && isUndo));
	
	ControlExec::ActivateRender(beforeP->GetVizNum(),Params::GetTagFromType(beforeP->GetParamsBaseTypeId()),instance,doEnable);
	return;
}
void Renderer::
buildLocalCoordTransform(double transformMatrix[12], double extraThickness, int timestep, float rotation, int axis){
	
	double theta, phi, psi;
	double ang[3];
	
	if (rotation != 0.f) {
		getRenderParams()->GetBox()->convertThetaPhiPsi(&theta,&phi,&psi, axis, rotation);
	} else {
		getRenderParams()->GetBox()->GetAngles(ang);
		theta = ang[0];
		phi = ang[1];
		psi = ang[2];
	}
	
	double boxSize[3];
	
	double locExts[6];
	getRenderParams()->GetBox()->GetLocalExtents(locExts, timestep);
	
	for (int i = 0; i< 3; i++) {
		locExts[i] -= extraThickness;
		locExts[i+3] += extraThickness;
		boxSize[i] = (locExts[i+3] - locExts[i]);
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
	transformMatrix[3] = .5f*(locExts[3]+locExts[0]);
	transformMatrix[7] = .5f*(locExts[4]+locExts[1]);
	transformMatrix[11] = .5f*(locExts[5]+locExts[2]);
	
}
void Renderer::buildLocal2DTransform(int dataOrientation, float a[2],float b[2],float constVal[2], int mappedDims[3]){
	
	mappedDims[2] = dataOrientation;
	mappedDims[0] = (dataOrientation == 0) ? 1 : 0;  // x or y
	mappedDims[1] = (dataOrientation < 2) ? 2 : 1; // z or y
	const vector<double>& exts = getRenderParams()->GetBox()->GetLocalExtents();
	constVal[0] = exts[dataOrientation];
	constVal[1] = exts[dataOrientation+3];
	//constant terms go to middle
	b[0] = 0.5*(exts[mappedDims[0]]+exts[3+mappedDims[0]]);
	b[1] = 0.5*(exts[mappedDims[1]]+exts[3+mappedDims[1]]);
	//linear terms send -1,1 to box min,max
	a[0] = b[0] - exts[mappedDims[0]];
	a[1] = b[1] - exts[mappedDims[1]];

}
void Renderer::getLocalContainingRegion(float regMin[3], float regMax[3]){
	//Determine the smallest axis-aligned cube that contains the rotated box local coordinates.  This is
	//obtained by mapping all 8 corners into the space.
	
	double transformMatrix[12];
	//Set up to transform from probe (coords [-1,1]) into volume:
	buildLocalCoordTransform(transformMatrix, 0.f, -1);
	const double* sizes = DataStatus::getInstance()->getFullSizes();

	//Calculate the normal vector to the probe plane:
	double zdir[3] = {0.f,0.f,1.f};
	double normEnd[3];  //This will be the unit normal
	double normBeg[3];
	double zeroVec[3] = {0.f,0.f,0.f};
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
		double startVec[3], resultVec[3];
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
//Obtain grids for a set of variables in requested extents. Pointer to requested LOD and refLevel, may change if not available 
//Extents are reduced if data not available at requested extents.
//Vector of varnames can include "0" for zero variable.
//Variables can be 2D or 3D depending on value of "varsAre2D"
//Returns 0 on failure
//
int Renderer::getGrids(size_t ts, const vector<string>& varnames, double extents[6], int* refLevel, int* lod, RegularGrid** grids){
	
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