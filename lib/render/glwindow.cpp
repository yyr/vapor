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
#include "glutil.h"	// Must be included first!!!
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
#include <vapor/GetAppPath.h>

#include "glwindow.h"
#include "trackball.h"
#include "renderer.h"
#include "viewpointparams.h"
#include "dvrparams.h"

#include "regionparams.h"
#include "probeparams.h"

#include "animationparams.h"
#include "manip.h"
#include "datastatus.h"
#include <vapor/MyBase.h>

#include <algorithm>
#include <vector>
#include <math.h>
#include <qgl.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qmutex.h>
#include <qrect.h>
#include "tiffio.h"
#include "assert.h"
#include <vapor/jpegapi.h>
#include <vapor/common.h>
#include <qapplication.h>
#include "flowrenderer.h"
#include "VolumeRenderer.h"
#include "spintimer.h"

#include "textRenderer.h"

using namespace VAPoR;
int GLWindow::activeWindowNum = 0;
bool GLWindow::regionShareFlag = true;
bool GLWindow::defaultTerrainEnabled = false;
bool GLWindow::defaultSpinAnimateEnabled = false;
bool GLWindow::spinAnimate = false;
bool GLWindow::defaultAxisArrowsEnabled = false;
bool GLWindow::nowPainting = false;
bool GLWindow::depthPeeling = false;
/* note: 
 * GL_ENUMS used by depth peeling for attaching the color buffers, currently 16 named points exist
 */
GLenum attach_points[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7, GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11, GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15};

int GLWindow::currentMouseMode = GLWindow::navigateMode;
vector<int> GLWindow::manipFromMode;
vector<string> GLWindow::modeName;
map<ParamsBase::ParamsBaseType, int> GLWindow::modeFromParams;
vector<ParamsBase::ParamsBaseType> GLWindow::paramsFromMode;
int GLWindow::jpegQuality = 100;
GLWindow::GLWindow( QGLFormat& fmt, QWidget* parent, int windowNum )
: QGLWidget(fmt, parent)

{

	_readyToDraw = false;
	

	currentParams.clear();
	for (int i = 0; i<= Params::GetNumParamsClasses(); i++)
		currentParams.push_back(0);

	MyBase::SetDiagMsg("GLWindow::GLWindow() begin");
	
	mySpinTimer = 0;
	isSpinning = false;

	winNum = windowNum;
	rendererMapping.clear();
	assert(rendererMapping.size() == 0);
	setViewerCoordsChanged(true);
	setAutoBufferSwap(false);
	setAutoFillBackground(false);
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
	timeAnnotDirty = true;
	axisLabelsDirty = true;
	textRenderersDirty = true;
	latLonAnnot = false;
	for (int axis=0; axis < 3; axis++) axisLabels[axis].clear();
	
	mouseDownHere = false;

	//values of features:
	timeAnnotColor = QColor(Qt::white);
	timeAnnotCoords[0] = 0.1f;
	timeAnnotCoords[1] = 0.1f;
	timeAnnotTextSize = 10;
	timeAnnotType = 0;

	
	colorbarBackgroundColor = QColor(Qt::black);
	
	
	axisArrowsEnabled = getDefaultAxisArrowsEnabled();
	axisAnnotationEnabled = false;
	
	
	for (int i = 0; i<3; i++){
	    axisArrowCoord[i] = 0.f;
		axisOriginCoord[i] = 0.;
		numTics[i] = 6;
		ticLength[i] = 0.05;
		minTic[i] = 0.;
		maxTic[i] = 1.;
		ticDir[i] = 0;
		axisLabelNums[i] = 0;
	}
	ticDir[0] = 1;
	labelHeight = 10;
	labelDigits = 4;
	ticWidth = 2.f;
	axisColor = QColor(Qt::white);

	colorbarFontsize = 10;
	colorbarDigits = 3;
	
	colorbarSize[0] = 0.15f;
	colorbarSize[1] = 0.3f;
	numColorbarTics = 6;
	numRenderers = 0;
	//Determine how many renderers have transfer functions:
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		RenderParams* p = dynamic_cast<RenderParams*>(Params::GetDefaultParams(i));
		if (!p) continue;
		if (!p->UsesMapperFunction()) continue;
		rendererTypeLookup.push_back(p->GetParamsBaseTypeId());
	}
	int numTFs = rendererTypeLookup.size();
	
	colorbarLLX.clear();
	colorbarLLY.clear();
	colorbarEnabled.clear();
	colorbarTitles.clear();
	float barLLX = 0.;
	for (int i = 0; i<numTFs; i++){
		colorbarLLX.push_back(barLLX);
		colorbarLLY.push_back(0.f);
		colorbarEnabled.push_back(false);
		colorbarTitles.push_back(" ");
		if (colorbarSize[0]+barLLX+.025 <=1.0) barLLX += colorbarSize[0]+0.025;
	}

	for (int i = 0; i< MAXNUMRENDERERS; i++){
		renderType[i] = 0;
		renderOrder[i] = 0;
		renderer[i] = 0;
	}

	vizDirtyBit.clear();
	
	setDirtyBit(RegionBit, true);
	
	setDirtyBit(LightingBit, true);
	setDirtyBit(NavigatingBit, true);
	setDirtyBit(ViewportBit, true);
	setDirtyBit(ProjMatrixBit, true);

	capturingFlow = false;
	
	capturingImage = false;
	newCaptureImage = false;

	//Create Manips for every mode except 0
	manipHolder.push_back(0);
	for (int i = 1; i< manipFromMode.size(); i++){
		if (manipFromMode[i]==1 || manipFromMode[i] == 2){
			TranslateStretchManip* manip = new TranslateStretchManip(this,0);
			manipHolder.push_back(manip);
		} else if (manipFromMode[i]==3){
			TranslateRotateManip* manip = new TranslateRotateManip(this, 0);
			manipHolder.push_back(manip);
		} else assert(0);
	}

	//Assume shaders in exec dir
	QString path = qApp->applicationDirPath();
	//GL_CONTEXT must be current if shaderMgr is to be instantiated
	makeCurrent();
	vector <string> paths;
	paths.push_back("shaders");
	string shaderPaths = GetAppPath("VAPOR", "share", paths);
#ifdef DEBUG
	cout << "shaderPaths " << shaderPaths << endl;
#endif
	/* note: 1
	 * instantiate the shaderMgr
	 * query the maxmium supported # of tex units, in order to set
	 *  the texture unit used by depth peeling to the last one possible
	 * reset the currentLayer, used by depth peeling to keep track of 
	 *  which layer it is on
	 */
	manager = new ShaderMgr(shaderPaths, this);
	currentLayer = 0;
	peelInitialized = false;
#ifdef DARWIN
	//Apple OpenGL driver reports an incorrect # of units for support, throws error if try to use higher than MAX_TEX_UNITS=MAX_TEXTURE_COORDINATES
	depthTexUnit = manager->maxTexUnits(true) - 1;
#else
	//we really want the last available programmable tex unit for the depth unit, otherwise chance of texture collision with raycaster on modern cards
	//as they usually have only 4 fixed function units
	depthTexUnit = manager->maxTexUnits(false) - 1; 
#endif    
	//note: force depth peeling data to null and check extension support
	layers = NULL;
	if (depthPeeling){
	  //depth peeling enabled, check to see if possible to use
	  if(!manager->supportsExtension("GL_ARB_framebuffer_object")){
	    SetErrMsg("OpenGL Context DOES NOT support depth peeling extensions");
	    depthPeeling = false;
	  }      
	}
#ifdef DEBUG // note: print out all available GL driver information
	cout << manager->GLVendor() << endl;
	cout << manager->GLRenderer() << endl;
	cout << manager->GLVersion() << endl;
	cout << manager->GLExtensions() << endl;
	cout << manager->GLShaderVersion() << endl;
	cout << "Max OpenGL programmable texture units: " << manager->maxTexUnits(false) << endl;
	cout << "Max OpenGL fixed texture units: " << manager->maxTexUnits(true) << endl;
#endif
    MyBase::SetDiagMsg("GLWindow::GLWindow() end");
}


/*!
  Release allocated resources.
*/

GLWindow::~GLWindow()
{
	if (isSpinning){
		stopSpin();
	}
	makeCurrent();
	for (int i = 0; i< getNumRenderers(); i++){
		delete renderer[i];
		clearTextObjects(renderer[i]);
	}
	setNumRenderers(0);
	
	delete manager;
	nowPainting = false;

}

void GLWindow::setDefaultPrefs(){
	defaultTerrainEnabled = false;
	defaultSpinAnimateEnabled = false;
	defaultAxisArrowsEnabled = false;
	jpegQuality = 100;
}
//
//  Set up the OpenGL view port, matrix mode, etc.
//

void GLWindow::resizeGL( int width, int height )
{
  setUpViewport(width, height);
  nowPainting = false;
  needsResize = false;

  /* note: 2
   * currently all depth peeling setup occurs in resizeGL, code that should be moved to
   *  initializeGL will be marked with an refactor note
   */
  depthWidth = width;
  depthHeight = height;
  manager->loadShaders();
#ifdef DEBUG
  //print out the loaded effects
  manager->printEffects();
  cout << "resizeGL called, depthpeeling?: " << (int)depthPeeling << endl;
#endif

  //clean up gl resources used if they were used before
  if(depthPeeling){
    if(glIsTexture(depthA))
      glDeleteTextures(1, &depthA);
    if(glIsTexture(depthB))
      glDeleteTextures(1, &depthB);
    if(glIsFramebuffer(fboA))
      glDeleteFramebuffers(1, &fboA);
    if(glIsFramebuffer(fboB))
      glDeleteFramebuffers(1, &fboB);
    if(layers !=NULL){
      glDeleteTextures(maxbuffers, layers);
      free(layers);
    }
    if(manager->effectExists("depthpeeling")){
	manager->undefEffect("depthpeeling");
    }
    if(manager->effectExists("texSampler")){
      manager->undefEffect("texSampler");
    }
#ifdef DEBUG
    printOpenGLError();
    cout << "resizeGL beginning" << endl;
#endif
    //create shaders used for depth peeling
    //note: refactor to initializeGL
    manager->defineEffect("depthpeeling", "", "depthpeeling");

    manager->defineEffect("texSampler", "", "texSampler");
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxbuffers);
    //note: hard code maxbuffers to an acceptable low value
    maxbuffers = maxbuffers > 4 ? 4:maxbuffers;
    glActiveTexture(GL_TEXTURE0 + depthTexUnit);
    GLsizei myheight =  (GLsizei)height;
    GLsizei mywidth = (GLsizei) width;
    //DEPTH PEELING
    //This depth peeling implementation makes use of OpenGL Frame Buffer Obects
    //Two depth and maximum amount of color textures will be used for rendering and depth peeling
	
    //Generate and configure depth textures 
    //note: refactor to initializeGL
    glGenTextures(1, &depthA);
    glGenTextures(1, &depthB); 

#ifdef DEBUG
    printOpenGLError();
    cout << "resizeGL created textures" << endl;
#endif
    glBindTexture(GL_TEXTURE_2D, depthA);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE); //no compare mode for depth texture
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
    glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, mywidth, myheight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	
    glBindTexture(GL_TEXTURE_2D, depthB);
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

    //Create framebuffers used in depth peeling
    //note: refactor to initializeGL
    glGenFramebuffers(1, &fboA);
    glGenFramebuffers(1, &fboB);
	  
    //Attach the first depth buffer to the first FBO
    //note: refactor to intializeGL
    glBindFramebuffer(GL_FRAMEBUFFER, fboA);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthA, 0);	

    //Attach the second depth buffer to the second FBO
    //note: refactor to initializeGL
    glBindFramebuffer(GL_FRAMEBUFFER, fboB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthB, 0);	
	
    //Depth buffers are setup, now we need to setup the color textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0); //prevent framebuffers from being messed with

#ifdef DEBUG
    printOpenGLError();
    cout << "resizeGL framebuffers complete" << endl;
#endif

    //We create as many color buffers as allowed on hardware
    //note: refactor to initializeGL
    layers = (GLuint*)malloc(maxbuffers * sizeof(GLuint));
    glGenTextures(maxbuffers, layers);

    //initialize the color texture information
    //note: keep here since this deals with texture size
    for (int i=0; i<maxbuffers; i++) {
      glBindTexture(GL_TEXTURE_2D, layers[i]);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);	
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mywidth, myheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);	
#ifdef DEBUG
      std::cout << "layers[" << i << "] :" << layers[i] << " created" << std::endl;
#endif
    }
    //prevent textures from being messed with
    glBindTexture(GL_TEXTURE_2D, 0); 
    printOpenGLError();

    //Attach all of the available color buffers to the FBO
    //note: refactor to initializeGL
    glBindFramebuffer(GL_FRAMEBUFFER, fboA);
    for (int i=0; i<maxbuffers; i++) {
      glFramebufferTexture2D(GL_FRAMEBUFFER, attach_points[i], GL_TEXTURE_2D, layers[i], 0);			
    }
    peelInitialized = true;
    //Check that the framebuffer is ready to render to
    //note: refactor to initializeGL
    glBindFramebuffer(GL_FRAMEBUFFER, fboA);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE){
      peelInitialized = false;
      SetErrMsg("framebuffer A not ready");
    }

    //Attach all of the available buffers to the FBO
    //note: refactor to initializeGL
    glBindFramebuffer(GL_FRAMEBUFFER, fboB);
    for (int i=0; i<maxbuffers; i++) {
      glFramebufferTexture2D(GL_FRAMEBUFFER, attach_points[i], GL_TEXTURE_2D, layers[i], 0);			
    }
    //Check that the framebuffer is ready to render to
    //note: refactor to initializeGL
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE){
      peelInitialized = false;
      SetErrMsg("framebuffer B not ready");
    }
	
#ifdef DEBUG
    cout << "resizeGL complete" << endl;
#endif
    //clear framebuffer binding, initialize depth peeling shaders with uniforms
    //note: leave here since this deals with texture size information
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    manager->uploadEffectData(std::string("depthpeeling"), std::string("previousPass"), depthTexUnit);
    manager->uploadEffectData(std::string("depthpeeling"), std::string("height"), (float)height);
    manager->uploadEffectData(std::string("depthpeeling"), std::string("width"), (float)width);	

    manager->uploadEffectData(std::string("texSampler"), std::string("texUnit"), depthTexUnit);
    manager->uploadEffectData(std::string("texSampler"), std::string("height"), (float)height);
    manager->uploadEffectData(std::string("texSampler"), std::string("width"), (float)width);
    glActiveTexture(GL_TEXTURE0);
    printOpenGLError();
  }		
}
void GLWindow::setUpViewport(int width,int height){
   
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
	printOpenGLError();
	
}
	
void GLWindow::resetView(ViewpointParams* vParams){
	//Get the nearest and furthest distance to region from current viewpoint
	float frbx, nrbx;
	vParams->getFarNearDist(&frbx, &nrbx);
	
	farDist = frbx * 4.f;
	nearDist = nrbx * 0.25f;
	needsResize = true;
}
//GL render calls separated for use in depth peeling.
void GLWindow::renderScene(float extents[6], float minFull[3], float maxFull[3], int timeStep){
  //If we are in regionMode, or if the preferences specify the full box will be rendered,
  //draw it now, before the renderers do their thing.
  if(regionFrameIsEnabled()|| GLWindow::getCurrentMouseMode() == GLWindow::regionMode){
    renderDomainFrame(extents, minFull, maxFull);
  } 
  //In regionMode or if preferences specify it, draw the subregion frame.
  if(subregionFrameIsEnabled() && 
     !(GLWindow::getCurrentMouseMode() == GLWindow::regionMode) )
    {
      drawSubregionBounds(extents);
    } 

  //Draw axis arrows if enabled.
  if (axisArrowsAreEnabled()) drawAxisArrows(extents);

  //render the region manipulator, if in region mode, and active visualizer, or region shared
  //with active visualizer.  Possibly redundant???
  if(getCurrentMouseMode() == GLWindow::regionMode) { 
    if( (windowIsActive() || 
	 (!getActiveRegionParams()->isLocal() && activeWinSharesRegion()))){
      TranslateStretchManip* regionManip = getManip(Params::_regionParamsTag);
      regionManip->setParams(getActiveRegionParams());
      regionManip->render();
    } 
  }
  //Render other manips, if we are in appropriate mode:
  //Note: Other manips don't have shared and local to deal with:
  else if ((getCurrentMouseMode() != navigateMode) && windowIsActive()){
    int mode = getCurrentMouseMode();
    ParamsBase::ParamsBaseType t = getModeParamType(mode);
    TranslateStretchManip* manip = manipHolder[mode];
    RenderParams* p = (RenderParams*)getActiveParams(t);
    manip->setParams(p);
    manip->render();
    int manipType = getModeManipType(mode);
    //For probe and 2D manips, render 3D cursor too
    if (manipType > 1) {
      const float *localPoint = p->getSelectedPointLocal();
			
      draw3DCursor(localPoint);
    }
  }
       
  //Now we are ready for all the different renderers to proceed.
  //Sort them;  If they are opaque, they go first.  If not opaque, they
  //are sorted back to front.  This only works if all the geometry of a renderer is ordered by
  //a simple depth test.
	
  printOpenGLError();
  //Now go through all the active renderers
  for (int i = 0; i< getNumRenderers(); i++){
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
}
/* note: special paintEvent for depth peeling, there are a few differences vs the regular one:
 *  no sortRenderers command
 *  aux geometry (domain frame, manipulators, etc) moved to renderScene
 *  addition of depth peeling loop and gl code
 */
void GLWindow::depthPeelPaintEvent(){
	MyBase::SetDiagMsg("GLWindow::paintGL()");
	//Following is needed in case undo/redo leaves a disabled renderer in the renderer list, so it can be deleted.
	removeDisabledRenderers();
	//The renderMutex ensures that only one thread can be in this method at a time.  But the latest version
	//ov vapor performs all the rendering in the main gui thread, so the mutex can probably be deleted.
	if(!renderMutex.tryLock()) return;
	//Following is probably not needed, too, although haven't checked it out.  Qt is supposed to always call paintGL in
	//the GL context, but maybe not paintEvent?
	makeCurrent();
	printOpenGLError();
	
	
	float extents[6] = {0.f,0.f,0.f,1.f,1.f,1.f};
	float minFull[3] = {0.f,0.f,0.f};
	float maxFull[3] = {1.f,1.f,1.f};
	//Again, mutex probably not needed.  At some point the "nowPainting" flag was used to indicate
	//that some thread was in this routine.
	if (nowPainting) {
		renderMutex.unlock();
		return;
	}
	nowPainting = true;
	
	//Paint background
	qglClearColor(getBackgroundColor()); 
	
	//Clear out the depth buffer in preparation for rendering
	glClearDepth(1);
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	
	DataStatus *dataStatus = DataStatus::getInstance();

	//If there is no data, or the session is not yet set up, stop here.
	if (!dataStatus->getDataMgr() ||!(dataStatus->renderReady())) {
		swapBuffers();
		renderMutex.unlock();
		nowPainting = false;
		printOpenGLError();
		return;
	}
	
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
	renderNew = (captureIsNewImage()|| isSpinning);
	if (renderNew) {previousTimeStep = -1; previousFrameNum = -1;} //reset saved time step
	setCaptureNewImage(false); //Next time don't capture if frameNum or timestep are unchanged.

	//Following line has been modified and moved to a later point in the code.  It is necessary to register with the 
	//AnimationController, but not yet, because we don't yet know the frame number.
	//(Tell the animation we are starting.  If it returns false, we are not
	//being monitored by the animation controller
	//bool isControlled = AnimationController::getInstance()->beginRendering(winNum);
	
	//If we are visualizing in latLon space, must update the local coordinates
	//and put them in the trackball, prior to setting up the trackball.
	//The frameNum is used with keyframe animation, it coincides with timeStep when keyframing is disabled.
	int frameNum = getActiveAnimationParams()->getCurrentFrameNumber();
	int timeStep = getActiveAnimationParams()->getCurrentTimestep();

	//When performing keyframe animation, all the viewpoints are in the loadedViewpoints vector.
	const vector<Viewpoint*>& loadedViewpoints = getActiveAnimationParams()->getLoadedViewpoints();
	
	//Prepare for keyframe animation
	if (getActiveAnimationParams()->keyframingEnabled() && loadedViewpoints.size()>0){
		//If we are performing keyframe animation, set the viewpoint (in the viewpoint params) to the appropriate one, based on
		//the frameNum. 
		//The viewerCoordsChanged flag is passed to the prerender callback and used to set the renderNew flag
		setViewerCoordsChanged(true);
		const Viewpoint* vp = loadedViewpoints[frameNum%loadedViewpoints.size()];
		Viewpoint* newViewpoint = new Viewpoint(*vp);
		getActiveViewpointParams()->setCurrentViewpoint(newViewpoint);
		//setValuesFromGui causes the trackball to reflect the current viewpoint in the params
		setValuesFromGui(getActiveViewpointParams());
	} else if (vizIsDirty(NavigatingBit)){  
		//If the viewpoint is not set by animation keyframing, but we are navigating, must still
		//set the trackball
		setValuesFromGui(getActiveViewpointParams());
	}
	//If using latlon coordinates the viewpoint changes every timeStep
	if (getActiveViewpointParams()->isLatLon()&& timeStep != previousTimeStep){
		setViewerCoordsChanged(true);
		getActiveViewpointParams()->convertLocalFromLonLat(timeStep);
		setValuesFromGui(getActiveViewpointParams());
	}
	if (timeStep != previousTimeStep) setTimeAnnotDirty(true);
	//resetView sets the near/far clipping planes based on the viewpoint in the viewpointParams
	resetView(getActiveViewpointParams());
	//Set the projection matrix
	setUpViewport(width(),height());
	// Move to trackball view of scene  
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	//Lights are positioned relative to the view direction, do this before the modelView matrix is changed
	placeLights();

	//Set the GL modelview matrix, based on the Trackball state.
	getTBall()->TrackballSetMatrix();

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
	if (timeStep != previousTimeStep || frameNum != previousFrameNum) {
		setRenderNew();
		previousTimeStep = timeStep;
		previousFrameNum = frameNum;
	}
	
	getActiveRegionParams()->calcStretchedBoxExtentsInCube(extents,timeStep);
	DataStatus::getInstance()->getMaxStretchedExtentsInCube(maxFull);
	
	//INITIAL RENDER, NO DEPTH PEELING YET
	if(!peelInitialized){
	  SetErrMsg("Advanced Transparency has not been properly initialized! Restart VAPOR to enable");
	  return;
	}
	printOpenGLError();
	//bind first framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fboA);
	printOpenGLError();
	currentBuffer = fboA;
	// clear buffers
	glActiveTexture(GL_TEXTURE0 + depthTexUnit);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glActiveTexture(GL_TEXTURE0);
	printOpenGLError();
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	printOpenGLError();
	glEnable(GL_DEPTH_TEST); //regular depth test for initial render pass
	glPushAttrib(GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT);
	printOpenGLError();
	//prepare framebuffer colorbuffers to draw/read from first attach_point
	glDrawBuffer(attach_points[0]); 
	printOpenGLError();
	glReadBuffer(attach_points[0]);		
	currentLayer = 0;
	printOpenGLError();
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	printOpenGLError();
	renderScene(extents, minFull, maxFull, timeStep);
	//scene rendered, enable depth peeling for transparency blending
	currentLayer = 1;
	for (int j=1; j< maxbuffers ; j++) {
	  if (j%2 != 0) {			
	    //Bind next framebuffer
	    glBindFramebuffer(GL_FRAMEBUFFER, fboB);
	    //Bind depth buffer A to last texture unit
	    currentBuffer = fboB;
	    currentDepth = depthA;
	    glDepthMask(GL_TRUE);
	    glDepthFunc(GL_LESS);
	    printOpenGLError();
	    glEnable(GL_DEPTH_TEST);
	    printOpenGLError();
	    glDrawBuffer(attach_points[j]);
	    printOpenGLError();
	    glReadBuffer(attach_points[j]);		
	    printOpenGLError();
	    glClearColor(0.0, 0.0, 0.0, 1.0);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    printOpenGLError();
	    glActiveTexture(GL_TEXTURE0 + depthTexUnit);
	    glDisable(GL_TEXTURE_1D);
	    glDisable(GL_TEXTURE_2D);
	    glDisable(GL_BLEND);
	    glBindTexture(GL_TEXTURE_2D, depthA);
	    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE); //ensure texture comparison was not turned on elsewhere
	    glActiveTexture(GL_TEXTURE0);		       
	    printOpenGLError();
	    if(!manager->enableEffect(std::string("depthpeeling")))
	      SetErrMsg(VAPOR_ERROR_GL_VENDOR,"depth peeling not enabled");	
	    //reset GL matrix stack
	    //resetView sets the near/far clipping planes based on the viewpoint in the viewpointParams
	    resetView(getActiveViewpointParams());
	    //Set the projection matrix
	    setUpViewport(width(),height());
	    // Move to trackball view of scene  
	    glMatrixMode(GL_MODELVIEW);

	    glLoadIdentity();

	    //Lights are positioned relative to the view direction, do this before the modelView matrix is changed
	    placeLights();

	    //Set the GL modelview matrix, based on the Trackball state.
	    getTBall()->TrackballSetMatrix();
	    renderScene(extents, minFull, maxFull, timeStep);

	    printOpenGLError();
	    manager->disableEffect();
	    glActiveTexture(GL_TEXTURE0 + depthTexUnit);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    glActiveTexture(GL_TEXTURE0);
	    glFlush();
	  }
	  else{
	    //Bind next framebuffer
	    glBindFramebuffer(GL_FRAMEBUFFER, fboA);
	    glDepthFunc(GL_LESS);
	    glDepthMask(GL_TRUE);
	    glEnable(GL_DEPTH_TEST);
	    glDisable(GL_BLEND);
	    currentBuffer = fboA;
	    currentDepth = depthB;
	    //Bind depth buffer B to last texture unit
	    glDrawBuffer(attach_points[j]);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    printOpenGLError();
	    glActiveTexture(GL_TEXTURE0 + depthTexUnit);
	    glDisable(GL_TEXTURE_2D);
	    glDisable(GL_TEXTURE_3D);
	    glBindTexture(GL_TEXTURE_2D, depthB);
	    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE); //ensure texture comparison was not turned on elsewhere
	    glActiveTexture(GL_TEXTURE0);			
	    printOpenGLError();
	    if(!manager->enableEffect(std::string("depthpeeling")))
	      SetErrMsg(VAPOR_ERROR_GL_VENDOR,"depth peeling not enabled");	
	    //reset GL matrix stack
	    //resetView sets the near/far clipping planes based on the viewpoint in the viewpointParams
	    resetView(getActiveViewpointParams());
	    //Set the projection matrix
	    setUpViewport(width(),height());
	    // Move to trackball view of scene  
	    glMatrixMode(GL_MODELVIEW);

	    glLoadIdentity();

	    //Lights are positioned relative to the view direction, do this before the modelView matrix is changed
	    placeLights();

	    //Set the GL modelview matrix, based on the Trackball state.
	    getTBall()->TrackballSetMatrix();
	    renderScene(extents, minFull, maxFull, timeStep);

	    printOpenGLError();
	    manager->disableEffect();
	    glActiveTexture(GL_TEXTURE0 + depthTexUnit);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    glActiveTexture(GL_TEXTURE0);
	    glFlush();
	  }
	  currentLayer++;
	}
	//SHOW PEELED COLORS
	printOpenGLError();
	glPopAttrib();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	printOpenGLError();
	glActiveTexture(GL_TEXTURE0 + depthTexUnit);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//makes use of programmable pipeline texture mapping to avoid state problems
	manager->enableEffect("texSampler");
	printOpenGLError();
	for (int i = maxbuffers - 1; i > -1 ; i--){
	  glBindTexture(GL_TEXTURE_2D, layers[i]);
	  printOpenGLError();
	  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	  glMatrixMode(GL_MODELVIEW); 
	  glPushMatrix();
	  glLoadIdentity(); 
	  glMatrixMode(GL_PROJECTION);
	  glPushMatrix();
	  glLoadIdentity();
	  glBegin(GL_QUADS);
	  //glColor3f(0.0, 0.0, 0.0); debug purposes
	  glVertex3i (-1, -1, -1);
	  //glColor3f(1.0, 0.0, 1.0);
	  glVertex3i (-1, 1, -1);
	  //glColor3f(1.0, 1.0, 1.0);
	  glVertex3i (1, 1, -1); 
	  //glColor3f(0.0, 1.0, 0.0);
	  glVertex3i (1, -1, -1); 
	  glEnd ();
	  glPopMatrix (); 
	  glMatrixMode (GL_MODELVIEW); 
	  glPopMatrix ();
	}
	manager->disableEffect();
	//Go back to MODELVIEW for other matrix stuff
	//By default the matrix is expected to be MODELVIEW
	glMatrixMode(GL_MODELVIEW);

	//Perform final touch-up on the final images, before capturing or displaying them.

	//Axis annotation must be drawn after all the renderers are complete so that the axes are not hidden.
	if (axisAnnotationIsEnabled()) {
		drawAxisTics(timeStep);
		//Now go to default 2D window
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		drawAxisLabels(timeStep);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	//Now go through all the active renderers, and draw colorbars as appropriate
	for (int i = 0; i< getNumRenderers(); i++){
		if(renderer[i]->isInitialized() && !(renderer[i]->doAlwaysBypass(timeStep))) {
			RenderParams* p = renderer[i]->getRenderParams();
			int indx = hasColorbarIndex(p);
			if (indx<0) continue;
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
		
			getRenderer(p)->renderColorscale(colorbarIsDirty(),indx);
		
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}
	setColorbarDirty(false);
	
	//Create time annotation from current time step.  Draw it over the image.
	if(getTimeAnnotType()){
		//Now go to default 2D window
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		
		renderTimeStamp(timeAnnotIsDirty());
		
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	
	//Capture the back-buffer image, if not navigating.  
	//This must be performed before swapBuffers after everything has been drawn.
	if (renderNew && !mouseDownHere) 
		doFrameCapture();

	glPopMatrix();
	swapBuffers();
	glFlush();

	//clear dirty bits for the next rendering.
	
	setDirtyBit(RegionBit,false);
	setDirtyBit(AnimationBit,false);
	setDirtyBit(NavigatingBit,false);
	setDirtyBit(ProjMatrixBit, false);
	setDirtyBit(ViewportBit, false);
	setDirtyBit(LightingBit, false);
	//Turn off the nowPainting flag (probably could be deleted)
	nowPainting = false;
	
	//When spinning update the viewpoint appropriately.
	if (isSpinning){
		getTBall()->TrackballSpin();
		setViewerCoordsChanged(true);
		setDirtyBit(ProjMatrixBit, true);
	}
	//Now release the rendering mutex.
	//Must be performed here or deadlock occurs!
	renderMutex.unlock();
	
	//Tell the animation controller that this frame is complete.  
	//This must be called exactly once after rendering is done.l
	postRenderCB(winNum, isControlled);
	
	glDisable(GL_NORMALIZE);
	printOpenGLError();
}
void GLWindow::paintEvent(QPaintEvent*){
	if (! _readyToDraw) return;

  if(depthPeeling)
    depthPeelPaintEvent();
  else
    regPaintEvent();
}
void GLWindow::regPaintEvent()
{
	MyBase::SetDiagMsg("GLWindow::paintGL()");
	//Following is needed in case undo/redo leaves a disabled renderer in the renderer list, so it can be deleted.
	removeDisabledRenderers();
	//The renderMutex ensures that only one thread can be in this method at a time.  But the latest version
	//ov vapor performs all the rendering in the main gui thread, so the mutex can probably be deleted.
	if(!renderMutex.tryLock()) return;
	//Following is probably not needed, too, although haven't checked it out.  Qt is supposed to always call paintGL in
	//the GL context, but maybe not paintEvent?
	makeCurrent();

#ifdef	DEAD
	//
	// The following is a workaround for bug #947: OpenGL errors on Mac 
	// Under Mac OS X 10.8, with Qt 4.8 the frame buffer becomes invalid
	// if data are re-loaded into vaporgui.
	//
	// Updated Tue Dec 31 13:32:37 MST 2013: The glCheckFramebufferStatus
	// results in a core dump on Linux systems when rendering is occuring 
	// remotely via X11.
	//
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE){
		renderMutex.unlock();
		return;
	}
#endif

	printOpenGLError();
	
	
	float extents[6] = {0.f,0.f,0.f,1.f,1.f,1.f};
	float minFull[3] = {0.f,0.f,0.f};
	float maxFull[3] = {1.f,1.f,1.f};
	//Again, mutex probably not needed.  At some point the "nowPainting" flag was used to indicate
	//that some thread was in this routine.
    if (nowPainting) {
		renderMutex.unlock();
		return;
	}
	nowPainting = true;
	
	//Paint background
	qglClearColor(getBackgroundColor()); 
	
	//Clear out the depth buffer in preparation for rendering
	glClearDepth(1);
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	
	DataStatus *dataStatus = DataStatus::getInstance();

	//If there is no data, or the session is not yet set up, stop here.
	if (!dataStatus->getDataMgr() ||!(dataStatus->renderReady())) {
		swapBuffers();
		renderMutex.unlock();
		nowPainting = false;
		printOpenGLError();
		return;
	}
	
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
	renderNew = (captureIsNewImage()|| isSpinning);
	if (renderNew) {previousTimeStep = -1; previousFrameNum = -1;} //reset saved time step
	setCaptureNewImage(false); //Next time don't capture if frameNum or timestep are unchanged.

	//Following line has been modified and moved to a later point in the code.  It is necessary to register with the 
	//AnimationController, but not yet, because we don't yet know the frame number.
	//(Tell the animation we are starting.  If it returns false, we are not
	//being monitored by the animation controller
	//bool isControlled = AnimationController::getInstance()->beginRendering(winNum);
	
	//If we are visualizing in latLon space, must update the local coordinates
	//and put them in the trackball, prior to setting up the trackball.
	//The frameNum is used with keyframe animation, it coincides with timeStep when keyframing is disabled.
	int frameNum = getActiveAnimationParams()->getCurrentFrameNumber();
	int timeStep = getActiveAnimationParams()->getCurrentTimestep();

	//When performing keyframe animation, all the viewpoints are in the loadedViewpoints vector.
	const vector<Viewpoint*>& loadedViewpoints = getActiveAnimationParams()->getLoadedViewpoints();
	
	//Prepare for keyframe animation
	if (getActiveAnimationParams()->keyframingEnabled() && loadedViewpoints.size()>0){
		//If we are performing keyframe animation, set the viewpoint (in the viewpoint params) to the appropriate one, based on
		//the frameNum. 
		//The viewerCoordsChanged flag is passed to the prerender callback and used to set the renderNew flag
		setViewerCoordsChanged(true);
		const Viewpoint* vp = loadedViewpoints[frameNum%loadedViewpoints.size()];
		Viewpoint* newViewpoint = new Viewpoint(*vp);
		getActiveViewpointParams()->setCurrentViewpoint(newViewpoint);
		//setValuesFromGui causes the trackball to reflect the current viewpoint in the params
		setValuesFromGui(getActiveViewpointParams());
	} else if (vizIsDirty(NavigatingBit)){  
		//If the viewpoint is not set by animation keyframing, but we are navigating, must still
		//set the trackball
		setValuesFromGui(getActiveViewpointParams());
	}
	//If using latlon coordinates the viewpoint changes every timeStep
	if (getActiveViewpointParams()->isLatLon()&& timeStep != previousTimeStep){
		setViewerCoordsChanged(true);
		getActiveViewpointParams()->convertLocalFromLonLat(timeStep);
		setValuesFromGui(getActiveViewpointParams());
	}
	if (timeStep != previousTimeStep) setTimeAnnotDirty(true);
	//resetView sets the near/far clipping planes based on the viewpoint in the viewpointParams
	resetView(getActiveViewpointParams());
	//Set the projection matrix
	setUpViewport(width(),height());
	// Move to trackball view of scene  
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	//Lights are positioned relative to the view direction, do this before the modelView matrix is changed
	placeLights();

	//Set the GL modelview matrix, based on the Trackball state.
	getTBall()->TrackballSetMatrix();

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
	if (timeStep != previousTimeStep || frameNum != previousFrameNum) {
		setRenderNew();
		previousTimeStep = timeStep;
		previousFrameNum = frameNum;
	}
	
    getActiveRegionParams()->calcStretchedBoxExtentsInCube(extents,timeStep);
    DataStatus::getInstance()->getMaxStretchedExtentsInCube(maxFull);
	
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	//and readable
	glEnable(GL_DEPTH_TEST);
	//Prepare for alpha values:
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//If we are in regionMode, or if the preferences specify the full box will be rendered,
	//draw it now, before the renderers do their thing.
    if(regionFrameIsEnabled()|| GLWindow::getCurrentMouseMode() == GLWindow::regionMode){
      renderDomainFrame(extents, minFull, maxFull);
    } 
	//In regionMode or if preferences specify it, draw the subregion frame.
    if(subregionFrameIsEnabled() && 
       !(GLWindow::getCurrentMouseMode() == GLWindow::regionMode) )
    {
      drawSubregionBounds(extents);
    } 

	//Draw axis arrows if enabled.
    if (axisArrowsAreEnabled()) drawAxisArrows(extents);
	


	//render the region manipulator, if in region mode, and active visualizer, or region shared
	//with active visualizer.  Possibly redundant???
	if(getCurrentMouseMode() == GLWindow::regionMode) { 
		if( (windowIsActive() || 
			(!getActiveRegionParams()->isLocal() && activeWinSharesRegion()))){
				TranslateStretchManip* regionManip = getManip(Params::_regionParamsTag);
				regionManip->setParams(getActiveRegionParams());
				regionManip->render();
		} 
	}
	//Render other manips, if we are in appropriate mode:
	//Note: Other manips don't have shared and local to deal with:
	else if ((getCurrentMouseMode() != navigateMode) && windowIsActive()){
		int mode = getCurrentMouseMode();
		ParamsBase::ParamsBaseType t = getModeParamType(mode);
		TranslateStretchManip* manip = manipHolder[mode];
		RenderParams* p = (RenderParams*)getActiveParams(t);
		manip->setParams(p);
		manip->render();
		int manipType = getModeManipType(mode);
		//For probe and 2D manips, render 3D cursor too
		if (manipType > 1) {
			const float *localPoint = p->getSelectedPointLocal();
			
			draw3DCursor(localPoint);
		}
	}
	
	renderText();
	
	//Now we are ready for all the different renderers to proceed.
	//Sort them;  If they are opaque, they go first.  If not opaque, they
	//are sorted back to front.  This only works if all the geometry of a renderer is ordered by
	//a simple depth test.

	sortRenderers(timeStep);
	
	printOpenGLError();
	//Now go through all the active renderers
	for (int i = 0; i< getNumRenderers(); i++){
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

	//Axis annotation must be drawn after all the renderers are complete so that the axes are not hidden.
	if (axisAnnotationIsEnabled()) {
		drawAxisTics(timeStep);
		//Now go to default 2D window
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		drawAxisLabels(timeStep);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	//renderText();
	textRenderersDirty = false;

	
	//Now go through all the active renderers, and draw colorbars as appropriate
	for (int i = 0; i< getNumRenderers(); i++){
		if(renderer[i]->isInitialized() && !(renderer[i]->doAlwaysBypass(timeStep))) {
			RenderParams* p = renderer[i]->getRenderParams();
			int indx = hasColorbarIndex(p);
			if (indx<0) continue;
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
		
			getRenderer(p)->renderColorscale(colorbarIsDirty(),indx);
		
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}
	setColorbarDirty(false);
	
	//Create time annotation from current time step.  Draw it over the image.
	
	if(getTimeAnnotType()){
		//Now go to default 2D window
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		
		renderTimeStamp(timeAnnotIsDirty());
		
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	
	//Capture the back-buffer image, if not navigating.  
	//This must be performed before swapBuffers after everything has been drawn.
	if (renderNew && !mouseDownHere) 
		doFrameCapture();

	glPopMatrix();
	swapBuffers();
	glFlush();

	//clear dirty bits for the next rendering.
	
	setDirtyBit(RegionBit,false);
	setDirtyBit(AnimationBit,false);
	setDirtyBit(NavigatingBit,false);
	setDirtyBit(ProjMatrixBit, false);
	setDirtyBit(ViewportBit, false);
	setDirtyBit(LightingBit, false);
	//Turn off the nowPainting flag (probably could be deleted)
	nowPainting = false;
	
	//When spinning update the viewpoint appropriately.
	if (isSpinning){
		getTBall()->TrackballSpin();
		setViewerCoordsChanged(true);
		setDirtyBit(ProjMatrixBit, true);
	}
	//Now release the rendering mutex.
	//Must be performed here or deadlock occurs!
	renderMutex.unlock();
	
	//Tell the animation controller that this frame is complete.  
	//This must be called exactly once after rendering is done.l
	postRenderCB(winNum, isControlled);
	
	glDisable(GL_NORMALIZE);
	printOpenGLError();
}

//Draw a 3D cursor at specified world coords
void GLWindow::draw3DCursor(const float position[3]){
	float cubePosition[3];
	ViewpointParams::localToStretchedCube(position, cubePosition);
	glLineWidth(3.f);
	glColor3f(1.f,1.f,1.f);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINES);
	glVertex3f(cubePosition[0]-0.05f,cubePosition[1],cubePosition[2]);
	glVertex3f(cubePosition[0]+0.05f,cubePosition[1],cubePosition[2]);
	glVertex3f(cubePosition[0],cubePosition[1]-0.05f,cubePosition[2]);
	glVertex3f(cubePosition[0],cubePosition[1]+0.05f,cubePosition[2]);
	glVertex3f(cubePosition[0],cubePosition[1],cubePosition[2]-0.05f);
	glVertex3f(cubePosition[0],cubePosition[1],cubePosition[2]+0.05f);
	glEnd();
	glDisable(GL_LINE_SMOOTH);
}

//
//  Set up the OpenGL rendering state, and define display list
//

void GLWindow::initializeGL()
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

#ifdef	DEAD
	if (GetVendor() == MESA){
		SetErrMsg(VAPOR_ERROR_GL_VENDOR,"GL Vendor String is MESA.\nGraphics drivers may need to be reinstalled");
		
	}
#endif


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

	//VizWinMgr::getInstance()->getDvrRouter()->initTypes();
    qglClearColor(getBackgroundColor()); 		// Let OpenGL clear to black
	//Initialize existing renderers:
	//
	if (printOpenGLError()) return;
	for (int i = 0; i< getNumRenderers(); i++){
		renderer[i]->initializeGL();
		printOpenGLErrorMsg(renderer[i]->getMyName().c_str());
	}
    glGenTextures(1,&_timeStampTexid);
	nowPainting = false;
	printOpenGLError();
	
	_readyToDraw = true;
#ifdef	Darwin
	_readyToDraw = false;
#endif

	//makeWriter();    
}

//projectPoint returns true if point is in front of camera
//resulting screen coords returned in 2nd argument.  Note that
//OpenGL coords are 0 at bottom of window!
//
bool GLWindow::projectPointToWin(double cubeCoords[3], double winCoords[2]){
	double depth;
	GLdouble wCoords[2];
	GLdouble cbCoords[3];
	for (int i = 0; i< 3; i++)
		cbCoords[i] = (double) cubeCoords[i];
	
	bool success = gluProject(cbCoords[0],cbCoords[1],cbCoords[2], getModelMatrix(),
		getProjectionMatrix(), getViewport(), wCoords, (wCoords+1),(GLdouble*)(&depth));
	if (!success) return false;
	winCoords[0] = wCoords[0];
	winCoords[1] = wCoords[1];
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
		ViewpointParams::localFromStretchedCube(v,dirVec);
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

//Produce an array based on current contents of the (back) buffer
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
	glPushAttrib(GL_CURRENT_BIT);

	glColor3fv(regionFrameColorFlt);	   
    glLineWidth( 2.0 );
	//Now draw the lines.  Divide each dimension into numLines[dim] sections.
	
	int x,y,z;
	//Do the lines in each z-plane
	//Turn on writing to the z-buffer
	glDepthMask(GL_TRUE);
	glEnable(GL_LINE_SMOOTH);		
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
	glDisable(GL_LINE_SMOOTH);
	glPopAttrib();
	

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
	glPushAttrib(GL_CURRENT_BIT);
	glLineWidth( 2.0 );
	glColor3fv(subregionFrameColorFlt);
	glEnable(GL_LINE_SMOOTH);
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
	glDisable(GL_LINE_SMOOTH);
	glPopAttrib();
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
void GLWindow::
buildTimeStampImage(){
	
	//Construct a string representing the text to be displayed:
	size_t timeStep = (size_t)(getActiveAnimationParams()->getCurrentTimestep()); 
	QString labelContents;
	if (getTimeAnnotType() == 1){
		labelContents = QString("TimeStep: ")+QString::number((int)timeStep) + " ";
	}
	else {//get string from metadata
		const DataMgr *dataMgr = DataStatus::getInstance()->getDataMgr();
		if (!dataMgr) labelContents = QString("");
		else {
			string timeStamp;
			dataMgr->GetTSUserTimeStamp(timeStep, timeStamp);
			labelContents = QString("Date/Time: ")+QString(timeStamp.c_str());
		}
	}

	//Determine the size of the rectangle holding the text:
	QFont f;
	f.setPointSize(getTimeAnnotTextSize());
	QFontMetrics metrics = QFontMetrics(f);
	QRect rect = metrics.boundingRect(0,1000, width(), int(height()*0.25),Qt::AlignCenter, labelContents);

	//create a QPixmap (specified background color) and draw the text on it.
	QPixmap timeStampPixmap(rect.width(), rect.height());

	QColor bgColor = getBackgroundColor();

	//Draw the text into the pixmap
	QPainter painter(&timeStampPixmap);
	painter.setFont(f);
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.fillRect(QRect(0, 0, rect.width(), rect.height()), bgColor);
	timeStampPixmap.fill(bgColor);
	QColor fgColor = getTimeAnnotColor();
	QPen myPen(fgColor, 6);
	painter.setPen(myPen);
	
	//Setup font:
	int textHeight = rect.height();
	
	textHeight = (int) (textHeight*getTimeAnnotTextSize()*0.1);
	f.setPixelSize(textHeight);
	
	painter.drawText(QRect(0, 0, rect.width(), rect.height()),Qt::AlignCenter, labelContents);
	
	//Then, convert the pxmap to a QImage
	QImage timeStampImage = timeStampPixmap.toImage();

	//Finally create the gl-formatted texture 
	//assert(colorbarImage.depth()==32);
	glTimeStampImage = QGLWidget::convertToGLFormat(timeStampImage);
	painter.end();
	
}
void GLWindow::
renderTimeStamp(bool dorebuild){
	float whitecolor[4] = {1.,1.,1.,1.f};
	if (dorebuild) buildTimeStampImage();

	setTimeAnnotDirty(false);
	glColor4fv(whitecolor);
	glBindTexture(GL_TEXTURE_2D,_timeStampTexid);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	//Disable z-buffer compare, always overwrite:
	glDepthFunc(GL_ALWAYS);
    glEnable( GL_TEXTURE_2D );
	int txtWidth = glTimeStampImage.width();
	int txtHeight = glTimeStampImage.height();
	float fltTxtWidth = (float)txtWidth/(float)width();
	float fltTxtHeight = (float)txtHeight/(float)height();
	//create a polygon appropriately positioned in the scene.  It's inside the unit cube--
	glTexImage2D(GL_TEXTURE_2D, 0, 3, txtWidth, txtHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		glTimeStampImage.bits());
	
	float llx = 2.f*getTimeAnnotCoord(0) - 1.f; 
	float lly = 2.f*getTimeAnnotCoord(1)-2.*fltTxtHeight - 1.f; //Move down so it won't overlap default color scale location.
	float urx = llx+2.*fltTxtWidth;
	float ury = lly+2.*fltTxtHeight;
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(llx, lly, 0.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(llx, ury, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(urx, ury, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(urx, lly, 0.0f);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	//Reset to default:
	glDepthFunc(GL_LESS);
}
void GLWindow::buildAxisLabels(int timestep){
	//Set up the painter and font metrics
	
	double lorigin[3], lticMin[3], lticMax[3];
	double ticMin[3], ticMax[3];
	double origin[3];
	//Convert user to local by subtracting extent min.
	//Local must be user local coords (not lat lon)
	const vector<double>& exts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	double axOrigin[3],mnTic[3],mxTic[3],tcLen[3];
	if (useLatLonAnnotation()){
		ConvertAxes(false,ticDir,minTic,maxTic,axisOriginCoord,ticLength,mnTic,mxTic,axOrigin,tcLen);
	} else {
		for (int i = 0; i<3; i++){ //copy values to temporary _A variables
			mnTic[i] = minTic[i];
			mxTic[i] = maxTic[i];
			tcLen[i] = ticLength[i];
			axOrigin[i] = axisOriginCoord[i];
		}
	}
	for (int i = 0; i<3; i++){
		lorigin[i] = axOrigin[i] - exts[i];
		lticMin[i] = mnTic[i] - exts[i];
		lticMax[i] = mxTic[i] - exts[i];
	}
	ViewpointParams::localToStretchedCube(lorigin, origin);
	//minTic and maxTic can be regarded as points in world space, defining
	//corners of a box that's projected to axes.
	ViewpointParams::localToStretchedCube(lticMin, ticMin);
	ViewpointParams::localToStretchedCube(lticMax, ticMax);
	double pointOnAxis[3];
	double winCoords[2];
	
	for (int axis = 0; axis < 3; axis++){
		
		if (numTics[axis] > 1){
			vcopy(origin, pointOnAxis);
			for (int i = 0; i< numTics[axis]; i++){
				if (axisLabels[axis].size() <= i) axisLabels[axis].push_back(*(new QImage()));
				pointOnAxis[axis] = ticMin[axis] + (float)i* (ticMax[axis] - ticMin[axis])/(float)(numTics[axis]-1);
				float labelValue = minTic[axis] + (float)i* (maxTic[axis] - minTic[axis])/(float)(numTics[axis]-1);
				projectPointToWin(pointOnAxis, winCoords);
				int x = (int)(winCoords[0]+2*ticWidth);
				int y = (int)(height()-winCoords[1]+2*ticWidth);
				if (x < 0 || x > width()) continue;
				if (y < 0 || y > height()) continue;
				//Note: Qt won't work right if we take the following out of the loop:  
				QFont f;
				f.setPointSize(labelHeight);
				QString textLabel = QString::number(labelValue,'g',labelDigits);
				QFontMetrics metrics = QFontMetrics(f);
				QRect rect = metrics.boundingRect(0,1000, width(), height(), Qt::AlignCenter, textLabel);

				//create a QPixmap (specified background color) and draw the text on it.
				QPixmap labelPixmap(rect.width(), rect.height());
				QColor bgColor = getBackgroundColor();
				//Draw the text into the pixmap
				QPainter painter(&labelPixmap);
				painter.setFont(f);
				painter.setRenderHint(QPainter::TextAntialiasing);
				painter.fillRect(QRect(0, 0, rect.width(), rect.height()), bgColor);
				labelPixmap.fill(bgColor);
				QPen myPen(axisColor,6);
				painter.setPen(myPen);
	
				//Setup font:
				int textHeight = rect.height();
	
				textHeight = (int) (textHeight*labelHeight*0.1);
				f.setPixelSize(textHeight);
	
				painter.drawText(QRect(0, 0, rect.width(), rect.height()),Qt::AlignCenter, textLabel);
	
				//Then, convert the pxmap to a QImage
				QImage labelImage = labelPixmap.toImage();

				//Finally create the gl-formatted texture
				//
				axisLabels[axis][i] = QGLWidget::convertToGLFormat(labelImage);
				painter.end();
		
			}
		}
	}
	axisLabelsDirty = false;
}
void GLWindow::drawAxisLabels(int timestep) {
	//Modify minTic, maxTic, ticLength, axisOriginCoord to user coords
	//if using latLon
	double minTicA[3],maxTicA[3],ticLengthA[3], axisOriginCoordA[3];
	
	//set axis annotation in user coordinates, if it's in lat/lon
	for (int i = 0; i<3; i++){ //copy values to temporary _A variables
		minTicA[i] = minTic[i];
		maxTicA[i] = maxTic[i];
		ticLengthA[i] = ticLength[i];
		axisOriginCoordA[i] = axisOriginCoord[i];
	}
	if (useLatLonAnnotation()){
		ConvertAxes(false,ticDir,minTic,maxTic,axisOriginCoord,ticLength,minTicA,maxTicA,axisOriginCoordA,ticLengthA);
	} else {
		for (int i = 0; i<3; i++){ //copy values to temporary _A variables
			minTicA[i] = minTic[i];
			maxTicA[i] = maxTic[i];
			ticLengthA[i] = ticLength[i];
			axisOriginCoordA[i] = axisOriginCoord[i];
		}
	}
	double origin[3], ticMin[3], ticMax[3];
	double lorigin[3], lticMin[3], lticMax[3];
	if (labelHeight <= 0) return;
	if (axisLabelsDirty) buildAxisLabels(timestep);
	
	//Convert user to local by subtracting extent min.
	const vector<double>& exts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	for (int i = 0; i<3; i++){
		lorigin[i] = axisOriginCoordA[i] - exts[i];
		lticMin[i] = minTicA[i] - exts[i];
		lticMax[i] = maxTicA[i] - exts[i];
	}
	ViewpointParams::localToStretchedCube(lorigin, origin);
	//minTic and maxTic can be regarded as points in world space, defining
	//corners of a box that's projected to axes.
	ViewpointParams::localToStretchedCube(lticMin, ticMin);
	ViewpointParams::localToStretchedCube(lticMax, ticMax);
	
	double pointOnAxis[3];
	double winCoords[2] = {0.,0.};
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDepthFunc(GL_ALWAYS);
    glEnable( GL_TEXTURE_2D );
	for (int axis = 0; axis < 3; axis++){
		if (numTics[axis] > 1){
			vcopy(origin, pointOnAxis);
			for (int i = 0; i< numTics[axis]; i++){
				pointOnAxis[axis] = ticMin[axis] + (float)i* (ticMax[axis] - ticMin[axis])/(float)(numTics[axis]-1);
				projectPointToWin(pointOnAxis, winCoords);
				int x = (int)(winCoords[0]+2*ticWidth);
				int y = (int)(height()-winCoords[1]+2*ticWidth);
				if (x < 0 || x > width()) continue;
				if (y < 0 || y > height()) continue;
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				//Disable z-buffer compare, always overwrite:
	
				int txtWidth = axisLabels[axis][i].width();
				int txtHeight = axisLabels[axis][i].height();
				double dblTxtWidth = (double)txtWidth/(double)width();
				double dblTxtHeight = (double)txtHeight/(double)height();
				//create a textured polygon appropriately positioned in the scene. 
				glTexImage2D(GL_TEXTURE_2D, 0, 3, txtWidth, txtHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
					axisLabels[axis][i].bits());
	
				double llx = 2.*(double)x/(double)width() -1.;
				double lly = 1. - 2.*(double)y/(double)height();
				double urx = llx+2.*dblTxtWidth;
				double ury = lly+2.*dblTxtHeight;
				glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
				glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f); glVertex3d(llx, lly, 0.0f);
				glTexCoord2f(0.0f, 1.0f); glVertex3d(llx, ury, 0.0f);
				glTexCoord2f(1.0f, 1.0f); glVertex3d(urx, ury, 0.0f);
				glTexCoord2f(1.0f, 0.0f); glVertex3d(urx, lly, 0.0f);
				glEnd();
	
			}
		}
	}
	glDisable(GL_TEXTURE_2D);
	//Reset to default:
	glDepthFunc(GL_LESS);
	glPopAttrib();
}


void GLWindow::drawAxisTics(int timestep){
	// Preserve the current GL color state
	glPushAttrib(GL_CURRENT_BIT);	
	
	//Modify minTic, maxTic, ticLength, axisOriginCoord to user coords
	//if using latLon, convert annotation axes to user coords
	double minTicA[3],maxTicA[3],ticLengthA[3], axisOriginCoordA[3];
	if (useLatLonAnnotation()){
		ConvertAxes(false,ticDir,minTic,maxTic,axisOriginCoord,ticLength,minTicA,maxTicA,axisOriginCoordA,ticLengthA);
	} else {
		for (int i = 0; i<3; i++){ //copy values to temporary _A variables
			minTicA[i] = minTic[i];
			maxTicA[i] = maxTic[i];
			ticLengthA[i] = ticLength[i];
			axisOriginCoordA[i] = axisOriginCoord[i];
		}
	}
	
	double origin[3], ticMin[3], ticMax[3], ticLen[3];
	double lorigin[3], lticMin[3], lticMax[3];
	//Convert user to local by subtracting extent min.
	
	const vector<double>& exts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	for (int i = 0; i<3; i++){
		lorigin[i] = axisOriginCoordA[i] - exts[i];
		lticMin[i] = minTicA[i] - exts[i];
		lticMax[i] = maxTicA[i] - exts[i];
	}
	ViewpointParams::localToStretchedCube(lorigin, origin);
	//minTic and maxTic can be regarded as points in world space, defining
	//corners of a box that's projected to axes.
	ViewpointParams::localToStretchedCube(lticMin, ticMin);
	ViewpointParams::localToStretchedCube(lticMax, ticMax);
	//TicLength needs to be stretched based on which axes are used for tic direction
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	float maxStretchedCubeSide = ViewpointParams::getMaxStretchedCubeSide();
	for (int i = 0; i<3; i++){
		int j = ticDir[i];
		ticLen[i] = ticLengthA[i]*stretch[j]/maxStretchedCubeSide;
	}
	
	glDisable(GL_LIGHTING);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f((float)axisColor.red()/255.f,(float)axisColor.green()/255.f, (float)axisColor.blue()/255.f);
	glLineWidth(ticWidth);
	//Draw lines on x-axis:
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINES);
	glVertex3d(ticMin[0],origin[1],origin[2]);
	glVertex3d(ticMax[0],origin[1],origin[2]);
	glVertex3d(origin[0],ticMin[1],origin[2]);
	glVertex3d(origin[0],ticMax[1],origin[2]);
	glVertex3d(origin[0],origin[1],ticMin[2]);
	glVertex3d(origin[0],origin[1],ticMax[2]);
	double pointOnAxis[3];
	double ticVec[3], drawPosn[3];
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
			glVertex3dv(drawPosn);
			vadd(pointOnAxis, ticVec, drawPosn);
			glVertex3dv(drawPosn);
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
			glVertex3dv(drawPosn);
			vadd(pointOnAxis, ticVec, drawPosn);
			glVertex3dv(drawPosn);
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
			glVertex3dv(drawPosn);
			vadd(pointOnAxis, ticVec, drawPosn);
			glVertex3dv(drawPosn);
		}
	}
	glEnd();
	glDisable(GL_LINE_SMOOTH);
	glPopAttrib();
}

void GLWindow::drawAxisArrows(float* extents){
	// Preserve the current GL color state
	glPushAttrib(GL_CURRENT_BIT);

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
	glEnable(GL_LINE_SMOOTH);
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
	
	glDisable(GL_LINE_SMOOTH);
	// Revert to previous GL color state
	glPopAttrib();
}

/*
 * Insert a renderer to this visualization window
 * Add it after all renderers of lower render order
 */
void GLWindow::
insertRenderer(RenderParams* rp, Renderer* ren, int newOrder)
{
	Params::ParamsBaseType rendType = rp->GetParamsBaseTypeId();
	
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
// Remove all renderers.  This is needed when we load new data into
// an existing session
void GLWindow::removeAllRenderers(){
	int saveNumRenderers = getNumRenderers();
	//Prevent new rendering while we do this:
	setNumRenderers(0);
	for (int i = 0; i< saveNumRenderers; i++){
		delete renderer[i];
	}
	//setNumRenderers(0);
	
	nowPainting = false;
	numRenderers = 0;
	for (int i = 0; i< MAXNUMRENDERERS; i++){
		renderType[i] = 0;
		renderOrder[i] = 0;
		renderer[i] = 0;
	}
	rendererMapping.clear();
	
}
/* 
 * Remove renderer of specified renderParams
 */
bool GLWindow::removeRenderer(RenderParams* rp){
	int i;
	assert(!nowPainting);
	renderMutex.lock();
	map<RenderParams*,Renderer*>::iterator find_iter = rendererMapping.find(rp);
	if (find_iter == rendererMapping.end()) {
		renderMutex.unlock();
		return false;
	}
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
	renderMutex.unlock();
	update();
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
RenderParams* GLWindow::findARenderer(Params::ParamsBaseType renType){
	
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
			clearRendererBypass((Params::GetTypeFromTag(Params::_dvrParamsTag)));
			clearRendererBypass((Params::GetTypeFromTag(Params::_isoParamsTag)));
			clearRendererBypass((Params::GetTypeFromTag(Params::_flowParamsTag)));
		} else if (t == NavigatingBit) {
			clearRendererBypass((Params::GetTypeFromTag(Params::_isoParamsTag)));
			clearRendererBypass((Params::GetTypeFromTag(Params::_dvrParamsTag)));
		}
	}
}
//Clear all render bypass flags for active renderers that match the specified type(s)

void GLWindow::
clearRendererBypass(Params::ParamsBaseType t){
	for (int i = 0; i< getNumRenderers(); i++){
		if(renderer[i]->isInitialized() && (renderType[i] == t))
			renderer[i]->setAllBypass(false);
	}
}

bool GLWindow::
vizIsDirty(DirtyBitType t) {
	return vizDirtyBit[t];
}
void GLWindow::
doFrameCapture(){
	if (capturingImage == 0) return;
	//Create a string consisting of captureName, followed by nnnn (framenum), 
	//followed by .jpg or .tif
	QString filename;
	if (capturingImage == 2){
		filename = captureNameImage;
		filename += (QString("%1").arg(captureNumImage)).rightJustified(4,'0');
		if (capturingTif) filename +=  ".tif";
		else filename +=  ".jpg";
	} //Otherwise we are just capturing one frame:
	else filename = captureNameImage;
	if (!(filename.endsWith(".jpg"))&&!(filename.endsWith(".tif"))) filename += ".jpg";
	
	FILE* jpegFile = NULL;
	TIFF* tiffFile = NULL;
	if (filename.endsWith(".tif")){
		tiffFile = TIFFOpen((const char*)filename.toAscii(), "wb");
		if (!tiffFile) {
			SetErrMsg(VAPOR_ERROR_IMAGE_CAPTURE,"Image Capture Error: Error opening output Tiff file: %s",(const char*)filename.toAscii());
			capturingImage = 0;
			return;
		}
	} else {
		// open the jpeg file:
		jpegFile = fopen((const char*)filename.toAscii(), "wb");
		if (!jpegFile) {
			SetErrMsg(VAPOR_ERROR_IMAGE_CAPTURE,"Image Capture Error: Error opening output Jpeg file: %s",(const char*)filename.toAscii());
			capturingImage = 0;
			return;
		}
	}
	//Get the image buffer 
	unsigned char* buf = new unsigned char[3*width()*height()];
	//Use openGL to fill the buffer:
	if(!getPixelData(buf)){
		//Error!
		SetErrMsg(VAPOR_ERROR_IMAGE_CAPTURE,"Image Capture Error; error obtaining GL data");
		capturingImage = 0;
		delete [] buf;
		return;
	}
	
	//Now call the Jpeg or tiff library to compress and write the file
	//
	if (filename.endsWith(".jpg")){
		int quality = getJpegQuality();
		int rc = write_JPEG_file(jpegFile, width(), height(), buf, quality);
		if (rc){
			//Error!
			SetErrMsg(VAPOR_ERROR_IMAGE_CAPTURE,"Image Capture Error; Error writing jpeg file %s",
				(const char*)filename.toAscii());
			capturingImage = 0;
			delete [] buf;
			return;
		}
	} else { //capture the tiff file, one scanline at a time
		uint32 imagelength = (uint32) height();
		uint32 imagewidth = (uint32) width();
		TIFFSetField(tiffFile, TIFFTAG_IMAGELENGTH, imagelength);
		TIFFSetField(tiffFile, TIFFTAG_IMAGEWIDTH, imagewidth);
		TIFFSetField(tiffFile, TIFFTAG_PLANARCONFIG, 1);
		TIFFSetField(tiffFile, TIFFTAG_SAMPLESPERPIXEL, 3);
		TIFFSetField(tiffFile, TIFFTAG_ROWSPERSTRIP, 8);
		TIFFSetField(tiffFile, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField(tiffFile, TIFFTAG_PHOTOMETRIC, 2);
		for (int row = 0; row < imagelength; row++){
			int rc = TIFFWriteScanline(tiffFile, buf+row*3*imagewidth, row);
			if (rc != 1){
				SetErrMsg(VAPOR_ERROR_IMAGE_CAPTURE,"Image Capture Error; Error writing tiff file %s",
				(const char*)filename.toAscii());
				break;
			}
		}
		TIFFClose(tiffFile);
	}

	//If just capturing single frame, turn it off, otherwise advance frame number
	if(capturingImage > 1) captureNumImage++;
	else capturingImage = 0;
	delete [] buf;
	
}

float GLWindow::getPixelSize(){
	float temp[3];
	//Window height is subtended by viewing angle (45 degrees),
	//at viewer distance (dist from camera to view center)
	ViewpointParams* vpParams = getActiveViewpointParams();
	vsub(vpParams->getRotationCenterLocal(),vpParams->getCameraPosLocal(),temp);
	//Apply stretch factor:
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<3; i++) temp[i] = stretch[i]*temp[i];
	float distToScene = vlength(temp);
	//tan(45 deg *0.5) is ratio between half-height and dist to scene
	float halfHeight = tan(M_PI*0.125)* distToScene;
	return (2.f*halfHeight/(float)height()); 
	
}

void GLWindow::placeLights(){
	printOpenGLError();
	ViewpointParams* vpParams = getActiveViewpointParams();
	int nLights = vpParams->getNumLights();
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
		printOpenGLError();
		if (nLights > 1){
			printOpenGLError();
			glLightfv(GL_LIGHT1, GL_POSITION, vpParams->getLightDirection(1));
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
			glLightfv(GL_LIGHT2, GL_POSITION, vpParams->getLightDirection(2));
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
	float winCoords[2] = {0.f,0.f};
	float dispCoords[2];
	
	if (handleNum > 2) handleNum = handleNum-3;
	else handleNum = 2 - handleNum;
	float boxExtents[6];
	int timestep = getActiveAnimationParams()->getCurrentTimestep();
	manipParams->calcStretchedBoxExtentsInCube(boxExtents, timestep);
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
	
	mySpinTimer = new SpinTimer();
	mySpinTimer->setWindow(this);
	mySpinTimer->start(renderMS);
	isSpinning = true;
	//Increment the rotation and request another render..
	getTBall()->TrackballSpin();
	setViewerCoordsChanged(true);
	setDirtyBit(ProjMatrixBit, true);
	update();
}
bool GLWindow::stopSpin(){
	if (isSpinning){
		mySpinTimer->stop();
		delete mySpinTimer;
		mySpinTimer = 0;
		isSpinning = false;
		return true;
	}
	return false;
}

void GLWindow::
setValuesFromGui(ViewpointParams* vpparams){
	
	//Same as the version in vizwin, but doesn't force redraw.
	Viewpoint* vp = vpparams->getCurrentViewpoint();
	float transCameraPos[3];
	float cubeCoords[3];
	//Must transform from world coords to unit cube coords for trackball.
	ViewpointParams::localToStretchedCube(vpparams->getCameraPosLocal(), transCameraPos);
	ViewpointParams::localToStretchedCube(vpparams->getRotationCenterLocal(), cubeCoords);
	myTBall->setFromFrame(transCameraPos, vp->getViewDir(), vp->getUpVec(), cubeCoords, vp->hasPerspective());
	
	//If the perspective was changed, a resize event will be triggered at next redraw:
	
	setPerspective(vp->hasPerspective());
	setDirtyBit(ProjMatrixBit,true);
	
}
void GLWindow::
sortRenderers(int timestep){
	//Reorder the priority 5 renderers with their distance from camera
	
	//Indices point to places in renderer list:
	int start5Renderers = -1; //will point to where the priority 5 renderers start
	int numOpaque5Renderers = 0; //count number of pri 5 renderers that are opaque
	int numToSort = 0; //This will be num5Renderers - numOpaque5Renderers;
	int num5Renderers = 0; //Count of all pri 5 renderers;
	std::vector<RenderListElt*> sortList;
	for (int i = 0; i<numRenderers; i++){
		if (renderOrder[i] < 5) continue;
		if (renderOrder[i] > 5) break;//found renderer with higher order
		if (start5Renderers < 0) start5Renderers = i;  //save first sorted renderer position
		//Move all opaque renderers up to the start.  Put the rest into the sort list
		RenderParams* rParams = renderer[i]->getRenderParams();
		if (rParams->IsOpaque()){ //move it up
			assert(start5Renderers+numOpaque5Renderers <= i);
			renderer[start5Renderers+numOpaque5Renderers] = renderer[i];
			renderType[start5Renderers+numOpaque5Renderers] = renderType[i];
			numOpaque5Renderers++;
		} else { //Add it to the ones to sort:
			RenderListElt* newRenListElt = new RenderListElt();
			newRenListElt->ren = renderer[i];
			newRenListElt->renType = renderType[i];
			sortList.push_back(newRenListElt);
			numToSort++;
		}
		num5Renderers++;
	}
	if(numToSort < 2) {
		//No need to sort but may need to put transparent after opaque:
		if (numToSort > 0){
			int startSortPos = start5Renderers+numOpaque5Renderers;
			renderer[startSortPos] = sortList[0]->ren;
			renderType[startSortPos] = sortList[0]->renType;
			delete sortList[0];
		}
		return; 
	}
	for (int i = 0; i<numToSort; i++){
		//Determine the camera distance of each renderer
		RenderParams* rParams = sortList[i]->ren->getRenderParams();
		sortList[i]->camDist = rParams->getCameraDistance(getActiveViewpointParams(),getActiveRegionParams(), timestep);
	}

	sort(sortList.begin(), sortList.end(), renderPriority);
	
	//Now stuff the renderers, etc., back into numToSort positions of the
	//renderer queue, starting at position = start5Renderers+numOpaque5Renderers;
	//renderOrder remains unchanged
	int startSortPos = start5Renderers+numOpaque5Renderers;
	for (int i = 0; i<sortList.size(); i++){
		renderer[i+startSortPos] = sortList[i]->ren;
		renderType[i+startSortPos] = sortList[i]->renType;
		delete sortList[i];
	}
	return;
}
void GLWindow::removeDisabledRenderers(){
	//Repeat until we don't find any renderers to disable:
	
	while(1){
		bool retry = false;
		for (int i = 0; i< numRenderers; i++){
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
		
		
int GLWindow::AddMouseMode(const std::string paramsTag, int manipType, const char* name){
	
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
void GLWindow::TransformToUnitBox(){
	DataStatus* ds = DataStatus::getInstance();
	size_t timeStep = (size_t)getActiveAnimationParams()->getCurrentTimestep();
	if (!ds->getDataMgr()) return;
	const vector<double>& fullUsrExts = ds->getDataMgr()->GetExtents(timeStep);
	float sceneScaleFactor = 1.f/ViewpointParams::getMaxStretchedCubeSide();
	const float* scales = ds->getStretchFactors();
	glScalef(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);
	float transVec[3];
	for (int i = 0; i<3; i++) transVec[i] = fullUsrExts[i]*scales[i];
	glTranslatef(-transVec[0],-transVec[1], -transVec[2]);
}

void GLWindow::ConvertAxes(bool toLatLon, const int ticDirs[3], const double fromMinTic[3], const double fromMaxTic[3], const double fromOrigin[3], const double fromTicLength[3],
		double toMinTic[3],double toMaxTic[3], double toOrigin[3], double toTicLength[3]){
	double ticLengthFactor[3], xmin[2],ymin[2],xmax[2],ymax[2];
	//Copy the z coordinates.
	toMinTic[2]=fromMinTic[2];
	toMaxTic[2] = fromMaxTic[2];
	toTicLength[2] = fromTicLength[2];
	//Put inputs into xmin, xmax, ymin, ymax
	xmin[0] = fromMinTic[0];
	xmin[1] = fromOrigin[1];
	xmax[0] = fromMaxTic[0];
	xmax[1] = fromOrigin[1];
	ymin[1] = fromMinTic[1];
	ymin[0] = fromOrigin[0];
	ymax[1] = fromMaxTic[1];
	ymax[0] = fromOrigin[0];
	for (int j = 0; j<3; j++){
		toOrigin[j] = fromOrigin[j];  //copy the origin coords
		//Determine ticLength as a fraction of maxTic-minTic along the direction in which the tic is pointed
		//Ignore tics that point in the z direction
		if (ticDirs[j] == 2) continue;
		double den = (fromMaxTic[ticDirs[j]]- fromMinTic[ticDirs[j]]);
		if (den > 0.) ticLengthFactor[j] = fromTicLength[j]/den;
		else ticLengthFactor[j] = 1.;
		
	}
	if (toLatLon){
		//convert user coords  at axis annotation ends to lat lon
		
		DataStatus::convertToLonLat(xmin);
		DataStatus::convertToLonLat(ymin);
		DataStatus::convertToLonLat(xmax);
		
		DataStatus::convertToLonLat(ymax);
		DataStatus::convertToLonLat(toOrigin);
		if (xmax[0] < xmin[0]) xmax[0]+= 360.;
		if (xmax[0] > 361.){
			xmax[0] -= 360.;
			xmin[0] -= 360.;
			toOrigin[0] -= 360.;
		}
		if (xmin[0] <= -360.){
			xmax[0] += 360.;
			xmin[0] += 360.;
			toOrigin[0] += 360.;
		}
		
	} else {
		//Convert lat/lon to user
		//First find lat/lon of min/max extents:
		const vector<double>& exts = DataStatus::getInstance()->getDataMgr()->GetExtents();
		double cvExts[4];
		cvExts[0] = exts[0];
		cvExts[1] = exts[1];
		cvExts[2] = exts[3];
		cvExts[3] = exts[4];
		DataStatus::convertToLonLat(cvExts,2);
		if (cvExts[2] <= cvExts[0]) cvExts[2] = cvExts[2]+360.;
		if (cvExts[2] > 360.){
			cvExts[2] -= 360.;
			cvExts[0] -= 360.;
		}
		//convert user coords at axis annotation ends from lat/lon to user
	
		flatConvertFromLonLat(xmin, cvExts[0], cvExts[2], exts[0], exts[3]);
		flatConvertFromLonLat(xmax, cvExts[0], cvExts[2], exts[0], exts[3]);
		flatConvertFromLonLat(ymin, cvExts[0], cvExts[2], exts[0], exts[3]);
		flatConvertFromLonLat(ymax, cvExts[0], cvExts[2], exts[0], exts[3]);
		flatConvertFromLonLat(toOrigin, cvExts[0], cvExts[2], exts[0], exts[3]);
		
	}
	//Set min/max tic from xmin,xmax,ymin,ymax
	// Only set min and max on each axis.
	toMinTic[0] = xmin[0];
	toMaxTic[0] = xmax[0];
	toMinTic[1] = ymin[1];
	toMaxTic[1] = ymax[1];
	//Adjust ticLength to be in user coords
	for (int j = 0; j<3; j++){
		if (ticDirs[j] == 2) continue;
		double dst = (toMaxTic[ticDirs[j]]- toMinTic[ticDirs[j]]);
		toTicLength[j] = dst*ticLengthFactor[j];
	}
	
}


//Add a textObject to the set of objects to be used.  Return its index.
int GLWindow::addTextObject(Renderer* ren, const char* fontPath, int textSize, float textColor[4], float bgColor[4], int type, string text){
	if(textObjectMap.count(ren) == 0){
		vector<TextObject*> txtObjs = *(new vector<TextObject*>);
		textObjectMap[ren] = txtObjs;
	}
	float dummyCoords[3] = {.23,.32,.0};//not used

	TextObject *to = new TextObject();
	to->Initialize(fontPath,text,textSize,dummyCoords,type,textColor,bgColor,this);
	textObjectMap[ren].push_back(to);//new TextObject(fontPath,text,textSize,dummyCoords,type,textColor,bgColor,this));
		
	int objectIndex = textObjectMap[ren].size()-1;

	pair<Renderer*, int> coordPair = make_pair(ren, objectIndex);
	vector<float*>* coordinates = new vector<float*>;
	textCoordMap[coordPair] = coordinates;
	
	return objectIndex;
}
//Add an instance of text at specified position, using specified writer
void GLWindow::addText(Renderer* ren, int objectIndex, float posn[3]){
	float* newPosn = new float[3];
	for (int i = 0; i<3; i++) {
		newPosn[i] = posn[i];
		//cout << posn[i] << " ";
	}
	//cout << endl;
	pair<Renderer*, int> coordPair = make_pair(ren, objectIndex);
	textCoordMap[coordPair]->push_back(newPosn);
}
//Render all the text; 
void GLWindow::renderText(){
	//iterate over all valid text objects, paint them at specified coordinates.
	int timestep = getActiveAnimationParams()->getCurrentTimestep();
	map< Renderer*, vector<TextObject*> >::iterator iter;
	for (iter = textObjectMap.begin(); iter != textObjectMap.end(); iter++){
		Renderer* ren = iter->first;
		vector<TextObject*> txtObjs = iter->second;
		
		for (int i = 0; i<txtObjs.size(); i++){
			TextObject* txtObj = txtObjs[i];
			pair<Renderer*, int> coordPair = make_pair(ren, i);
			vector<float*> textCoords = *textCoordMap[coordPair];
			for (int j = 0; j<textCoords.size(); j++){
				float crds[3];
				for (int k=0; k<3; k++) crds[k] = textCoords[j][k];
				txtObj->drawMe(crds, timestep);
			}
		}
	}

}
void GLWindow::clearTextObjects(Renderer* ren){
	//verify that the renderer has text objects:
	if (textObjectMap.count(ren) == 0) return;
	
	vector<TextObject*> txtObjs = textObjectMap[ren];
	for (int i = 0; i<txtObjs.size(); i++){
			//first delete the coordinates used by the text object
			TextObject* txtObj = txtObjs[i];
			pair<Renderer*, int> coordPair = make_pair(ren, i);
			vector<float*> textCoords = *textCoordMap[coordPair];
			for (int j = 0; j<textCoords.size(); j++){
				delete textCoords[j];
			}
			textCoords.clear();
			//Remove the entry from the textcoordinate map:
			map<pair<Renderer*, int>, vector<float*>*>::iterator it2 = textCoordMap.find(coordPair);
			textCoordMap.erase(it2);
			//And delete the text object
			delete txtObj;
		}
	txtObjs.clear();
	//Now remove these textObjects from the mapping.
	map<Renderer*, vector<TextObject*> >::iterator it = textObjectMap.find(ren);
	textObjectMap.erase(it);
}
void GLWindow::flatConvertFromLonLat(double x[2], double minLon, double maxLon, double minX, double maxX){
	if (x[1] > 90.) x[1] = 90.;
	if (x[1] < -90.) x[1] = -90.;
	//Extend the conversion linearly if the point is outside the lon extents.
	
	if( x[0] < minLon){
		x[0] = 2.*minLon - x[0];
		DataStatus::convertFromLonLat(x);
		x[0] = 2.*minX - x[0];
	} else if (x[0] > maxLon){
		x[0] = 2.*maxLon -x[0];
		DataStatus::convertFromLonLat(x);
		x[0] = 2.*maxX - x[0];
	} else {
		DataStatus::convertFromLonLat(x);
	}
}

#ifdef	Darwin
//
// Workaround for:
//	743: core dump when rendering over X11
//	947: OpenGL errors on Mac
//
// With Qt 4.8 paint events are generated before the window is exposed. 
// This causes problems on Mac systems. Unforunately, there is no way to
// detect an expose event with Qt 4.8. This method looks for a "PolishRequest"
// event before allowing vaporgui to perform any OpenGL rendering. Hopefully
// Qt 5.x will either not generate paint events until after the window
// is exposed, or will provide a true expose event.
//
bool GLWindow::event ( QEvent * e ){
	if (e->type() == QEvent::PolishRequest) {
		_readyToDraw = true;
	}
    return(QGLWidget::event(e));
}
#endif
