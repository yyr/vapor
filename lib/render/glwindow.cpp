//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glwindow.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Implementation of GLWindow class: 
//		It performs the opengl rendering for visualizers
//
#include <GL/glew.h>

#include "glwindow.h"
#include "trackball.h"
#include "glutil.h"
#include "renderer.h"
#include "viewpointparams.h"
#include "dvrparams.h"
#include "regionparams.h"
#include "probeparams.h"
#include "animationparams.h"
#include "manip.h"
#include "datastatus.h"

#include <math.h>
#include <qgl.h>
#include "assert.h"
#include "vaporinternal/jpegapi.h"
#include "common.h"
#include "flowrenderer.h"
#include "VolumeRenderer.h"
using namespace VAPoR;
int GLWindow::activeWindowNum = 0;
bool GLWindow::regionShareFlag = true;
VAPoR::GLWindow::mouseModeType VAPoR::GLWindow::currentMouseMode = GLWindow::navigateMode;
int GLWindow::jpegQuality = 75;
GLWindow::GLWindow( const QGLFormat& fmt, QWidget* parent, const char* name, int windowNum )
: QGLWidget(fmt, parent, name)

{
	winNum = windowNum;
	rendererMapping.clear();
	assert(rendererMapping.size() == 0);
	setViewerCoordsChanged(true);
	setAutoBufferSwap(false);
	wCenter[0] = 0.f;
	wCenter[1] = 0.f;
	wCenter[2] = 0.f;
	maxDim = 1.f;
	perspective = true;
	oldPerspective = false;
	renderNew = false;
	nowPainting = false;
	needsResize = true;
	farDist = 100.f;
	nearDist = 0.1f;
	colorbarDirty = true;
	mouseDownHere = false;

	//values of features:
	backgroundColor =  QColor(black);
	regionFrameColor = QColor(white);
	subregionFrameColor = QColor(red);
	colorbarBackgroundColor = QColor(black);
	axesEnabled = false;
	regionFrameEnabled = true;
	subregionFrameEnabled = false;
	colorbarEnabled = false;
	for (int i = 0; i<3; i++)
	    axisCoord[i] = -0.f;
	colorbarLLCoord[0] = 0.1f;
	colorbarLLCoord[1] = 0.1f;
	colorbarURCoord[0] = 0.3f;
	colorbarURCoord[1] = 0.5f;
	numColorbarTics = 11;
	numRenderers = 0;
    for (int i = 0; i< MAXNUMRENDERERS; i++)
		renderer[i] = 0;

	vizDirtyBit.clear();
	//setDirtyBit(Params::DvrParamsType,DvrClutBit, true);
	setDirtyBit(Params::DvrParamsType,ProbeTextureBit, true);
	//setDirtyBit(Params::DvrParamsType,DvrDatarangeBit, true);
	setDirtyBit(Params::RegionParamsType,RegionBit, true);
	setDirtyBit(Params::ViewpointParamsType,NavigatingBit, true);
	setDirtyBit(Params::DvrParamsType,ColorscaleBit, true);
    setDirtyBit(Params::DvrParamsType,LightingBit, true);

	capturing = false;
	newCapture = false;

	//Create Manips:
	
	myProbeManip = new TranslateRotateManip(this, 0);
	
	myFlowManip = new TranslateStretchManip(this, 0);
	myRegionManip = new TranslateStretchManip(this, 0);
	
}


/*!
  Release allocated resources.
*/

GLWindow::~GLWindow()
{
    makeCurrent();
	for (int i = 0; i< getNumRenderers(); i++){
		delete renderer[i];
	}
	setNumRenderers(0);
}


//
//  Set up the OpenGL view port, matrix mode, etc.
//

void GLWindow::resizeGL( int width, int height )
{
	//int winViewport[4];
	//double projMtx[16];
	

	glViewport( 0, 0, (GLint)width, (GLint)height );
	//Save the current value...
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (perspective) {
		GLfloat w = (float) width / (float) height;
		
		//float mindist = Max(0.2f, wCenter[2]-maxDim-1.0f);
		//glFrustum( -w, w, -h, h, mindist,(wCenter[2]+ maxDim + 1.0) );
		//gluPerspective(45., w, mindist, (wCenter[2]+ maxDim + 10.0) );
		gluPerspective(45., w, nearDist, farDist );
		//save the current value...
		glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
		
	} else {
		if (width > height) {
			float l, r, s;
			s = (float) width / (float) height;
			l = wCenter[0] - (maxDim*s);
			r = wCenter[0] + (maxDim*s);
			
			glOrtho(l, r, wCenter[1]-maxDim, wCenter[1]+maxDim, wCenter[2]-10.*maxDim-1.0, wCenter[2]+10.*maxDim+1.0); 
			//fprintf(stderr, "glOrtho(%f, %f, %f, %f, %f, %f\n", l,r,yc-hmaxDim, yc+hmaxDim, zc-hmaxDim-1.0, zc+hmaxDim+1.0);
		}
		else {
			float b, t, s;
			s = (float) height / (float) width;
			b = wCenter[1] - (maxDim*s);
			t = wCenter[1] + (maxDim*s);
			
			glOrtho(wCenter[0]-maxDim, wCenter[1]+maxDim, b, t, wCenter[2]-10.*maxDim-1.0, wCenter[2]+10.*maxDim+1.0); 
				//fprintf(stderr, "glOrtho(%f, %f, %f, %f, %f, %f\n", xc-hmaxDim, xc+hmaxDim, b, t, zc-hmaxDim-1.0, zc+hmaxDim+1.0);
			
				//glFrustum((wCenter[0]-maxDim)*.5, (wCenter[1]+maxDim)*.5, b*.5, t*.5, wCenter[2]-maxDim-1.0, wCenter[2]+maxDim+1.0); 
		}
	}
	glMatrixMode(GL_MODELVIEW);
	needsResize = false;

}
	
void GLWindow::resetView(RegionParams* rParams, ViewpointParams* vParams){
	//Get the nearest and furthest distance to region from current viewpoint
	float fr, nr;
	vParams->getFarNearDist(rParams, &fr, &nr);
	if (fr <= 0.f) { //region is behind camera
		fr = Max(1.f,-fr);
	}
	if (nr <= 0.f){ //camera is inside region
		nr = 0.01f*fr;
	}
	farDist = fr*4.f;  
	nearDist = nr*0.25f;
	needsResize = true;
}


void GLWindow::paintGL()
{
	
	GLenum	buffer;
	float extents[6];
	float minFull[3] = {0.f,0.f,0.f};
	float maxFull[3];
	
	if (nowPainting) return;
	nowPainting = true;
	//Force a resize if perspective has changed:
	if (perspective != oldPerspective || needsResize){
		resizeGL(width(), height());
		oldPerspective = perspective;
	}
	
	
	qglClearColor(getBackgroundColor()); 
	
	glGetIntegerv(GL_DRAW_BUFFER, (GLint *) &buffer);
	//Clear out in preparation for rendering
	glDrawBuffer(GL_BACK);
	glClearDepth(1);
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	
	glDrawBuffer(buffer);
	
	if (!DataStatus::getInstance() ||!(DataStatus::getInstance()->renderReady())) {
		swapBuffers();
		nowPainting = false;
		return;
	}
	
	//Modified 2/28/06:  Go ahead and "render" even if no active renderers:
	
	//If we are doing the first capture of a sequence then set the
	//newRender flag to true, whether or not it's a real new render.
	//Then turn off the flag, subsequent renderings will only be captured
	//if they really are new.
	//
	renderNew = captureIsNew();
	setCaptureNew(false);

	//Tell the animation we are starting.  If it returns false, we are not
	//being monitored by the animation controller
	//bool isControlled = AnimationController::getInstance()->beginRendering(winNum);
	
	// Move to trackball view of scene  
	glPushMatrix();
	glLoadIdentity();
	getTBall()->TrackballSetMatrix();

	

	//The prerender callback is set in the vizwin. 
	//It registers with the animation controller, 
	//and tells the window the current viewer frame.
	bool isControlled = preRenderCB(winNum, viewerCoordsChanged());
	//If there are new coords, get them from GL, send them to the gui
	if (viewerCoordsChanged()){ 
		setRenderNew();
		setViewerCoordsChanged(false);
	}

	
	getActiveRegionParams()->calcBoxExtentsInCube(extents);
	DataStatus::getInstance()->getMaxExtentsInCube(maxFull);

	
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	//and readable
	glEnable(GL_DEPTH_TEST);
	//Prepare for alpha values:
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(regionFrameIsEnabled()|| GLWindow::getCurrentMouseMode() == GLWindow::regionMode){
		renderDomainFrame(extents, minFull, maxFull);
	} 
	if(subregionFrameIsEnabled()&& !(GLWindow::getCurrentMouseMode() == GLWindow::regionMode)){
		drawSubregionBounds(extents);
	} 
	if (axesAreEnabled()) drawAxes(extents);

	//render the region geometry, if in region mode, and active visualizer, or region shared
	//with active visualizer
	if(GLWindow::getCurrentMouseMode() == GLWindow::regionMode && ( windowIsActive() ||
		(!getActiveRegionParams()->isLocal() && activeWinSharesRegion()))){

		TranslateStretchManip* regionManip = getRegionManip();
		regionManip->setParams(getActiveRegionParams());
		regionManip->render();
		
	} 
	//render the rake geometry, if in rake mode, on active visualizer
	else if((GLWindow::getCurrentMouseMode() == GLWindow::rakeMode)&& windowIsActive()){
		
		TranslateStretchManip* flowManip = getFlowManip();
		flowManip->setParams((Params*)getActiveFlowParams());
		flowManip->render();
	} //or render the probe geometry, if in probe mode on active visualizer
	else if((GLWindow::getCurrentMouseMode() == GLWindow::probeMode)&&windowIsActive()){
		
		
		TranslateRotateManip* probeManip = getProbeManip();
		probeManip->setParams(getActiveProbeParams());
		
		probeManip->render();
		//Also render the cursor
		draw3DCursor(getActiveProbeParams()->getSelectedPoint());
	} 

	for (int i = 0; i< getNumRenderers(); i++){
		renderer[i]->paintGL();
	}
	
	swapBuffers();
	glPopMatrix();
	//Always clear the regionDirty flag:
	
	setDirtyBit(Params::RegionParamsType,RegionBit,false);
	bool mouseIsDown = postRenderCB(winNum, isControlled);
	//Capture the image, if not navigating:
	if (renderNew && !mouseIsDown) doFrameCapture();
	nowPainting = false;
}
//Draw a 3D cursor at specified world coords
void GLWindow::draw3DCursor(const float position[3]){
	float cubePosition[3];
	ViewpointParams::worldToCube(position, cubePosition);
	glLineWidth(3.f);
	glColor3f(1.f,1.f,1.f);
	glBegin(GL_LINES);
	glVertex3f(cubePosition[0]-0.05f,cubePosition[1],cubePosition[2]);
	glVertex3f(cubePosition[0]+0.05f,cubePosition[1],cubePosition[2]);
	glVertex3f(cubePosition[0],cubePosition[1]-0.05f,cubePosition[2]);
	glVertex3f(cubePosition[0],cubePosition[1]+0.05f,cubePosition[2]);
	glVertex3f(cubePosition[0],cubePosition[1],cubePosition[2]-0.05f);
	glVertex3f(cubePosition[0],cubePosition[1],cubePosition[2]+0.05f);
	glEnd();
}

//
//  Set up the OpenGL rendering state, and define display list
//

void GLWindow::initializeGL()
{
    glewInit();
	//VizWinMgr::getInstance()->getDvrRouter()->initTypes();
    qglClearColor(getBackgroundColor()); 		// Let OpenGL clear to black
	//Initialize existing renderers:
	//
	for (int i = 0; i< getNumRenderers(); i++){
		renderer[i]->initializeGL();
	}
    
    //
    // Initialize the graphics-dependent dvrparam state
    //
    
}

//projectPoint returns true if point is in front of camera
//resulting screen coords returned in 2nd argument.  Note that
//OpenGL coords are 0 at bottom of window!
//
bool GLWindow::projectPointToWin(float cubeCoords[3], float winCoords[2]){
	double depth;
	GLdouble wCoords[2];
	GLdouble cbCoords[3];
	for (int i = 0; i< 3; i++)
		cbCoords[i] = (double) cubeCoords[i];
	//double* mmtrx = getModelMatrix(); 
	//double* pmtrx = getProjectionMatrix();
	//double mMtrx[16], pMtrx[16];
	//for (int q = 0; q<16; q++){
	//	mMtrx[q]=mmtrx[q];
	//	pMtrx[q]=pmtrx[q];
	//}
	//int* vprt = getViewport();

	bool success = gluProject(cbCoords[0],cbCoords[1],cbCoords[2], getModelMatrix(),
		getProjectionMatrix(), getViewport(), wCoords, (wCoords+1),(GLdouble*)(&depth));
	if (!success) return false;
	winCoords[0] = (float)wCoords[0];
	winCoords[1] = (float)wCoords[1];
	return (depth > 0.0);
}
//Convert a screen coord to a direction vector, representing the direction
//from the camera associated with the screen coords.  Note screen coords
//are OpenGL style
//
bool GLWindow::pixelToVector(int x, int y, const float camPos[3], float dirVec[3]){
	GLdouble pt[3];
	float v[3];
	//Obtain the coords of a point in view:
	bool success = gluUnProject((GLdouble)x,(GLdouble)y,(GLdouble)1.0, getModelMatrix(),
		getProjectionMatrix(), getViewport(),pt, pt+1, pt+2);
	if (success){
		//Convert point to float
		v[0] = (float)pt[0];
		v[1] = (float)pt[1];
		v[2] = (float)pt[2];
		//transform position to world coords
		ViewpointParams::worldFromCube(v,dirVec);
		//Subtract viewer coords to get a direction vector:
		vsub(dirVec, camPos, dirVec);
	}
	return success;
}
//Test if the screen projection of a 3D quad encloses a point on the screen.
//The 4 corners of the quad must be specified in counter-clockwise order
//as viewed from the outside (pickable side) of the quad.  
//Window coords are as in OpenGL (0 at bottom of window)
//
bool GLWindow::
pointIsOnQuad(float cor1[3], float cor2[3], float cor3[3], float cor4[3], float pickPt[2])
{
	float winCoord1[2];
	float winCoord2[2];
	float winCoord3[2];
	float winCoord4[2];
	if(!projectPointToWin(cor1, winCoord1)) return false;
	if (!projectPointToWin(cor2, winCoord2)) return false;
	if (pointOnRight(winCoord1, winCoord2, pickPt)) return false;
	if (!projectPointToWin(cor3, winCoord3)) return false;
	if (pointOnRight(winCoord2, winCoord3, pickPt)) return false;
	if (!projectPointToWin(cor4, winCoord4)) return false;
	if (pointOnRight(winCoord3, winCoord4, pickPt)) return false;
	if (pointOnRight(winCoord4, winCoord1, pickPt)) return false;
	return true;
}
//Test whether the pickPt is over (and outside) the box (as specified by 8 points)
int GLWindow::
pointIsOnBox(float corners[8][3], float pickPt[2]){
	//front (-Z)
	if (pointIsOnQuad(corners[0],corners[1],corners[3],corners[2],pickPt)) return 2;
	//back (+Z)
	if (pointIsOnQuad(corners[4],corners[6],corners[7],corners[5],pickPt)) return 3;
	//right (+X)
	if (pointIsOnQuad(corners[1],corners[5],corners[7],corners[3],pickPt)) return 5;
	//left (-X)
	if (pointIsOnQuad(corners[0],corners[2],corners[6],corners[4],pickPt)) return 0;
	//top (+Y)
	if (pointIsOnQuad(corners[2],corners[3],corners[7],corners[6],pickPt)) return 4;
	//bottom (-Y)
	if (pointIsOnQuad(corners[0],corners[4],corners[5],corners[1],pickPt)) return 1;
	return -1;
}

//Produce an array based on current contents of the (front) buffer
bool GLWindow::
getPixelData(unsigned char* data){
	// set the current window 
	makeCurrent();
	 // Must clear previous errors first.
	while(glGetError() != GL_NO_ERROR);

	//if (front)
	//
	//glReadBuffer(GL_FRONT);
	//
	//else
	//  {
	glReadBuffer(GL_BACK);
	//  }
	glDisable( GL_SCISSOR_TEST );


 
	// Turn off texturing in case it is on - some drivers have a problem
	// getting / setting pixels with texturing enabled.
	glDisable( GL_TEXTURE_2D );

	// Calling pack alignment ensures that we can grab the any size window
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glReadPixels(0, 0, width(), height(), GL_RGB,
				GL_UNSIGNED_BYTE, data);
	if (glGetError() != GL_NO_ERROR)
		return false;
	//Unfortunately gl reads these in the reverse order that jpeg expects, so
	//Now we need to swap top and bottom!
	unsigned char val;
	for (int j = 0; j< height()/2; j++){
		for (int i = 0; i<width()*3; i++){
			val = data[i+width()*3*j];
			data[i+width()*3*j] = data[i+width()*3*(height()-j-1)];
			data[i+width()*3*(height()-j-1)] = val;
		}
	}
	
	return true;
		
  
}
//Routine to obtain gl state from viewpointparams
GLdouble* GLWindow:: 
getModelMatrix() {
	return (GLdouble*)getActiveViewpointParams()->getModelViewMatrix();
}
//Issue OpenGL commands to draw a grid of lines of the full domain.
//Grid resolution is up to 2x2x2
//
void GLWindow::renderDomainFrame(float* extents, float* minFull, float* maxFull){

	int i; 
	int numLines[3];
	float regionSize, fullSize[3], modMin[3],modMax[3];
	setRegionFrameColorFlt(getRegionFrameColor());
	
	
	//Instead:  either have 2 or 1 lines in each dimension.  2 if the size is < 1/3
	for (i = 0; i<3; i++){
		regionSize = extents[i+3]-extents[i];
		//Stretch size by 1%
		fullSize[i] = (maxFull[i] - minFull[i])*1.01;
		float mid = 0.5f*(maxFull[i]+minFull[i]);
		modMin[i] = mid - 0.5f*fullSize[i];
		modMax[i] = mid + 0.5f*fullSize[i];
		if (regionSize < fullSize[i]*.3) numLines[i] = 2;
		else numLines[i] = 1;
	}
	

	glColor3fv(regionFrameColorFlt);	   
    glLineWidth( 2.0 );
	//Now draw the lines.  Divide each dimension into numLines[dim] sections.
	
	int x,y,z;
	//Do the lines in each z-plane
	//Turn on writing to the z-buffer
	glDepthMask(GL_TRUE);
	
	glBegin( GL_LINES );
	for (z = 0; z<=numLines[2]; z++){
		float zCrd = modMin[2] + ((float)z/(float)numLines[2])*fullSize[2];
		//Draw lines in x-direction for each y
		for (y = 0; y<=numLines[1]; y++){
			float yCrd = modMin[1] + ((float)y/(float)numLines[1])*fullSize[1];
			
			glVertex3f(  modMin[0],  yCrd, zCrd );   
			glVertex3f( modMax[0],  yCrd, zCrd );
			
		}
		//Draw lines in y-direction for each x
		for (x = 0; x<=numLines[0]; x++){
			float xCrd = modMin[0] + ((float)x/(float)numLines[0])*fullSize[0];
			
			glVertex3f(  xCrd, modMin[1], zCrd );   
			glVertex3f( xCrd, modMax[1], zCrd );
			
		}
	}
	//Do the lines in each y-plane
	
	for (y = 0; y<=numLines[1]; y++){
		float yCrd = modMin[1] + ((float)y/(float)numLines[1])*fullSize[1];
		//Draw lines in x direction for each z
		for (z = 0; z<=numLines[2]; z++){
			float zCrd = modMin[2] + ((float)z/(float)numLines[2])*fullSize[2];
			
			glVertex3f(  modMin[0],  yCrd, zCrd );   
			glVertex3f( modMax[0],  yCrd, zCrd );
			
		}
		//Draw lines in z direction for each x
		for (x = 0; x<=numLines[0]; x++){
			float xCrd = modMin[0] + ((float)x/(float)numLines[0])*fullSize[0];
		
			glVertex3f(  xCrd, yCrd, modMin[2] );   
			glVertex3f( xCrd, yCrd, modMax[2]);
			
		}
	}
	
	//Do the lines in each x-plane
	for (x = 0; x<=numLines[0]; x++){
		float xCrd = modMin[0] + ((float)x/(float)numLines[0])*fullSize[0];
		//Draw lines in y direction for each z
		for (z = 0; z<=numLines[2]; z++){
			float zCrd = modMin[2] + ((float)z/(float)numLines[2])*fullSize[2];
			
			glVertex3f(  xCrd, modMin[1], zCrd );   
			glVertex3f( xCrd, modMax[1], zCrd );
			
		}
		//Draw lines in z direction for each y
		for (y = 0; y<=numLines[1]; y++){
			float yCrd = modMin[1] + ((float)y/(float)numLines[1])*fullSize[1];
			
			glVertex3f(  xCrd, yCrd, modMin[2] );   
			glVertex3f( xCrd, yCrd, modMax[2]);
			
		}
	}
	
	glEnd();//GL_LINES
	
	
	

}

float* GLWindow::cornerPoint(float* extents, int faceNum){
	if(faceNum&1) return extents+3;
	else return extents;
}
bool GLWindow::faceIsVisible(float* extents, float* viewerCoords, int faceNum){
	float temp[3];
	//Calc vector from a corner to the viewer.  Face is visible if
	//the outward normal to the face points in same direction as this vector.
	vsub (viewerCoords, cornerPoint(extents, faceNum), temp);
	switch (faceNum) { //visible if temp points in direction of outward normal:
		case (0): //norm is 0,0,-1
			return (temp[2]<0.f);
		case (1):
			return (temp[2]>0.f);
		case (2): //norm is 0,-1,0
			return (temp[1]<0.f);
		case (3):
			return (temp[1]>0.f);
		case (4): //norm is -1,0,0
			return (temp[0]<0.f);
		case (5):
			return (temp[0]>0.f);
		default: 
			assert(0);
			return false;
	}
}
void GLWindow::drawSubregionBounds(float* extents) {
	setSubregionFrameColorFlt(getSubregionFrameColor());
	glLineWidth( 2.0 );
	glColor3fv(subregionFrameColorFlt);
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[2]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glEnd();		
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents[0], extents[4], extents[2]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[0], extents[1], extents[5]);
	glEnd();
}
//Draw the lines bounding the region, and the selected one in
//highlighted color.
void GLWindow::drawRegionFaceLines(float* extents, int selectedFace){
	
	//draw the highlighted face, then do the other lines:
	glLineWidth( 3.0 );
	glColor4f(1.f,1.f,0.f,1.f);
	switch (selectedFace){
		case 4://Do left (x=0)
			
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glLineWidth( 2.0 );
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			//draw 4 other lines, x small to x-large
			glBegin(GL_LINES);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[0], extents[4], extents[5]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[2]);
				glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			break;
		
		case 5:
		//do right 
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			glLineWidth( 2.0 );
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[0], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			//draw 4 other lines, x small to x-large
			glBegin(GL_LINES);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[0], extents[4], extents[5]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[2]);
				glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
		case(3)://top
			
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[4], extents[2]);
				glVertex3f(extents[3], extents[4], extents[2]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glLineWidth( 2.0 );
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[0], extents[1], extents[5]);
			glEnd();
			//draw 4 other lines, y small to y-large
			glBegin(GL_LINES);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[0], extents[4], extents[2]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[4], extents[2]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[0], extents[4], extents[5]);
				
			glEnd();
			break;
		case(2)://bottom
			
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[2]);
			glEnd();
			glLineWidth( 2.0 );
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[4], extents[2]);
				glVertex3f(extents[0], extents[4], extents[5]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			//draw 4 other lines, y small to y-large
			glBegin(GL_LINES);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[0], extents[4], extents[2]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[4], extents[2]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			break;
	
		case(0):
			//back
			
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[4], extents[2]);
				glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glLineWidth( 2.0 );
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glBegin(GL_LINES);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[3], extents[4], extents[2]);
				glVertex3f(extents[0], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			break;
		case(1):
			//do the front:
			//
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glLineWidth( 2.0 );
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[4], extents[2]);
				glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glBegin(GL_LINES);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[3], extents[4], extents[2]);
				glVertex3f(extents[0], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			break;
		default: //Nothing selected.  Just do all in subregion frame color.
			glLineWidth( 2.0 );
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[4], extents[2]);
				glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glBegin(GL_LINE_LOOP);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glBegin(GL_LINES);
				glVertex3f(extents[0], extents[1], extents[5]);
				glVertex3f(extents[0], extents[1], extents[2]);
				glVertex3f(extents[3], extents[1], extents[5]);
				glVertex3f(extents[3], extents[1], extents[2]);
				glVertex3f(extents[3], extents[4], extents[5]);
				glVertex3f(extents[3], extents[4], extents[2]);
				glVertex3f(extents[0], extents[4], extents[5]);
				glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			break;
	}
}


//This only draws the outline of the face, as of 12/05.
void GLWindow::drawRegionFace(float* extents, int faceNum, bool isSelected){
	
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT, GL_FILL);
	if (isSelected){
		glLineWidth( 3.0 );
		glColor4f(1.f,1.f,0.f,1.f);
	}
	else {
		//glColor4f(.8f,.8f,.8f,.2f);
		glLineWidth( 2.0 );
		glColor3fv(subregionFrameColorFlt);
	}
	switch (faceNum){
		case 4://Do left (x=0)
			
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			break;
		
		case 5:
		//do right 
			
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			break;
		case(3)://top
			
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			break;
		case(2)://bottom
			
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glEnd();
			break;
	
		case(0):
			//back
			
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			break;
		case(1):
			//do the front:
			//
			
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			break;
		default: 
			break;
	}
	glColor4f(1,1,1,1);
	glDisable(GL_BLEND);
}
void GLWindow::drawProbeFace(float* corners, int faceNum, bool isSelected){
	glLineWidth( 2.0 );
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT, GL_FILL);
	if (isSelected)
			glColor4f(.8f,.8f,0.f,.6f);
		else 
			glColor4f(.8f,.8f,.8f,.2f);
	switch (faceNum){
		case 4://Do left (x=0)
			glBegin(GL_QUADS);
			glVertex3fv(corners);
			glVertex3fv(corners+3*3);
			glVertex3fv(corners+7*3);
			glVertex3fv(corners+4*3);
			glEnd();
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
			glVertex3fv(corners);
			glVertex3fv(corners+3*3);
			glVertex3fv(corners+7*3);
			glVertex3fv(corners+4*3);
			glEnd();
			break;
		
		case 5:
		//do right 
			glBegin(GL_QUADS);
			glVertex3fv(corners+1*3);
			glVertex3fv(corners+5*3);
			glVertex3fv(corners+6*3);
			glVertex3fv(corners+2*3);
			glEnd();
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
			glVertex3fv(corners+1*3);
			glVertex3fv(corners+5*3);
			glVertex3fv(corners+6*3);
			glVertex3fv(corners+2*3);
			glEnd();
			break;
		case(3)://top
			glBegin(GL_QUADS);
			glVertex3fv(corners+2*3);
			glVertex3fv(corners+6*3);
			glVertex3fv(corners+7*3);
			glVertex3fv(corners+3*3);
			
			glEnd();
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
			glVertex3fv(corners+2*3);
			glVertex3fv(corners+6*3);
			glVertex3fv(corners+7*3);
			glVertex3fv(corners+3*3);
			glEnd();
			break;
		case(2)://bottom
			glBegin(GL_QUADS);
			glVertex3fv(corners+0*3);
			glVertex3fv(corners+4*3);
			glVertex3fv(corners+5*3);
			glVertex3fv(corners+1*3);
			glEnd();
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
			glVertex3fv(corners+0*3);
			glVertex3fv(corners+4*3);
			glVertex3fv(corners+5*3);
			glVertex3fv(corners+1*3);
			glEnd();
			break;
	
		case(0):
			//back
			glBegin(GL_QUADS);
			glVertex3fv(corners+4*3);
			glVertex3fv(corners+7*3);
			glVertex3fv(corners+6*3);
			glVertex3fv(corners+5*3);
			glEnd();
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
			glVertex3fv(corners+4*3);
			glVertex3fv(corners+7*3);
			glVertex3fv(corners+6*3);
			glVertex3fv(corners+5*3);
			glEnd();
			break;
		case(1):
			//do the front:
			//
			glBegin(GL_QUADS);
			glVertex3fv(corners+0*3);
			glVertex3fv(corners+1*3);
			glVertex3fv(corners+2*3);
			glVertex3fv(corners+3*3);
			glEnd();
			glColor3fv(subregionFrameColorFlt);
			glBegin(GL_LINE_LOOP);
			glVertex3fv(corners+0*3);
			glVertex3fv(corners+1*3);
			glVertex3fv(corners+2*3);
			glVertex3fv(corners+3*3);
			glEnd();
			break;
		default: 
			break;
	}
	glColor4f(1,1,1,1);
	glDisable(GL_BLEND);
}
// This method draws the faces of the subregion-cube.
// The surface of the cube is drawn partially transparent. 
// This is drawn after the cube is drawn.
// If a face is selected, it is drawn yellow
// The z-buffer will continue to be
// read-only, but is left on so that the grid lines will continue to be visible.
// Faces of the cube are numbered 0..5 based on view from pos z axis:
// back, front, bottom, top, left, right
// selectedFace is -1 if nothing selected
//	
// The viewer direction determines which faces are rendered.  If a coordinate
// of the viewDirection is positive (resp., negative), 
// then the back side (resp front side) of the corresponding cube side is rendered

void GLWindow::renderRegionBounds(float* extents, int selectedFace,  float faceDisplacement){
	setSubregionFrameColorFlt(getSubregionFrameColor());
	//Copy the extents so they can be stretched
	int i;
	float cpExtents[6];
	int stretchCrd = -1;

	//Determine which coord direction is being stretched:
	if (selectedFace >= 0) {
		stretchCrd = (5-selectedFace)/2;
		if (selectedFace%2) stretchCrd +=3;
	}
	for (i = 0; i< 6; i++) {
		cpExtents[i] = extents[i];
		//Stretch the "extents" associated with selected face
		if(i==stretchCrd) cpExtents[i] += faceDisplacement;
	}
	//for (i = 0; i< 6; i++){
	//	if (faceIsVisible(extents, camPos, i)){
	//		drawRegionFace(cpExtents, i, (i==selectedFace));
	//	}
	//}
	drawRegionFaceLines(cpExtents, selectedFace);
}

//Set colors to use in bound rendering:
void GLWindow::setSubregionFrameColorFlt(QColor& c){
	subregionFrameColorFlt[0]= (float)c.red()/255.;
	subregionFrameColorFlt[1]= (float)c.green()/255.;
	subregionFrameColorFlt[2]= (float)c.blue()/255.;
}
void GLWindow::setRegionFrameColorFlt(QColor& c){
	regionFrameColorFlt[0]= (float)c.red()/255.;
	regionFrameColorFlt[1]= (float)c.green()/255.;
	regionFrameColorFlt[2]= (float)c.blue()/255.;
}
void GLWindow::drawAxes(float* extents){
	float origin[3];
	float maxLen = -1.f;
	for (int i = 0; i<3; i++){
		origin[i] = extents[i] + (getAxisCoord(i))*(extents[i+3]-extents[i]);
		if (extents[i+3] - extents[i] > maxLen) {
			maxLen = extents[i+3] - extents[i];
		}
	}
	float len = maxLen*0.2f;
	glColor3f(1.f,0.f,0.f);
	glLineWidth( 4.0 );
	glBegin(GL_LINES);
	glVertex3fv(origin);
	glVertex3f(origin[0]+len,origin[1],origin[2]);
	
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex3f(origin[0]+len,origin[1],origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1]+.1*len, origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1], origin[2]+.1*len);

	glVertex3f(origin[0]+len,origin[1],origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1], origin[2]+.1*len);
	glVertex3f(origin[0]+.8*len, origin[1]-.1*len, origin[2]);

	glVertex3f(origin[0]+len,origin[1],origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1]-.1*len, origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1], origin[2]-.1*len);

	glVertex3f(origin[0]+len,origin[1],origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1], origin[2]-.1*len);
	glVertex3f(origin[0]+.8*len, origin[1]+.1*len, origin[2]);
	glEnd();

	glColor3f(0.f,1.f,0.f);
	glBegin(GL_LINES);
	glVertex3fv(origin);
	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glVertex3f(origin[0]+.1*len, origin[1]+.8*len, origin[2]);
	glVertex3f(origin[0], origin[1]+.8*len, origin[2]+.1*len);

	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glVertex3f(origin[0], origin[1]+.8*len, origin[2]+.1*len);
	glVertex3f(origin[0]-.1*len, origin[1]+.8*len, origin[2]);

	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glVertex3f(origin[0]-.1*len, origin[1]+.8*len, origin[2]);
	glVertex3f(origin[0], origin[1]+.8*len, origin[2]-.1*len);

	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glVertex3f(origin[0], origin[1]+.8*len, origin[2]-.1*len);
	glVertex3f(origin[0]+.1*len, origin[1]+.8*len, origin[2]);
	glEnd();
	glColor3f(0.f,0.3f,1.f);
	glBegin(GL_LINES);
	glVertex3fv(origin);
	glVertex3f(origin[0],origin[1],origin[2]+len);
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex3f(origin[0],origin[1],origin[2]+len);
	glVertex3f(origin[0]+.1*len, origin[1], origin[2]+.8*len);
	glVertex3f(origin[0], origin[1]+.1*len, origin[2]+.8*len);

	glVertex3f(origin[0],origin[1],origin[2]+len);
	glVertex3f(origin[0], origin[1]+.1*len, origin[2]+.8*len);
	glVertex3f(origin[0]-.1*len, origin[1], origin[2]+.8*len);

	glVertex3f(origin[0],origin[1],origin[2]+len);
	glVertex3f(origin[0]-.1*len, origin[1], origin[2]+.8*len);
	glVertex3f(origin[0], origin[1]-.1*len, origin[2]+.8*len);

	glVertex3f(origin[0],origin[1],origin[2]+len);
	glVertex3f(origin[0], origin[1]-.1*len, origin[2]+.8*len);
	glVertex3f(origin[0]+.1*len, origin[1], origin[2]+.8*len);
	glEnd();
}



/*
 * Add a renderer to this visualization window
 * Add it at the end of the list.
 * This happens when the dvr renderer is enabled, or if
 * the local/global switch causes a new one to be enabled
 * Currently only volume renderers are appended.
 */
void GLWindow::
appendRenderer(RenderParams* rp, Renderer* ren)
{
	Params::ParamType renType = rp->getParamType();
	if (numRenderers < MAXNUMRENDERERS){
		assert(renType == Params::DvrParamsType);
		mapRenderer(rp, ren);
		renderer[numRenderers] = ren;
		renderType[numRenderers++] = renType;
		ren->initializeGL();
		update();
	}
}
/*
 * Insert a renderer to this visualization window
 * Add it at the start of the list.
 * This happens when a flow renderer or iso renderer is added
 */
void GLWindow::
prependRenderer(RenderParams* rp, Renderer* ren)
{
	Params::ParamType rendType = rp->getParamType();
	assert(rendType == Params::FlowParamsType || rendType == Params::ProbeParamsType);
	if (numRenderers < MAXNUMRENDERERS){
		mapRenderer(rp, ren);
		for (int i = numRenderers; i> 0; i--){
			renderer[i] = renderer[i-1];
			renderType[i] = renderType[i-1];
		}
		renderer[0] = ren;
		renderType[0] = rendType;
		numRenderers++;
		ren->initializeGL();
		update();
	}
}

/* 
 * Remove renderer of specified renderParams
 */
bool GLWindow::removeRenderer(RenderParams* rp){
	int i;
	map<RenderParams*,Renderer*>::iterator find_iter = rendererMapping.find(rp);
	if (find_iter == rendererMapping.end()) return false;
	Renderer* ren = find_iter->second;
	
	makeCurrent();

	//get it from the renderer list, and delete it:
	for (i = 0; i<numRenderers; i++) {		
		if (renderer[i] != ren) continue;
		delete renderer[i];
		renderer[i] = 0;
		break;
	}
	int foundIndex = i;
	assert(foundIndex < numRenderers);
	//Remove it from the mapping:
	rendererMapping.erase(find_iter);
	//Move renderers up.
	numRenderers--;
	for (int j = foundIndex; j<numRenderers; j++){
		renderer[j] = renderer[j+1];
		renderType[j] = renderType[j+1];
	}
	updateGL();
	return true;
}
//Remove a <params,renderer> pair from the list.  But leave them
//both alone.  This is needed when the params change and another params
//needs to use the existing renderer.
//
bool GLWindow::
unmapRenderer(RenderParams* rp){
	map<RenderParams*,Renderer*>::iterator find_iter = rendererMapping.find(rp);
	if (find_iter == rendererMapping.end()) return false;
	rendererMapping.erase(find_iter);
	return true;
}

//find (first) renderer params of specified type:
RenderParams* GLWindow::findARenderer(Params::ParamType renType){
	
	for (int i = 0; i<numRenderers; i++) {		
		if (renderType[i] != renType) continue;
		RenderParams* rParams = renderer[i]->getRenderParams();
		return rParams;
	}
	return 0;
}

Renderer* GLWindow::getRenderer(RenderParams* p){
	if(rendererMapping.size() == 0) return 0;
	map<RenderParams*, Renderer*>::iterator found_iter = rendererMapping.find(p);
	if (found_iter == rendererMapping.end()) return 0;
	return found_iter->second;
}

//In addition to setting the dirty bit, call the renderer's setDirty if
//the bit is being turned on.
void GLWindow::
setDirtyBit(Params::ParamType renType, DirtyBitType t, bool nowDirty){
	vizDirtyBit[t] = nowDirty;
	if (nowDirty){
		for (int i = 0; i< getNumRenderers(); i++){
			if (getRendererType(i) == renType){
				getRenderer(i)->setDirty(t);
			}
		}
	}
}
bool GLWindow::
vizIsDirty(DirtyBitType t) {
	return vizDirtyBit[t];
}
void GLWindow::
doFrameCapture(){
	if (capturing == 0) return;
	//Create a string consisting of captureName, followed by nnnn (framenum), 
	//followed by .jpg
	QString filename;
	if (capturing == 2){
		filename = captureName;
		filename += (QString("%1").arg(captureNum)).rightJustify(4,'0');
		filename +=  ".jpg";
	} //Otherwise we are just capturing one frame:
	else filename = captureName;
	if (!filename.endsWith(".jpg")) filename += ".jpg";
	//Now open the jpeg file:
	FILE* jpegFile = fopen(filename.ascii(), "wb");
	if (!jpegFile) {
		SetErrMsg("Image Capture Error: Error opening output Jpeg file: %s",filename.ascii());
		capturing = 0;
		return;
	}
	//Get the image buffer 
	unsigned char* buf = new unsigned char[3*width()*height()];
	//Use openGL to fill the buffer:
	if(!getPixelData(buf)){
		//Error!
		SetErrMsg("Image Capture Error; error obtaining GL data");
		capturing = 0;
		delete buf;
		return;
	}
	//Now call the Jpeg library to compress and write the file
	//
	int quality = getJpegQuality();
	int rc = write_JPEG_file(jpegFile, width(), height(), buf, quality);
	if (rc){
		//Error!
		SetErrMsg("Image Capture Error; Error writing jpeg file %s",
			filename.ascii());
		capturing = 0;
		delete buf;
		return;
	}
	//If just capturing single frame, turn it off, otherwise advance frame number
	if(capturing > 1) captureNum++;
	else capturing = 0;
	delete buf;
}

float GLWindow::getPixelSize(){
	float temp[3];
	//Window height is subtended by viewing angle (45 degrees),
	//at viewer distance (dist from camera to view center)
	ViewpointParams* vpParams = getActiveViewpointParams();
	vsub(vpParams->getRotationCenter(),vpParams->getCameraPos(),temp);
	float distToScene = vlength(temp);
	//tan(45 deg *0.5) is ratio between half-height and dist to scene
	float halfHeight = tan(M_PI*0.125)* distToScene;
	return (2.f*halfHeight/(float)height());

}
void GLWindow::setActiveParams(Params* p, Params::ParamType t){
	switch (t) {
		case Params::ProbeParamsType :
			setActiveProbeParams((ProbeParams*)p);
			return;
		case Params::DvrParamsType :
			setActiveDvrParams((DvrParams*)p);
			return;
		case Params::RegionParamsType :
			setActiveRegionParams((RegionParams*)p);
			return;
		case Params::FlowParamsType :
			setActiveFlowParams((FlowParams*)p);
			return;
		case Params::ViewpointParamsType :
			setActiveViewpointParams((ViewpointParams*)p);
			return;
		case Params::AnimationParamsType :
			setActiveAnimationParams((AnimationParams*)p);
			return;
		default: assert(0);
	}
	return;
}

int GLWindow::getJpegQuality() {return jpegQuality;}
void GLWindow::setJpegQuality(int qual){jpegQuality = qual;}

