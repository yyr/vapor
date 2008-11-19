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

using namespace VAPoR;

TwoDRenderer::TwoDRenderer(GLWindow* glw, TwoDParams* pParams )
:Renderer(glw, pParams)
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
		enableFullClippingPlanes();
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

	if (myTwoDParams->isMappedToTerrain()){
		drawElevationGrid(currentFrameNum);
		return;
	}

	float corners[8][3];
	myTwoDParams->calcBoxCorners(corners, 0.f);
	for (int cor = 0; cor < 8; cor++)
		ViewpointParams::worldToStretchedCube(corners[cor],corners[cor]);
	
	//determine the corners of the textured plane.
	//the front corners are numbered 4 more than the back.
	//Average the front and back to get the middle:
	//
	float midCorners[4][3];
	for (int i = 0; i<4; i++){
		for(int j=0; j<3; j++){
			midCorners[i][j] = 0.5f*(corners[i][j]+corners[i+4][j]);
		}
	}
	//Draw the textured rectangle:
	glBegin(GL_QUADS);
	glTexCoord2f(0.f,0.f); glVertex3fv(midCorners[0]);
	glTexCoord2f(0.f, 1.f); glVertex3fv(midCorners[2]);
	glTexCoord2f(1.f,1.f); glVertex3fv(midCorners[3]);
	glTexCoord2f(1.f, 0.f); glVertex3fv(midCorners[1]);
	
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
	
	//First, check if we have already constructed the elevation grid vertices.
	//If not, rebuild them:
	if (!elevVert || !elevVert[timeStep]) {
		if(!rebuildElevationGrid(timeStep)) return;
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
	
	TwoDParams* tParams = (TwoDParams*)currentRenderParams;
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
	myReader->MapUserToVox(timeStep, regMin, min_dim, elevGridRefLevel);
	myReader->MapUserToVox(timeStep, regMax, max_dim, elevGridRefLevel);
	//Convert back to user coords:
	myReader->MapVoxToUser(timeStep, min_dim, regMin, elevGridRefLevel);
	myReader->MapVoxToUser(timeStep, max_dim, regMax, elevGridRefLevel);
	//Extend by 1 voxel in x and y if it is smaller than original domain
	for (int i = 0; i< 2; i++){
		if(regMin[i] > origRegMin[i] && min_dim[i]>0) min_dim[i]--;
		if(regMax[i] < origRegMax[i] && max_dim[i]< ds->getFullSizeAtLevel(elevGridRefLevel,i)-1)
			max_dim[i]++;
	}
	
	//Convert increased vox dims to user coords:
	myReader->MapVoxToUser(timeStep, min_dim, regMin, elevGridRefLevel);
	myReader->MapVoxToUser(timeStep, max_dim, regMax, elevGridRefLevel);
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
				timeStep
				 );
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
		}
	}
	//Now calculate normals:
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