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
//		it's a pure abstract class for doing openGL in VAPoR
//

#include "glwindow.h"
#include "trackball.h"
#include "glutil.h"
#include "vizwin.h"
#include "renderer.h"
#include "viewpointparams.h"
#include "vizwinmgr.h"
#include "animationcontroller.h"

#include <math.h>
#include <qgl.h>
#include "assert.h"

using namespace VAPoR;


GLWindow::GLWindow( const QGLFormat& fmt, QWidget* parent, const char* name, VizWin* vw )
: QGLWidget(fmt, parent, name)

{
	myVizWin = vw;
	setAutoBufferSwap(false);
	wCenter[0] = 0.f;
	wCenter[1] = 0.f;
	wCenter[2] = 0.f;
	maxDim = 1.f;
	perspective = false;
	oldPerspective = false;
	renderNew = false;
	nowPainting = false;
}


/*!
  Release allocated resources.
*/

GLWindow::~GLWindow()
{
    makeCurrent();
	for (int i = 0; i< myVizWin->getNumRenderers(); i++){
		delete myVizWin->renderer[i];
	}
	myVizWin->setNumRenderers(0);
}


//
//  Set up the OpenGL view port, matrix mode, etc.
//

void GLWindow::resizeGL( int width, int height )
{
	//int winViewport[4];
	//double projMtx[16];
	/*
    glViewport( 0, 0, (GLint)width, (GLint)height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	GLfloat w = (float) width / (float) height;
    GLfloat h = 1.0;
	//Set for orthogonal view:
    glOrtho(-2*w, 2*w, -2*h, 2*h, 5.0, 15.0);
	//glFrustum( -w, w, -h, h, 5.0, 15.0 );
    glMatrixMode( GL_MODELVIEW );
	*/

	glViewport( 0, 0, (GLint)width, (GLint)height );
	//Save the current value...
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (perspective) {
		GLfloat w = (float) width / (float) height;
		
		float mindist = Max(0.2f, wCenter[2]-maxDim-1.0f);
		//glFrustum( -w, w, -h, h, mindist,(wCenter[2]+ maxDim + 1.0) );
		gluPerspective(45., w, mindist, (wCenter[2]+ maxDim + 10.0) );
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

}
	

/*
 * Obtain current view frame from gl model matrix
 */
void GLWindow::
changeViewerFrame(){
	GLfloat m[16], minv[16];
	GLdouble modelViewMtx[16];
	//Get the frame from GL:
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	//Also, save the modelview matrix for picking purposes:
	glGetDoublev(GL_MODELVIEW_MATRIX, modelViewMtx);
	//save the modelViewMatrix in the viewpoint params (it may be shared!)
	VizWinMgr::getInstance()->getViewpointParams(myVizWin->getWindowNum())->
		setModelViewMatrix(modelViewMtx);

	//Invert it:
	minvert(m, minv);
	vscale(minv+8, -1.f);
	if (!perspective){
		//Note:  This is a bad hack.  Putting off the time when I correctly implement
		//Ortho coords to actually send perspective viewer to infinity.
		//get the scale out of the (1st 3 elements of ) matrix:
		//
		float scale = vlength(m);
		float trans;
		if (scale < 5.f) trans = 1.-5./scale;
		else trans = scale - 5.f;
		minv[14] = -trans;
	}
	myVizWin->changeCoords(minv+12, minv+8, minv+4);
	myVizWin->setViewerCoordsChanged(false);
}

void GLWindow::paintGL()
{
	
	GLenum	buffer;
	float extents[6];
	float minFull[3], maxFull[3];
	int max_dim[3];
	int min_dim[3];
	size_t max_bdim[3];
	size_t min_bdim[3];
	if (nowPainting) return;
	nowPainting = true;
	int winNum = myVizWin->getWindowNum();
	//Force a resize if perspective has changed:
	if (perspective != oldPerspective){
		resizeGL(width(), height());
		oldPerspective = perspective;
	}
	makeCurrent();
	qglClearColor(myVizWin->getBackgroundColor()); 
	
	glGetIntegerv(GL_DRAW_BUFFER, (GLint *) &buffer);
	//Clear out in preparation for rendering
	glDrawBuffer(GL_BACK);
	glClearDepth(1);
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	
	glDrawBuffer(buffer);
	
	if (!Session::getInstance()->renderReady()) {
		swapBuffers();
		nowPainting = false;
		return;
	}
	//Check if there are any active renderers:
	if (myVizWin->getNumRenderers()==0){
		swapBuffers();
		nowPainting = false;
		return;
	}
	//If we are doing the first capture of a sequence then set the
	//newRender flag to true, whether or not it's a real new render.
	//Then turn off the flag, subsequent renderings will only be captured
	//if they really are new.
	//
	renderNew = myVizWin->captureIsNew();
	myVizWin->setCaptureNew(false);

	//Tell the animation we are starting.  If it returns false, we are not
	//being monitored by the animation controller
	bool isControlled = AnimationController::getInstance()->beginRendering(winNum);
	
	// Move to trackball view of scene  
	glPushMatrix();
	glLoadIdentity();
	getTBall()->TrackballSetMatrix();

	//If there are new coords, get them from GL, send them to the gui
	if (myVizWin->viewerCoordsChanged()){ 
		setRenderNew();
		changeViewerFrame();
	}
	RegionParams* myRegionParams = VizWinMgr::getInstance()->getRegionParams(winNum);
	ViewpointParams* myViewpointParams = VizWinMgr::getInstance()->getViewpointParams(winNum);
	//Note:  We are redundantly calling calcRegionExtents in both the dvr renderer
	//and here.  It also is unnecessarily called every rendering.
	int numxforms = myRegionParams->getNumTrans();
	myRegionParams->calcRegionExtents(min_dim, max_dim, min_bdim, max_bdim, numxforms, minFull, maxFull, extents);
	
	if(myVizWin->regionFrameIsEnabled()|| MainForm::getInstance()->getCurrentMouseMode() == Command::regionMode){
		renderDomainFrame(extents, minFull, maxFull);
	} 
	if(myVizWin->subregionFrameIsEnabled()&& !(MainForm::getInstance()->getCurrentMouseMode() == Command::regionMode)){
		drawSubregionBounds(extents);
	} 
	if (myVizWin->axesAreEnabled()) drawAxes(extents);

	for (int i = 0; i< myVizWin->getNumRenderers(); i++){
		myVizWin->renderer[i]->paintGL();
	}
	//Finally render the region geometry, if in region mode
	if(MainForm::getInstance()->getCurrentMouseMode() == Command::regionMode){
		float camVec[3];
		ViewpointParams::worldToCube(myViewpointParams->getCameraPos(), camVec);
		//Obtain the face displacement in world coordinates,
		//Then normalize to unit cube coords:
		float disp = myRegionParams->getFaceDisplacement();
		disp /= ViewpointParams::getMaxCubeSide();
		int selectedFace = myRegionParams->getSelectedFaceNum();
		assert(selectedFace >= -1 && selectedFace < 6);
		renderRegionBounds(extents, selectedFace,
			camVec, disp);
	} 
	//or render the seed geometry, if in probe mode
	else if(MainForm::getInstance()->getCurrentMouseMode() == Command::probeMode){
		float camVec[3];
		float seedExtents[6];
		FlowParams* myFlowParams = VizWinMgr::getInstance()->getFlowParams(winNum);
		myFlowParams->calcSeedExtents(seedExtents);
		ViewpointParams::worldToCube(myViewpointParams->getCameraPos(), camVec);
		//Obtain the face displacement in world coordinates,
		//Then normalize to unit cube coords:
		float disp = myFlowParams->getSeedFaceDisplacement();
		disp /= ViewpointParams::getMaxCubeSide();
		int selectedFace = myFlowParams->getSelectedFaceNum();
		assert(selectedFace >= -1 && selectedFace < 6);
		renderRegionBounds(seedExtents, selectedFace,
			camVec, disp);
	} 
	swapBuffers();
	glPopMatrix();
	//Always clear the regionDirty flag:
	myVizWin->setRegionDirty(false);
	if (isControlled) AnimationController::getInstance()->endRendering(winNum);
	//Capture the image, if not navigating:
	if (renderNew && !myVizWin->mouseIsDown()) myVizWin->doFrameCapture();
	nowPainting = false;
}

//
//  Set up the OpenGL rendering state, and define display list
//

void GLWindow::initializeGL()
{
    qglClearColor(myVizWin->getBackgroundColor()); 		// Let OpenGL clear to black
	//Initialize existing renderers:
	//
	for (int i = 0; i< myVizWin->getNumRenderers(); i++){
		myVizWin->renderer[i]->initializeGL();
	}
    
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
	return (GLdouble*)VizWinMgr::getInstance()->getViewpointParams(myVizWin->getWindowNum())->getModelViewMatrix();
}
//Issue OpenGL commands to draw a grid of lines of the full domain.
//Grid resolution is up to 2x2x2
//
void GLWindow::renderDomainFrame(float* extents, float* minFull, float* maxFull){

	int i; 
	int numLines[3];
	float regionSize, fullSize[3], modMin[3],modMax[3];
	setRegionFrameColor( myVizWin->getRegionFrameColor());
	
	
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
	

	glColor3fv(regionFrameColor);	   
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
	setSubregionFrameColor(myVizWin->getSubregionFrameColor());
	glLineWidth( 2.0 );
	glColor3fv(subregionFrameColor);
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
void GLWindow::drawRegionFace(float* extents, int faceNum, bool isSelected){
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
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			break;
		
		case 5:
		//do right 
			glBegin(GL_QUADS);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			break;
		case(3)://top
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			break;
		case(2)://bottom
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glEnd();
			break;
	
		case(0):
			//back
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glColor3fv(subregionFrameColor);
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
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glColor3fv(subregionFrameColor);
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

void GLWindow::renderRegionBounds(float* extents, int selectedFace, float* camPos, float faceDisplacement){
	setSubregionFrameColor(myVizWin->getSubregionFrameColor());
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
	for (i = 0; i< 6; i++){
		if (faceIsVisible(extents, camPos, i)){
			drawRegionFace(cpExtents, i, (i==selectedFace));
		}
	}
}
//Set colors to use in bound rendering:
void GLWindow::setSubregionFrameColor(QColor& c){
	subregionFrameColor[0]= (float)c.red()/255.;
	subregionFrameColor[1]= (float)c.green()/255.;
	subregionFrameColor[2]= (float)c.blue()/255.;
}
void GLWindow::setRegionFrameColor(QColor& c){
	regionFrameColor[0]= (float)c.red()/255.;
	regionFrameColor[1]= (float)c.green()/255.;
	regionFrameColor[2]= (float)c.blue()/255.;
}
void GLWindow::drawAxes(float* extents){
	float origin[3];
	float maxLen = -1.f;
	for (int i = 0; i<3; i++){
		origin[i] = extents[i] + (myVizWin->getAxisCoord(i))*(extents[i+3]-extents[i]);
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




