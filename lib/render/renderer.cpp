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
//		Methods are called by the glwindow class as needed.
//
#include "glutil.h"	// Must be included first!!!

#include "renderer.h"
#include <vapor/DataMgr.h>
#include "regionparams.h"
#include "viewpointparams.h"
#include "animationparams.h"
#ifdef Darwin
#include <gl.h>
#else
#include <GL/gl.h>
#endif

using namespace VAPoR;

Renderer::Renderer( GLWindow* glw, RenderParams* rp, string name)
{
	//Establish the data sources for the rendering:
	//
	
	_myName = name;
    myGLWindow = glw;
	currentRenderParams = rp;
	initialized = false;
	rp->initializeBypassFlags();
}

// Destructor
Renderer::~Renderer()
{
   
}




void Renderer::enableClippingPlanes(const double extents[6]){

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

	glLoadMatrixd(myGLWindow->getModelMatrix());

    //cerr << "transforming everything to unit box coords :-(\n";
   // myGLWindow->TransformToUnitBox();

    const float* scales = DataStatus::getInstance()->getStretchFactors();
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

	AnimationParams *myAnimationParams = myGLWindow->getActiveAnimationParams();
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
	
void Renderer::disableFullClippingPlanes(){
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
	
	const float* sizes = DataStatus::getInstance()->getFullStretchedSizes();
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

	AnimationParams *myAnimationParams = myGLWindow->getActiveAnimationParams();
    size_t timeStep = myAnimationParams->getCurrentTimestep();
	RegionParams* myRegionParams = myGLWindow->getActiveRegionParams();

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

void Renderer::disableRegionClippingPlanes(){
	Renderer::disableFullClippingPlanes();
}
