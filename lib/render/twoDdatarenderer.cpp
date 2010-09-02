//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDdatarenderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2008
//
//	Description:	Implementation of the TwoDDataRenderer class
//

#include "params.h"
#include "twoDdatarenderer.h"
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

TwoDDataRenderer::TwoDDataRenderer(GLWindow* glw, TwoDDataParams* pParams )
:TwoDRenderer(glw, pParams)
{

}


/*
  Release allocated resources
*/

TwoDDataRenderer::~TwoDDataRenderer()
{
	
}


// Perform the rendering
//
  

void TwoDDataRenderer::paintGL()
{
	
	AnimationParams* myAnimationParams = myGLWindow->getActiveAnimationParams();
	TwoDDataParams* myTwoDParams = (TwoDDataParams*)currentRenderParams;
	
	int currentFrameNum = myAnimationParams->getCurrentFrameNumber();
	
	unsigned char* twoDTex = 0;
	
	if (myTwoDParams->twoDIsDirty(currentFrameNum)){
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		twoDTex = getTwoDTexture(myTwoDParams,currentFrameNum,true);
		QApplication::restoreOverrideCursor();
		if(!twoDTex) {setBypass(currentFrameNum); return;}
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
	myTwoDParams->getTextureSize(imgSize, currentFrameNum);
	int imgWidth = imgSize[0];
	int imgHeight = imgSize[1];
	if (twoDTex){
		if(myTwoDParams->imageCrop()) enableFullClippingPlanes();
		else disableFullClippingPlanes();
		
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		//Bind if the image changed
		if (myTwoDParams->getLastTwoDTexture() != twoDTex){
			glDeleteTextures(1,&_twoDid);
		}
		glBindTexture(GL_TEXTURE_2D, _twoDid);
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);// will not correct blending, but will be OK wrt other opaque geometry.
//		Following is done every render because of interference with Probe ibfv renderer
//      We should fix this to use texture objects properly
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth,imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, twoDTex);
//
		myTwoDParams->setLastTwoDTexture(twoDTex);
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
	if (myTwoDParams->isMappedToTerrain()){
		drawElevationGrid(currentFrameNum);
		return;
	}

	//Following code renders a textured rectangle inside the 2D extents.
	float corners[8][3];
	myTwoDParams->calcBoxCorners(corners, 0.f);
	for (int cor = 0; cor < 8; cor++)
		ViewpointParams::worldToStretchedCube(corners[cor],corners[cor]);
	
	
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

bool TwoDDataRenderer::rebuildElevationGrid(size_t timeStep){
	//Reconstruct the elevation grid.
	

	//find the grid coordinate ranges
	size_t min_dim[3], max_dim[3], min_bdim[3],max_bdim[3];
	double regMin[3],regMax[3];
	
	DataStatus* ds = DataStatus::getInstance();
	const float* extents = ds->getExtents();
	//See if there is a HGT variable
	
	float* elevData = 0;
	float* hgtData = 0;
	DataMgr* dataMgr = ds->getDataMgr();

    LayeredIO* dataMgrLayered = dynamic_cast<LayeredIO*> (dataMgr);
    if (! dataMgrLayered) return false;

	TwoDParams* tParams = (TwoDParams*) currentRenderParams;
	float displacement = tParams->getTwoDMin(2);
	int varnum = DataStatus::getSessionVariableNum2D("HGT");
	
	
	double origRegMin[2], origRegMax[2];
	for (int i = 0; i< 2; i++){
		regMin[i] = tParams->getTwoDMin(i);
		regMax[i] = tParams->getTwoDMax(i);
		origRegMin[i] = regMin[i];
		origRegMax[i] = regMax[i];
		//Test for empty box:
		if (regMax[i] <= regMin[i]){
			maxXElev = 0;
			maxYElev = 0;
			return false;
		}

	}
	
	regMin[2] = extents[2];
	regMax[2] = extents[5];
	//Convert to voxels:
	
	int elevGridRefLevel = tParams->getNumRefinements();
	//Do mapping to voxel coords at current ref level:
	dataMgrLayered->MapUserToVox((size_t)-1, regMin, min_dim, elevGridRefLevel);
	dataMgrLayered->MapUserToVox((size_t)-1, regMax, max_dim, elevGridRefLevel);
	//Convert back to user coords:
	dataMgrLayered->MapVoxToUser((size_t)-1, min_dim, regMin, elevGridRefLevel);
	dataMgrLayered->MapVoxToUser((size_t)-1, max_dim, regMax, elevGridRefLevel);
	//Extend by 1 voxel in x and y if it is smaller than original domain
	for (int i = 0; i< 2; i++){
		if(regMin[i] > origRegMin[i] && min_dim[i]>0) min_dim[i]--;
		if(regMax[i] < origRegMax[i] && max_dim[i]< ds->getFullSizeAtLevel(elevGridRefLevel,i)-1)
			max_dim[i]++;
	}
	
	//Convert increased vox dims to user coords:
	dataMgrLayered->MapVoxToUser((size_t)-1, min_dim, regMin, elevGridRefLevel);
	dataMgrLayered->MapVoxToUser((size_t)-1, max_dim, regMax, elevGridRefLevel);
	//Don't allow the terrain surface to be below the minimum extents:
	float minElev = extents[2]+(0.0001)*(extents[5] - extents[2]);
	
	//Try to get requested refinement level or the nearest acceptable level:
	int refLevel = RegionParams::shrinkToAvailableVoxelCoords(elevGridRefLevel, min_dim, max_dim, min_bdim, max_bdim, 
			timeStep, &varnum, 1, regMin, regMax, true);
	
	if(refLevel < 0) {
		setBypass(timeStep);
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, "Terrain elevation data unavailable \nfor 2D rendering at timestep %d",timeStep);
		dataMgrLayered->SetInterpolateOnOff(true);
		return false;
	}
	
	
	//Ignore vertical extents
	min_dim[2] = max_dim[2] = 0;
	min_bdim[2] = max_bdim[2] = 0;
	//Then, ask the Datamgr to retrieve the HGT data
	
	hgtData = dataMgr->GetRegion(timeStep, "HGT", refLevel, -1, min_bdim, max_bdim, 0);
	
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
	int maxx = maxXElev = max_dim[0] - min_dim[0] +1;
	int maxy = maxYElev = max_dim[1] - min_dim[1] +1;
	if (elevVert) {
		delete elevVert; 
		delete elevNorm;
	}
	
	elevVert = new float[3*maxx*maxy];
	elevNorm = new float[3*maxx*maxy];
	cachedTimeStep = timeStep;
	float deltax = (regMax[0]-regMin[0])/(maxx-1); 
	float deltay = (regMax[1]-regMin[1])/(maxy-1); 

	//set minTex, maxTex values less than deltax, deltay so that the texture will
	//map exactly to the original region bounds

	if (regMin[0] < origRegMin[0] && regMax[0] > origRegMax[0]){
		minXTex = (regMin[0]+deltax - origRegMin[0])/(origRegMax[0]-origRegMin[0]);
		maxXTex = minXTex + deltax*(maxx-3)/(origRegMax[0]-origRegMin[0]);
	} else if (regMin[0] < origRegMin[0]){
		minXTex = (regMin[0]+deltax - origRegMin[0])/(origRegMax[0]-origRegMin[0]);
		maxXTex = 1.f;
	} else if (regMax[0] > origRegMax[0]){
		minXTex = 0.f;
		maxXTex = deltax*(maxx-2)/(origRegMax[0]-origRegMin[0]);
	} else {
		minXTex = 0.f;
		maxXTex = 1.f;
	}
	assert (maxXTex <= 1.f && maxXTex >= 0.f);
	assert (minXTex <= 1.f && minXTex >= 0.f);
	if (regMin[1] < origRegMin[1] && regMax[1] > origRegMax[1]){
		minYTex = (regMin[1]+deltay - origRegMin[1])/(origRegMax[1]-origRegMin[1]);
		maxYTex =  minYTex + deltay*(maxy-3)/(origRegMax[1]-origRegMin[1]);
	} else if (regMin[1] < origRegMin[1]){
		minYTex = (regMin[1]+deltay - origRegMin[1])/(origRegMax[1]-origRegMin[1]);
		maxYTex = 1.f;
	} else if (regMax[1] > origRegMax[1]){
		minYTex = 0.f;
		maxYTex = deltay*(maxy-2)/(origRegMax[1]-origRegMin[1]);
	} else {
		minYTex = 0.f;
		maxYTex = 1.f;
	}
	assert (maxYTex <= 1.f && maxYTex >= 0.f);
	assert (minYTex <= 1.f && minYTex >= 0.f);


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
	size_t bs[3];
	dataMgr->GetBlockSize(bs, refLevel);
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
			ViewpointParams::worldToStretchedCube(worldCoord,elevVert+pntPos);
			for (int k = 0; k< 3; k++){
				if( *(elevVert + pntPos+k) > maxvals[k])
					maxvals[k] = *(elevVert + pntPos+k);
				if( *(elevVert + pntPos+k) < minvals[k])
					minvals[k] = *(elevVert + pntPos+k);
			}
		}
	}
	//qWarning("min,max coords: %f %f %f %f %f %f",
	//	minvals[0],minvals[1],minvals[2],maxvals[0],maxvals[1],maxvals[2]);
	//Now calculate normals:
	calcElevGridNormals(timeStep);
	return true;
}
