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
#include "glutil.h"
#include <vapor/errorcodes.h>

#include <qgl.h>
#include <qcolor.h>
#include <qapplication.h>
#include <qcursor.h>
#include "renderer.h"

using namespace VAPoR;


ProbeRenderer::ProbeRenderer(GLWindow* glw, ProbeParams* pParams )
:Renderer(glw, pParams, "ProbeRenderer")
{
	_fbTexid = 0;
	_probeTexid = 0;
	_framebufferid = 0;
}


/*
  Release allocated resources
*/

ProbeRenderer::~ProbeRenderer()
{
	if (_probeTexid) glDeleteTextures(1, &_probeTexid);
	if (_fbTexid) glDeleteTextures(1, &_fbTexid);
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
		probeTex = getProbeTexture(myProbeParams,currentFrameNum, true, _framebufferid, _fbTexid);
		QApplication::restoreOverrideCursor();
		myGLWindow->setRenderNew();
	} else { //existing texture is OK:
		probeTex = getProbeTexture(myProbeParams,currentFrameNum,true, _framebufferid, _fbTexid);
	}
	int imgSize[2];
	myProbeParams->getTextureSize(imgSize);
	int imgWidth = imgSize[0];
	int imgHeight = imgSize[1];
	if (probeTex){
		enableFullClippingPlanes();
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, _probeTexid);
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);// will not correct blending, but will be OK wrt other opaque geometry.
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth,imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, probeTex);

		//Do write to the z buffer
		glDepthMask(GL_TRUE);
		
	} else {
		return;
	}

	float corners[8][3];
	myProbeParams->calcLocalBoxCorners(corners, 0.f, -1);
	for (int cor = 0; cor < 8; cor++)
		ViewpointParams::localToStretchedCube(corners[cor],corners[cor]);
	
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
	disableFullClippingPlanes();
}


/*
  Set up the OpenGL rendering state, 
*/

void ProbeRenderer::initializeGL()
{
	myGLWindow->makeCurrent();
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
	glGenTextures(1, &_probeTexid);
	glGenTextures(1, &_fbTexid);
	//glBindTexture(GL_TEXTURE_2D, _probeTexid);
	if(GLEW_EXT_framebuffer_object) glGenFramebuffersEXT(1, &_framebufferid);
		
	
	//glBindTexture(GL_TEXTURE_2D, _probetex);
	pushState(256,256,_framebufferid, _fbTexid, true);
	
	popState();
	initialized = true;
}

// IBFV constants:

//Number of different spot noise patterns
#define Npat  32

//Following method is called from paintGL() if no texture exists, prior to displaying the texture.
//It uses openGL to build the texture.
//It can be called from probeRenderer, or from glProbeWindow.
//Every time it's called it generates a new list of patterns.

unsigned char* ProbeRenderer::buildIBFVTexture(ProbeParams* pParams, int tstep, GLuint fbid, GLuint fbtexid){
	
	
	int txsize[2];
	pParams->adjustTextureSize(txsize);
	bool ok = pParams->buildIBFVFields(tstep);
	if (!ok) return 0;
	int wid = txsize[0];
	int ht = txsize[1];
   
	static int listNum = -1;
	
	listNum = makeIBFVPatterns(pParams, listNum);
	unsigned char* imageBuffer = new unsigned char[wid*ht*4];
	pushState(wid,ht, fbid, fbtexid, false);
	
	glClearColor(0,0,0,0);
	glClearDepth(0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	float alpha = pParams->getAlpha();
	int numSetupFrames = alpha > 0.f ? (int)(4./alpha) : 100;
	
	for(int iframe = 0; iframe <= numSetupFrames; iframe++){
		glDisable(GL_BLEND);
		glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
		stepIBFVTexture(pParams, tstep, iframe, listNum);
	}
	
    //save in image buffer
	glReadPixels(0,0,wid, ht, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
	popState();
	//merge color or eliminate transparency.
	unsigned char* dataTex = 0;
	if (pParams->ibfvColorMerged()){
		dataTex = pParams->getCurrentProbeTexture(tstep, 0);
		assert(dataTex);
	}
	for (int q = 0; q<wid*ht; q++){
		if (pParams->ibvfPointIsValid(tstep, q)){
			if (dataTex){
				//copy over transparency:
				imageBuffer[4*q+3] = dataTex[4*q+3];
				//multiply color by ibfv value, then divide by 256
				for (int c = 0; c<3; c++){
					int val = (int)(imageBuffer[4*q+c])*(int)(dataTex[4*q+c]);
					imageBuffer[4*q+c] = (unsigned char)(val>>8);
				}
				
			} else { //make opaque:
				imageBuffer[4*q+3] = (unsigned char)255;
			}
		} else { //invalid point
			for (int r = 0; r<3; r++)
				imageBuffer[4*q+r] = (unsigned char)0;
		}
	}
    return imageBuffer;

}
int ProbeRenderer::makeIBFVPatterns(ProbeParams* pParams, int prevListNum) 
{ 
	//Determine the size of the image, to get the appropriate values for NPN and NMESH
   int lut[256];

   
   int *phase;
   GLubyte* pat;

   int npn = pParams->getNPN();
   
   int i, j, k, t;
   int alpha = (int)(255*(pParams->getAlpha()));
   phase = new int[npn*npn];
   pat = new GLubyte[npn*npn*4];
   
   for (i = 0; i < 256; i++) lut[i] = i < 127 ? 0 : 255;
   for (i = 0; i < npn; i++)
   for (j = 0; j < npn; j++) phase[npn*i+j] = rand() % 256; 
   if (prevListNum > 0) glDeleteLists(prevListNum,Npat+1);
   int newListNum = glGenLists(Npat+1);
   for (k = 0; k < Npat; k++) {
      t = k*256/Npat;
      for (i = 0; i < npn; i++) 
      for (j = 0; j < npn; j++) {
          pat[4*npn*i+4*j] =
          pat[4*npn*i+4*j+1] =
          pat[4*npn*i+4*j+2] = lut[(t + phase[i*npn+j]) % 255];
          pat[4*npn*i+4*j+3] = alpha;
      }
      glNewList(newListNum+k, GL_COMPILE);
      glTexImage2D(GL_TEXTURE_2D, 0, 4, npn, npn, 0, 
                   GL_RGBA, GL_UNSIGNED_BYTE, pat);
      glEndList();
   }
   delete [] phase;
   delete [] pat;
   return newListNum;
}

//Set up the gl state for doing the ibfv in the FBO buffer.
//Set "first" to true when calling from initializeGL()
void ProbeRenderer::pushState(int wid, int ht, GLuint fbid, GLuint fbtexid, bool first){
	
	
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
	
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	   
	glDisable(GL_CULL_FACE);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
		
	glStencilMask(GL_TRUE);
	glPixelZoom(1.0,1.0);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	
	// List of acceptable frame buffer object internal types 
	//
	GLint colorInternalFormats[] = {
		GL_RGBA, GL_RGBA16F_ARB, GL_RGBA16, GL_RGBA16, GL_RGBA8
	};
	GLenum colorInternalTypes[] = {
		GL_UNSIGNED_BYTE, GL_FLOAT, GL_INT, GL_INT, GL_UNSIGNED_BYTE 
	};
	int num_color_fmts = 
		sizeof(colorInternalFormats)/sizeof(colorInternalFormats[0]);

	GLint _colorInternalFormat;
	GLenum _colorInternalType;
	
	if(first) glBindTexture(GL_TEXTURE_2D, fbtexid);
	if(GLEW_EXT_framebuffer_object) glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbid);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (first && GLEW_EXT_framebuffer_object ){
	// Try to find a supported format.  Is this really necessary?
	//
		GLenum status;
		for (int i=0; i<num_color_fmts; i++) {
			glTexImage2D(
				GL_TEXTURE_2D, 0,colorInternalFormats[i], 
				wid, ht, 0, GL_RGBA, colorInternalTypes[i], NULL
			);
			glFramebufferTexture2DEXT(
				GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 
				fbtexid, 0
			);
			status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
			if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
				_colorInternalFormat = colorInternalFormats[i];
				_colorInternalType = colorInternalTypes[i];
				break;
			}
		}
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
			//myRenderer->setAllBypass(true);
			SetErrMsg(VAPOR_ERROR_DRIVER_FAILURE,
				"Failed to create OpenGL color framebuffer_object : %d", status
			);
		}
	}
	printOpenGLError();
}
void ProbeRenderer::popState(){
	glBindTexture(GL_TEXTURE_2D, 0);
	if(GLEW_EXT_framebuffer_object) glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	glPopAttrib();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

// This code appears superflous and chokes on Mac OGL drivers
//
//	glDrawBuffer(GL_BACK);
//	glReadBuffer(GL_BACK);
	printOpenGLError();
}

//Following method is called from glProbeWindow to get the next frame of the IBFV sequence.
//If animFrame == 0, it starts from beginning.
//tstep is the current animation time step
//frameNum is the number in the ibfv animation sequence.
//

unsigned char* ProbeRenderer::getNextIBFVTexture(ProbeParams* pParams, int tstep, 
	int frameNum, bool isStarting, int* listNum, GLuint fbid, GLuint fbtexid){
	
	int sz[2];
	if (pParams->doBypass(tstep)) return 0;
	pParams->getTextureSize(sz);
	
	//if the texture cache is invalid need to adjust texture size:
	if (isStarting || sz[0] == 0) {
		pParams->adjustTextureSize(sz);
		bool ok = pParams->buildIBFVFields(tstep);
		if (!ok) {
			pParams->setBypass(tstep);
			return 0;
		}
	}
	int wid = sz[0];
	int ht = sz[1];
	// These should always be 256, since that's the best size to fit in the probe width
	assert(wid == 256 && ht == 256);

	unsigned char* imageBuffer = new unsigned char[wid*ht*4];

	if(isStarting) {
		pushState(wid,ht, fbid, fbtexid, false);
		*listNum = makeIBFVPatterns(pParams, *listNum);
		glClearColor(0,0,0,0);
		glClearDepth(0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
	else pushState(wid,ht, fbid, fbtexid, false);

	stepIBFVTexture(pParams, tstep, frameNum, *listNum);


	//save in image buffer
	glReadPixels(0,0,wid, ht, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
	popState();
	//Eliminate transparency 
	//also merge in color  here. 
	unsigned char* dataTex = 0;
	if (pParams->ibfvColorMerged()){
		dataTex = pParams->getCurrentProbeTexture(tstep, 0);
		if (!dataTex){
			dataTex = pParams->calcProbeDataTexture(tstep, 256,256);
			//Always put this in the data texture cache...
			pParams->setProbeTexture(dataTex,tstep,0);
			
		}
	}
	for (int q = 0; q<wid*ht; q++){
		if (pParams->ibvfPointIsValid(tstep, q)){
			if (dataTex){
				//copy over transparency:
				imageBuffer[4*q+3] = dataTex[4*q+3];
				//multiply color by ibfv value, then divide by 256
				for (int c = 0; c<3; c++){
					int val = (int)(imageBuffer[4*q+c])*(int)(dataTex[4*q+c]);
					imageBuffer[4*q+c] = (unsigned char)(val>>8);
				}
				
			} else { //make opaque:
				imageBuffer[4*q+3] = (unsigned char)255;
			}
		} else { //invalid point
			for (int r = 0; r<3; r++)
				imageBuffer[4*q+r] = (unsigned char)0;
		}
	}
	
	return imageBuffer;

}
void ProbeRenderer::stepIBFVTexture(ProbeParams* pParams, int timestep, int frameNum, int listNum){
	float x1, x2, y, px, py;
	
	
	int txsize[2];
	pParams->getTextureSize(txsize);
	int nmesh = pParams->getNMESH();
	int npn = pParams->getNPN();
	float scale = 4.f*pParams->getFieldScale();
	float tmaxx   = txsize[0]/(scale*npn);
	float tmaxy   = txsize[1]/(scale*npn);
	float DM = ((float) (0.999999/(nmesh-1.0)));
	
	for (int i = 0; i < nmesh-1; i++) {
		x1 = DM*i; x2 = x1 + DM;
		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j < nmesh; j++) {
			y = DM*j;
			glTexCoord2f(x1, y); 
			pParams->getIBFVValue(timestep, x1, y, &px, &py);
			glVertex2f(px, py);

			glTexCoord2f(x2, y); 
			pParams->getIBFVValue(timestep, x2, y, &px, &py);
			glVertex2f(px, py);
		}
		glEnd();
	}


	glEnable(GL_BLEND); 
	glCallList(frameNum % Npat + listNum);
	glBegin(GL_QUAD_STRIP);
		glTexCoord2f(0.0,  0.0);  glVertex2f(0.0, 0.0);
		glTexCoord2f(0.0,  tmaxy); glVertex2f(0.0, 1.0);
		glTexCoord2f(tmaxx, 0.0);  glVertex2f(1.0, 0.0);
		glTexCoord2f(tmaxx, tmaxy); glVertex2f(1.0, 1.0);
	glEnd();
	glDisable(GL_BLEND);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
						0, 0, txsize[0], txsize[1], 0);
}
//Static method to calculate the probe texture whether IBFV or data or both
unsigned char* ProbeRenderer::getProbeTexture(ProbeParams* pParams, 
		int frameNum, bool doCache, GLuint fbid, GLuint fbtexid){
	if (!pParams->probeIsDirty(frameNum)) 
		return pParams->getCurrentProbeTexture(frameNum, pParams->getProbeType());
	if (pParams->doBypass(frameNum)) return 0;
	if (pParams->getProbeType() == 0) {
		unsigned char* dtex = pParams->calcProbeDataTexture(frameNum, 0,0);
		if (!dtex) pParams->setBypass(frameNum);
		return dtex;
	}
	//OK, now handle IBFV texture:
	if (pParams->ibfvColorMerged()){
		unsigned char* dataTex = pParams->calcProbeDataTexture(frameNum, 256,256);
		//Always put this in the cache...
		pParams->setProbeTexture(dataTex,frameNum,0);
		if (!dataTex) pParams->setBypass(frameNum);
	}
	unsigned char* probeTex = buildIBFVTexture(pParams,  frameNum, fbid, fbtexid);
	if (!probeTex) pParams->setBypass(frameNum);
	if (doCache){
		pParams->setProbeTexture(probeTex, frameNum, 1);
	}
	return probeTex;
}
