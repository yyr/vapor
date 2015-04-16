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
#define ARROW_HEAD_FACTOR 3.0
// Specify how long the arrow tube is, in front of where the arrowhead is attached:
#define ARROW_LENGTH_FACTOR 0.9

#include "glutil.h"	// Must be included first!!!
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "arrowrenderer.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"
#include "visualizer.h"
#include "params.h"
#include "renderer.h"
#include "arrowparams.h"
#include <vapor/MyBase.h>
#include <vapor/errorcodes.h>
#include <vapor/DataMgrV3_0.h>

using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ArrowRenderer::ArrowRenderer(Visualizer* glw, RenderParams* rp) 
  : Renderer(glw,  rp, "ArrowRenderer")
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ArrowRenderer::~ArrowRenderer()
{
}

int ArrowRenderer::_initializeGL(){
	initialized = true;
	return(0);
}

int ArrowRenderer::_paintGL(DataMgrV3_0* dataMgr){

	
	ArrowParams* aParams = (ArrowParams*)getRenderParams();

	//
	//Set up the variable data required, while determining data extents to use in rendering
	//
	RegularGrid *varData[] = {NULL,NULL,NULL,NULL};
	vector<string>varnames;

	double validExts[6];
	size_t timestep = (size_t)myVisualizer->getActiveAnimationParams()->getCurrentTimestep();
	DataStatus* ds = DataStatus::getInstance();
	
	if (!dataMgr) return -1;
	
	if (doBypass(timestep)) return -1;

	int actualRefLevel = aParams->GetRefinementLevel();
	int lod = aParams->GetCompressionLevel();

	//Determine the extents of the data that is needed:
	double rakeExts[6];
	aParams->GetRakeLocalExtents(rakeExts);
	vector<double>minExts,maxExts;
	int rc = ds->GetExtents(timestep, minExts, maxExts);
	float maxVerticalOffset = 0.;
	for (int i = 0; i<3; i++){
		string vname = aParams->GetFieldVariableName(i);
		varnames.push_back(vname);
	}
	if (aParams->IsTerrainMapped()){
		string hname = aParams->GetHeightVariableName();
		varnames.push_back(hname);
		maxVerticalOffset = ds->getDefaultDataMax(hname);
	}
	
	
	double extents[6];
	float boxexts[6];
	aParams->GetBox()->GetLocalExtents(boxexts);

	for (int i = 0; i<6; i++){
		extents[i] = boxexts[i]+minExts[i];
	}
	extents[5] += maxVerticalOffset;
	
	rc = getGrids( (size_t)timestep, varnames, extents, &actualRefLevel, &lod, varData);
	if(!rc){
		return rc;
	}
	
	//Calculate the scale factors and radius to be used in rendering the arrows:
	//
	const vector<long> rakeGrid = aParams->GetRakeGrid();
	
	float vectorLengthScale = aParams->GetVectorScale();
	//
	//find the horizontal voxel size in user units at highest ref level.  This will be used to scale the vector radius.
	//
	size_t dim[3];
	string varname1;
	for (int i = 0; i<3; i++){
		if (!varData[i]) continue;
		varData[i]->GetDimensions(dim);
		varname1 = varnames[i];
		break;
	}
	//See what the dimensions are at full refinement level
	int totNumRefs = dataMgr->GetNumRefLevels(varname1);
	for (int i = 0; i<3; i++)
		dim[i] *= (2 << (totNumRefs-actualRefLevel+1));
	const double* fullSizes = DataStatus::getInstance()->getFullSizes();
	float maxVoxSize = 0.f;
	for (int i = 0; i<2; i++){
		double voxSide = (fullSizes[i])/(dim[i]);
		if (voxSide > maxVoxSize) maxVoxSize = voxSide;
	}
	if (maxVoxSize == 0.f) maxVoxSize = 1.f;
	float rad = 0.5*maxVoxSize*aParams->GetLineThickness();
	//
	//Perform OpenGL rendering of arrows
	//
	rc = performRendering(dataMgr, aParams, actualRefLevel, vectorLengthScale, rad, varData);
	if(rc) return rc;
	
	//Release the locks on the data:
	for (int k = 0; k<4; k++){
		if (varData[k])
			dataMgr->UnlockGrid(varData[k]);
	}
	return(0);
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
int ArrowRenderer::performRendering(DataMgrV3_0* dataMgr, const RenderParams* params,
	int actualRefLevel, float vectorLengthScale, float rad, 
	RegularGrid *variableData[4]
){

	ArrowParams* aParams = (ArrowParams*)params;
	
	size_t timestep = (size_t)myVisualizer->getActiveAnimationParams()->getCurrentTimestep();
	
	const vector<double> rExtents = aParams->GetRakeLocalExtents();
	//Convert to user coordinates:
	vector<double> minExts,maxExts;
	DataStatus::getInstance()->GetExtents(timestep, minExts,maxExts);

	const vector<long> rGrid = aParams->GetRakeGrid();
	int rakeGrid[3];
	double rakeExts[6];//rake extents in user coordinates

	//If grid is aligned to data, must calculate the rake grid and extents in local coordinates
	//before converting to user coordinates
	//(rakeExts[0],rakeExts[1]) is the starting corner, 
	//(rakeExts[3],rakeExts[4]) is the ending corner
	// (rakeGrid[0] and rakeGrid[1]) are the grid size that fits in the data with the 
	// prescribed strides
	if (aParams->IsAlignedToData()){
		aParams->getDataAlignment(rakeExts, rakeGrid, timestep);
		
	} else {

		for (int i = 0; i<3; i++) {
			rakeGrid[i] = rGrid[i];
			rakeExts[i] = rExtents[i]+minExts[i];
			rakeExts[i+3] = rExtents[i+3]+minExts[i];
		}
	}
	
	//Perform setup of OpenGL transform matrix.  This transforms the full stretched domain into the unit box
	//by scaling and translating.
	//Visualizer->TransformToUnitBox();

	//Then obtain stretch factors to use for coordinate mapping
	//Don't apply a glScale to stretch the scene, because that would distort the arrow shape
	const double* scales = DataStatus::getInstance()->getStretchFactors();
	
	//Set up lighting and color
	
	ViewpointParams* vpParams =  myVisualizer->getActiveViewpointParams();
	int nLights = vpParams->getNumLights();
	float fcolor[3];
	for (int i = 0; i<3; i++) fcolor[i] = (float)aParams->GetConstantColor()[i];
	if (nLights == 0) {
		glDisable(GL_LIGHTING);
	}
	else {
		glShadeModel(GL_SMOOTH);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,fcolor);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, vpParams->getExponent());
		//All the geometry will get a white specular color:
		float specColor[4];
		specColor[0]=specColor[1]=specColor[2]=0.8f;
		specColor[3] = 1.f;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
	}
	glColor3fv(fcolor);
	
	//Loop through the points in the rake grid, find the field vector at each grid point.
	//Render an arrow at each point.

	float dirVec[3], endPoint[3], fltPnt[3];
	double point[3];
	
	if (!aParams->IsAlignedToData()){
		for (int k = 0; k<rakeGrid[2]; k++){
			float pntz= (rakeExts[2]+(0.5+(float)k)* ((rakeExts[5]-rakeExts[2])/(float)rakeGrid[2]));
			
			for (int j = 0; j<rakeGrid[1]; j++){
				point[1]= (rakeExts[1]+(0.5+(float)j )* ((rakeExts[4]-rakeExts[1])/(float)rakeGrid[1]));
					
				for (int i = 0; i<rakeGrid[0]; i++){
					point[0] = (rakeExts[0]+ (0.5+(float)i )* ((rakeExts[3]-rakeExts[0])/(float)rakeGrid[0]));
					bool missing = false;
					float offset = 0.;
					if (variableData[3]){
						offset = variableData[3]->GetValue(
							point[0], point[1], 0.
						);
						if (offset == variableData[3]->GetMissingValue()) {
							missing = true;
							offset = 0.;
						}
					}
					point[2]=pntz+offset;
					for (int dim = 0; dim<3; dim++){
						dirVec[dim]=0.f;
						if (variableData[dim]){
							dirVec[dim] = variableData[dim]->GetValue(point[0], point[1], point[2]+offset);
							if (dirVec[dim] == variableData[dim]->GetMissingValue()) {
								missing = true;
							}
						}
						endPoint[dim] = scales[dim]*(point[dim]+vectorLengthScale*dirVec[dim]);
						fltPnt[dim]=(float)(point[dim]*scales[dim]);
					}
					
					if (! missing) drawArrow(fltPnt, endPoint, rad);
				}
			}
		}
	} else {
		for (int k = 0; k<rakeGrid[2]; k++){
			float pntz= (rakeExts[2]+(0.5+(float)k)* ((rakeExts[5]-rakeExts[2])/(float)rakeGrid[2]));
			
			for (int j = 0; j<rakeGrid[1]; j++){
				point[1]= (rakeExts[1]+((double)j )* ((rakeExts[4]-rakeExts[1])/(double)rakeGrid[1]));
					
				for (int i = 0; i<rakeGrid[0]; i++){
					bool missing = false;
					float offset = 0.;
					point[0] = rakeExts[0]+ ((double)i )* ((rakeExts[3]-rakeExts[0])/(double)rakeGrid[0]);
					if (variableData[3]){
						offset = variableData[3]->GetValue(
							point[0], point[1], 0.
						);
						if (offset == variableData[3]->GetMissingValue()) {
							missing = true;
						}
					}
					point[2]=pntz+offset;
					for (int dim = 0; dim<3; dim++){
						dirVec[dim]=0.f;
						if (variableData[dim]){
							dirVec[dim] = variableData[dim]->GetValue(
								point[0], point[1], point[2]
							);
							if (dirVec[dim] == variableData[dim]->GetMissingValue()) {
								missing = true;
							}
						}
						endPoint[dim] = scales[dim]*(point[dim]+vectorLengthScale*dirVec[dim]);
						fltPnt[dim]=(float)(point[dim]*scales[dim]);
					}
					
					if (! missing) drawArrow(fltPnt, endPoint, rad);
				}
			}
		}
	}
	return 0;
}
