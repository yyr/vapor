//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDrenderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2008
//
//	Description:	Implementation of the twoDrenderer class
//

#include "params.h"
#include "twoDrenderer.h"
#include "animationparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "datastatus.h"
#include "glwindow.h"
#include "glutil.h"

#include "vapor/errorcodes.h"
#include "vapor/DataMgr.h"
#include "vapor/LayeredIO.h"
#include <qgl.h>
#include <qcolor.h>
#include <qapplication.h>
#include <qcursor.h>
#include "renderer.h"
#include "proj_api.h"

using namespace VAPoR;

TwoDRenderer::TwoDRenderer(GLWindow* glw, TwoDParams* pParams )
:Renderer(glw, pParams, "TwoDRenderer")
{
	_twoDid = 0;
	maxXElev = maxYElev = 0;
	elevVert = 0;
	elevNorm = 0;
	numElevTimesteps = DataStatus::getInstance()->getMaxTimestep() + 1;
}


/*
  Release allocated resources
*/

TwoDRenderer::~TwoDRenderer()
{
	if (_twoDid) glDeleteTextures(1, &_twoDid);
	invalidateElevGrid();
}


// Perform the rendering
//
  

void TwoDRenderer::paintGL()
{
	
	AnimationParams* myAnimationParams = myGLWindow->getActiveAnimationParams();
	TwoDParams* myTwoDParams = (TwoDParams*)currentRenderParams;
	
	int currentFrameNum = myAnimationParams->getCurrentFrameNumber();
	
	
	unsigned char* twoDTex = 0;
	
	if (myTwoDParams->twoDIsDirty(currentFrameNum)){
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		twoDTex = getTwoDTexture(myTwoDParams,currentFrameNum,true);
		QApplication::restoreOverrideCursor();
		myGLWindow->setRenderNew();
	} else { //existing texture is OK:
		twoDTex = getTwoDTexture(myTwoDParams,currentFrameNum,true);
	}
	if (myTwoDParams->elevGridIsDirty()){
		invalidateElevGrid();
		myTwoDParams->setElevGridDirty(false);
		myGLWindow->setRenderNew();
	}
	int imgSize[2];
	myTwoDParams->getTextureSize(imgSize);
	int imgWidth = imgSize[0];
	int imgHeight = imgSize[1];
	if (twoDTex){
		if(myTwoDParams->imageCrop()) enableFullClippingPlanes();
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glBindTexture(GL_TEXTURE_2D, _twoDid);
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);// will not correct blending, but will be OK wrt other opaque geometry.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth,imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, twoDTex);

		//Do write to the z buffer
		glDepthMask(GL_TRUE);
		
	} else {
		
		return;
		//Don't write to the z-buffer, so won't obscure stuff behind that shows up later
		glDepthMask(GL_FALSE);
		glColor4f(.8f,.8f,0.f,0.2f);
	}
	//Draw elevation grid if we are mapped to terrain, or if
	//It's in georeferenced image mode:
	if ((!myTwoDParams->isDataMode() && myTwoDParams->isGeoreferenced()) || myTwoDParams->isMappedToTerrain()){
		drawElevationGrid(currentFrameNum);
		return;
	}

	//Following code renders a textured rectangle inside the 2D extents.
	float corners[8][3];
	myTwoDParams->calcBoxCorners(corners, 0.f);
	for (int cor = 0; cor < 8; cor++)
		ViewpointParams::worldToStretchedCube(corners[cor],corners[cor]);
	
	if (!myTwoDParams->isDataMode()) {
		//determine the corners of the textured plane.
		//If it's X-Y (orientation = 2) then use first 4. (2nd bit = 0)
		//If it's X-Z (orientation = 1) then use 0,1,4,5 (1st bit = 0)
		//If it's Y-Z (orientation = 0) then use even numbered (0th bit = 0)
		float rectCorners[4][3];
		int orient = myTwoDParams->getOrientation();
		//Copy the corners that will be the corners of the image: 
		int cor1 = 0;
		for(int rectCor = 0; rectCor< 4; rectCor++){
			for(int j=0; j<3; j++){
				rectCorners[rectCor][j] = corners[cor1][j];
			}
			cor1++; //always increment at least 1
			if (orient == 0) cor1++; //increment 2 each time
			if (orient == 1 && rectCor == 1) cor1 +=2; //increment 3 the 2nd time
		}

		//Now reorder based on rotation and inversion.
		//Rotation by 90, 180, 270 advances the vertex index by 1,2,3 (mod 4).
		//Inversion swaps first 2 and second 2.
		//Put the results back in the first four values of the original corners array:
		int ordering = myTwoDParams->getImagePlacement();
		
		for (int i = 0; i< 4; i++){
			//strange ordering for rotation:
			//bitrev, then add 2 to rotate 90 degrees
			//bitrev swaps 1 and 2
			int sourceIndex = i;
			for (int k = 0; k<ordering; k++){
				int bitrevIndex = sourceIndex;
				if (bitrevIndex == 2 || bitrevIndex == 1) bitrevIndex = 3 - bitrevIndex;
				sourceIndex = (bitrevIndex+2)%4;
			}

			if (ordering>3) //inversion
				sourceIndex+= 2;
			for(int j=0; j<3; j++){
				corners[i][j] = rectCorners[sourceIndex%4][j];
			}
		}

	}
	//Draw the textured rectangle:
	glBegin(GL_QUADS);
	glTexCoord2f(0.f,0.f); glVertex3fv(corners[0]);
	glTexCoord2f(0.f, 1.f); glVertex3fv(corners[2]);
	glTexCoord2f(1.f,1.f); glVertex3fv(corners[3]);
	glTexCoord2f(1.f, 0.f); glVertex3fv(corners[1]);
	
	glEnd();
	
	glFlush();
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	if (twoDTex) glDisable(GL_TEXTURE_2D);
	disableFullClippingPlanes();
}


/*
  Set up the OpenGL rendering state, 
*/

void TwoDRenderer::initializeGL()
{
	myGLWindow->makeCurrent();
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
	glGenTextures(1, &_twoDid);
	glBindTexture(GL_TEXTURE_2D, _twoDid);
	initialized = true;
}
//Static method to calculate the twoD texture 
unsigned char* TwoDRenderer::getTwoDTexture(TwoDParams* pParams, int frameNum,  bool doCache){
	if (!pParams->twoDIsDirty(frameNum)) 
		return pParams->getCurrentTwoDTexture(frameNum);

	return pParams->calcTwoDDataTexture(frameNum, 0,0);
	
}
//Draw an elevation grid (surface) inside the current region extents.
//Texture is already specified
void TwoDRenderer::drawElevationGrid(size_t timeStep){
	//If the twoD is dirty, or the timestep has changed, must rebuild.
	TwoDParams* tParams = (TwoDParams*)currentRenderParams;
	//First, check if we have already constructed the elevation grid vertices.
	//If not, rebuild them:
	if (!elevVert || !elevVert[timeStep]) {
		//Don't try to rebuild if we failed already..
		if (doBypass(timeStep)) return;
		if (tParams->isDataMode()){
			if(!rebuildElevationGrid(timeStep)) return;
		} else {//image mode:
			if(!rebuildImageGrid(timeStep)) return;
		}

	}
	int maxx = maxXElev[timeStep];
	int maxy = maxYElev[timeStep];
	if (maxx < 2 || maxy < 2) return;
	float firstx = minXTex[timeStep];
	float firsty = minYTex[timeStep];
	float lastx = maxXTex[timeStep];
	float lasty = maxYTex[timeStep];
	//Establish clipping planes:
	GLdouble topPlane[] = {0., -1., 0., 1.};
	GLdouble rightPlane[] = {-1., 0., 0., 1.0};
	GLdouble leftPlane[] = {1., 0., 0., 0.};
	GLdouble botPlane[] = {0., 1., 0., 0.};
	GLdouble frontPlane[] = {0., 0., -1., 1.};//z largest
	GLdouble backPlane[] = {0., 0., 1., 0.};
	//Apply a coord transform that moves the full domain to the unit cube.
	
	glPushMatrix();
	
	//scale:
	float sceneScaleFactor = 1.f/ViewpointParams::getMaxStretchedCubeSide();
	glScalef(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);

	//translate to put origin at corner:
	float* transVec = ViewpointParams::getMinStretchedCubeCoords();
	glTranslatef(-transVec[0],-transVec[1], -transVec[2]);
	
	//Set up clipping planes
	const float* scales = DataStatus::getInstance()->getStretchFactors();
	
	const float* extents = DataStatus::getInstance()->getExtents();

	topPlane[3] = tParams->getTwoDMax(1)*scales[1];
	botPlane[3] = -tParams->getTwoDMin(1)*scales[1];
	leftPlane[3] = -tParams->getTwoDMin(0)*scales[0];
	rightPlane[3] = tParams->getTwoDMax(0)*scales[0];
	frontPlane[3] = extents[5]*scales[2];
	backPlane[3] = -extents[2]*scales[2];

	
	glClipPlane(GL_CLIP_PLANE0, topPlane);
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE1, rightPlane);
	glEnable(GL_CLIP_PLANE1);
	glClipPlane(GL_CLIP_PLANE2, botPlane);
	glEnable(GL_CLIP_PLANE2);
	glClipPlane(GL_CLIP_PLANE3, leftPlane);
	glEnable(GL_CLIP_PLANE3);
	glClipPlane(GL_CLIP_PLANE4, frontPlane);
	glEnable(GL_CLIP_PLANE4);
	glClipPlane(GL_CLIP_PLANE5, backPlane);
	glEnable(GL_CLIP_PLANE5);

	glPopMatrix();
	//Set up  color
	float elevGridColor[4];
	elevGridColor[0] = 1.f;
	elevGridColor[1] = 1.f;
	elevGridColor[2] = 1.f;
	elevGridColor[3] = 1.f;
	ViewpointParams* vpParams = myGLWindow->getActiveViewpointParams();
	int nLights = vpParams->getNumLights();
	
	glShadeModel(GL_SMOOTH);
	if (nLights >0){
		glEnable(GL_LIGHTING);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, elevGridColor);
	} else {
		glDisable(GL_LIGHTING);
		glColor3fv(elevGridColor);
	}
	
	//texture is always enabled
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	//Put an identity on the Texture matrix stack.
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D, _twoDid);
	
	
	//Now we can just traverse the elev grid, one row at a time.
	//However, special case for start and end quads
	
	float deltax, deltay;
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (firsty == 0.f && lasty == 1.f) deltay = 1.f/(float)(maxy-1);
	else if (firsty == 0.f || lasty == 1.f) deltay = (lasty-firsty)/(float)(maxy -2);
	else deltay = (lasty-firsty)/(float)(maxy - 3);
	if (firstx == 0.f && lastx == 1.f) deltax = 1.f/(float)(maxx-1);
	else if (firstx == 0.f || lastx == 1.f) deltax = (lastx-firstx)/(float)(maxx -2);
	else deltax = (lastx-firstx)/(float)(maxx - 3);

	float tcrdy, tcrdyp, tcrdx;
	for (int j = 0; j< maxy-1; j++){
		
		if (firsty > 0) {
			if (j == 0) {
				tcrdy = 0.f;
				tcrdyp = firsty;
			} else {
				tcrdy = firsty + (j-1)*deltay;
				tcrdyp = firsty + j*deltay;
			} 
		} else {
			tcrdy = j*deltay;
			tcrdyp = (j+1)*deltay;
		}
		if (lasty < 1.f && (j == (maxy -2))){
			tcrdy = lasty;
			tcrdyp = 1.f;
		}

		glBegin(GL_QUAD_STRIP);
		for (int i = 0; i< maxx; i++){
			if (firstx > 0) {
				if (i == 0) {
					tcrdx = 0.f;
				} else {
					tcrdx = firstx + (i-1)*deltax;
				} 
			} else {
				tcrdx = i*deltax;
			}
			if (lastx < 1.f && (i == (maxx -1))){
				tcrdx = 1.f;
			}
			//Each quad is described by sending 4 vertices, i.e. the points indexed by
			//by (i,j+1), (i,j), (i+1,j+1), (i+1,j).  Only send 2 at a time.
			//

			glNormal3fv(elevNorm[timeStep]+3*(i+(j+1)*maxx));
			glTexCoord2f(tcrdx,tcrdyp);
			glVertex3fv(elevVert[timeStep]+3*(i+(j+1)*maxx));
			
			glNormal3fv(elevNorm[timeStep]+3*(i+j*maxx));
			glTexCoord2f(tcrdx,tcrdy);
			glVertex3fv(elevVert[timeStep]+3*(i+j*maxx));
			
		}
		glEnd();
	}
	
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);
	
	glDisable(GL_LIGHTING);
	//undo gl state changes
	
	glDepthMask(GL_FALSE);
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	

	printOpenGLError();
}


//Invalidate array.  Set pointers to zero before deleting so we
//can't accidentally get trapped with bad pointer.
void TwoDRenderer::invalidateElevGrid(){
	
	if (elevVert){
		for (int i = 0; i<numElevTimesteps; i++){
			if (elevVert[i]){
				float * elevPtr = elevVert[i];
				elevVert[i] = 0;
				delete elevPtr;
				delete elevNorm[i];
				elevNorm[i] = 0;
			}
		}
		float ** tmpArray = elevVert;
		elevVert = 0;
		delete tmpArray;
		delete elevNorm;
		elevNorm = 0;
		delete minXTex;
		delete minYTex;
		delete maxXTex;
		delete maxYTex;
		delete maxXElev;
		delete maxYElev;
	}
}
bool TwoDRenderer::rebuildElevationGrid(size_t timeStep){
	//Reconstruct the elevation grid.
	//First, check that the cache is OK:
	if (!elevVert){
		numElevTimesteps = DataStatus::getInstance()->getMaxTimestep() + 1;
		elevVert = new float*[numElevTimesteps];
		elevNorm = new float*[numElevTimesteps];
		maxXElev = new int[numElevTimesteps];
		maxYElev = new int[numElevTimesteps];
		minXTex = new float[numElevTimesteps];
		minYTex = new float[numElevTimesteps];
		maxXTex = new float[numElevTimesteps];
		maxYTex = new float[numElevTimesteps];
		for (int i = 0; i< numElevTimesteps; i++){
			elevVert[i] = 0;
			elevNorm[i] = 0;
			maxXElev[i] = 0;
			maxYElev[i] = 0;
			minXTex[i] = 0.f;
			maxXTex[i] = 1.f;
			minYTex[i] = 0.f;
			maxYTex[i] = 1.f;
		}
	}

	//find the grid coordinate ranges
	size_t min_dim[3], max_dim[3], min_bdim[3],max_bdim[3];
	double regMin[3],regMax[3];
	
	DataStatus* ds = DataStatus::getInstance();
	const float* extents = ds->getExtents();
	//See if there is a HGT variable
	
	float* elevData = 0;
	float* hgtData = 0;
	DataMgr* dataMgr = ds->getDataMgr();
	LayeredIO* myReader = (LayeredIO*)dataMgr->GetRegionReader();
	TwoDParams* tParams = (TwoDParams*) currentRenderParams;
	float displacement = tParams->getVerticalDisplacement();
	int varnum = DataStatus::getSessionVariableNum2D("HGT");
	
	
	double origRegMin[2], origRegMax[2];
	for (int i = 0; i< 2; i++){
		regMin[i] = tParams->getTwoDMin(i);
		regMax[i] = tParams->getTwoDMax(i);
		origRegMin[i] = regMin[i];
		origRegMax[i] = regMax[i];
		//Test for empty box:
		if (regMax[i] <= regMin[i]){
			maxXElev[timeStep] = 0;
			maxYElev[timeStep] = 0;
			return false;
		}

	}
	
	regMin[2] = extents[2];
	regMax[2] = extents[5];
	//Convert to voxels:
	
	int elevGridRefLevel = tParams->getNumRefinements();
	//Do mapping to voxel coords at current ref level:
	myReader->MapUserToVox((size_t)-1, regMin, min_dim, elevGridRefLevel);
	myReader->MapUserToVox((size_t)-1, regMax, max_dim, elevGridRefLevel);
	//Convert back to user coords:
	myReader->MapVoxToUser((size_t)-1, min_dim, regMin, elevGridRefLevel);
	myReader->MapVoxToUser((size_t)-1, max_dim, regMax, elevGridRefLevel);
	//Extend by 1 voxel in x and y if it is smaller than original domain
	for (int i = 0; i< 2; i++){
		if(regMin[i] > origRegMin[i] && min_dim[i]>0) min_dim[i]--;
		if(regMax[i] < origRegMax[i] && max_dim[i]< ds->getFullSizeAtLevel(elevGridRefLevel,i)-1)
			max_dim[i]++;
	}
	
	//Convert increased vox dims to user coords:
	myReader->MapVoxToUser((size_t)-1, min_dim, regMin, elevGridRefLevel);
	myReader->MapVoxToUser((size_t)-1, max_dim, regMax, elevGridRefLevel);
	//Don't allow the terrain surface to be below the minimum extents:
	float minElev = extents[2]+(0.0001)*(extents[5] - extents[2]);
	
	//Try to get requested refinement level or the nearest acceptable level:
	int refLevel = RegionParams::shrinkToAvailableVoxelCoords(elevGridRefLevel, min_dim, max_dim, min_bdim, max_bdim, 
			timeStep, &varnum, 1, regMin, regMax, true);
	
	if(refLevel < 0) {
		setBypass(timeStep);
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, "Terrain elevation data unavailable \nfor 2D rendering at timestep %d",timeStep);
		myReader->SetInterpolateOnOff(true);
		return false;
	}
	
	
	//Ignore vertical extents
	min_dim[2] = max_dim[2] = 0;
	min_bdim[2] = max_bdim[2] = 0;
	//Then, ask the Datamgr to retrieve the HGT data
	
	hgtData = dataMgr->GetRegion(timeStep, "HGT", refLevel, min_bdim, max_bdim, 0);
	
	if (!hgtData) {
		setBypass(timeStep);
		if (ds->warnIfDataMissing()){
			SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"HGT data unavailable at timestep %d.", 
				timeStep);
		}
		ds->setDataMissing2D(timeStep, refLevel, ds->getSessionVariableNum2D(std::string("HGT")));
		
		return false;
	}
	
	//Then create arrays to hold the vertices and their normals:
	//Make each size be grid size + 2. 
	int maxx = maxXElev[timeStep] = max_dim[0] - min_dim[0] +1;
	int maxy = maxYElev[timeStep] = max_dim[1] - min_dim[1] +1;
	elevVert[timeStep] = new float[3*maxx*maxy];
	elevNorm[timeStep] = new float[3*maxx*maxy];

	float deltax = (regMax[0]-regMin[0])/(maxx-1); 
	float deltay = (regMax[1]-regMin[1])/(maxy-1); 

	//set minTex, maxTex values less than deltax, deltay so that the texture will
	//map exactly to the original region bounds

	if (regMin[0] < origRegMin[0] && regMax[0] > origRegMax[0]){
		minXTex[timeStep] = (regMin[0]+deltax - origRegMin[0])/(origRegMax[0]-origRegMin[0]);
		maxXTex[timeStep] = minXTex[timeStep] + deltax*(maxx-3)/(origRegMax[0]-origRegMin[0]);
	} else if (regMin[0] < origRegMin[0]){
		minXTex[timeStep] = (regMin[0]+deltax - origRegMin[0])/(origRegMax[0]-origRegMin[0]);
		maxXTex[timeStep] = 1.f;
	} else if (regMax[0] > origRegMax[0]){
		minXTex[timeStep] = 0.f;
		maxXTex[timeStep] = deltax*(maxx-2)/(origRegMax[0]-origRegMin[0]);
	} else {
		minXTex[timeStep] = 0.f;
		maxXTex[timeStep] = 1.f;
	}
	assert (maxXTex[timeStep] <= 1.f && maxXTex[timeStep] >= 0.f);
	assert (minXTex[timeStep] <= 1.f && minXTex[timeStep] >= 0.f);
	if (regMin[1] < origRegMin[1] && regMax[1] > origRegMax[1]){
		minYTex[timeStep] = (regMin[1]+deltay - origRegMin[1])/(origRegMax[1]-origRegMin[1]);
		maxYTex[timeStep] =  minYTex[timeStep] + deltay*(maxy-3)/(origRegMax[1]-origRegMin[1]);
	} else if (regMin[1] < origRegMin[1]){
		minYTex[timeStep] = (regMin[1]+deltay - origRegMin[1])/(origRegMax[1]-origRegMin[1]);
		maxYTex[timeStep] = 1.f;
	} else if (regMax[1] > origRegMax[1]){
		minYTex[timeStep] = 0.f;
		maxYTex[timeStep] = deltay*(maxy-2)/(origRegMax[1]-origRegMin[1]);
	} else {
		minYTex[timeStep] = 0.f;
		maxYTex[timeStep] = 1.f;
	}
	assert (maxYTex[timeStep] <= 1.f && maxYTex[timeStep] >= 0.f);
	assert (minYTex[timeStep] <= 1.f && minYTex[timeStep] >= 0.f);


	//Then loop over all the vertices in the Elevation or HGT data. 
	//For each vertex, construct the corresponding 3d point as well as the normal vector.
	//These must be converted to
	//stretched cube coordinates.  The x and y coordinates are determined by
	//scaling them to the extents. (Need to know the actual min/max stretched cube extents
	//that correspond to the min/max grid extents)
	//The z coordinate is taken from the data array, converted to 
	//stretched cube coords
	//using parameters in the viewpoint params.
	
	float minvals[3], maxvals[3];
	for (int i = 0; i< 3; i++){
		minvals[i] = 1.e30;
		maxvals[i] = -1.e30;
	}
	float worldCoord[3];
	const size_t* bs = ds->getMetadata()->GetBlockSize();
	for (int j = 0; j<maxy; j++){
		worldCoord[1] = regMin[1] + (float)j*deltay;
		if (worldCoord[1] < origRegMin[1]) worldCoord[1] = origRegMin[1];
		if (worldCoord[1] > origRegMax[1]) worldCoord[1] = origRegMax[1];
		size_t ycrd = (min_dim[1] - bs[1]*min_bdim[1]+j)*(max_bdim[0]-min_bdim[0]+1)*bs[0];
			
		for (int i = 0; i<maxx; i++){
			int pntPos = 3*(i+j*maxx);
			worldCoord[0] = regMin[0] + (float)i*deltax;
			if (worldCoord[0] < origRegMin[0]) worldCoord[0] = origRegMin[0];
			if (worldCoord[0] > origRegMax[0]) worldCoord[0] = origRegMax[0];
			size_t xcrd = min_dim[0] - bs[0]*min_bdim[0]+i;
			if (elevData)
				worldCoord[2] = elevData[xcrd+ycrd] + displacement;
			else
				worldCoord[2] = hgtData[xcrd+ycrd] + displacement;
			if (worldCoord[2] < minElev) worldCoord[2] = minElev;
			//Convert and put results into elevation grid vertices:
			ViewpointParams::worldToStretchedCube(worldCoord,elevVert[timeStep]+pntPos);
			for (int k = 0; k< 3; k++){
				if( *(elevVert[timeStep] + pntPos+k) > maxvals[k])
					maxvals[k] = *(elevVert[timeStep] + pntPos+k);
				if( *(elevVert[timeStep] + pntPos+k) < minvals[k])
					minvals[k] = *(elevVert[timeStep] + pntPos+k);
			}
		}
	}
	qWarning("min,max coords: %f %f %f %f %f %f",
		minvals[0],minvals[1],minvals[2],maxvals[0],maxvals[1],maxvals[2]);
	//Now calculate normals:
	calcElevGridNormals(timeStep);
	return true;
}
bool TwoDRenderer::rebuildImageGrid(size_t timeStep){
	//Reconstruct the elevation grid using the map projection specified
	//It may or may not be terrain following.  
	//The following code assumes horizontal orientation and a defined
	//map projection.
	//First, set up the cache if necessary:
	if (!elevVert){
		numElevTimesteps = DataStatus::getInstance()->getMaxTimestep() + 1;
		//Following arrays hold vertices and normals
		elevVert = new float*[numElevTimesteps];
		elevNorm = new float*[numElevTimesteps];
		//maxXelev, maxYelev are the dimensions of the elevation grid
		maxXElev = new int[numElevTimesteps];
		maxYElev = new int[numElevTimesteps];
		//minXTex, minYTex are min/max texture coordinates (between 0 and 1).
		//tex coords will be uniformly spaced (only the vertex coords are moved)
		minXTex = new float[numElevTimesteps];
		minYTex = new float[numElevTimesteps];
		maxXTex = new float[numElevTimesteps];
		maxYTex = new float[numElevTimesteps];
		for (int i = 0; i< numElevTimesteps; i++){
			elevVert[i] = 0;
			elevNorm[i] = 0;
			maxXElev[i] = 0;
			maxYElev[i] = 0;
			minXTex[i] = 0.f;
			maxXTex[i] = 1.f;
			minYTex[i] = 0.f;
			maxYTex[i] = 1.f;
		}
	}

	//Specify the parameters that are needed to define the elevation grid:

	

	//Determine the grid size, the data extents, and the image size:
	DataStatus* ds = DataStatus::getInstance();
	const float* extents = ds->getExtents();
	DataMgr* dataMgr = ds->getDataMgr();
	LayeredIO* myReader = (LayeredIO*)dataMgr->GetRegionReader();
	TwoDParams* tParams = (TwoDParams*) currentRenderParams;
	const float* imgExts = tParams->getCurrentTwoDImageExtents(timeStep);
	int refLevel = tParams->getNumRefinements();

	//Find the corners of the image (to find size of image in scene.
	//Get these relative to actual extents in projection space
	double displayCorners[8];
	if (!tParams->getImageCorners(timeStep,displayCorners))
		return false;
	
	int gridsize[2];
	for (int i = 0; i<2; i++){
		gridsize[i] = (int)(ds->getFullSizeAtLevel(refLevel,i)*
			(displayCorners[i+4]-displayCorners[i])/(extents[i+3]-extents[i])+0.5f);
	}
	
	// intersect the twoD box with the imageExtents.  This will
	// determine the region used for the elevation grid.  
	// Construct
	// an elevation grid that covers the full image.  Later may
	// decide it's more efficient to construct a smaller one.
	// Need to determine the approximate size of the image, to decide
	// how large a grid to use. 

	
	//Set up proj.4:
	projPJ dst_proj;
	projPJ src_proj; 
	
	src_proj = pj_init_plus(tParams->getImageProjectionString().c_str());
	dst_proj = pj_init_plus(DataStatus::getProjectionString().c_str());
	bool doProj = (src_proj != 0 && dst_proj != 0);
	if (!doProj) return false;

	//If a projection string is latlon, the coordinates are in Radians!
	bool latlonSrc = pj_is_latlong(src_proj);
	bool latlonDst = pj_is_latlong(dst_proj);

	static const double RAD2DEG = 180./M_PI;
	static const double DEG2RAD = M_PI/180.0;
	
	
	int maxx, maxy;
	maxXElev[timeStep] = maxx = gridsize[0] +1;
	maxYElev[timeStep] = maxy = gridsize[1] +1;
	
	elevVert[timeStep] = new float[3*maxx*maxy];
	elevNorm[timeStep] = new float[3*maxx*maxy];
	
	//texture coordinate range goes from 0 to 1
	minXTex[timeStep] = 0.f;
	maxXTex[timeStep] = 1.f;
	minYTex[timeStep] = 0.f;
	maxYTex[timeStep] = 1.f;

	
	float minvals[3], maxvals[3];
	for (int i = 0; i< 3; i++){
		minvals[i] = 1.e30;
		maxvals[i] = -1.e30;
	}

	//If data is not mapped to terrain, elevation is determined
	//by twoDParams z (max or min are same).  
	//If data is mapped to terrain, but we are outside data, then
	//take elevation to be the min (which is just vert displacement)
	float constElev = tParams->getTwoDMin(2);

	//Set up for doing terrain mapping:
	size_t min_dim[3], max_dim[3], min_bdim[3], max_bdim[3];
	double regMin[3], regMax[3];
	float* hgtData;
	float horizFact, vertFact, horizOffset, vertOffset, minElev;
	const size_t* bs = ds->getMetadata()->GetBlockSize();
	float displacement = tParams->getVerticalDisplacement();
	if (tParams->isMappedToTerrain()){
		//We shall retrieve HGT for the full extents of the data
		//at the current refinement level.
		for (int i = 0; i<3; i++){
			min_dim[i] = 0;
			max_dim[i] = ds->getFullSizeAtLevel(refLevel,i) - 1;
		}
		//Convert to user coords in non-moving extents:
		myReader->MapVoxToUser((size_t)-1, min_dim, regMin, refLevel);
		myReader->MapVoxToUser((size_t)-1, max_dim, regMax, refLevel);

		int varnum = DataStatus::getSessionVariableNum2D("HGT");
		//Try to get requested refinement level or the nearest acceptable level:
		int refLevel1 = RegionParams::shrinkToAvailableVoxelCoords(refLevel, min_dim, max_dim, min_bdim, max_bdim, 
				timeStep, &varnum, 1, regMin, regMax, true);
		if(refLevel1 < 0) {
			setBypass(timeStep);
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, "Terrain elevation data unavailable \nfor 2D rendering at timestep %d",timeStep);
			myReader->SetInterpolateOnOff(true);
			return false;
		}
		//Make sure the region is nonempty:
		if (min_dim[0]>= max_dim[0] || min_dim[1]>= max_dim[1]) return false;
		
		//Ignore vertical extents
		min_dim[2] = max_dim[2] = 0;
		min_bdim[2] = max_bdim[2] = 0;
		//Then, ask the Datamgr to retrieve the HGT data
		
		hgtData = dataMgr->GetRegion(timeStep, "HGT", refLevel, min_bdim, max_bdim, 0);
		
		if (!hgtData) {
			setBypass(timeStep);
			if (ds->warnIfDataMissing()){
				SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"HGT data unavailable at timestep %d.", 
					timeStep);
			}
			ds->setDataMissing2D(timeStep, refLevel, ds->getSessionVariableNum2D(std::string("HGT")));
			return false;
		}


		//Linear Conversion terms between user coords and grid coords;
		
		//regMin maps to reg_min, regMax to max_dim 
		horizFact = ((double)(max_dim[0] - min_dim[0]))/(regMax[0] - regMin[0]);
		vertFact = ((double)(max_dim[1] - min_dim[1]))/(regMax[1] - regMin[1]);
		horizOffset = min_dim[0]  - regMin[0]*horizFact;
		vertOffset = min_dim[1]  - regMin[1]*vertFact;

		//Don't allow the terrain surface to be below the minimum extents:
		minElev = extents[2]+(0.0001)*(extents[5] - extents[2]);
		

	}
	
	//Use a line buffer to hold 3d coordinates for transforming
	double* elevVertLine = new double[3*maxx];
	float locCoords[3];
	const float* timeVaryingExtents = DataStatus::getExtents(timeStep);
	
	for (int j = 0; j<maxy; j++){
		
		for (int i = 0; i<maxx; i++){
			
			//determine the x,y coordinates of each point
			//in the image coordinate space:
			//int pntPos = 3*(i+j*maxx);
			elevVertLine[3*i] = imgExts[0] + (imgExts[2]-imgExts[0])*(double)i/(double)(maxx-1);
			elevVertLine[3*i+1] = imgExts[1] + (imgExts[3]-imgExts[1])*(double)j/(double)(maxy-1);
			
			//ViewpointParams::worldToStretchedCube(worldCoord,elevVert[timeStep]+pntPos);
		}
		
		//apply proj4 to transform the line.  If source is in degrees, convert to radians:
		if (latlonSrc)
			for(int i = 0; i< maxx; i++) {
				elevVertLine[3*i] *= DEG2RAD;
				elevVertLine[3*i+1] *= DEG2RAD;
			}
		pj_transform(src_proj,dst_proj,maxx,3, elevVertLine,elevVertLine+1, 0);
		//If the scene is latlon, convert to degrees:
		if (latlonDst) 
			for(int i = 0; i< maxx; i++) {
				elevVertLine[3*i] *= RAD2DEG;
				elevVertLine[3*i+1] *= RAD2DEG;
			}
		//Copy the result back to elevVert. 
		//Translate by local offset
		//then convert to stretched cube coords
		//The coordinates are in projection space, which is associated
		//with the moving extents.  Need to get them back into the
		//local (non-moving) extents for rendering:
		
		for (int i = 0; i< maxx; i++){
			locCoords[0] = (float)elevVertLine[3*i] + extents[0]-timeVaryingExtents[0];
			locCoords[1] = (float)elevVertLine[3*i+1]+ extents[1]-timeVaryingExtents[1];
			if (tParams->isMappedToTerrain()){
				//Find cell coordinates of locCoords in data grid space
				int gridLL[2];
				float fltGridLL[2];
				fltGridLL[0] = (locCoords[0]*horizFact + horizOffset);
				fltGridLL[1] = (locCoords[1]*vertFact + vertOffset);
				gridLL[0] = (int)fltGridLL[0];
				gridLL[1] = (int)fltGridLL[1];
				
				//check that all 4 corners are inside valid grid coordinates:
				if (gridLL[0] < min_dim[0] || gridLL[1] < min_dim[1] || gridLL[0] > max_dim[0]-2 ||
						gridLL[1] > (max_dim[1] -2)){ 
					//outside points go to minimum elevation
					locCoords[2] = constElev;
				} else { 
					//find locCoords [0..1]relative to grid cell:???
					float x = (fltGridLL[0] - gridLL[0]);
					float y = (fltGridLL[1] - gridLL[1]);
					//To get elevations at corners, need grid coordinates:
					size_t xcrd = min_dim[0] - bs[0]*min_bdim[0]+gridLL[0];
					size_t ycrd = (min_dim[1] - bs[1]*min_bdim[1]+gridLL[1])*(max_bdim[0]-min_bdim[0]+1)*bs[0];
					size_t ycrdP1 = (min_dim[1] - bs[1]*min_bdim[1]+gridLL[1]+1)*(max_bdim[0]-min_bdim[0]+1)*bs[0];
					
					
					float elevLL = hgtData[xcrd+ycrd] + displacement;
					float elevLR = hgtData[1+xcrd+ycrd] + displacement;
					float elevUL = hgtData[xcrd+ycrdP1] + displacement;
					float elevUR = hgtData[1+xcrd+ycrdP1] + displacement;
					//Bilinear interpolate:
					locCoords[2] = (1-y)*((1-x)*elevLL + x*elevLR) +
						y*((1-x)*elevUL + x*elevUR);
					if (locCoords[2] < minElev) locCoords[2] = minElev;
				}
			}
			else { //not mapped to terrain, use constant elevation
				locCoords[2] = constElev;
			}
			//Convert to stretched cube coords.  Note that following
			//routine requires local coords, not global world coords, despite name of method:
			ViewpointParams::worldToStretchedCube(locCoords,elevVert[timeStep]+3*(i+j*maxx));
			for (int k = 0; k< 3; k++){
				if( *(elevVert[timeStep] + 3*(i+j*maxx)+k) > maxvals[k])
					maxvals[k] = *(elevVert[timeStep] + 3*(i+j*maxx)+k);
				if( *(elevVert[timeStep] + 3*(i+j*maxx)+k) < minvals[k])
					minvals[k] = *(elevVert[timeStep] + 3*(i+j*maxx)+k);
			}
		
	
		}
		
		//Set the z coordinate using the elevation data:
		//float* elevs = elevData != 0 ? elevData : hgtData;
		//interpHeights(elevVert[timeStep]+3*j*maxx, maxx, maxx, maxy, imgExts, elevs);

	}

	//qWarning("min,max coords: %f %f %f %f %f %f",
	//	minvals[0],minvals[1],minvals[2],maxvals[0],maxvals[1],maxvals[2]);
	
	//Now calculate normals.  For now, just do it as if the points are
	//on a regular grid:
	delete elevVertLine;
	calcElevGridNormals(timeStep);
	return true;
}
//Once the elevation grid vertices are determined, calculate the normals.  Use the stretched
//cube coords in the elevVert array.
//The normal vectors are determined by looking at z-coords of adjacent vertices in both 
//x and y.  Suppose that dzx is the change in z associated with an x-change of dz,
//and that dzy is the change in z associated with a y-change of dy.  Let the normal
//vector be (a,b,c).  Then a/c is equal to dzx/dx, and b/c is equal to dzy/dy.  So
// (a,b,c) is proportional to (dzx*dy, dzy*dx, 1) 
//Must compensate for stretching, since the actual z-differences between
//adjacent vertices will be miniscule
void TwoDRenderer::calcElevGridNormals(size_t timeStep){
	const float* stretchFac = DataStatus::getInstance()->getStretchFactors();
	int maxx = maxXElev[timeStep];
	int maxy = maxYElev[timeStep];
	//Go over the grid of vertices, calculating normals
	//by looking at adjacent x,y,z coords.
	for (int j = 0; j < maxy; j++){
		for (int i = 0; i< maxx; i++){
			float* point = elevVert[timeStep]+3*(i+maxx*j);
			float* norm = elevNorm[timeStep]+3*(i+maxx*j);
			//do differences of right point vs left point,
			//except at edges of grid just do differences
			//between current point and adjacent point:
			float dx=0.f, dy=0.f, dzx=0.f, dzy=0.f;
			if (i>0 && i <maxx-1){
				dx = *(point+3) - *(point-3);
				dzx = *(point+5) - *(point-1);
			} else if (i == 0) {
				dx = *(point+3) - *(point);
				dzx = *(point+5) - *(point+2);
			} else if (i == maxx-1) {
				dx = *(point) - *(point-3);
				dzx = *(point+2) - *(point-1);
			}
			if (j>0 && j <maxy-1){
				dy = *(point+1+3*maxx) - *(point+1 - 3*maxx);
				dzy = *(point+2+3*maxx) - *(point+2 - 3*maxx);
			} else if (j == 0) {
				dy = *(point+1+3*maxx) - *(point+1);
				dzy = *(point+2+3*maxx) - *(point+2);
			} else if (j == maxy-1) {
				dy = *(point+1) - *(point+1 - 3*maxx);
				dzy = *(point+2) - *(point+2 - 3*maxx);
			}
			norm[0] = dy*dzx;
			norm[1] = dx*dzy;
			//Making the following small accentuates the angular differences when lit:
			norm[2] = 1.f/ELEVATION_GRID_ACCENT;
			for (int k = 0; k < 3; k++) norm[k] /= stretchFac[k];
			vnormal(norm);
		}
	}
}
//Interpolate the elevations of an array of points, packed as x,y,z's
//Assumes that an array of elevations is provided, and that array
//is linearly mapped to world elevation extents specified in wElevExtents.
//The dimensions of the height data are elevWid, elevHt
//points that are outside wElevExtents get elevation = defaultElev.

//Also need to handle stretch and displacement of elev grid.
void TwoDRenderer::
interpHeights(float* rowData, int numPts, int elevWid, int elevHt, 
	float defaultElev, const float wElevExtents[4], const float* elevData)
{
	for (int i = 0; i<numPts; i++){
		float x = rowData[i*3];
		float y = rowData[i*3+1];
		//Assume default elevation:
		rowData[i*3+2] = defaultElev;
		//find where it fits relative to exts:
		
		float xgrid = ((x-wElevExtents[0])/(wElevExtents[2]-wElevExtents[0])*elevWid);
		float ygrid = ((y-wElevExtents[1])/(wElevExtents[3]-wElevExtents[1])*elevHt);
		//Get grid corners at lower-left
		int xll = (int)(xgrid);
		int yll = (int)(ygrid);
		if (xll < 0 || yll < 0) continue;
		if(xll > elevWid-2) continue;
		if(yll > elevHt-2) continue;
		//find values at corners:
		float elevll = elevData[xll + yll*elevWid];
		float elevlr = elevData[(xll+1) + yll*elevWid];
		float elevul = elevData[xll + (yll+1)*elevWid];
		float elevur = elevData[xll+1 + (yll+1)*elevWid];
		//Do bilinear interpolate of the four corners 
		//coeffs are (xgrid-x11) and (ygrid-y11)
		float xcoeff = xgrid -xll;
		float ycoeff = ygrid -yll;
		
		rowData[i*3+2] = ((1-ycoeff)*elevll + ycoeff*elevul)*(1-xcoeff) +
			((1-ycoeff)*elevlr + ycoeff*elevur)*xcoeff;
	}

}
