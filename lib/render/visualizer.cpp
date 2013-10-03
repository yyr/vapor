//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		visualizer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September, 2013
//
//	Description:  Implementation of Visualizer class: 
//		It performs the opengl rendering for visualizers
//
#include "glutil.h"	// Must be included first!!!
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
#include <vapor/GetAppPath.h>
#include "visualizer.h"
#include "renderer.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "animationparams.h"

#include "datastatus.h"
#include <vapor/MyBase.h>
#include <vapor/errorcodes.h>
#include "glutil.h"

#include <algorithm>
#include <vector>
#include <math.h>

#include "tiffio.h"
#include "assert.h"
#include <vapor/jpegapi.h>
#include <vapor/common.h>
#ifdef Darwin
#include <gl.h>
#else
#include <GL/gl.h>
#endif

using namespace VAPoR;
int Visualizer::activeWindowNum = 0;
bool Visualizer::regionShareFlag = true;
bool Visualizer::defaultTerrainEnabled = false;

bool Visualizer::defaultAxisArrowsEnabled = false;
bool Visualizer::nowPainting = false;

/* note: 
 * GL_ENUMS used by depth peeling for attaching the color buffers, currently 16 named points exist
 */
GLenum attach_points[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7, GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11, GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15};

int Visualizer::currentMouseMode = Visualizer::navigateMode;
vector<int> Visualizer::manipFromMode;
vector<string> Visualizer::modeName;
map<ParamsBase::ParamsBaseType, int> Visualizer::modeFromParams;
vector<ParamsBase::ParamsBaseType> Visualizer::paramsFromMode;
int Visualizer::jpegQuality = 100;
Visualizer::Visualizer(int windowNum )

{
	currentParams.clear();
	for (int i = 0; i<= Params::GetNumParamsClasses(); i++)
		currentParams.push_back(0);

	MyBase::SetDiagMsg("Visualizer::Visualizer() begin");
	

	winNum = windowNum;
	rendererMapping.clear();
	assert(rendererMapping.size() == 0);
	setViewerCoordsChanged(true);
	
	wCenter[0] = 0.f;
	wCenter[1] = 0.f;
	wCenter[2] = 0.f;
	maxDim = 1.f;
	
	renderNew = false;
	nowPainting = false;
	
	needsResize = true;
	farDist = 100.f;
	nearDist = 0.1f;
	
	mouseDownHere = false;

	renderType.clear();
	renderOrder.clear();
	renderer.clear();
	


	newCaptureImage = false;

    MyBase::SetDiagMsg("Visualizer::Visualizer() end");
}


/*!
  Release allocated resources.
*/

Visualizer::~Visualizer()
{
	
	
	for (int i = 0; i< renderer.size(); i++){
		delete renderer[i];
	}
	
	
	nowPainting = false;
	
}

void Visualizer::setDefaultPrefs(){
	defaultTerrainEnabled = false;
	
	defaultAxisArrowsEnabled = false;
	jpegQuality = 100;
}
//
//  Set up the OpenGL view port, matrix mode, etc.
//

void Visualizer::resizeGL( int width, int height )
{
  setUpViewport(width, height);
  nowPainting = false;
  needsResize = false;

  /* note: 2
   * currently all depth peeling setup occurs in resizeGL, code that should be moved to
   *  initializeGL will be marked with an refactor note
   */
 
#ifdef DEBUG
    printOpenGLError();
    cout << "resizeGL beginning" << endl;
#endif
    
   
    GLsizei myheight =  (GLsizei)height;
    GLsizei mywidth = (GLsizei) width;
	
   

#ifdef DEBUG
    printOpenGLError();
    cout << "resizeGL created textures" << endl;
#endif
  
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE); //no compare mode for depth texture
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
    glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, mywidth, myheight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	
  
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE); //no compare mode for depth texture
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
    glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, mywidth, myheight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

#ifdef DEBUG
    printOpenGLError();
    cout << "resizeGL textures complete" << endl;
#endif

   
    //Depth buffers are setup, now we need to setup the color textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0); //prevent framebuffers from being messed with

#ifdef DEBUG
    printOpenGLError();
    cout << "resizeGL framebuffers complete" << endl;
#endif



    
    //prevent textures from being messed with
    glBindTexture(GL_TEXTURE_2D, 0); 
    printOpenGLError();

    //Attach all of the available color buffers to the FBO
    //note: refactor to initializeGL
   
	
#ifdef DEBUG
    cout << "resizeGL complete" << endl;
#endif
    //clear framebuffer binding, initialize depth peeling shaders with uniforms
    //note: leave here since this deals with texture size information
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
   
    glActiveTexture(GL_TEXTURE0);
    printOpenGLError();

}
void Visualizer::setUpViewport(int width,int height){
   
	glViewport( 0, 0, (GLint)width, (GLint)height );
	//Save the current value...
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	GLfloat w = (float) width / (float) height;
		
	//float mindist = Max(0.2f, wCenter[2]-maxDim-1.0f);
	//glFrustum( -w, w, -h, h, mindist,(wCenter[2]+ maxDim + 1.0) );
	//gluPerspective(45., w, mindist, (wCenter[2]+ maxDim + 10.0) );
	//qWarning("setting near, far dist: %f %f", nearDist, farDist);
	gluPerspective(45., w, nearDist, farDist );
	//gluPerspective(45., w, 1.0, 5. );
	//save the current value...
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
		
	glMatrixMode(GL_MODELVIEW);
	printOpenGLError();
	
}
	
void Visualizer::resetView(ViewpointParams* vParams){
	//Get the nearest and furthest distance to region from current viewpoint
	float frbx, nrbx;
	vParams->getFarNearDist(&frbx, &nrbx);
	
	farDist = frbx * 4.f;
	nearDist = nrbx * 0.25f;
	needsResize = true;
}


void Visualizer::paintEvent(bool force)
{
	MyBase::SetDiagMsg("Visualizer::paintGL()");
	//Following is needed in case undo/redo leaves a disabled renderer in the renderer list, so it can be deleted.
	removeDisabledRenderers();
	
	//Following is probably not needed, too, although haven't checked it out.  Qt is supposed to always call paintGL in
	//the GL context, but maybe not paintEvent?
	//makeCurrent();
	printOpenGLError();
	
	
	float extents[6] = {0.f,0.f,0.f,1.f,1.f,1.f};
	float minFull[3] = {0.f,0.f,0.f};
	float maxFull[3] = {1.f,1.f,1.f};
	//Again, mutex probably not needed.  At some point the "nowPainting" flag was used to indicate
	//that some thread was in this routine.
    
	nowPainting = true;
	
	//Paint background
	//glClearColor(getBackgroundColor()); 
	
	//Clear out the depth buffer in preparation for rendering
	glClearDepth(1);
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	
	DataStatus *dataStatus = DataStatus::getInstance();

	
	//Improve polygon antialiasing
	glEnable(GL_MULTISAMPLE);
	
	//automatically renormalize normals (especially needed for flow rendering)
	glEnable(GL_NORMALIZE);

	//If we are doing the first capture of an image sequence then set the
	//newRender flag to true, whether or not it's a real new render.
	//Then turn off the flag, subsequent renderings will only be captured
	//if they really are new, or if we are spinning.
	
	//We need to prevent multiple frame capture, which could result from extraneous rendering.
	//The renderNew flag is set to allow performing a jpeg capture.
	//The newCaptureImage flag is set when we start a new image capture sequence.  When renderNew is
	//set, the previousTimeStep and previousFrameNum indices are set to -1, to guarantee that the image
	//will be captured.  Subsequent rendering will only be captured if the frame/timestep counters are changed, or 
	//if the image is spinning.
	

	//Following line has been modified and moved to a later point in the code.  It is necessary to register with the 
	//AnimationController, but not yet, because we don't yet know the frame number.
	//(Tell the animation we are starting.  If it returns false, we are not
	//being monitored by the animation controller
	//bool isControlled = AnimationController::getInstance()->beginRendering(winNum);
	
	//If we are visualizing in latLon space, must update the local coordinates
	//and put them in the trackball, prior to setting up the trackball.
	//The frameNum is used with keyframe animation, it coincides with timeStep when keyframing is disabled.
	int timeStep = getActiveAnimationParams()->getCurrentTimestep();

	
	
	
	setValuesFromGui(getActiveViewpointParams());

	//resetView sets the near/far clipping planes based on the viewpoint in the viewpointParams
	resetView(getActiveViewpointParams());
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	//Lights are positioned relative to the view direction, do this before the modelView matrix is changed
	placeLights();

	//Set the GL modelview matrix, based on the Trackball state.
	glLoadMatrixd(getModelViewMatrix());

	//The prerender callback is set in the vizwin. 
	//It registers with the animation controller, 
	//and tells the window the current viewer frame.
	//This must be called prior to rendering.  The boolean "isControlled" indicates 
	//whether or not the rendering is under the control of the animationController.
	//If it is true, then the postRenderCallback must be called after the full rendering is complete.
	bool isControlled = preRenderCB(winNum, viewerCoordsChanged());
	//If there are new coords, get them from GL, send them to the gui
	if (viewerCoordsChanged()){ 
		setRenderNew();
		setViewerCoordsChanged(false);
	}

	
	//make sure to capture whenever the time step or frame index changes
	if (timeStep != previousTimeStep) {
		setRenderNew();
		previousTimeStep = timeStep;
	}
	
    //getActiveRegionParams()->calcStretchedBoxExtentsInCube(extents,timeStep);
    //DataStatus::getInstance()->getMaxStretchedExtentsInCube(maxFull);
	
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	//and readable
	glEnable(GL_DEPTH_TEST);
	//Prepare for alpha values:
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Now we are ready for all the different renderers to proceed.
	//Sort them;  If they are opaque, they go first.  If not opaque, they
	//are sorted back to front.  This only works if all the geometry of a renderer is ordered by
	//a simple depth test.

	printOpenGLError();
	//Now go through all the active renderers
	for (int i = 0; i< renderer.size(); i++){
		//If a renderer is not initialized, or if its bypass flag is set, then don't render.
		//Otherwise push and pop the GL matrix stack and all attribs
		if(renderer[i]->isInitialized() && !(renderer[i]->doAlwaysBypass(timeStep))) {
			// Push or reset state
			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glPushAttrib(GL_ALL_ATTRIB_BITS);

			renderer[i]->paintGL();

			glPopAttrib();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			printOpenGLErrorMsg(renderer[i]->getMyName().c_str());
		}
	}
	//Go back to MODELVIEW for other matrix stuff
	//By default the matrix is expected to be MODELVIEW
	glMatrixMode(GL_MODELVIEW);

	//Perform final touch-up on the final images, before capturing or displaying them.

	glPopMatrix();
	//swapBuffers();
	glFlush();

	
	//Turn off the nowPainting flag (probably could be deleted)
	nowPainting = false;
	
	glDisable(GL_NORMALIZE);
	printOpenGLError();
}



//
//  Set up the OpenGL rendering state, and define display list
//

void Visualizer::initializeGL()
{
	
	GLenum glErr;
	glErr = glGetError();
	if (glErr != GL_NO_ERROR){
		MyBase::SetErrMsg(VAPOR_ERROR_DRIVER_FAILURE,"Error: No Usable Graphics Driver.\n%s",
			"Check that drivers are properly installed.");
		return;
	}
    previousTimeStep = -1;
	previousFrameNum = -1;
	glewInit();
	glEnable(GL_MULTISAMPLE);
	if (printOpenGLError()) return;
	//Check to see if we are using MESA:
	if (GetVendor() == MESA){
		SetErrMsg(VAPOR_ERROR_GL_VENDOR,"GL Vendor String is MESA.\nGraphics drivers may need to be reinstalled");
		
	}

	SetDiagMsg(
		"OpenGL Capabilities : GLEW_VERSION_2_0 %s",
		GLEW_VERSION_2_0 ? "ok" : "missing"
	);
	SetDiagMsg(
		"OpenGL Capabilities : GLEW_EXT_framebuffer_object %s",
		GLEW_EXT_framebuffer_object ? "ok" : "missing"
	);
	SetDiagMsg(
		"OpenGL Capabilities : GLEW_ARB_vertex_buffer_object %s",
		GLEW_ARB_vertex_buffer_object ? "ok" : "missing"
	);
	SetDiagMsg(
		"OpenGL Capabilities : GLEW_ARB_multitexture %s",
		GLEW_ARB_multitexture ? "ok" : "missing"
	);
	SetDiagMsg(
		"OpenGL Capabilities : GLEW_ARB_shader_objects %s",
		GLEW_ARB_shader_objects ? "ok" : "missing"
	);



	
	//Initialize existing renderers:
	//
	if (printOpenGLError()) return;
	for (int i = 0; i< renderer.size(); i++){
		renderer[i]->initializeGL();
		printOpenGLErrorMsg(renderer[i]->getMyName().c_str());
	}
	nowPainting = false;
	printOpenGLError();
	
    
}

//projectPoint returns true if point is in front of camera
//resulting screen coords returned in 2nd argument.  Note that
//OpenGL coords are 0 at bottom of window!
//
bool Visualizer::projectPointToWin(float cubeCoords[3], float winCoords[2]){
	double depth;
	GLdouble wCoords[2];
	GLdouble cbCoords[3];
	for (int i = 0; i< 3; i++)
		cbCoords[i] = (double) cubeCoords[i];
	
	bool success = gluProject(cbCoords[0],cbCoords[1],cbCoords[2], getModelViewMatrix(),
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
bool Visualizer::pixelToVector(float winCoords[2], const float camPos[3], float dirVec[3]){
	GLdouble pt[3];
	float v[3];
	//Obtain the coords of a point in view:
	bool success = gluUnProject((GLdouble)winCoords[0],(GLdouble)winCoords[1],(GLdouble)1.0, getModelViewMatrix(),
		getProjectionMatrix(), getViewport(),pt, pt+1, pt+2);
	if (success){
		//Convert point to float
		v[0] = (float)pt[0];
		v[1] = (float)pt[1];
		v[2] = (float)pt[2];
		//transform position to world coords
		//ViewpointParams::localFromStretchedCube(v,dirVec);
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
bool Visualizer::
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
int Visualizer::
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


//Routine to obtain gl matrix from viewpointparams
GLdouble* Visualizer:: 
getModelViewMatrix() {
	return (GLdouble*)getActiveViewpointParams()->getModelViewMatrix();
}
//Routine to set gl matrix in viewpointparams
void Visualizer:: 
setModelViewMatrix(const double mtx[16]) {
	getActiveViewpointParams()->setModelViewMatrix(mtx);
}
//Issue OpenGL commands to draw a grid of lines of the full domain.
//Grid resolution is up to 2x2x2
//
void Visualizer::renderDomainFrame(float* extents, float* minFull, float* maxFull){
	int i; 
	int numLines[3];
	float regionSize, fullSize[3], modMin[3],modMax[3];
	
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

float* Visualizer::cornerPoint(float* extents, int faceNum){
	if(faceNum&1) return extents+3;
	else return extents;
}
bool Visualizer::faceIsVisible(float* extents, float* viewerCoords, int faceNum){
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





/*
 * Insert a renderer to this visualizer
 * Add it after all renderers of lower render order
 */
int Visualizer::
insertRenderer(RenderParams* rp, Renderer* ren, int newOrder)
{
	Params::ParamsBaseType rendType = rp->GetParamsBaseTypeId();
	
	
	mapRenderer(rp, ren);
	//Find a renderer of lower order
	int i;
	for (i = renderer.size()-1; i>= 0; i--){
		if (renderOrder[i] < newOrder) break;	
	}
	int lastPosn = i;
	int maxPosn = renderer.size()-1;
	renderer.push_back(renderer[maxPosn]);
	renderType.push_back(renderType[maxPosn]);
	renderOrder.push_back(renderOrder[maxPosn]);
	for (i = maxPosn-1; i>lastPosn+1; i--){
		renderer[i] = renderer[i-1];
		renderType[i] = renderType[i-1];
		renderOrder[i] = renderOrder[i-1];
	}
	renderer[lastPosn+1] = ren;
	renderType[lastPosn+1] = rendType;
	renderOrder[lastPosn+1] = newOrder;
	ren->initializeGL();
	return lastPosn+1;
}
// Remove all renderers.  This is needed when we load new data into
// an existing session
void Visualizer::removeAllRenderers(){
	
	//Prevent new rendering while we do this?
	
	for (int i = renderer.size()-1; i>=0; i--){
		delete renderer[i];
	}
	
	nowPainting = false;
	
	renderType.clear();
	renderOrder.clear();
	renderer.clear();
		
	rendererMapping.clear();
	
}
/* 
 * Remove renderer of specified renderParams
 */
bool Visualizer::removeRenderer(RenderParams* rp){
	int i;
	assert(!nowPainting);
	
	map<RenderParams*,Renderer*>::iterator find_iter = rendererMapping.find(rp);
	if (find_iter == rendererMapping.end()) {
		return false;
	}
	Renderer* ren = find_iter->second;

	//get it from the renderer list, and delete it:
	for (i = 0; i<renderer.size(); i++) {		
		if (renderer[i] != ren) continue;
		delete renderer[i];
		renderer[i] = 0;
		break;
	}
	int foundIndex = i;
	
	//Remove it from the mapping:
	rendererMapping.erase(find_iter);
	//Move renderers up.
	int numRenderers = renderer.size()-1;
	for (int j = foundIndex; j<numRenderers; j++){
		renderer[j] = renderer[j+1];
		renderType[j] = renderType[j+1];
		renderOrder[j] = renderOrder[j+1];
	}
	renderer.resize(numRenderers);
	renderType.resize(numRenderers);
	renderOrder.resize(numRenderers);
	return true;
}
//Remove a <params,renderer> pair from the list.  But leave them
//both alone.  This is needed when the params change and another params
//needs to use the existing renderer.
//
bool Visualizer::
unmapRenderer(RenderParams* rp){
	map<RenderParams*,Renderer*>::iterator find_iter = rendererMapping.find(rp);
	if (find_iter == rendererMapping.end()) return false;
	rendererMapping.erase(find_iter);
	return true;
}

//find (first) renderer params of specified type:
RenderParams* Visualizer::findARenderer(Params::ParamsBaseType renType){
	
	for (int i = 0; i<renderer.size(); i++) {		
		if (renderType[i] != renType) continue;
		RenderParams* rParams = renderer[i]->getRenderParams();
		return rParams;
	}
	return 0;
}

Renderer* Visualizer::getRenderer(RenderParams* p){
	if(rendererMapping.size() == 0) return 0;
	map<RenderParams*, Renderer*>::iterator found_iter = rendererMapping.find(p);
	if (found_iter == rendererMapping.end()) return 0;
	return found_iter->second;
}


//Clear all render bypass flags for active renderers that match the specified type(s)

void Visualizer::
clearRendererBypass(Params::ParamsBaseType t){
	for (int i = 0; i< renderer.size(); i++){
		if(renderer[i]->isInitialized() && (renderType[i] == t))
			renderer[i]->setAllBypass(false);
	}
}


void Visualizer::placeLights(){
	printOpenGLError();
	ViewpointParams* vpParams = getActiveViewpointParams();
	int nLights = vpParams->getNumLights();
	float lightDirs[3][3];
	for (int j = 0; j<nLights; j++){
		for (int i = 0; i<3; i++){
			lightDirs[j][i] = vpParams->getLightDirection(j,i);
		}
	}
	if (nLights >0){
		printOpenGLError();
		float specColor[4], ambColor[4];
		float diffLight[3], specLight[3];
		GLfloat lmodel_ambient[4];
		specColor[0]=specColor[1]=specColor[2]=0.8f;
		ambColor[0]=ambColor[1]=ambColor[2]=0.f;
		specColor[3]=ambColor[3]=lmodel_ambient[3]=1.f;
		
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, vpParams->getExponent());
		lmodel_ambient[0]=lmodel_ambient[1]=lmodel_ambient[2] = vpParams->getAmbientCoeff();
		//All the geometry will get a white specular color:
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
		glLightfv(GL_LIGHT0, GL_POSITION, lightDirs[0]);
			
		specLight[0] = specLight[1] = specLight[2] = vpParams->getSpecularCoeff(0);
			
		diffLight[0] = diffLight[1] = diffLight[2] = vpParams->getDiffuseCoeff(0);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffLight);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specLight);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambColor);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
		//Following has unpleasant effects on flow line lighting
		//glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
		
		glEnable(GL_LIGHT0);
		printOpenGLError();
		if (nLights > 1){
			printOpenGLError();
			glLightfv(GL_LIGHT1, GL_POSITION, lightDirs[1]);
			specLight[0] = specLight[1] = specLight[2] = vpParams->getSpecularCoeff(1);
			diffLight[0] = diffLight[1] = diffLight[2] = vpParams->getDiffuseCoeff(1);
			glLightfv(GL_LIGHT1, GL_DIFFUSE, diffLight);
			glLightfv(GL_LIGHT1, GL_SPECULAR, specLight);
			glLightfv(GL_LIGHT1, GL_AMBIENT, ambColor);
			glEnable(GL_LIGHT1);
			printOpenGLError();
		} else {
			printOpenGLError();
			glDisable(GL_LIGHT1);
			printOpenGLError();
		}
		if (nLights > 2){
			printOpenGLError();
			glLightfv(GL_LIGHT2, GL_POSITION, lightDirs[2]);
			specLight[0] = specLight[1] = specLight[2] = vpParams->getSpecularCoeff(2);
			diffLight[0] = diffLight[1] = diffLight[2] = vpParams->getDiffuseCoeff(2);
			glLightfv(GL_LIGHT2, GL_DIFFUSE, diffLight);
			glLightfv(GL_LIGHT2, GL_SPECULAR, specLight);
			glLightfv(GL_LIGHT2, GL_AMBIENT, ambColor);
			glEnable(GL_LIGHT2);
			printOpenGLError();
		} else {
			printOpenGLError();
			glDisable(GL_LIGHT2);
			printOpenGLError();
		}
		
	} 
}


// Project the current mouse coordinates to a line in screen space.
// The line starts at the mouseDownPosition, and points in the
// direction resulting from projecting to the screen the axis 
// associated with the dragHandle.  Returns false on error.
bool Visualizer::projectPointToLine(float mouseCoords[2], float projCoords[2]){
	//  State saved at a mouse press is:
	//	mouseDownPoint[2] = P
	//  handleProjVec[2] unit vector (U)
	//
	// When the mouse is moved, project to the line:
	// point Q projects to P + aU, where a = (Q-P).U = dotprod
	float diff[2];
	if (!mouseDownHere) return false;
	diff[0] = mouseCoords[0] - mouseDownPoint[0];
	diff[1] = mouseCoords[1] - mouseDownPoint[1];
	float dotprod = diff[0]*handleProjVec[0]+diff[1]*handleProjVec[1];
	projCoords[0] = mouseDownPoint[0] + dotprod*handleProjVec[0];
	projCoords[1] = mouseDownPoint[1] + dotprod*handleProjVec[1];
	
	return true;
}
bool Visualizer::startHandleSlide(float mouseCoords[2], int handleNum, Params* manipParams){
	// When the mouse is first pressed over a handle, 
	// need to save the
	// windows coordinates of the click, as well as
	// calculate a 2D unit vector in the direction of the slide,
	// projected into the window.

	mouseDownPoint[0] = mouseCoords[0];
	mouseDownPoint[1] = mouseCoords[1];
	//Get the cube coords of the rotation center:
	
	float boxCtr[3]; 
	float winCoords[2] = {0.f,0.f};
	float dispCoords[2];
	
	if (handleNum > 2) handleNum = handleNum-3;
	else handleNum = 2 - handleNum;
	float boxExtents[6];
	int timestep = getActiveAnimationParams()->getCurrentTimestep();
	//manipParams->calcStretchedBoxExtentsInCube(boxExtents, timestep);
	for (int i = 0; i<3; i++){boxCtr[i] = (boxExtents[i] + boxExtents[i+3])*0.5f;}
	// project the boxCtr and one more point, to get a direction vector
	
	if (!projectPointToWin(boxCtr, winCoords)) return false;
	boxCtr[handleNum] += 0.1f;
	if (!projectPointToWin(boxCtr, dispCoords)) return false;
	//Direction vector is difference:
	handleProjVec[0] = dispCoords[0] - winCoords[0];
	handleProjVec[1] = dispCoords[1] - winCoords[1];
	float vecNorm = sqrt(handleProjVec[0]*handleProjVec[0]+handleProjVec[1]*handleProjVec[1]);
	if (vecNorm == 0.f) return false;
	handleProjVec[0] /= vecNorm;
	handleProjVec[1] /= vecNorm;
	return true;
}



Visualizer::OGLVendorType Visualizer::GetVendor()
{
	string ven_str((const char *) glGetString(GL_VENDOR));
	string ren_str((const char *) glGetString(GL_RENDERER));

	for (int i=0; i<ven_str.size(); i++) {
		if (isupper(ven_str[i])) ven_str[i] = tolower(ven_str[i]);
	}
	for (int i=0; i<ren_str.size(); i++) {
		if (isupper(ren_str[i])) ren_str[i] = tolower(ren_str[i]);
	}

	if (
		(ven_str.find("mesa") != string::npos) || 
		(ren_str.find("mesa") != string::npos))  {

		return(MESA);
	}
	else if (
		(ven_str.find("nvidia") != string::npos) || 
		(ren_str.find("nvidia") != string::npos))  {

		return(NVIDIA);
	}
	else if (
		(ven_str.find("ati") != string::npos) || 
		(ren_str.find("ati") != string::npos))  {

		return(ATI);
	}
	else if (
		(ven_str.find("intel") != string::npos) || 
		(ren_str.find("intel") != string::npos))  {

		return(INTEL);
	}

	return(UNKNOWN);
}




void Visualizer::
setValuesFromGui(ViewpointParams* vpparams){
	
	//Same as the version in vizwin, but doesn't force redraw.
	Viewpoint* vp = vpparams->getCurrentViewpoint();
	double transCameraPos[3];
	double cubeCoords[3];
	double vdir[3],upv[3];
	for (int i = 0; i<3; i++) {
		vdir[i] = vp->getViewDir()[i];
		upv[i] = vp->getUpVec()[i];
	}
	
	double mtx[16];
	setMatrixFromFrame(transCameraPos, vdir, upv, cubeCoords,mtx);
	vpparams->setModelViewMatrix(mtx);
	
}

void Visualizer::removeDisabledRenderers(){
	//Repeat until we don't find any renderers to disable:
	
	while(1){
		bool retry = false;
		for (int i = 0; i< renderer.size(); i++){
			RenderParams* rParams = renderer[i]->getRenderParams();
			if (!rParams->isEnabled()) {
				removeRenderer(rParams);
				retry = true;
				break;
			}
		}
		if (!retry) break;
	}
}
		
		
int Visualizer::AddMouseMode(const std::string paramsTag, int manipType, const char* name){
	
	ParamsBase::ParamsBaseType pType = ParamsBase::GetTypeFromTag(paramsTag);
	paramsFromMode.push_back(pType);
	manipFromMode.push_back(manipType);
	modeName.push_back(std::string(name));
	int mode = (int)modeName.size()-1;
	//Setup reverse mapping
	modeFromParams[pType] = mode;
	assert(mode == (int)paramsFromMode.size()-1);
	assert(manipFromMode.size() == mode+1);
	return (mode);
}

// Set the quaternion and translation from a viewer frame
// Also happens to construct modelview matrix, but we don't use its translation
void Visualizer::
setMatrixFromFrame(double* posvec, double* dirvec, double* upvec, double* centerRot, double mvmtrx[16]){
	//First construct the rotation matrix:
	double mtrx1[16];
	double trnsMtrx[16];
	double mtrx[16];
	makeTransMatrix(centerRot, trnsMtrx);
	makeModelviewMatrixD(posvec, dirvec, upvec, mtrx);
	
	//Translate on both sides by translation
	//first on left,
	mmult(trnsMtrx, mtrx, mtrx1);
	//then on right by negative:
	trnsMtrx[12] = -trnsMtrx[12];
	trnsMtrx[13] = -trnsMtrx[13];
	trnsMtrx[14] = -trnsMtrx[14];
	mmult(mtrx1,trnsMtrx, mvmtrx);
}