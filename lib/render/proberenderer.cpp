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


int glPixelAlign;
ProbeRenderer::ProbeRenderer(GLWindow* glw, ProbeParams* pParams )
:Renderer(glw, pParams)
{
	_probeid = 0;
}


/*
  Release allocated resources
*/

ProbeRenderer::~ProbeRenderer()
{
	if (_probeid) glDeleteTextures(1, &_probeid);
}


// Perform the rendering
//
  
int wid = 256;
int ht = 256;
void ProbeRenderer::paintGL()
{
	
	AnimationParams* myAnimationParams = myGLWindow->getActiveAnimationParams();
	ProbeParams* myProbeParams = (ProbeParams*)currentRenderParams;
	size_t fullHeight = myGLWindow->getActiveRegionParams()->getFullGridHeight();
	int currentFrameNum = myAnimationParams->getCurrentFrameNumber();
	unsigned char* probeTex = 0;
	
	if (myProbeParams->probeIsDirty(currentFrameNum)){
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		if (myProbeParams->getProbeType() == 1) {  //IBFV texture
			probeTex = buildIBFVTexture(myProbeParams,  currentFrameNum);
			myProbeParams->setProbeTexture(probeTex, currentFrameNum);
			myProbeParams->setTextureSize(wid,ht);
			
		} else probeTex = myProbeParams->getProbeTexture(currentFrameNum,fullHeight);
		QApplication::restoreOverrideCursor();
	} else { //existing texture is OK:
		probeTex = myProbeParams->getProbeTexture(currentFrameNum,fullHeight);
	}
	int imgWidth = myProbeParams->getImageWidth();
	int imgHeight = myProbeParams->getImageHeight();
	if (probeTex){
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glBindTexture(GL_TEXTURE_2D, _probeid);
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);// will not correct blending, but will be OK wrt other opaque geometry.
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

	glGenTextures(1, &_probeid);
	glBindTexture(GL_TEXTURE_2D, _probeid);
	
	initialized = true;
	
}
#define M_PI 3.141592653589793
#define	NPN 64

#define STEADYTIME 110

int    Npat   = 32;
float  sa;
int firstListNum;
float dmaxx, dmaxy;
float tmaxx, tmaxy;


//Following method is called from paintGL() if no texture exists, prior to displaying the texture.
//It uses openGL to build the texture.
//It can be called from probeRenderer, or from glProbeWindow.

unsigned char* ProbeRenderer::buildIBFVTexture(ProbeParams* pParams, int tstep){
	
   float scale = pParams->getFieldScale();
   
   tmaxx   = wid/(scale*NPN);
   tmaxy   = ht/(scale*NPN);
   dmaxx   = scale/wid;
	dmaxy   = scale/ht;
	
	makeIBFVPatterns(pParams);
	unsigned char* imageBuffer = new unsigned char[wid*ht*4];
	pushState();
	
	glClearColor(0,0,0,0);
	glClearDepth(0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	

	
	//makeIBFVPatterns();
	int numSetupFrames = pParams->getSetUpFrames();
	
	for(int iframe = 0; iframe <= numSetupFrames; iframe++){//Calc image in back buffer:
		stepIBFVTexture(pParams, iframe);
	}
	

    //save in image buffer
	glReadPixels(0,0,wid, ht, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
	popState();
	//Eliminate transparency 
	//later just merge in color and transparency here. 
	for (int q = 0; q<wid*ht; q++){imageBuffer[4*q+3] = (unsigned char)255;}
    return imageBuffer;

}
void ProbeRenderer::makeIBFVPatterns(ProbeParams* pParams) 
{ 
   int lut[256];
   int phase[NPN][NPN];
   GLubyte pat[NPN][NPN][4];
   int i, j, k, t;
   int alpha = 255*(pParams->getAlpha());
   
   for (i = 0; i < 256; i++) lut[i] = i < 127 ? 0 : 255;
   for (i = 0; i < NPN; i++)
   for (j = 0; j < NPN; j++) phase[i][j] = rand() % 256; 
   firstListNum = glGenLists(Npat+1);
   for (k = 0; k < Npat; k++) {
      t = k*256/Npat;
      for (i = 0; i < NPN; i++) 
      for (j = 0; j < NPN; j++) {
          pat[i][j][0] =
          pat[i][j][1] =
          pat[i][j][2] = lut[(t + phase[i][j]) % 255];
          pat[i][j][3] = alpha;
      }
      glNewList(firstListNum+k, GL_COMPILE);
      glTexImage2D(GL_TEXTURE_2D, 0, 4, NPN, NPN, 0, 
                   GL_RGBA, GL_UNSIGNED_BYTE, pat);
      glEndList();
   }
   
}
//Toy vector field integrator:  input x,y (in probe slice) get back position.
//Need to modify so that works on 2d slice in 3d volume.  Will therefore need to have 2d 
//vector field data slice to work on
void ProbeRenderer::getDP(float x, float y, float *px, float *py, float dmaxx, float dmaxy) 
{
   float dx, dy, vx, vy, r;
  
   dx = x - 0.5;         
   dy = y - 0.5; 
   r  = dx*dx + dy*dy; 
   if (r < 0.0001) r = 0.0001;
   vx = sa*dx/r + 0.02;  
   vy = sa*dy/r;
   r  = vx*vx + vy*vy;
   if (r > dmaxx*dmaxy) { 
      r  = sqrt(r); 
      vx *= dmaxx/r; 
      vy *= dmaxy/r; 
   }
   *px = x + vx;         
   *py = y + vy;
   
}
//Set up the gl state for doing the ibfv in the aux buffer.
void ProbeRenderer::pushState(){
	glDrawBuffer(GL_AUX0);
	glReadBuffer(GL_AUX0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
   glLoadIdentity(); 
   glTranslatef(-1.0, -1.0, 0.0); 
   glScalef(2.0, 2.0, 1.0);
  
   glMatrixMode(GL_TEXTURE);
   glPushMatrix();
   glLoadIdentity();
   
   glPushAttrib(GL_ALL_ATTRIB_BITS);
     
   glViewport(0, 0, (GLsizei) wid, (GLsizei) ht);
   glTexParameteri(GL_TEXTURE_2D, 
                   GL_TEXTURE_WRAP_S, GL_REPEAT); 
   glTexParameteri(GL_TEXTURE_2D, 
                   GL_TEXTURE_WRAP_T, GL_REPEAT); 
   glTexParameteri(GL_TEXTURE_2D, 
                   GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, 
                   GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexEnvf(GL_TEXTURE_ENV, 
                   GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glEnable(GL_TEXTURE_2D);
   glShadeModel(GL_FLAT);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   
   glDisable(GL_CULL_FACE);
   //glGetIntegerv(GL_UNPACK_ALIGNMENT, &glPixelAlign);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
   glDisable(GL_LIGHTING);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	
	glStencilMask(GL_TRUE);
   glPixelZoom(1.0,1.0);
   glDisable(GL_SCISSOR_TEST);
   glDisable(GL_STENCIL_TEST);
}
void ProbeRenderer::popState(){
	
	glPopAttrib();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glDrawBuffer(GL_BACK);
	glReadBuffer(GL_BACK);
	
}

//Following method is called from glProbeWindow to get the next frame of the IBFV sequence.
//If animFrame == 0, it starts from beginning.
//

unsigned char* ProbeRenderer::getNextIBFVTexture(ProbeParams* pParams, int frameNum, bool isStarting){
	float scale = pParams->getFieldScale();
	tmaxx   = wid/(scale*NPN);
    tmaxy   = ht/(scale*NPN);
    dmaxx   = scale/wid;
	dmaxy   = scale/ht;
	unsigned char* imageBuffer = new unsigned char[wid*ht*4];
	pushState();
	
	

	if(isStarting) {
		
		makeIBFVPatterns(pParams);
		glClearColor(0,0,0,0);
		glClearDepth(0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
	
	stepIBFVTexture(pParams, frameNum);
	
	

    //save in image buffer
	glReadPixels(0,0,wid, ht, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
	popState();
	//Eliminate transparency 
	//later just merge in color  here. 
	for (int q = 0; q<wid*ht; q++){imageBuffer[4*q+3] = (unsigned char)255;}
    return imageBuffer;

}
void ProbeRenderer::stepIBFVTexture(ProbeParams* pParams, int frameNum){
	 float x1, x2, y, px, py;
	 int nmesh = pParams->getNMesh();
	 
	 float DM = ((float) (1.0/(nmesh-1.0)));
		sa = 0.010*cos(STEADYTIME*2.0*M_PI/200.0);
		for (int i = 0; i < nmesh-1; i++) {
			x1 = DM*i; x2 = x1 + DM;
			glBegin(GL_QUAD_STRIP);
			for (int j = 0; j < nmesh; j++) {
				y = DM*j;
				glTexCoord2f(x1, y); 
				getDP(x1, y, &px, &py, dmaxx, dmaxy); 
				glVertex2f(px, py);

				glTexCoord2f(x2, y); 
				getDP(x2, y, &px, &py, dmaxx, dmaxy); 
				glVertex2f(px, py);
			}
			glEnd();
		}
   

		glEnable(GL_BLEND); 
		glCallList(frameNum % Npat + 1);
		glBegin(GL_QUAD_STRIP);
			glTexCoord2f(0.0,  0.0);  glVertex2f(0.0, 0.0);
			glTexCoord2f(0.0,  tmaxy); glVertex2f(0.0, 1.0);
			glTexCoord2f(tmaxx, 0.0);  glVertex2f(1.0, 0.0);
			glTexCoord2f(tmaxx, tmaxy); glVertex2f(1.0, 1.0);
		glEnd();
		glDisable(GL_BLEND);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
							0, 0, wid, ht, 0);
		
		
	
}