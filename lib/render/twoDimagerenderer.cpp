//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDimagegrenderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2008
//
//	Description:	Implementation of the twoDImageRenderer class
//

#include "params.h"
#include "twoDimagerenderer.h"
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

TwoDImageRenderer::TwoDImageRenderer(GLWindow* glw, TwoDImageParams* pParams )
: TwoDRenderer(glw, pParams)
{

}


/*
  Release allocated resources
*/

TwoDImageRenderer::~TwoDImageRenderer()
{
	
}


// Perform the rendering
//
  

void TwoDImageRenderer::paintGL()
{
	
	AnimationParams* myAnimationParams = myGLWindow->getActiveAnimationParams();
	TwoDImageParams* myTwoDImageParams = (TwoDImageParams*)currentRenderParams;
	
	int currentFrameNum = myAnimationParams->getCurrentFrameNumber();
	
	
	unsigned char* twoDTex = 0;
	
	if (myTwoDImageParams->twoDIsDirty(currentFrameNum)){
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		twoDTex = getTwoDTexture(myTwoDImageParams,currentFrameNum,true);
		QApplication::restoreOverrideCursor();
		if(!twoDTex) {setBypass(currentFrameNum); return;}
		myGLWindow->setRenderNew();
	} else { //existing texture is OK:
		twoDTex = getTwoDTexture(myTwoDImageParams,currentFrameNum,true);
	}
	if (myTwoDImageParams->elevGridIsDirty()){
		invalidateElevGrid();
		myTwoDImageParams->setElevGridDirty(false);
		myGLWindow->setRenderNew();
	}
	int imgSize[2];
	myTwoDImageParams->getTextureSize(imgSize, currentFrameNum);
	int imgWidth = imgSize[0];
	int imgHeight = imgSize[1];
	if (twoDTex){
		if(myTwoDImageParams->imageCrop()) enableFullClippingPlanes();
		else disableFullClippingPlanes();
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
		
	}
	//Draw elevation grid if we are mapped to terrain, or if
	//It's in georeferenced image mode:
	if (myTwoDImageParams->isGeoreferenced() || myTwoDImageParams->isMappedToTerrain()){
		drawElevationGrid(currentFrameNum);
		return;
	}

	//Following code renders a textured rectangle inside the 2D extents.
	float corners[8][3];
	myTwoDImageParams->calcBoxCorners(corners, 0.f);
	for (int cor = 0; cor < 8; cor++)
		ViewpointParams::worldToStretchedCube(corners[cor],corners[cor]);
	
	
	//determine the corners of the textured plane.
	//If it's X-Y (orientation = 2) then use first 4. (2nd bit = 0)
	//If it's X-Z (orientation = 1) then use 0,1,4,5 (1st bit = 0)
	//If it's Y-Z (orientation = 0) then use even numbered (0th bit = 0)
	float rectCorners[4][3];
	int orient = myTwoDImageParams->getOrientation();
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
	int ordering = myTwoDImageParams->getImagePlacement();
	
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

bool TwoDImageRenderer::rebuildElevationGrid(size_t timeStep){
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
	TwoDImageParams* tParams = (TwoDImageParams*) currentRenderParams;
	const float* imgExts = tParams->getCurrentTwoDImageExtents(timeStep);
	int refLevel = tParams->getNumRefinements();

	//Find the corners of the image (to find size of image in scene.
	//Get these relative to actual extents in projection space
	double displayCorners[8];
	if (tParams->isGeoreferenced()){
		if (!tParams->getImageCorners(timeStep,displayCorners))
			return false;
	} else {
		//Use corners of 2D extents:
		float boxmin[3], boxmax[3];
		tParams->getBox(boxmin, boxmax);
		displayCorners[0] = boxmin[0];
		displayCorners[1] = boxmin[1];
		displayCorners[4] = boxmax[0];
		displayCorners[5] = boxmax[1];
	}
	
	int gridsize[2];
	for (int i = 0; i<2; i++){
		gridsize[i] = (int)(ds->getFullSizeAtLevel(refLevel,i)*
			abs(displayCorners[i+4]-displayCorners[i])/(extents[i+3]-extents[i])+0.5f);
		//No sense in making the grid bigger than display size
		if (gridsize[i] > 1000) gridsize[i] = 1000;
	}
	
	// intersect the twoD box with the imageExtents.  This will
	// determine the region used for the elevation grid.  
	// Construct
	// an elevation grid that covers the full image.  Later may
	// decide it's more efficient to construct a smaller one.
	// Need to determine the approximate size of the image, to decide
	// how large a grid to use. 

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
	
	if (tParams->isGeoreferenced()) {
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
						
						
						float elevLL = hgtData[xcrd+ycrd] + constElev;
						float elevLR = hgtData[1+xcrd+ycrd] + constElev;
						float elevUL = hgtData[xcrd+ycrdP1] + constElev;
						float elevUR = hgtData[1+xcrd+ycrdP1] + constElev;
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
			
		}
		delete elevVertLine;
	} else {
		//Not georeferenced, but elevation mapped.
		//Uses twoD extents, not image extents;
		
		float locCoords[3];
	
		for (int j = 0; j<maxy; j++){
			
			//obtain coords based on 2D extents,
			for (int i = 0; i< maxx; i++){
				locCoords[0] = displayCorners[0] + (displayCorners[4]-displayCorners[0])*(double)i/(double)(maxx-1);
				locCoords[1] = displayCorners[1] + (displayCorners[5]-displayCorners[1])*(double)j/(double)(maxy-1);
				assert(tParams->isMappedToTerrain());
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
					//find locCoords [0..1] relative to grid cell:
					float x = (fltGridLL[0] - gridLL[0]);
					float y = (fltGridLL[1] - gridLL[1]);
					//To get elevations at corners, need grid coordinates:
					size_t xcrd = min_dim[0] - bs[0]*min_bdim[0]+gridLL[0];
					size_t ycrd = (min_dim[1] - bs[1]*min_bdim[1]+gridLL[1])*(max_bdim[0]-min_bdim[0]+1)*bs[0];
					size_t ycrdP1 = (min_dim[1] - bs[1]*min_bdim[1]+gridLL[1]+1)*(max_bdim[0]-min_bdim[0]+1)*bs[0];
					
					
					float elevLL = hgtData[xcrd+ycrd] + constElev;
					float elevLR = hgtData[1+xcrd+ycrd] + constElev;
					float elevUL = hgtData[xcrd+ycrdP1] + constElev;
					float elevUR = hgtData[1+xcrd+ycrdP1] + constElev;
					//Bilinear interpolate:
					locCoords[2] = (1-y)*((1-x)*elevLL + x*elevLR) +
						y*((1-x)*elevUL + x*elevUR);
					if (locCoords[2] < minElev) locCoords[2] = minElev;
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
			
		}
	}

	//qWarning("min,max coords: %f %f %f %f %f %f",
	// minvals[0],minvals[1],minvals[2],maxvals[0],maxvals[1],maxvals[2]);
	
	//Now calculate normals.  For now, just do it as if the points were
	//on a regular grid:
	calcElevGridNormals(timeStep);
	return true;
}
