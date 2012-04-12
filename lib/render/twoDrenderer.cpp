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

#include <vapor/errorcodes.h>
#include <vapor/DataMgr.h>
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
	cachedTimeStep = -1;
	pParams->setLastTwoDTexture(0);
	lambertOrMercator = false;
}


/*
  Release allocated resources
*/

TwoDRenderer::~TwoDRenderer()
{
	if (_twoDid) glDeleteTextures(1, &_twoDid);
	invalidateElevGrid();
}


/*
  Set up the OpenGL rendering state, 
*/

void TwoDRenderer::initializeGL()
{
	myGLWindow->makeCurrent();
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
	glGenTextures(1, &_twoDid);
	initialized = true;
}
//Static method to calculate the twoD texture 
unsigned char* TwoDRenderer::getTwoDTexture(TwoDParams* pParams, int frameNum,  bool doCache){
	unsigned char* tex;
	if (!pParams->twoDIsDirty(frameNum)) {
		tex =  pParams->getCurrentTwoDTexture(frameNum);
		if (tex) {
			return tex;
		}
	}
	return pParams->calcTwoDDataTexture(frameNum, 0,0);
	
}
//Draw an elevation grid (surface) inside the current region extents.
//Texture is already specified
void TwoDRenderer::drawElevationGrid(size_t timeStep){
	//If the twoD is dirty, or the timestep has changed, must rebuild.
	TwoDParams* tParams = (TwoDParams*)currentRenderParams;
	//First, check if we have already constructed the elevation grid vertices.
	//If not, rebuild them:
	if (!elevVert || (timeStep!= cachedTimeStep)) {
		//Don't try to rebuild if we failed already..
		if (doBypass(timeStep)) return;
		
		if(!rebuildElevationGrid(timeStep)) {
			setBypass(timeStep);
			return;
		}

	}
	int maxx = maxXElev;
	int maxy = maxYElev;
	if (maxx < 2 || maxy < 2) return;
	float firstx = minXTex;
	float firsty = minYTex;
	float lastx = maxXTex;
	float lasty = maxYTex;
	//Establish clipping planes:
	GLdouble topPlane[] = {0., -1., 0., 1.};
	GLdouble rightPlane[] = {-1., 0., 0., 1.0};
	GLdouble leftPlane[] = {1., 0., 0., 0.};
	GLdouble botPlane[] = {0., 1., 0., 0.};
	
	//Apply a coord transform that moves the full domain to the unit cube.
	
	glPushMatrix();
	
	//scale:
	float sceneScaleFactor = 1.f/ViewpointParams::getMaxStretchedCubeSide();
	glScalef(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);

	//translate to put origin at corner:
	float* transVec = ViewpointParams::getMinStretchedCubeCoords();
	glTranslatef(-transVec[0],-transVec[1], -transVec[2]);
	
	//Set up clipping planes
	if (tParams->imageCrop()){
		const float* scales = DataStatus::getInstance()->getStretchFactors();
	
		const float* extents = DataStatus::getInstance()->getExtents();

		topPlane[3] = tParams->getLocalTwoDMax(1)*scales[1];
		botPlane[3] = -tParams->getLocalTwoDMin(1)*scales[1];
		leftPlane[3] = -tParams->getLocalTwoDMin(0)*scales[0];
		rightPlane[3] = tParams->getLocalTwoDMax(0)*scales[0];
		

	
		glClipPlane(GL_CLIP_PLANE0, topPlane);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE1, rightPlane);
		glEnable(GL_CLIP_PLANE1);
		glClipPlane(GL_CLIP_PLANE2, botPlane);
		glEnable(GL_CLIP_PLANE2);
		glClipPlane(GL_CLIP_PLANE3, leftPlane);
		glEnable(GL_CLIP_PLANE3);
		

		glPopMatrix();
	} else {
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
		glDisable(GL_CLIP_PLANE2);
		glDisable(GL_CLIP_PLANE3);
		glDisable(GL_CLIP_PLANE4);
		glDisable(GL_CLIP_PLANE5);
		glPopMatrix();
	}
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
	
	//texture is already bound
	
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	//Put an identity on the Texture matrix stack.
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	
	
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

//		glBegin(GL_QUAD_STRIP);
		for (int i = 0; i< maxx-1; i++){
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

			//
			// Mapping projections can result in grids that wrap
			// around themselves.
			//
			// With lambert or mercator, 
			// Only draw quad if vertex coordinates are monotonically 
			// increasing with the i,j index. Note: we're only checking
			// X coordinate for (i,j), (i+1,j), and Y coordinate for
			// (i,j), (i,j+1). Should we check all coords for all verts?
			//
			//
		if (!lambertOrMercator || ((elevVert[3*(i+j*maxx)+0] < elevVert[3*(i+1+j*maxx)+0]) &&
				(elevVert[3*(i+j*maxx)+1] < elevVert[3*(i+(j+1)*maxx)+1]))) {
				glBegin(GL_QUADS);

				glNormal3fv(elevNorm+3*(i+j*maxx));
				glTexCoord2f(tcrdx,tcrdy);
				glVertex3fv(elevVert+3*(i+j*maxx));

				glNormal3fv(elevNorm+3*(i+1+j*maxx));
				glTexCoord2f(tcrdx+deltax,tcrdy);
				glVertex3fv(elevVert+3*(i+1+j*maxx));


				glNormal3fv(elevNorm+3*(i+1+(j+1)*maxx));
				glTexCoord2f(tcrdx+deltax,tcrdyp);
				glVertex3fv(elevVert+3*(i+1+(j+1)*maxx));

				glNormal3fv(elevNorm+3*(i+(j+1)*maxx));
				glTexCoord2f(tcrdx, tcrdyp);
				glVertex3fv(elevVert+3*(i+(j+1)*maxx));
				
				glEnd();
			}
			
		}
//		glEnd();
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


//Invalidate elevation grid data  
void TwoDRenderer::invalidateElevGrid(){
	
	if (elevVert){
		delete [] elevVert;
		delete [] elevNorm;	
		elevVert = 0;
		elevNorm = 0;
	}
	cachedTimeStep = -1;
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
	int maxx = maxXElev;
	int maxy = maxYElev;
	//Go over the grid of vertices, calculating normals
	//by looking at adjacent x,y,z coords.
	for (int j = 0; j < maxy; j++){
		for (int i = 0; i< maxx; i++){
			float* point = elevVert+3*(i+maxx*j);
			float* norm = elevNorm+3*(i+maxx*j);
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

