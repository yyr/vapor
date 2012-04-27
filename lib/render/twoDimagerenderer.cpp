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

#include <vapor/errorcodes.h>
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
		
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		//Bind if the image changed
		if (myTwoDImageParams->getLastTwoDTexture() != twoDTex){
			glDeleteTextures(1,&_twoDid);
		}
		glBindTexture(GL_TEXTURE_2D, _twoDid);
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);// will not correct blending, but will be OK wrt other opaque geometry.
		
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth,imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, twoDTex);
		
		myTwoDImageParams->setLastTwoDTexture(twoDTex);
		//Do write to the z buffer
		glDepthMask(GL_TRUE);
		
		
	} else {
		
		return;
		
	}
	//Draw elevation grid if we are mapped to terrain, or if
	//It's in georeferenced image mode:
	if (myTwoDImageParams->isGeoreferenced() || myTwoDImageParams->isMappedToTerrain()){
		drawElevationGrid(currentFrameNum);
		glDisable(GL_TEXTURE_2D);
		return;
	}

	//Following code renders a textured rectangle inside the 2D extents.
	float corners[8][3];
	myTwoDImageParams->calcBoxCorners(corners, 0.f);
	for (int cor = 0; cor < 8; cor++)
		ViewpointParams::localToStretchedCube(corners[cor],corners[cor]);
	
	
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
	
}

bool TwoDImageRenderer::rebuildElevationGrid(size_t timeStep){
	//Reconstruct the elevation grid using the map projection specified
	//It may or may not be terrain following.  
	//The following code assumes horizontal orientation and a defined
	//map projection.
	//Assume no lambert or mercator:
	lambertOrMercator = false;

	//Specify the parameters that are needed to define the elevation grid:

	

	//Determine the grid size, the data extents, and the image size:
	DataStatus* ds = DataStatus::getInstance();
	const float* extents = ds->getLocalExtents();
	DataMgr* dataMgr = ds->getDataMgr();

	TwoDImageParams* tParams = (TwoDImageParams*) currentRenderParams;
	const float* imgExts = tParams->getCurrentTwoDImageExtents(timeStep);
	int refLevel = tParams->GetRefinementLevel();
	int lod = tParams->GetCompressionLevel();

	//Find the corners of the image (to find size of image in scene.)
	//Get these relative to actual extents in projection space
	double displayCorners[8];
	if (tParams->isGeoreferenced()){
		if (!tParams->getImageCorners(timeStep,displayCorners))
			return false;
	} else {
		//Use corners of 2D extents:
		float boxmin[3], boxmax[3];
		tParams->getLocalBox(boxmin, boxmax);
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
	// an elevation grid that covers the full image.  Note, with large images 
	// it could be more efficient to construct a smaller one.
	// Need to determine the approximate size of the image, to decide
	// how large a grid to use. 

	int maxx, maxy;
	maxXElev = maxx = gridsize[0] +1;
	maxYElev = maxy = gridsize[1] +1;
	if (elevVert){
		delete [] elevVert;
		delete [] elevNorm;
	}
	elevVert = new float[3*maxx*maxy];
	elevNorm = new float[3*maxx*maxy];
	cachedTimeStep = timeStep;
	//texture coordinate range goes from 0 to 1
	minXTex = 0.f;
	maxXTex = 1.f;
	minYTex = 0.f;
	maxYTex = 1.f;

	
	float minvals[3], maxvals[3];
	for (int i = 0; i< 3; i++){
		minvals[i] = 1.e30;
		maxvals[i] = -1.e30;
	}

	//If data is not mapped to terrain, elevation is determined
	//by twoDParams z (max or min are same).  
	//If data is mapped to terrain, but we are outside data, then
	//take elevation to be the min (which is just vert displacement)
	float constElev = tParams->getLocalTwoDMin(2);

	//Set up for doing terrain mapping:
	size_t min_dim[3], max_dim[3];
	if (!DataStatus::getInstance()->getDataMgr()) return false;
	vector<double> fullexts = dataMgr->GetExtents(timeStep);
	double regExts[6];
	for(int i = 0; i<6; i++) regExts[i] = fullexts[i];
	 
	float horizFact=0.f, vertFact=0.f, horizOffset=0.f, vertOffset=0.f;
	RegularGrid* hgtGrid = 0;
		
	if (tParams->isMappedToTerrain()){
		
		//We shall retrieve HGT for the full extents of the data
		//at the current refinement level.
		for (int i = 0; i<3; i++){
			min_dim[i] = 0;
			max_dim[i] = ds->getFullSizeAtLevel(refLevel,i) - 1;
		}
		vector<string>varname;
		varname.push_back("HGT");
		
		//Try to get requested refinement level or the nearest acceptable level:
		int rc = tParams->getGrids(timeStep,varname, regExts, &refLevel, &lod,  &hgtGrid); 
		
		if(!rc){
			setBypass(timeStep);
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, "Terrain elevation data unavailable \nfor 2D rendering at timestep %d",timeStep);
			return false;
		}
		
		dataMgr->MapUserToVox(timeStep, regExts, min_dim,refLevel);
		dataMgr->MapUserToVox(timeStep, regExts+3, max_dim,refLevel);
		
		//Ignore vertical extents
		min_dim[2] = max_dim[2] = 0;
		

		//Linear Conversion terms between user coords and grid coords;
		
		//regExts maps to reg_min, regExts+3 to max_dim 
		horizFact = ((double)(max_dim[0] - min_dim[0]))/(regExts[3] - regExts[0]);
		vertFact = ((double)(max_dim[1] - min_dim[1]))/(regExts[4] - regExts[1]);
		horizOffset = min_dim[0]  - regExts[0]*horizFact;
		vertOffset = min_dim[1]  - regExts[1]*vertFact;
		
	}
	
	if (tParams->isGeoreferenced()) {
		//Set up proj.4:
		projPJ dst_proj;
		projPJ src_proj; 
		
		src_proj = pj_init_plus(tParams->getImageProjectionString().c_str());
		dst_proj = pj_init_plus(DataStatus::getProjectionString().c_str());
		bool doProj = (src_proj != 0 && dst_proj != 0);
		if (!doProj) return false;


		//If a projection string is latlon, proj uses coordinates in Radians!
		bool radSrc = pj_is_latlong(src_proj)||(string::npos != tParams->getImageProjectionString().find("ob_tran"));
		bool rotlatlonDst = (string::npos != DataStatus::getProjectionString().find("ob_tran"));
		bool radDst = pj_is_latlong(dst_proj)||rotlatlonDst;

		lambertOrMercator = pj_is_latlong(src_proj) && ((string::npos != DataStatus::getProjectionString().find("=merc")) ||
			(string::npos != DataStatus::getProjectionString().find("lcc")));



		static const double RAD2DEG = 180./M_PI;
		static const double DEG2RAD = M_PI/180.0;

		//Use a line buffer to hold 3d coordinates for transforming
		double* elevVertLine = new double[3*maxx];
		float locCoords[3];
		const float* timeVaryingExtents = DataStatus::getExtents(timeStep);
		float prevLocCoords[2];
		for (int j = 0; j<maxy; j++){
			
			for (int i = 0; i<maxx; i++){
				
				//determine the x,y coordinates of each point
				//in the image coordinate space:
				//int pntPos = 3*(i+j*maxx);
				elevVertLine[3*i] = imgExts[0] + (imgExts[2]-imgExts[0])*(double)i/(double)(maxx-1);
				elevVertLine[3*i+1] = imgExts[1] + (imgExts[3]-imgExts[1])*(double)j/(double)(maxy-1);
				
			}
			
			//apply proj4 to transform the line.  If source is in degrees, convert to radians:
			if (radSrc)
				for(int i = 0; i< maxx; i++) {
					elevVertLine[3*i] *= DEG2RAD;
					elevVertLine[3*i+1] *= DEG2RAD;
				}
			pj_transform(src_proj,dst_proj,maxx,3, elevVertLine,elevVertLine+1, 0);
			//If the scene is latlon or rotlatlon, convert to degrees, then to meters:
			if (radDst) 
				for(int i = 0; i< maxx; i++) {
					elevVertLine[3*i] *= (RAD2DEG*111177.);
					elevVertLine[3*i+1] *= (RAD2DEG*111177.);
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

				prevLocCoords[0]=locCoords[0];
				prevLocCoords[1]=locCoords[1];
				if (tParams->isMappedToTerrain()){
					//Find cell coordinates of locCoords in data grid space
					float elev = hgtGrid->GetValue((double)locCoords[0], (double)locCoords[1],0.);
					if (elev == hgtGrid->GetMissingValue()) elev = 0.;
					locCoords[2] = elev+constElev;
				}
				else { //not mapped to terrain, use constant elevation
					locCoords[2] = constElev;
				}
				//Convert to stretched cube coords.  Note that following
				//routine requires local coords, not global world coords, despite name of method:
				ViewpointParams::localToStretchedCube(locCoords,elevVert+3*(i+j*maxx));
				for (int k = 0; k< 3; k++){
					if( *(elevVert + 3*(i+j*maxx)+k) > maxvals[k])
						maxvals[k] = *(elevVert + 3*(i+j*maxx)+k);
					if( *(elevVert + 3*(i+j*maxx)+k) < minvals[k])
						minvals[k] = *(elevVert + 3*(i+j*maxx)+k);
				}
			
		
			}
			
		}
		delete [] elevVertLine;
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
				float elev = hgtGrid->GetValue((double)locCoords[0], (double)locCoords[1],0.);
				if (elev == hgtGrid->GetMissingValue()) elev = 0.;
				locCoords[2] = elev+constElev;
				
				//Convert to stretched cube coords.  Note that following
				//routine requires local coords, not global world coords, despite name of method:
				ViewpointParams::localToStretchedCube(locCoords,elevVert+3*(i+j*maxx));
				for (int k = 0; k< 3; k++){
					if( *(elevVert + 3*(i+j*maxx)+k) > maxvals[k])
						maxvals[k] = *(elevVert + 3*(i+j*maxx)+k);
					if( *(elevVert + 3*(i+j*maxx)+k) < minvals[k])
						minvals[k] = *(elevVert + 3*(i+j*maxx)+k);
				}
			
			}
			
		}
	}
	if (hgtGrid) {dataMgr->UnlockGrid(hgtGrid); delete hgtGrid;}

	//qWarning("min,max coords: %f %f %f %f %f %f",
	// minvals[0],minvals[1],minvals[2],maxvals[0],maxvals[1],maxvals[2]);
	
	//Now calculate normals.  For now, just do it as if the points were
	//on a regular grid:
	calcElevGridNormals(timeStep);
	return true;
}
