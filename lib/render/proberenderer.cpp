//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		proberenderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2006
//
//	Description:	Implementation of the proberenderer class
//

#include "params.h"
#include "proberenderer.h"
#include "animationparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "datastatus.h"
#include "glwindow.h"

#include <qgl.h>
#include <qcolor.h>
#include <qapplication.h>
#include <qcursor.h>
#include "renderer.h"

using namespace VAPoR;
ProbeRenderer::ProbeRenderer(GLWindow* glw, ProbeParams* pParams )
:Renderer(glw, pParams)
{
    
}


/*
  Release allocated resources
*/

ProbeRenderer::~ProbeRenderer()
{
	
}


// Perform the rendering
//
  

void ProbeRenderer::paintGL()
{
	
	AnimationParams* myAnimationParams = myGLWindow->getActiveAnimationParams();
	ProbeParams* myProbeParams = (ProbeParams*)currentRenderParams;
	int currentFrameNum = myAnimationParams->getCurrentFrameNumber();
	unsigned char* probeTex = 0;
	
	if (myProbeParams->probeIsDirty(currentFrameNum)){
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		probeTex = myProbeParams->getProbeTexture(currentFrameNum);
		QApplication::restoreOverrideCursor();
	} else {
		probeTex = myProbeParams->getProbeTexture(currentFrameNum);
	}
	int imgWidth = myProbeParams->getImageWidth();
	int imgHeight = myProbeParams->getImageHeight();
	if (probeTex){
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);// will not correct blending, but will be OK wrt other opaque geometry.
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth,imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, probeTex);
		//Do write to the z buffer
		glDepthMask(GL_TRUE);
	} else {
		return;
		//Don't write to the z-buffer, so won't obscure stuff behind that shows up later
		glDepthMask(GL_FALSE);
		glColor4f(.8f,.8f,0.f,0.2f);
	}

	float corners[8][3];
	myProbeParams->calcBoxCorners(corners, 0.f);
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
	if (probeTex) glDisable(GL_TEXTURE_2D);
}


/*
  Set up the OpenGL rendering state, 
*/

void ProbeRenderer::initializeGL()
{
	myGLWindow->makeCurrent();
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
}
