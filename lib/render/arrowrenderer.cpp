//-- arrowrenderer.cpp ----------------------------------------------------------
//   
//                   Copyright (C)  2011
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
//----------------------------------------------------------------------------
//
//      File:           arrowrenderer.cpp
//
//      Author:         Alan Norton
//
//      Description:  Implementation of ArrowRenderer class
//
//----------------------------------------------------------------------------

// Specify the arrowhead width compared with arrow diameter:
#define ARROW_HEAD_FACTOR 2.0
// Specify how long the arrow tube is, in front of where the arrowhead is attached:
#define ARROW_LENGTH_FACTOR 0.9

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <qgl.h>
#include "arrowrenderer.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"
#include "glwindow.h"
#include "params.h"
#include "renderer.h"
#include "glutil.h"
#include "arrowparams.h"
#include <vapor/MyBase.h>
#include <vapor/errorcodes.h>

using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ArrowRenderer::ArrowRenderer(GLWindow* glw, RenderParams* rp) 
  : Renderer(glw,  rp, "ArrowRenderer")
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ArrowRenderer::~ArrowRenderer()
{
}

void ArrowRenderer::initializeGL(){
	initialized = true;
}

void ArrowRenderer::paintGL(){
	
	ArrowParams* aParams = (ArrowParams*)currentRenderParams;
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;

	//
	//Set up the variable data required, while determining data extents to use in rendering
	//
	float* varData[4] = {0,0,0,0};
	vector<string>varnames;
	size_t min_bdim[3], max_bdim[3], voxExts[6];
	double validExts[6];
	int actualRefLevel = setupVariableData(varnames, varData, validExts, voxExts, min_bdim, max_bdim);
	if (actualRefLevel < 0) return;
	//
	//Calculate the scale factors and radius to be used in rendering the arrows:
	//
	const vector<long> rakeGrid = aParams->GetRakeGrid();
	double rakeExts[6];
	aParams->GetRakeExtents(rakeExts);
	
	float vectorLengthScale = aParams->GetVectorScale();
	//
	//Also find the horizontal voxel size in user units at highest ref level.  This will be used to scale the vector radius.
	//
	size_t dim[3];
	dataMgr->GetDim(dim, -1);
	const float* fullexts = ds->getExtents();
	float maxVoxSize = 0.f;
	for (int i = 0; i<2; i++){
		float voxSide = (fullexts[i+3]-fullexts[i])/(dim[i]);
		if (voxSide > maxVoxSize) maxVoxSize = voxSide;
	}
	if (maxVoxSize == 0.f) maxVoxSize = 1.f;
	float rad = 0.5*maxVoxSize*aParams->GetLineThickness();
	//
	//Perform OpenGL rendering of arrows
	//
	performRendering(min_bdim, max_bdim, actualRefLevel, vectorLengthScale, rad, varData);
	
	//Release the locks on the data:
	for (int k = 0; k<4; k++){
		if (varData[k])
			dataMgr->UnlockRegion(varData[k]);
	}
}

//Issue OpenGL calls to draw a cylinder with orthogonal ends from one point to another.
//Then put an arrow head on the end
//
void ArrowRenderer::drawArrow(const float startPoint[3], const float endPoint[3], float radius) {
	//Constants are needed for cosines and sines, at 60 degree intervals.
	//The arrow is really a hexagonal tube, but the shading makes it look round.
	const float sines[6] = {0.f, sqrt(3.)/2., sqrt(3.)/2., 0.f, -sqrt(3.)/2., -sqrt(3.)/2.};
	const float coses[6] = {1.f, 0.5, -0.5, -1., -.5, 0.5};

	float nextPoint[3];
	float vertexPoint[3];
	float headCenter[3];
	float startNormal[18];
	float nextNormal[18];
	float startVertex[18];
	float nextVertex[18];
	float testVec[3];
	float testVec2[3];
	
	//Calculate an orthonormal frame, dirVec, uVec, bVec.  dirVec is the arrow direction
	float dirVec[3], bVec[3], uVec[3];
	vsub(endPoint,startPoint,dirVec);
	float len = vlength(dirVec);
	if (len == 0.f) return;
	vscale(dirVec,1./len);

	//Calculate uVec, orthogonal to dirVec:
	vset(testVec, 1.,0.,0.);
	vcross(dirVec, testVec, uVec);
	len = vdot(uVec,uVec);
	if (len == 0.f){
		vset(testVec, 0.,1.,0.);
		vcross(dirVec, testVec, uVec);
		len = vdot(uVec, uVec);
	} 
	vscale(uVec, 1.f/sqrt(len));
	vcross(uVec, dirVec, bVec);
	
	int i;
	//Calculate nextPoint and vertexPoint, for arrowhead
	
	for (i = 0; i< 3; i++){
		nextPoint[i] = (1. - ARROW_LENGTH_FACTOR)*startPoint[i]+ ARROW_LENGTH_FACTOR*endPoint[i];
		//Assume a vertex angle of 45 degrees:
		vertexPoint[i] = nextPoint[i] + dirVec[i]*radius;
		headCenter[i] = nextPoint[i] - dirVec[i]*(ARROW_HEAD_FACTOR*radius-radius); 
	}
	//calculate 6 points in plane orthog to dirVec, in plane of point
	for (i = 0; i<6; i++){
		//testVec and testVec2 are components of point in plane
		vmult(uVec, coses[i], testVec);
		vmult(bVec, sines[i], testVec2);
		//Calc outward normal as a sideEffect..
		//It is the vector sum of x,y components (norm 1)
		vadd(testVec, testVec2, startNormal+3*i);
		//stretch by radius to get current displacement
		vmult(startNormal+3*i, radius, startVertex+3*i);
		//add to current point
		vadd(startVertex+3*i, startPoint, startVertex+3*i);
	}
	
	glBegin(GL_POLYGON);
	glNormal3fv(dirVec);
	for (int k = 0; k<6; k++){
		glVertex3fv(startVertex+3*k);
	}
	glEnd();
	
			
	//calc for endpoints:
	for (i = 0; i<6; i++){
		//testVec and testVec2 are components of point in plane
		vmult(uVec, coses[i], testVec);
		vmult(bVec, sines[i], testVec2);
		//Calc outward normal as a sideEffect..
		//It is the vector sum of x,y components (norm 1)
		vadd(testVec, testVec2, nextNormal+3*i);
		//stretch by radius to get current displacement
		vmult(nextNormal+3*i, radius, nextVertex+3*i);
		//add to current point
		vadd(nextVertex+3*i, nextPoint, nextVertex+3*i);		
	}
	
	
	//Now make a triangle strip for cylinder sides:
	glBegin(GL_TRIANGLE_STRIP);
	
	for (i = 0; i< 6; i++){
			
		glNormal3fv(nextNormal+3*i);
		glVertex3fv(nextVertex+3*i);
			
		glNormal3fv(startNormal+3*i);
		glVertex3fv(startVertex+3*i);
	}
	//repeat first two vertices to close cylinder:
		
	glNormal3fv(nextNormal);
	glVertex3fv(nextVertex);
		
	glNormal3fv(startNormal);
	glVertex3fv(startVertex);
	
	glEnd();
	//Now draw the arrow head.  First calc 6 vertices at back of arrowhead
	//Reuse startNormal and startVertex vectors
	//calc for endpoints:
	for (i = 0; i<6; i++){
		//testVec and testVec2 are components of point in plane
		//Can reuse them from previous (cylinder end) calculation
		vmult(uVec, coses[i], testVec);
		vmult(bVec, sines[i], testVec2);
		//Calc outward normal as a sideEffect..
		//It is the vector sum of x,y components (norm 1)
		vadd(testVec, testVec2, startNormal+3*i);
		//stretch by radius to get current displacement
		vmult(startNormal+3*i, ARROW_HEAD_FACTOR*radius, startVertex+3*i);
		//add to current point
		vadd(startVertex+3*i, headCenter, startVertex+3*i);
		//Now tilt normals in direction of arrow:
		for (int k = 0; k<3; k++){
			startNormal[3*i+k] = 0.5*startNormal[3*i+k] + 0.5*dirVec[k];
		}
	}
	//Create a triangle fan from these 6 vertices.  
	glBegin(GL_TRIANGLE_FAN);
	glNormal3fv(dirVec);
	glVertex3fv(vertexPoint);
	for (i = 0; i< 6; i++){
		glNormal3fv(startNormal+3*i);
		glVertex3fv(startVertex+3*i);
	}
	//Repeat first point to close fan:
	glNormal3fv(startNormal);
	glVertex3fv(startVertex);
	glEnd();
}
//Perform the openGL rendering:
void ArrowRenderer::performRendering(const size_t min_bdim[3], const size_t max_bdim[3], int actualRefLevel,
									 float vectorLengthScale, float rad, float* variableData[4]){

	ArrowParams* aParams = (ArrowParams*)currentRenderParams;
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	
	const vector<double> rExtents = aParams->GetRakeExtents();
	const vector<long> rGrid = aParams->GetRakeGrid();
	int rakeGrid[3];
	double rakeExts[6];
	for (int i = 0; i<3; i++) {
		rakeGrid[i] = rGrid[i];
		rakeExts[i] = rExtents[i];
		rakeExts[i+3] = rExtents[i+3];
	}
	bool is3D = aParams->VariablesAre3D();

	//If grid is aligned to data, must recalculate the rake grid and extents.
	//(rakeExts[0],rakeExts[1]) is the starting corner, 
	//(rakeExts[3],rakeExts[4]) is the ending corner
	// (rakeGrid[0] and rakeGrid[1]) are the grid size that fits in the data with the 
	// prescribed strides
	if (aParams->IsAlignedToData()){
		aParams->calcDataAlignment(rakeExts, rakeGrid);
	}

	//Perform setup of OpenGL transform matrix

	float sceneScaleFactor = 1.f/ViewpointParams::getMaxStretchedCubeSide();
	glScalef(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);
	float* transVec = ViewpointParams::getMinStretchedCubeCoords();
	glTranslatef(-transVec[0],-transVec[1], -transVec[2]);

	//Then obtain stretch factors to use for coordinate mapping
	//Don't apply a glScale to stretch the scene, because that would distort the arrow shape
	const float* scales = DataStatus::getInstance()->getStretchFactors();
	
	//Set up lighting and color
	
	ViewpointParams* vpParams =  myGLWindow->getActiveViewpointParams();
	int nLights = vpParams->getNumLights();
	if (nLights == 0) {
		glDisable(GL_LIGHTING);
	}
	else {
		glShadeModel(GL_SMOOTH);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, aParams->GetConstantColor());
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, vpParams->getExponent());
		//All the geometry will get a white specular color:
		float specColor[4];
		specColor[0]=specColor[1]=specColor[2]=0.8f;
		specColor[3] = 1.f;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
	}
	glColor3fv(aParams->GetConstantColor());
	
	//Loop through the points in the rake grid, find the field vector at each grid point.
	//Render an arrow at each point.

	float dirVec[3], endPoint[3], fltPnt[3];
	double point[3];
	size_t voxCoord[3];
	
	size_t bs[3];
	dataMgr->GetBlockSize(bs,actualRefLevel);
	if (!aParams->IsAlignedToData()){
		for (int i = 0; i<rakeGrid[0]; i++){
			point[0] = (rakeExts[0]+ (0.5+(float)i )* ((rakeExts[3]-rakeExts[0])/(float)rakeGrid[0]));
			
			for (int j = 0; j<rakeGrid[1]; j++){
				point[1]= (rakeExts[1]+(0.5+(float)j )* ((rakeExts[4]-rakeExts[1])/(float)rakeGrid[1]));
					
				for (int k = 0; k<rakeGrid[2]; k++){
					point[2]= (rakeExts[2]+(0.5+(float)k)* ((rakeExts[5]-rakeExts[2])/(float)rakeGrid[2]));
					dataMgr->MapUserToVox((size_t)-1,point,voxCoord,actualRefLevel);
					if (variableData[3]){
						float offset = RegionParams::IndexIn2DData(variableData[3], voxCoord, bs, min_bdim, max_bdim);
						point[2] += offset;
						//Map again for displaced coordinate:
						dataMgr->MapUserToVox((size_t)-1,point,voxCoord,actualRefLevel);
					}
					for (int dim = 0; dim<3; dim++){
						dirVec[dim]=0.f;
						if (variableData[dim]){
							if (is3D)
								dirVec[dim] = RegionParams::IndexIn3DData(variableData[dim],voxCoord,bs,min_bdim, max_bdim);
							else 
								dirVec[dim] = RegionParams::IndexIn2DData(variableData[dim],voxCoord,bs,min_bdim, max_bdim);
						}
						endPoint[dim] = scales[dim]*(point[dim]+vectorLengthScale*dirVec[dim]);
						fltPnt[dim]=(float)(point[dim]*scales[dim]);
					}
					
					drawArrow(fltPnt, endPoint, rad);
				}
			}
		}
	} else {
		for (int i = 0; i<rakeGrid[0]; i++){
			point[0] = rakeExts[0]+ ((double)i )* ((rakeExts[3]-rakeExts[0])/(double)rakeGrid[0]);
			
			for (int j = 0; j<rakeGrid[1]; j++){
				point[1]= (rakeExts[1]+((double)j )* ((rakeExts[4]-rakeExts[1])/(double)rakeGrid[1]));
					
				for (int k = 0; k<rakeGrid[2]; k++){
					point[2]= (rakeExts[2]+(0.5+(float)k)* ((rakeExts[5]-rakeExts[2])/(float)rakeGrid[2]));
					dataMgr->MapUserToVox((size_t)-1,point,voxCoord,actualRefLevel);
					if (variableData[3]){
						float offset = RegionParams::IndexIn2DData(variableData[3], voxCoord, bs, min_bdim, max_bdim);
						point[2] += offset;
						//Map again for displaced coordinate:
						dataMgr->MapUserToVox((size_t)-1,point,voxCoord,actualRefLevel);
					}
					for (int dim = 0; dim<3; dim++){
						dirVec[dim]=0.f;
						if (variableData[dim]){
							if (is3D)
								dirVec[dim] = RegionParams::IndexIn3DData(variableData[dim],voxCoord,bs,min_bdim, max_bdim);
							else 
								dirVec[dim] = RegionParams::IndexIn2DData(variableData[dim],voxCoord,bs,min_bdim, max_bdim);
						}
						endPoint[dim] = scales[dim]*(point[dim]+vectorLengthScale*dirVec[dim]);
						fltPnt[dim]=(float)(point[dim]*scales[dim]);
					}
					
					drawArrow(fltPnt, endPoint, rad);
				}
			}
		}
	}
}
//Prepare data needed for rendering
int ArrowRenderer::
setupVariableData(vector<string>&varnames, float** variableData, double validExts[6], size_t voxExts[6], size_t min_bdim[3], size_t max_bdim[3]){

	ArrowParams* aParams = (ArrowParams*)currentRenderParams;
	size_t timestep = (size_t)myGLWindow->getActiveAnimationParams()->getCurrentFrameNumber();
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();

	//Determine the extents of the data that is needed:
	double rakeExts[6];
	aParams->GetRakeExtents(rakeExts);
	size_t min_dim[3], max_dim[3];
	
	float maxVerticalOffset = 0.f;
	for (int i = 0; i<3; i++){
		string vname = aParams->GetFieldVariableName(i);
		varnames.push_back(vname);
	}
	if (aParams->IsTerrainMapped()){
		int sesvarnum = ds->getSessionVariableNum2D("HGT");
		varnames.push_back("HGT");
		maxVerticalOffset = ds->getDataMax2D(sesvarnum,(int)timestep);
	}
	
	//Determine the region extents needed to include a voxel beyond all the rake vertices at ref level 0.
	
	for (int i = 0; i< 3; i++) {
		validExts[i] = rakeExts[i];
		validExts[i+3] = rakeExts[i+3];
	}
	validExts[5] += maxVerticalOffset;
	size_t dim[3];
	dataMgr->MapUserToVox((size_t)-1, validExts, voxExts);
	dataMgr->MapUserToVox((size_t)-1, validExts+3, voxExts+3);
	
	dataMgr->GetDim(dim,0);
	for (int i = 0; i<3; i++){
		if (voxExts[i] > 0) voxExts[i]--;
		if (voxExts[i+3] < dim[i]-1) voxExts[i+3]++;
	}
	dataMgr->MapVoxToUser((size_t)-1,voxExts, validExts,0);
	dataMgr->MapVoxToUser((size_t)-1,voxExts+3, validExts+3,0);

	//Call RegionParams::PrepareCoordsForRetrieval to get the block extents needed in the GetRegion call
	//It may reduce the refinement level or indicate that the required data is not available.

	int numxforms = aParams->GetRefinementLevel();
	int actualRefLevel = RegionParams::PrepareCoordsForRetrieval(numxforms, timestep, varnames, 
		validExts, validExts+3,
		min_dim, max_dim, 
		min_bdim, max_bdim);
	if (actualRefLevel < 0) {
		SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Arrow data unavailable at timestep %d\n", timestep);
		aParams->setBypass(timestep);
		return -1;
	}

	//Obtain the required data from the DataMgr.  The LOD of the data may need to be reduced.
	
	for (int i = 0; i<3; i++){
		if(varnames[i] == "0") continue;
		int useLOD = aParams->GetCompressionLevel();
		int maxLOD = ds->maxLODPresent(varnames[i],timestep);
		if (maxLOD < useLOD){
			if (!ds->useLowerAccuracy()){
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Arrow data unavailable at LOD %d\n", aParams->GetCompressionLevel());
				aParams->setBypass(timestep);
				for (int k = 0; k<i; i++){
					if (variableData[k])
						dataMgr->UnlockRegion(variableData[k]);
				}
				return -1;
			}
			useLOD = maxLOD;
		}
		variableData[i] = dataMgr->GetRegion(timestep, varnames[i].c_str(),actualRefLevel, useLOD,min_bdim,max_bdim,1);
		if (!variableData[i]){
			SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Arrow data unavailable for %s\n", varnames[i].c_str());
			aParams->setBypass(timestep);
			for (int k = 0; k<i; i++){
				if (variableData[k])
					dataMgr->UnlockRegion(variableData[k]);
			}
			return -1;
		}
	}
	//Obtain the HGT data, if required. Save it in variableData[3]
	
	if (aParams->IsTerrainMapped()){
		int useLOD = aParams->GetCompressionLevel();
		int maxLOD = ds->maxLODPresent("HGT",timestep);
		if (maxLOD >= useLOD){
			variableData[3] = dataMgr->GetRegion(timestep, "HGT", actualRefLevel, useLOD,min_bdim,max_bdim,1);
		} else if (ds->useLowerAccuracy()){
			variableData[3] = dataMgr->GetRegion(timestep, "HGT", actualRefLevel, maxLOD,min_bdim,max_bdim,1);
		} else {
			SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Height data unavailable at LOD %d\n", aParams->GetCompressionLevel());
			aParams->setBypass(timestep);
			variableData[3] = 0;
		}
	}
	return actualRefLevel;
}
