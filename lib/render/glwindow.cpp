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
#include "ParamsIso.h"
#include "regionparams.h"
#include "probeparams.h"
#include "twoDparams.h"
#include "animationparams.h"
#include "manip.h"
#include "datastatus.h"
#include "vapor/MyBase.h"

#include <math.h>
#include <qgl.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qmutex.h>
#include "assert.h"
#include "vaporinternal/jpegapi.h"
#include "common.h"
#include "flowrenderer.h"
#include "VolumeRenderer.h"

#include "vapor/LayeredIO.h"
using namespace VAPoR;
int GLWindow::activeWindowNum = 0;
bool GLWindow::regionShareFlag = true;
bool GLWindow::defaultTerrainEnabled = false;
bool GLWindow::defaultAxisArrowsEnabled = false;


VAPoR::GLWindow::mouseModeType VAPoR::GLWindow::currentMouseMode = GLWindow::navigateMode;
int GLWindow::jpegQuality = 100;
GLWindow::GLWindow( const QGLFormat& fmt, QWidget* parent, const char* name, int windowNum )
: QGLWidget(fmt, parent, name)

{
	spinThread = 0;
	isSpinning = false;
	timeAnnotLabel = 0;
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
	timeAnnotColor = QColor(white);
	timeAnnotCoords[0] = 0.1f;
	timeAnnotCoords[1] = 0.1f;
	timeAnnotTextSize = 10;
	timeAnnotType = 0;

	
	colorbarBackgroundColor = QColor(black);
	elevColor = QColor(150,75,0);
	renderElevGrid = getDefaultTerrainEnabled();
	surfaceTextureEnabled = false;
	textureRotation = 0;
	textureUpsideDown = false;
	textureFilename = QString("imageFile.jpg");
	elevGridTexture = 0;
	elevGridRefLevel = 0;
	axisArrowsEnabled = getDefaultAxisArrowsEnabled();
	axisAnnotationEnabled = false;
	
	colorbarEnabled = false;
	for (int i = 0; i<3; i++){
	    axisArrowCoord[i] = 0.f;
		axisOriginCoord[i] = 0.f;
		numTics[i] = 6;
		ticLength[i] = 0.05f;
		minTic[i] = 0.f;
		maxTic[i] = 1.f;
		ticDir[i] = 0;
		axisLabelNums[i] = 0;
		axisTextLabels[i] = 0;
	}
	ticDir[0] = 1;
	labelHeight = 10;
	labelDigits = 4;
	ticWidth = 2.f;
	axisColor = QColor(white);

	colorbarLLCoord[0] = 0.1f;
	colorbarLLCoord[1] = 0.1f;
	colorbarURCoord[0] = 0.3f;
	colorbarURCoord[1] = 0.5f;
	numColorbarTics = 11;
	numRenderers = 0;
	for (int i = 0; i< MAXNUMRENDERERS; i++){
		renderType[i] = Params::UnknownParamsType;
		renderOrder[i] = 0;
		renderer[i] = 0;
	}

	vizDirtyBit.clear();
	//setDirtyBit(Params::DvrParamsType,DvrClutBit, true);
	setDirtyBit(ProbeTextureBit, true);
	setDirtyBit(TwoDTextureBit, true);
	//setDirtyBit(Params::DvrParamsType,DvrDatarangeBit, true);
	setDirtyBit(RegionBit, true);
	
	setDirtyBit(ColorscaleBit, true);
    setDirtyBit(LightingBit, true);
	setDirtyBit(DvrRegionBit, true);
	setDirtyBit(ViewportBit, true);
	setDirtyBit(ProjMatrixBit, true);

	capturing = false;
	newCapture = false;

	//Create Manips:
	
	myProbeManip = new TranslateRotateManip(this, 0);
	myTwoDManip = new TranslateStretchManip(this, 0);
	myFlowManip = new TranslateStretchManip(this, 0);
	myRegionManip = new TranslateStretchManip(this, 0);

	elevVert = 0;
	elevNorm = 0;
	maxXElev = 0;
	maxYElev = 0;
	xbeg = ybeg = xfactr = yfactr = 0;
	numElevTimesteps = 0;
	_elevTexid = 0;
	displacement = 0;
	
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
	invalidateElevGrid();
	if (_elevTexid) glDeleteTextures(1, &_elevTexid);
	if (timeAnnotLabel) delete timeAnnotLabel;
}

void GLWindow::setDefaultPrefs(){
	defaultTerrainEnabled = false;
	defaultAxisArrowsEnabled = false;
	jpegQuality = 100;
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
	setDirtyBit(ViewportBit, true);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (perspective) {
		GLfloat w = (float) width / (float) height;
		
		//float mindist = Max(0.2f, wCenter[2]-maxDim-1.0f);
		//glFrustum( -w, w, -h, h, mindist,(wCenter[2]+ maxDim + 1.0) );
		//gluPerspective(45., w, mindist, (wCenter[2]+ maxDim + 10.0) );
		//qWarning("setting near, far dist: %f %f", nearDist, farDist);
		gluPerspective(45., w, nearDist, farDist );
		//gluPerspective(45., w, 1.0, 5. );
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
	float fr, nr, frbx, nrbx;
	vParams->getFarNearDist(rParams, &fr, &nr, &frbx, &nrbx);
	
	farDist = frbx*4.f;
	nearDist = nrbx*0.25f;
	
	needsResize = true;
}


void GLWindow::paintGL()
{
	static int previousTimeStep = -1;
	GLenum	buffer;
	float extents[6] = {0.f,0.f,0.f,1.f,1.f,1.f};
	float minFull[3] = {0.f,0.f,0.f};
	float maxFull[3] = {1.f,1.f,1.f};

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

	DataStatus *dataStatus = DataStatus::getInstance();

	if (!dataStatus->getDataMgr() ||!(dataStatus->renderReady())) {
		swapBuffers();
		nowPainting = false;
		return;
	}

    bool sphericalTransform = (dataStatus && dataStatus->sphericalTransform());
	
	//Modified 2/28/06:  Go ahead and "render" even if no active renderers:
	
	//If we are doing the first capture of a sequence then set the
	//newRender flag to true, whether or not it's a real new render.
	//Then turn off the flag, subsequent renderings will only be captured
	//if they really are new.
	//
	renderNew = captureIsNew();
	if (renderNew) previousTimeStep = -1; //reset saved time step
	setCaptureNew(false);

	//Tell the animation we are starting.  If it returns false, we are not
	//being monitored by the animation controller
	//bool isControlled = AnimationController::getInstance()->beginRendering(winNum);
	
	// Move to trackball view of scene  
	glPushMatrix();
	glLoadIdentity();
	placeLights();

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

	int timeStep = getActiveAnimationParams()->getCurrentFrameNumber();
	//make sure to capture whenever the time step changes
	if (timeStep != previousTimeStep) {
		setRenderNew();
		previousTimeStep = timeStep;
	}
    getActiveRegionParams()->calcStretchedBoxExtentsInCube(extents);
    DataStatus::getInstance()->getMaxStretchedExtentsInCube(maxFull);
	
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

    if(subregionFrameIsEnabled() && 
       !(GLWindow::getCurrentMouseMode() == GLWindow::regionMode) &&
       !sphericalTransform)
    {
      drawSubregionBounds(extents);
    } 

    if (axisArrowsAreEnabled()) drawAxisArrows(extents);
	if (axisAnnotationIsEnabled() && !sphericalTransform) drawAxisTics();


	//render the region geometry, if in region mode, and active visualizer, or region shared
	//with active visualizer
	if(GLWindow::getCurrentMouseMode() == GLWindow::regionMode && 
       (windowIsActive() || 
        (!getActiveRegionParams()->isLocal() && activeWinSharesRegion())) &&
       !sphericalTransform)
    {
		TranslateStretchManip* regionManip = getRegionManip();
		regionManip->setParams(getActiveRegionParams());
		regionManip->render();
	} 
	//render the twoD geometry, if in twoD mode, on active visualizer
	else if((GLWindow::getCurrentMouseMode() == GLWindow::twoDMode) && 
            windowIsActive() && !sphericalTransform){
		
		TranslateStretchManip* twoDManip = getTwoDManip();
		twoDManip->setParams((Params*)getActiveTwoDParams());
		twoDManip->render();
		//Also render the cursor
		draw3DCursor(getActiveTwoDParams()->getSelectedPoint());
	}
	//render the rake geometry, if in rake mode, on active visualizer
	else if((GLWindow::getCurrentMouseMode() == GLWindow::rakeMode) && 
            windowIsActive() && !sphericalTransform){
		
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
	
	if (renderElevGrid) {
			drawElevationGrid(timeStep);
	}
	if (axisAnnotationIsEnabled() && !sphericalTransform) drawAxisLabels();
	else if (axisLabelNums[0]||axisLabelNums[1]||axisLabelNums[2]) deleteAxisLabels();

	//Apply time annotation:
	if (getTimeAnnotType()){
		drawTimeAnnotation();
	} else {
		if(timeAnnotLabel) timeAnnotLabel->hide();
	}
	
	for (int i = 0; i< getNumRenderers(); i++){
		if(renderer[i]->isInitialized() && !(renderer[i]->doAlwaysBypass(timeStep))) renderer[i]->paintGL();
	}
	
	swapBuffers();

	
	//Capture the image, if not navigating:
	if (renderNew && !mouseDownHere) doFrameCapture();

	glPopMatrix();
	//clear dirty bits
	
	setDirtyBit(RegionBit,false);
	setDirtyBit(AnimationBit,false);
	setDirtyBit(DvrRegionBit,false);
	setDirtyBit(ProjMatrixBit, false);
	setDirtyBit(ViewportBit, false);
	setDirtyBit(LightingBit, false);
	nowPainting = false;
	if (isSpinning){
		getTBall()->TrackballSpin();
		setViewerCoordsChanged(true);
		setDirtyBit(ProjMatrixBit, true);
	}

	postRenderCB(winNum, isControlled);
	
}
//Draw a 3D cursor at specified world coords
void GLWindow::draw3DCursor(const float position[3]){
	float cubePosition[3];
	ViewpointParams::worldToStretchedCube(position, cubePosition);
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
	//Check to see if we are using MESA:
	if (GetVendor() == MESA){
		SetErrMsg(VAPOR_ERROR_GL_VENDOR,"GL Vendor String is MESA.\nGraphics drivers may need to be reinstalled");
		
	}
	//VizWinMgr::getInstance()->getDvrRouter()->initTypes();
    qglClearColor(getBackgroundColor()); 		// Let OpenGL clear to black
	//Initialize existing renderers:
	//
	for (int i = 0; i< getNumRenderers(); i++){
		renderer[i]->initializeGL();
	}
    glGenTextures(1, &_elevTexid);
	glBindTexture(GL_TEXTURE_2D, _elevTexid);
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
bool GLWindow::pixelToVector(float winCoords[2], const float camPos[3], float dirVec[3]){
	GLdouble pt[3];
	float v[3];
	//Obtain the coords of a point in view:
	bool success = gluUnProject((GLdouble)winCoords[0],(GLdouble)winCoords[1],(GLdouble)1.0, getModelMatrix(),
		getProjectionMatrix(), getViewport(),pt, pt+1, pt+2);
	if (success){
		//Convert point to float
		v[0] = (float)pt[0];
		v[1] = (float)pt[1];
		v[2] = (float)pt[2];
		//transform position to world coords
		ViewpointParams::worldFromStretchedCube(v,dirVec);
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
	setRegionFrameColorFlt(DataStatus::getRegionFrameColor());
	
	
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
	setSubregionFrameColorFlt(DataStatus::getSubregionFrameColor());
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
	setSubregionFrameColorFlt(DataStatus::getSubregionFrameColor());
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
void GLWindow::setSubregionFrameColorFlt(const QColor& c){
	subregionFrameColorFlt[0]= (float)c.red()/255.;
	subregionFrameColorFlt[1]= (float)c.green()/255.;
	subregionFrameColorFlt[2]= (float)c.blue()/255.;
}
void GLWindow::setRegionFrameColorFlt(const QColor& c){
	regionFrameColorFlt[0]= (float)c.red()/255.;
	regionFrameColorFlt[1]= (float)c.green()/255.;
	regionFrameColorFlt[2]= (float)c.blue()/255.;
}
//Draw an elevation grid (surface) inside the current region extents:
void GLWindow::drawElevationGrid(size_t timeStep){
	//If the region is dirty, or the timestep has changed, must rebuild.
	if (regionIsDirty()) invalidateElevGrid();
	//First, check if we have already constructed the elevation grid vertices.
	//If not, rebuild them:
	if (!elevVert || !elevVert[timeStep]) {
		if(!rebuildElevationGrid(timeStep)) return;
	}
	mxx = maxXElev[timeStep];
	mxy = maxYElev[timeStep];
	xfct = xfactr[timeStep];
	xbg = xbeg[timeStep];
	yfct = yfactr[timeStep];
	ybg = ybeg[timeStep];
	//Establish clipping planes:
	GLdouble topPlane[] = {0., -1., 0., 1.};
	GLdouble rightPlane[] = {-1., 0., 0., 1.0};
	GLdouble leftPlane[] = {1., 0., 0., 0.};
	GLdouble botPlane[] = {0., 1., 0., 0.};
	GLdouble frontPlane[] = {0., 0., -1., 1.};//z largest
	GLdouble backPlane[] = {0., 0., 1., 0.};
	//Apply a coord transform that moves the full domain to the unit cube.
	
	glPushMatrix();
	
	//scale:
	float sceneScaleFactor = 1.f/ViewpointParams::getMaxStretchedCubeSide();
	glScalef(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);

	//translate to put origin at corner:
	float* transVec = ViewpointParams::getMinStretchedCubeCoords();
	glTranslatef(-transVec[0],-transVec[1], -transVec[2]);
	
	//Set up clipping planes
	const float* scales = DataStatus::getInstance()->getStretchFactors();
	RegionParams* myRegionParams = getActiveRegionParams();
	topPlane[3] = myRegionParams->getRegionMax(1)*scales[1];
	botPlane[3] = -myRegionParams->getRegionMin(1)*scales[1];
	leftPlane[3] = -myRegionParams->getRegionMin(0)*scales[0];
	rightPlane[3] = myRegionParams->getRegionMax(0)*scales[0];
	frontPlane[3] = myRegionParams->getRegionMax(2)*scales[2];
	backPlane[3] = -myRegionParams->getRegionMin(2)*scales[2];

	//Calculate x and y stretch factors, translation factors for texture mapping:
	const float* extents = DataStatus::getInstance()->getExtents();
	float stretchX = (myRegionParams->getRegionMax(0)-myRegionParams->getRegionMin(0))/
		(extents[3]-extents[0]);
	float stretchY = (myRegionParams->getRegionMax(1)-myRegionParams->getRegionMin(1))/
		(extents[4]-extents[1]);
	float transX = myRegionParams->getRegionMin(0)/(extents[3]-extents[0]);
	float transY = myRegionParams->getRegionMin(1)/(extents[4]-extents[1]);
	
	glClipPlane(GL_CLIP_PLANE0, topPlane);
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE1, rightPlane);
	glEnable(GL_CLIP_PLANE1);
	glClipPlane(GL_CLIP_PLANE2, botPlane);
	glEnable(GL_CLIP_PLANE2);
	glClipPlane(GL_CLIP_PLANE3, leftPlane);
	glEnable(GL_CLIP_PLANE3);
	glClipPlane(GL_CLIP_PLANE4, frontPlane);
	glEnable(GL_CLIP_PLANE4);
	glClipPlane(GL_CLIP_PLANE5, backPlane);
	glEnable(GL_CLIP_PLANE5);

	glPopMatrix();
	//Set up  color
	float elevGridColor[4];
	elevGridColor[0] = ((float)elevColor.red())/255.f;
	elevGridColor[1] = ((float)elevColor.green())/255.f;
	elevGridColor[2] = ((float)elevColor.blue())/255.f;
	elevGridColor[3] = 1.f;
	ViewpointParams* vpParams = getActiveViewpointParams();
	int nLights = vpParams->getNumLights();
	
	glShadeModel(GL_SMOOTH);
	if (nLights >0){
		glEnable(GL_LIGHTING);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, elevGridColor);
	} else {
		glDisable(GL_LIGHTING);
		glColor3fv(elevGridColor);
	}
	//Check if there is a texture to apply:
	
	if (elevGridTextureEnabled() && !elevGridTexture){
		//Read the texture file:
		
		int rc = read_JPEG_file(textureFilename.ascii(),&elevGridTexture,&elevGridWidth,&elevGridHeight);
		if (!rc) {enableElevGridTexture(false);}
	}
	if (elevGridTextureEnabled()){
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		//Put an identity on the Texture matrix stack.
		glLoadIdentity();
		glTranslatef(transX,transY,0.);
		glScalef(stretchX,stretchY,1.);
		
		glMatrixMode(GL_MODELVIEW);
		glBindTexture(GL_TEXTURE_2D, _elevTexid);
		glEnable(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, elevGridWidth, elevGridHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, elevGridTexture);
	}
	//Now we can just traverse the elev grid, one row at a time:
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	
	for (int j = 0; j< mxy-1; j++){
		glBegin(GL_QUAD_STRIP);
		//float vert[3], norm[3];
		if(elevGridTextureEnabled()) for (int i = 0; i< mxx; i++){
		
			//Each quad is described by sending 4 vertices, i.e. the points indexed by
			//by (i,j+1), (i,j), (i+1,j+1), (i+1,j).  Send 2 at a time.
			
			glNormal3fv(elevNorm[timeStep]+3*(i+(j+1)*mxx));
			glTexCoord2fv(setTexCrd(i,j+1));
			glVertex3fv(elevVert[timeStep]+3*(i+(j+1)*mxx));
			
			glNormal3fv(elevNorm[timeStep]+3*(i+j*mxx));
			glTexCoord2fv(setTexCrd(i,j));
			glVertex3fv(elevVert[timeStep]+3*(i+j*mxx));
		}
		
		else for (int i = 0; i< mxx; i++){ //No texture
		
			//Each quad is described by sending 4 vertices, i.e. the points indexed by
			//by (i,j+1), (i,j), (i+1,j+1), (i+1,j)  
			
			glNormal3fv(elevNorm[timeStep]+3*(i+(j+1)*mxx));
			glVertex3fv(elevVert[timeStep]+3*(i+(j+1)*mxx));
			
			glNormal3fv(elevNorm[timeStep]+3*(i+j*mxx));
			glVertex3fv(elevVert[timeStep]+3*(i+j*mxx));
			
		}
		glEnd();
	}
	
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);
	glDisable(GL_LIGHTING);
	//undo gl state changes
	if (elevGridTextureEnabled()){
		glDepthMask(GL_FALSE);
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
	}

	printOpenGLError();
}
//Handle reorientation of texture by calc of tex coords:
GLfloat* GLWindow::
setTexCrd(int i, int j){
	static GLfloat tcoord[2];
	switch (textureRotation) {
		case (0) :
			tcoord[0] = xbg + (float)i*xfct;
			tcoord[1] = ybg + (float)j*yfct;
			break;
		case (90) :
			tcoord[0] = ybg + (float)j*yfct;
			tcoord[1] = xbg + (float)(mxx - i -1)*xfct;
			break;
		case(180) :
			tcoord[0] = xbg + (float)(mxx - i -1)*xfct;
			tcoord[1] = ybg + (float)(mxy - j -1)*yfct;
			break;
		case(270) :
			tcoord[1] = xbg + (float)i*xfct;
			tcoord[0] = ybg + (float)(mxy - j -1)*yfct;
			break;
		default:
			assert(0);
	}
	if (textureUpsideDown) {
		tcoord[0] = (mxx-1)*xfct - tcoord[0];
	}
	return tcoord;
	
}

//Invalidate array.  Set pointers to zero before deleting so we
//can't accidentally get trapped with bad pointer.
void GLWindow::invalidateElevGrid(){
	if (elevVert){
		for (int i = 0; i<numElevTimesteps; i++){
			if (elevVert[i]){
				float * elevPtr = elevVert[i];
				elevVert[i] = 0;
				delete elevPtr;
				delete elevNorm[i];
				elevNorm[i] = 0;
			}
		}
		float ** tmpArray = elevVert;
		elevVert = 0;
		delete tmpArray;
		delete elevNorm;
		elevNorm = 0;
		numElevTimesteps = 0;
		delete maxXElev;
		delete maxYElev;
		delete xfactr;
		delete yfactr;
		delete xbeg;
		delete ybeg;
	}
}
bool GLWindow::rebuildElevationGrid(size_t timeStep){
	//Reconstruct the elevation grid.
	//First, check that the cache is OK:
	if (!elevVert){
		numElevTimesteps = DataStatus::getInstance()->getMaxTimestep() + 1;
		elevVert = new float*[numElevTimesteps];
		elevNorm = new float*[numElevTimesteps];
		maxXElev = new int[numElevTimesteps];
		maxYElev = new int[numElevTimesteps];
		xfactr = new float[numElevTimesteps];
		yfactr = new float[numElevTimesteps];
		xbeg = new float[numElevTimesteps];
		ybeg = new float[numElevTimesteps];
		for (int i = 0; i< numElevTimesteps; i++){
			elevVert[i] = 0;
			elevNorm[i] = 0;
			maxXElev[i] = 0;
			maxYElev[i] = 0;
			xfactr[i] = 0.f;
			yfactr[i] = 0.f;
			xbeg[i] = 0.f;
			ybeg[i] = 0.f;
		}
	}

	//find the grid coordinate ranges
	size_t min_dim[3], max_dim[3], min_bdim[3],max_bdim[3];
	double regMin[3],regMax[3];
	double regMinOrig[3], regMaxOrig[3];
	DataStatus* ds = DataStatus::getInstance();
	//See if there is a HGT variable
	
	float* elevData = 0;
	float* hgtData = 0;
	DataMgr* dataMgr = ds->getDataMgr();
	LayeredIO* myReader = (LayeredIO*)dataMgr->GetRegionReader();
	float displacement = getDisplacement();
	//Don't allow the terrain surface to be below the minimum extents:
	const float* extents = ds->getExtents();
	float minElev = extents[2]+(0.0001)*(extents[5] - extents[2]);
	int varnum = DataStatus::getSessionVariableNum2D("HGT");
	if (varnum < 0 || !DataStatus::getInstance()->dataIsPresent2D(varnum, timeStep)) {
		//Use Elevation variable as backup:
		varnum = DataStatus::getSessionVariableNum("ELEVATION");
		if (varnum >= 0) {
			// NOTE:  Currently we are clearing cache here just because we need
			// turn interpolation off.  This will not be so painful if we
			// allowed both interpolated and uninterpolated volumes to coexist
			// in the data manager.

			dataMgr->Clear();
			myReader->SetInterpolateOnOff(false);
			//Try to get requested refinement level or the nearest acceptable level:
			int refLevel = getActiveRegionParams()->getAvailableVoxelCoords(elevGridRefLevel, min_dim, max_dim, min_bdim, max_bdim, 
					timeStep, &varnum, 1, regMin, regMax);
			

			if(refLevel < 0) {
				myReader->SetInterpolateOnOff(true);
				return false;
			}
				
			
			//Modify 3rd coord of region extents to obtain only bottom layer:
			min_dim[2] = max_dim[2] = 0;
			min_bdim[2] = max_bdim[2] = 0;
			//Then, ask the Datamgr to retrieve the lowest layer of the ELEVATION data, without
			//performing the interpolation step
			
			elevData = dataMgr->GetRegion(timeStep, "ELEVATION", refLevel, min_bdim, max_bdim, 0);
			myReader->SetInterpolateOnOff(true);
			if (!elevData) {
				if (ds->warnIfDataMissing()){
					SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"ELEVATION data unavailable at timestep %d.\n %s", 
						timeStep,
						"This message can be silenced \nusing the User Preference Panel settings." );
				}
				ds->setDataMissing(timeStep, refLevel, ds->getSessionVariableNum(std::string("ELEVATION")));
				return false;
			}
		}
		else { //setup for just doing flat plane:
			RegionParams* rParams = getActiveRegionParams();
			size_t bbmin[3], bbmax[3];
			rParams->getRegionVoxelCoords(elevGridRefLevel, min_dim, max_dim, bbmin, bbmax);
			for (int i = 0; i<3; i++){
				regMin[i] = rParams->getRegionMin(i);
				regMax[i] = rParams->getRegionMax(i);
				regMinOrig[i] = regMin[i];
				regMaxOrig[i] = regMax[i];
			}
		}
	} else {
		
		RegionParams* rParams = getActiveRegionParams();
		for (int i = 0; i< 3; i++){
			regMin[i] = rParams->getRegionMin(i);
			regMax[i] = rParams->getRegionMax(i);
			regMinOrig[i] = regMin[i];
			regMaxOrig[i] = regMax[i];
		}
		//Try to get requested refinement level or the nearest acceptable level:
		int refLevel = rParams->shrinkToAvailableVoxelCoords(elevGridRefLevel, min_dim, max_dim, min_bdim, max_bdim, 
				timeStep, &varnum, 1, regMin, regMax, true);
		

		if(refLevel < 0) {
			myReader->SetInterpolateOnOff(true);
			return false;
		}
			
		
		//Ignore vertical extents
		min_dim[2] = max_dim[2] = 0;
		min_bdim[2] = max_bdim[2] = 0;
		//Then, ask the Datamgr to retrieve the HGT data
		
		hgtData = dataMgr->GetRegion(timeStep, "HGT", refLevel, min_bdim, max_bdim, 0);
		
		if (!hgtData) {
			if (ds->warnIfDataMissing()){
				SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"HGT data unavailable at timestep %d.\n %s", 
					timeStep,
					"This message can be silenced \nusing the User Preference Panel settings." );
			}
			ds->setDataMissing2D(timeStep, refLevel, ds->getSessionVariableNum2D(std::string("HGT")));
			return false;
		}
	}
	//Then create arrays to hold the vertices and their normals:
	mxx = maxXElev[timeStep] = max_dim[0] - min_dim[0] +1;
	mxy = maxYElev[timeStep] = max_dim[1] - min_dim[1] +1;
	elevVert[timeStep] = new float[3*mxx*mxy];
	elevNorm[timeStep] = new float[3*mxx*mxy];
	
	//Determine linear function that maps points of grid into the
	//subset of interval (0,1)  associated with region.  Interval [0,1] is because the
	// extents are mapped into unit box.
	//There are mxx points evenly spaced
	//going from xbeg to (mxx-1)*xfactr.  
	xbeg[timeStep] = (regMin[0]-regMinOrig[0])/(regMaxOrig[0]-regMinOrig[0]);
	//Bx is the max x coordinate, xbeg is min x coord.
	float Bx = (regMax[0] - regMinOrig[0])/(regMaxOrig[0]-regMinOrig[0]);
	xfactr[timeStep] = (Bx - xbeg[timeStep])/(float)(mxx-1);
	ybeg[timeStep] = (regMin[1]-regMinOrig[1])/(regMaxOrig[1]-regMinOrig[1]);
	float By = (regMax[1] - regMinOrig[1])/(regMaxOrig[1]-regMinOrig[1]);
	yfactr[timeStep] = (By - ybeg[timeStep])/(float)(mxy-1);


	//Then loop over all the vertices in the Elevation or HGT data. 
	//For each vertex, construct the corresponding 3d point as well as the normal vector.
	//These must be converted to
	//stretched cube coordinates.  The x and y coordinates are determined by
	//scaling them to the extents. (Need to know the actual min/max stretched cube extents
	//that correspond to the min/max grid extents)
	//The z coordinate is taken from the data array, converted to 
	//stretched cube coords
	//using parameters in the viewpoint params.

	
	float worldCoord[3];
	const size_t* bs = ds->getMetadata()->GetBlockSize();
	for (int j = 0; j<mxy; j++){
		worldCoord[1] = regMin[1] + (float)j*(regMax[1] - regMin[1])/(float)(mxy-1);
		size_t ycrd = 0; 
		if (varnum >= 0) ycrd = (min_dim[1] - bs[1]*min_bdim[1]+j)*(max_bdim[0]-min_bdim[0]+1)*bs[0];
		for (int i = 0; i<mxx; i++){
			int pntPos = 3*(i+j*mxx);
			worldCoord[0] = regMin[0] + (float)i*(regMax[0] - regMin[0])/(float)(mxx-1);
			if (varnum < 0) { //No elevation data, just use min extents:
				worldCoord[2] = minElev+displacement;
			} else {
				size_t xcrd = min_dim[0] - bs[0]*min_bdim[0]+i;
				if (elevData)
					worldCoord[2] = elevData[xcrd+ycrd] + displacement;
				else {
					worldCoord[2] = hgtData[xcrd+ycrd] + displacement;
				} 
			}
			if (worldCoord[2] < minElev) worldCoord[2] = minElev;
			//Convert and put results into elevation grid vertices:
			ViewpointParams::worldToStretchedCube(worldCoord,elevVert[timeStep]+pntPos);
		}
	}
	//Now calculate normals:
	calcElevGridNormals(timeStep);
	
	return true;
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
void GLWindow::calcElevGridNormals(size_t timeStep){
	const float* stretchFac = DataStatus::getInstance()->getStretchFactors();
	mxx = maxXElev[timeStep];
	mxy = maxYElev[timeStep];
	//Go over the grid of vertices, calculating normals
	//by looking at adjacent x,y,z coords.
	for (int j = 0; j < mxy; j++){
		for (int i = 0; i< mxx; i++){
			float* point = elevVert[timeStep]+3*(i+mxx*j);
			float* norm = elevNorm[timeStep]+3*(i+mxx*j);
			//do differences of right point vs left point,
			//except at edges of grid just do differences
			//between current point and adjacent point:
			float dx=0.f, dy=0.f, dzx=0.f, dzy=0.f;
			if (i>0 && i <mxx-1){
				dx = *(point+3) - *(point-3);
				dzx = *(point+5) - *(point-1);
			} else if (i == 0) {
				dx = *(point+3) - *(point);
				dzx = *(point+5) - *(point+2);
			} else if (i == mxx-1) {
				dx = *(point) - *(point-3);
				dzx = *(point+2) - *(point-1);
			}
			if (j>0 && j <mxy-1){
				dy = *(point+1+3*mxx) - *(point+1 - 3*mxx);
				dzy = *(point+2+3*mxx) - *(point+2 - 3*mxx);
			} else if (j == 0) {
				dy = *(point+1+3*mxx) - *(point+1);
				dzy = *(point+2+3*mxx) - *(point+2);
			} else if (j == mxy-1) {
				dy = *(point+1) - *(point+1 - 3*mxx);
				dzy = *(point+2) - *(point+2 - 3*mxx);
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
void GLWindow::deleteAxisLabels(){
	
	for (int i = 0; i<3; i++){
		if (axisTextLabels[i]) {
			for (int k = 0; k < axisLabelNums[i]; k++) delete axisTextLabels[i][k];
			delete axisTextLabels[i];
			axisTextLabels[i] = 0;
		}
		axisLabelNums[i] = 0;
	}
}
void GLWindow::drawTimeAnnotation(){

	//Always need to check the time:
	size_t timeStep = (size_t)currentAnimationParams->getCurrentFrameNumber(); 
	QString labelContents;
	if (timeAnnotType == 1){
		labelContents = QString("TimeStep: ")+QString::number((int)timeStep) + " ";
	}
	else {//get string from metadata
		const Metadata* md = DataStatus::getInstance()->getMetadata();
		if (!md) labelContents = QString("");
		else {
			const string& timeStamp = md->GetTSUserDataString(timeStep,"UserTimeStampString");
			labelContents = QString("Date/Time: ")+QString(timeStamp.c_str());
		}
	}
	if(!timeAnnotLabel) { //Do we need a new label?
		QFont f;
		f.setPointSize(timeAnnotTextSize);
		timeAnnotLabel = new QLabel(this);
		timeAnnotLabel->setFont(f);
		timeAnnotLabel->setPaletteBackgroundColor(getBackgroundColor());
		timeAnnotLabel->setPaletteForegroundColor(timeAnnotColor);
	}
	
	timeAnnotLabel->setText(labelContents);
	timeAnnotLabel->adjustSize();
	int xposn = (int)(width()*timeAnnotCoords[0]);
	int yposn = (int)(height()*(1.f-timeAnnotCoords[1]));
	timeAnnotLabel->move(xposn, yposn);
	
	if(timeAnnotTextSize>0) timeAnnotLabel->show();
	else timeAnnotLabel->hide();
	
}
void GLWindow::drawAxisLabels() {
	float origin[3], ticMin[3], ticMax[3];
	//Due to a QT bug, must delete all labels and recreate them (during another rendering)
	//if there's a change:
	for (int i = 0; i<3; i++) {
		if (axisLabelNums[i] != 0 && axisLabelNums[i] != numTics[i]) {
			deleteAxisLabels();
			return;
		}
	}
	//Create new QLabels, if necessary:
	for (int i = 0; i<3; i++){
		if (axisLabelNums[i] != numTics[i]){
			assert (!axisTextLabels[i]);
			axisTextLabels[i] = new QLabel*[numTics[i]];
			for (int k = 0; k<numTics[i]; k++){
				axisTextLabels[i][k] = new QLabel(this);
			}
			axisLabelNums[i] = numTics[i];
		}
	}
	//Set up the labels with right colors, size
	QFont f;
	f.setPointSize(labelHeight);
	for (int i = 0; i<3; i++){
		for (int k = 0; k<numTics[i]; k++){
			axisTextLabels[i][k]->setFont(f);
			axisTextLabels[i][k]->setPaletteForegroundColor(axisColor);
			axisTextLabels[i][k]->setPaletteBackgroundColor(getBackgroundColor());
		}
	}

	ViewpointParams::worldToStretchedCube(axisOriginCoord, origin);
	//minTic and maxTic can be regarded as points in world space, defining
	//corners of a box that's projected to axes.
	ViewpointParams::worldToStretchedCube(minTic, ticMin);
	ViewpointParams::worldToStretchedCube(maxTic, ticMax);
	
	float pointOnAxis[3];
	float winCoords[2];
	for (int axis = 0; axis < 3; axis++){
		if (numTics[axis] > 1){
			vcopy(origin, pointOnAxis);
			for (int i = 0; i< numTics[axis]; i++){
				pointOnAxis[axis] = ticMin[axis] + (float)i* (ticMax[axis] - ticMin[axis])/(float)(numTics[axis]-1);
				float labelValue = minTic[axis] + (float)i* (maxTic[axis] - minTic[axis])/(float)(numTics[axis]-1);
				projectPointToWin(pointOnAxis, winCoords);
				int x = (int)(winCoords[0]+2*ticWidth);
				int y = (int)(height()-winCoords[1]+2*ticWidth);
				axisTextLabels[axis][i]->setText(QString::number(labelValue,'g',labelDigits));
				axisTextLabels[axis][i]->adjustSize();
				axisTextLabels[axis][i]->move(x,y);
				if(labelHeight>0) axisTextLabels[axis][i]->show();
				else axisTextLabels[axis][i]->hide();
				
			}
		}
	}
}
void GLWindow::addAxisLabels(unsigned char* buff){
	//apply the existing axis labels to the image in the buffer
	//The buffer must match the dimensions of the current window.
	float origin[3], ticMin[3], ticMax[3];
	
	//The labels must already exist.
	if(labelHeight <= 0) return;

	ViewpointParams::worldToStretchedCube(axisOriginCoord, origin);
	//minTic and maxTic can be regarded as points in world space, defining
	//corners of a box that's projected to axes.
	ViewpointParams::worldToStretchedCube(minTic, ticMin);
	ViewpointParams::worldToStretchedCube(maxTic, ticMax);
	
	float pointOnAxis[3];
	float winCoords[2];
	QFont f;
	f.setPointSize(labelHeight);
	for (int axis = 0; axis < 3; axis++){
		if (numTics[axis] > 1){
			vcopy(origin, pointOnAxis);
			for (int n = 0; n< numTics[axis]; n++){
				pointOnAxis[axis] = ticMin[axis] + (float)n* (ticMax[axis] - ticMin[axis])/(float)(numTics[axis]-1);
				projectPointToWin(pointOnAxis, winCoords);
				int x = (int)(winCoords[0]+2*ticWidth);
				int y = (int)(height()-winCoords[1]+2*ticWidth);
				//Create a new QPixmap to paint into.  
				//Then create a QPainter for the new Pixmap
				//Then use QPainter::drawText() to put the text from the label
				//into the QPixmap
				//convert the QPixmap to a QImage
				
				int wid = axisTextLabels[axis][n]->width();
				int ht = axisTextLabels[axis][n]->height();
				QPixmap myPixmap(wid,ht);
				myPixmap.fill(getBackgroundColor());
				QPainter myPainter(&myPixmap);
				myPainter.setPen(axisColor);

				myPainter.setFont(f);
				
				const QString& labelText = axisTextLabels[axis][n]->text();
				myPainter.drawText(0,0,wid,ht,Qt::AlignCenter|Qt::SingleLine,labelText,-1);	
				QImage image = myPixmap.convertToImage();
				assert(image != 0);
				//Write the image to the buffer:
				int stride = 3*width();
				for (int j = 0; j<image.height(); j++){
					if (j+y >= height()) continue;
					for (int i = 0; i< image.width(); i++){
						if (i+x >= width()) continue;
						QRgb pixval = image.pixel(i,j);
						//assert(pixval == 0);
						buff[(x+i)*3 + stride*(y+j)] = (unsigned char)(qRed(pixval));
						buff[(x+i)*3 + stride*(y+j)+1] = (unsigned char)(qGreen(pixval));
						buff[(x+i)*3 + stride*(y+j)+2] = (unsigned char)(qBlue(pixval));
					}
				}
				
			}
		}
	}

}
//Apply the time stamps to the buffer so it will
//show up in jpegs
void GLWindow::addTimeToBuffer(unsigned char* buff){
	if (!timeAnnotLabel || timeAnnotTextSize <= 0) return;
	//Create a new QPixmap to paint into.  
	//Then create a QPainter for the new Pixmap
	//Then use QPainter::drawText() to put the text from the label
	//into the QPixmap
	//convert the QPixmap to a QImage
	int xposn = (int)(width()*timeAnnotCoords[0]);
	int yposn = (int)(height()*(1.f-timeAnnotCoords[1]));
	int wid = timeAnnotLabel->width();
	int ht = timeAnnotLabel->height();
	QPixmap myPixmap(wid,ht);
	myPixmap.fill(getBackgroundColor());
	QPainter myPainter(&myPixmap);
	myPainter.setPen(timeAnnotColor);
	QFont f;
	f.setPointSize(timeAnnotTextSize);

	myPainter.setFont(f);
				
	const QString& labelText = timeAnnotLabel->text();
	myPainter.drawText(0,0,wid,ht,Qt::AlignCenter|Qt::SingleLine,labelText,-1);	
	QImage image = myPixmap.convertToImage();
	assert(image != 0);
	//Write the image to the buffer:
	int stride = 3*width();
	for (int j = 0; j<image.height(); j++){
		if (j+yposn >= height()) continue;
		for (int i = 0; i< image.width(); i++){
			if (i+xposn >= width()) continue;
			QRgb pixval = image.pixel(i,j);
			//assert(pixval == 0);
			buff[(xposn+i)*3 + stride*(yposn+j)] = (unsigned char)(qRed(pixval));
			buff[(xposn+i)*3 + stride*(yposn+j)+1] = (unsigned char)(qGreen(pixval));
			buff[(xposn+i)*3 + stride*(yposn+j)+2] = (unsigned char)(qBlue(pixval));
		}
	}
				
}
void GLWindow::drawAxisTics(){
	float origin[3], ticMin[3], ticMax[3], ticLen[3];
	ViewpointParams::worldToStretchedCube(axisOriginCoord, origin);
	//minTic and maxTic can be regarded as points in world space, defining
	//corners of a box that's projected to axes.
	ViewpointParams::worldToStretchedCube(minTic, ticMin);
	ViewpointParams::worldToStretchedCube(maxTic, ticMax);
	//TicLength needs to be stretched based on which axes are used for tic direction
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	float maxStretchedCubeSide = ViewpointParams::getMaxStretchedCubeSide();
	for (int i = 0; i<3; i++){
		int j = ticDir[i];
		ticLen[i] = ticLength[i]*stretch[j]/maxStretchedCubeSide;
	}
	
	glDisable(GL_LIGHTING);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f((float)axisColor.red()/255.f,(float)axisColor.green()/255.f, (float)axisColor.blue()/255.f);
	glLineWidth(ticWidth);
	//Draw lines on x-axis:
	glBegin(GL_LINES);
	glVertex3f(ticMin[0],origin[1],origin[2]);
	glVertex3f(ticMax[0],origin[1],origin[2]);
	glVertex3f(origin[0],ticMin[1],origin[2]);
	glVertex3f(origin[0],ticMax[1],origin[2]);
	glVertex3f(origin[0],origin[1],ticMin[2]);
	glVertex3f(origin[0],origin[1],ticMax[2]);
	float pointOnAxis[3];
	float ticVec[3], drawPosn[3];
	//Now draw tic marks for x:
	if (numTics[0] > 1 && ticLength[0] > 0.f){
		pointOnAxis[1] = origin[1];
		pointOnAxis[2] = origin[2];
		ticVec[0] = 0.f; ticVec[1] = 0.f; ticVec[2] = 0.f;
		if (ticDir[0] == 1) ticVec[1] = ticLen[0]; 
		else ticVec[2] = ticLen[0];
		for (int i = 0; i< numTics[0]; i++){
			pointOnAxis[0] = ticMin[0] + (float)i* (ticMax[0] - ticMin[0])/(float)(numTics[0]-1);
			vsub(pointOnAxis, ticVec, drawPosn);
			glVertex3fv(drawPosn);
			vadd(pointOnAxis, ticVec, drawPosn);
			glVertex3fv(drawPosn);
		}
	}
	//Now draw tic marks for y:
	if (numTics[1] > 1 && ticLen[1] > 0.f){
		pointOnAxis[0] = origin[0];
		pointOnAxis[2] = origin[2];
		ticVec[0] = 0.f; ticVec[1] = 0.f; ticVec[2] = 0.f;
		if (ticDir[1] == 0) ticVec[0] = ticLen[1]; 
		else ticVec[2] = ticLen[1];
		for (int i = 0; i< numTics[1]; i++){
			pointOnAxis[1] = ticMin[1] + (float)i* (ticMax[1] - ticMin[1])/(float)(numTics[1]-1);
			vsub(pointOnAxis, ticVec, drawPosn);
			glVertex3fv(drawPosn);
			vadd(pointOnAxis, ticVec, drawPosn);
			glVertex3fv(drawPosn);
		}
	}
	//Now draw tic marks for z:
	if (numTics[2] > 1 && ticLen[2] > 0.f){
		pointOnAxis[0] = origin[0];
		pointOnAxis[1] = origin[1];
		ticVec[0] = 0.f; ticVec[1] = 0.f; ticVec[2] = 0.f;
		if (ticDir[2] == 0) ticVec[0] = ticLen[2]; 
		else ticVec[1] = ticLen[2];
		for (int i = 0; i< numTics[2]; i++){
			pointOnAxis[2] = ticMin[2] + (float)i* (ticMax[2] - ticMin[2])/(float)(numTics[2]-1);
			vsub(pointOnAxis, ticVec, drawPosn);
			glVertex3fv(drawPosn);
			vadd(pointOnAxis, ticVec, drawPosn);
			glVertex3fv(drawPosn);
		}
	}
	glEnd();
	
}

void GLWindow::drawAxisArrows(float* extents){
	float origin[3];
	float maxLen = -1.f;
	for (int i = 0; i<3; i++){
		origin[i] = extents[i] + (getAxisArrowCoord(i))*(extents[i+3]-extents[i]);
		if (extents[i+3] - extents[i] > maxLen) {
			maxLen = extents[i+3] - extents[i];
		}
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
 * Insert a renderer to this visualization window
 * Add it after all renderers of lower render order
 */
void GLWindow::
insertRenderer(RenderParams* rp, Renderer* ren, int newOrder)
{
	Params::ParamType rendType = rp->getParamType();
	
	if (numRenderers < MAXNUMRENDERERS){
		mapRenderer(rp, ren);
		//Move every renderer with higher order up:
		int i;
		for (i = numRenderers; i> 0; i--){
			if (renderOrder[i-1] >= newOrder){
				renderOrder[i] = renderOrder[i-1];
				renderer[i] = renderer[i-1];
				renderType[i] = renderType[i-1];
			}
			else break; //found a renderer that has lower order
		}
		renderer[i] = ren;
		renderType[i] = rendType;
		renderOrder[i] = newOrder;
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
	assert(!isPainting());
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
		renderOrder[j] = renderOrder[j+1];
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

//set the dirty bit, 
void GLWindow::
setDirtyBit(DirtyBitType t, bool nowDirty){
	vizDirtyBit[t] = nowDirty;
	if (nowDirty){
		if (t == RegionBit){
			//Must clear all iso, dvr, and flow bypass flags
			clearRendererBypass((Params::ParamType)(Params::DvrParamsType|Params::IsoParamsType|Params::FlowParamsType));
		} else if (t == DvrRegionBit) {
			clearRendererBypass((Params::ParamType)(Params::DvrParamsType|Params::IsoParamsType));
		}
	}
}
//Clear all render bypass flags for active renderers that match the specified type(s)
//The renderer type can be an 'or' of multiple types.
void GLWindow::
clearRendererBypass(Params::ParamType t){
	for (int i = 0; i< getNumRenderers(); i++){
		if(renderer[i]->isInitialized() && (renderType[i]&t))
			renderer[i]->setAllBypass(false);
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
		SetErrMsg(VAPOR_ERROR_IMAGE_CAPTURE,"Image Capture Error: Error opening output Jpeg file: %s",filename.ascii());
		capturing = 0;
		return;
	}
	//Get the image buffer 
	unsigned char* buf = new unsigned char[3*width()*height()];
	//Use openGL to fill the buffer:
	if(!getPixelData(buf)){
		//Error!
		SetErrMsg(VAPOR_ERROR_IMAGE_CAPTURE,"Image Capture Error; error obtaining GL data");
		capturing = 0;
		delete buf;
		return;
	}
	//Add axis labels if necessary:
	if (axisAnnotationIsEnabled() && !DataStatus::getInstance()->sphericalTransform()) {
		addAxisLabels(buf);
	}
	//Add time stamps if necessary
	if (timeAnnotType && timeAnnotLabel){
		addTimeToBuffer(buf);
	}
	//Now call the Jpeg library to compress and write the file
	//
	int quality = getJpegQuality();
	int rc = write_JPEG_file(jpegFile, width(), height(), buf, quality);
	if (rc){
		//Error!
		SetErrMsg(VAPOR_ERROR_IMAGE_CAPTURE,"Image Capture Error; Error writing jpeg file %s",
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
	//Apply stretch factor:
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<3; i++) temp[i] = stretch[i]*temp[i];
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
		case Params::TwoDParamsType :
			setActiveTwoDParams((TwoDParams*)p);
			return;
		case Params::DvrParamsType :
			setActiveDvrParams((DvrParams*)p);
			return;
		case Params::IsoParamsType :
			setActiveIsoParams((ParamsIso*)p);
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

void GLWindow::placeLights(){
	ViewpointParams* vpParams = getActiveViewpointParams();
	int nLights = vpParams->getNumLights();
	if (nLights >0){
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
		glLightfv(GL_LIGHT0, GL_POSITION, vpParams->getLightDirection(0));
			
		specLight[0] = specLight[1] = specLight[2] = vpParams->getSpecularCoeff(0);
			
		diffLight[0] = diffLight[1] = diffLight[2] = vpParams->getDiffuseCoeff(0);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffLight);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specLight);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambColor);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
		//Following has unpleasant effects on flow line lighting
		//glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
		
		glEnable(GL_LIGHT0);
		if (nLights > 1){
			glLightfv(GL_LIGHT1, GL_POSITION, vpParams->getLightDirection(1));
			specLight[0] = specLight[1] = specLight[2] = vpParams->getSpecularCoeff(1);
			diffLight[0] = diffLight[1] = diffLight[2] = vpParams->getDiffuseCoeff(1);
			glLightfv(GL_LIGHT1, GL_DIFFUSE, diffLight);
			glLightfv(GL_LIGHT1, GL_SPECULAR, specLight);
			glLightfv(GL_LIGHT1, GL_AMBIENT, ambColor);
			glEnable(GL_LIGHT1);
		} else {
			glDisable(GL_LIGHT1);
		}
		if (nLights > 2){
			glLightfv(GL_LIGHT2, GL_POSITION, vpParams->getLightDirection(2));
			specLight[0] = specLight[1] = specLight[2] = vpParams->getSpecularCoeff(2);
			diffLight[0] = diffLight[1] = diffLight[2] = vpParams->getDiffuseCoeff(2);
			glLightfv(GL_LIGHT2, GL_DIFFUSE, diffLight);
			glLightfv(GL_LIGHT2, GL_SPECULAR, specLight);
			glLightfv(GL_LIGHT2, GL_AMBIENT, ambColor);
			glEnable(GL_LIGHT2);
		} else {
			glDisable(GL_LIGHT2);
		}
		
	} 
}

int GLWindow::getJpegQuality() {return jpegQuality;}
void GLWindow::setJpegQuality(int qual){jpegQuality = qual;}

// Project the current mouse coordinates to a line in screen space.
// The line starts at the mouseDownPosition, and points in the
// direction resulting from projecting to the screen the axis 
// associated with the dragHandle.  Returns false on error.
bool GLWindow::projectPointToLine(float mouseCoords[2], float projCoords[2]){
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
bool GLWindow::startHandleSlide(float mouseCoords[2], int handleNum, Params* manipParams){
	// When the mouse is first pressed over a handle, 
	// need to save the
	// windows coordinates of the click, as well as
	// calculate a 2D unit vector in the direction of the slide,
	// projected into the window.

	mouseDownPoint[0] = mouseCoords[0];
	mouseDownPoint[1] = mouseCoords[1];
	//Get the cube coords of the rotation center:
	
	float boxCtr[3]; 
	float winCoords[2];
	float dispCoords[2];
	
	if (handleNum > 2) handleNum = handleNum-3;
	else handleNum = 2 - handleNum;
	float boxExtents[6];
	manipParams->calcStretchedBoxExtentsInCube(boxExtents);
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



GLWindow::OGLVendorType GLWindow::GetVendor()
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

void GLWindow::startSpin(int renderMS){
	spinThread = new SpinThread(this, renderMS);
	spinThread->start();
	isSpinning = true;
	//Increment the rotation and request another render..
	getTBall()->TrackballSpin();
	setViewerCoordsChanged(true);
	setDirtyBit(ProjMatrixBit, true);
	update();
}
bool GLWindow::stopSpin(){
	if (isSpinning && spinThread){
		spinThread->setWindow(0);
		spinThread->finish();
		delete spinThread;
		spinThread = 0;
		isSpinning = false;
		return true;
	}
	return false;
}
//control the repeated display of spinning scene, by repeatedly doing updateGL() on the GLWindow
void SpinThread::run(){

	while(1){
		if (!spinningWindow) return;
		spinningWindow->update();
		msleep(renderTime);
		
	}
}