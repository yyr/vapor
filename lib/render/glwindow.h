//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glwindow.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Definition of GLWindow class: 
//	A GLWindow object is embedded in each visualizer window.  It performs basic
//	 navigation and resize, and defers drawing to the viz window's list of 
//	 registered renderers.

#ifndef GLWINDOW_H
#define GLWINDOW_H
#include <map>
#include "trackball.h"
#include <qthread.h>
#include <QMutex>
#include <jpeglib.h>
#include "qcolor.h"
#include "qlabel.h"
#include "params.h"
#include "manip.h"
#include <vapor/MyBase.h>
#include <vapor/common.h>
#include "datastatus.h"
#include "ShaderMgr.h"

//No more than 20 renderers in a window:
//Eventually this may be dynamic.
#define MAXNUMRENDERERS 20

class QLabel;
class QThread;


namespace VAPoR {
typedef bool (*renderCBFcn)(int winnum, bool newCoords);
class SpinTimer;
class ViewpointParams;
class RegionParams;
class DvrParams;
class AnimationParams;
class FlowParams;
class ProbeParams;
class TwoDDataParams;
class TwoDImageParams;
class Renderer;
class TranslateStretchManip;
class TranslateRotateManip;
class FlowRenderer;
class VolumeRenderer;
class TextObject;

//! \class GLWindow
//! \brief A class for performing OpenGL rendering in a VAPOR Visualizer
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!
//! 
//! The GLWindow class is an OpenGL rendering window.  There is one instance for 
//! each of the VAPOR visualizers.  The GLWindow is embedded in a VizWin instance.
//! It performs basic setup for OpenGL rendering, renders incidental geometry,
//! and calls all the enabled VAPOR Renderer instances.
//! The GLWindow maintains "Dirty bits" that indicate window-specific state that
//! may require refreshing
class RENDER_API GLWindow : public MyBase, public QGLWidget
{
public:

	//! Method for setting window-specific dirty bits.
	//! All the bits are defined in the params.h file, in the VAPoR namespace.
	//! \param[in] DirtyBitType t identifies the property being set
	//! \param[in] bool val sets the bit true (dirty) or false (clean)
	void setDirtyBit(DirtyBitType t, bool val);

	//! Method for checking window-specific dirty bits.
	//! All the bits are defined in the params.h file, in the VAPoR namespace.
	//! \param[in] DirtyBitType t identifies the property being checked
	//! \retval bool is true if it is dirty.
	bool vizIsDirty(DirtyBitType t);

	//! Method that returns the ViewpointParams that is active in this window.
	//! \retval ViewpointParams* current active ViewpointParams
	ViewpointParams* getActiveViewpointParams() {return (ViewpointParams*)getActiveParams(Params::_viewpointParamsTag);}

	//! Method that returns the RegionParams that is active in this window.
	//! \retval RegionParams* current active RegionParams
	RegionParams* getActiveRegionParams() {return (RegionParams*)getActiveParams(Params::_regionParamsTag);}

	//! Method that returns the AnimationParams that is active in this window.
	//! \retval AnimationParams* current active AnimationParams
	AnimationParams* getActiveAnimationParams() {return (AnimationParams*)getActiveParams(Params::_animationParamsTag);}

	//! Method that transforms the world into the unit box, should be called by each renderer before rendering
	void TransformToUnitBox();

	//MouseMode support
	//! Static method that indicates the manipulator type that is associated with a mouse mode.
	//! \sa VizWinMgr
	//! \param[in] modeIndex is a positive integer indexing the current mouse modes
	//! \retval int manipulator type is 1 (region box) 2 (2D box) or 3 (rotated 3D box).
	static int getModeManipType(int modeIndex){
		return manipFromMode[modeIndex];
	}
	//! Static method that returns the Params type associated with a mouse mode.
	//! \param[in] int modeIndex  The mouse mode.
	//! \retval ParamsBase::ParamsBaseType  The type of the params associated with the mouse mode.
	static ParamsBase::ParamsBaseType getModeParamType(int modeIndex){
		return paramsFromMode[modeIndex];
	}

	//! Static method that identifies the current mouse mode for a particular params type.
	//! \param[in] ParamsBase::ParamsBaseType t must be the type of a params with an associated mouse mode
	//! \retval int Mouse mode
	static int getModeFromParams(ParamsBase::ParamsBaseType t){return modeFromParams[t];}

	//! Static method that identifies the name associated with a mouse mode.
	//! This is the text that is displayed in the mouse mode selector.
	//! \param[in] int Mouse Mode
	//! \retval const string& Name associated with mode
	static const string& getModeName(int index) {return modeName[index];}

	//! Static method used to add a new Mouse Mode to the list of available modes.
	//! \param[in] const string& tag associated with the Params class
	//! \param[in] int Manipulator type (1,2,or 3)
	//! \param[in] const char* name of mouse mode
	//! \retval int Resulting mouse mode
	static int AddMouseMode(const std::string paramsTag, int manipType, const char* name);

	//! Static method to convert axis coordinates between user and lat-lon
	//! It is OK for outputs to equal corresponding inputs.
	//! \param[in] toLatLon indicates whether conversion is to LatLon (true) or to user (false)
	//! \param[in] ticDirs are directions of tics on each axis.
	//! \param[in] fromMinTic is a 3-vector indicating minimum tic coordinates being mapped.
	//! \param[in] fromMaxTic is a 3-vector indicating maximum tic coordinates being mapped.
	//! \param[in] fromOrigin is a 3-vector indicating origin coordinates being mapped.
	//! \param[in] fromTicLength is a 3-vector indicating tic lengths before conversion
	//! \param[out] toMinTic is result 3-vector of minimum tic coordinates 
	//! \param[out] toMaxTic is result 3-vector of maximum tic coordinates
	//! \param[out] toOrigin is result 3-vector of origin coordinates
	//! \param[out] toTicLength is a 3-vector indicating tic lengths after conversion
	static void ConvertAxes(bool toLatLon, const int ticDirs[3], const double fromMinTic[3], const double fromMaxTic[3], const double fromOrigin[3], const double fromTicLength[3],
		double toMinTic[3],double toMaxTic[3], double toOrigin[3], double toTicLength[3]);

	//Added for FTGL 
	//Add a textObject to the set of text to be used.  Return its index.
	int addTextObject(Renderer*, const char* fontPath, int textSize, float textColor[4], float bgColor[4], int type, string text); 
	//Add an instance of text at specified position, using specified object
	void addText(Renderer*, int objectNum, float posn[3]);
	void clearTextObjects(Renderer*);
	//Check whether the textObjects have been built for this renderer
	bool isTextValid(Renderer* ren) {return textValidFlag[ren];}
	void setTextValid(Renderer* ren, bool val) {textValidFlag[ren] = val;}

#ifndef DOXYGEN_SKIP_THIS
	 GLWindow( QGLFormat& fmt, QWidget* parent, int winnum);
    ~GLWindow();
	Trackball* myTBall;
	
	Trackball* getTBall() {return myTBall;}
	void setPerspective(bool isPersp) {perspective = isPersp;}
	bool getPerspective() {return perspective;}

	//Enum describes various mouse modes:
	enum mouseModeType {
		unknownMode=1000,
		navigateMode=0,
		regionMode=1,
		probeMode=3,
		twoDDataMode=4,
		twoDImageMode=5,
		rakeMode=2,
		barbMode=7,
		isolineMode=6
	};

	//Reset the GL perspective, so that the near and far clipping planes are wide enough
	//to view twice the entire region from the current camera position.  If the camera is
	//inside the doubled region, region, make the near clipping plane 1% of the region size.

	void resetView(ViewpointParams* vpParams);
	void setMaxSize(float wsize) {maxDim = wsize;}
	void setCenter(float cntr[3]) { wCenter[0] = cntr[0]; wCenter[1] = cntr[1]; wCenter[2] = cntr[2];}
	//Test if the screen projection of a 3D quad encloses a point on the screen.
	//The 4 corners of the quad must be specified in counter-clockwise order
	//as viewed from the outside (pickable side) of the quad.  
	//
	bool pointIsOnQuad(float cor1[3],float cor2[3],float cor3[3],float cor4[3], float pickPt[2]);

	//Determine if a point is over (and not inside) a box, specified by 8 3-D coords.
	//the first four corners are the counter-clockwise (from outside) vertices of one face,
	//the last four are the corresponding back vertices, clockwise from outside
	//Returns index of face (0..5), or -1 if not on Box
	//
	int pointIsOnBox(float corners[8][3], float pickPt[2]);

	//Project a 3D point (in cube coord system) to window coords.
	//Return true if in front of camera
	//
	bool projectPointToWin(float cubeCoords[3], float winCoords[2]){
		double cCoords[3], wCoords[2];
		for (int i = 0; i<3; i++) cCoords[i] = cubeCoords[i];
		bool rc = projectPointToWin(cCoords,wCoords);
		winCoords[0]=wCoords[0];winCoords[1]=wCoords[1];
		return rc;
	}
	bool projectPointToWin(double cubeCoords[3], double winCoords[2]);

	// Project the current mouse coordinates to a line in screen space.
	// The line starts at the mouseDownPosition, and points in the
	// direction resulting from projecting to the screen the axis 
	// associated with the dragHandle.  Returns false on error.

	bool projectPointToLine(float mouseCoords[2], float projCoords[2]);

	//Params argument is the params that owns the manip
	bool startHandleSlide(float mouseCoords[2], int handleNum, Params* p);
	
	//Determine a unit direction vector associated with a pixel.  Uses OpenGL screencoords
	// I.e. y = 0 at bottom.  Returns false on failure.
	//
	bool pixelToVector(float winCoords[2], const float cameraPos[3], float dirVec[3]);

	//Get the current image in the front buffer;
	bool getPixelData(unsigned char* data);

	void setRenderNew() {renderNew = true;}
	void draw3DCursor(const float position[3]);

	void renderTimeStamp(bool rebuild);
	void buildTimeStampImage();
	
	//Get/set methods for vizfeatures
	bool useLatLonAnnotation() {return latLonAnnot;}
	void setLatLonAnnotation(bool val){ latLonAnnot = val;}
	QColor getBackgroundColor() {return DataStatus::getInstance()->getBackgroundColor();}
	QColor getRegionFrameColor() {return DataStatus::getInstance()->getRegionFrameColor();}
	QColor getSubregionFrameColor() {return DataStatus::getInstance()->getSubregionFrameColor();}
	QColor& getColorbarBackgroundColor() {return colorbarBackgroundColor;}
	bool axisArrowsAreEnabled() {return axisArrowsEnabled;}
	bool axisAnnotationIsEnabled() {return axisAnnotationEnabled;}
	bool colorbarIsEnabled() {return colorbarEnabled;}
	int getColorbarParamsTypeId() {return colorbarParamsTypeId;}
	void setColorbarParamsTypeId(int val) {colorbarParamsTypeId = val;}
	bool regionFrameIsEnabled() {return DataStatus::getInstance()->regionFrameIsEnabled();}
	bool subregionFrameIsEnabled() {return DataStatus::getInstance()->subregionFrameIsEnabled();}
	float getAxisArrowCoord(int i){return axisArrowCoord[i];}
	double getAxisOriginCoord(int i){return axisOriginCoord[i];}
	double getMinTic(int i){return minTic[i];}
	double getMaxTic(int i){return maxTic[i];}
	double getTicLength(int i){return ticLength[i];}
	int getNumTics(int i){return numTics[i];}
	int getTicDir(int i){return ticDir[i];}
	int getLabelHeight() {return labelHeight;}
	int getLabelDigits() {return labelDigits;}
	float getTicWidth(){return ticWidth;}
	QColor& getAxisColor() {return axisColor;}
	float getColorbarLLCoord(int i) {return colorbarLLCoord[i];}
	float getColorbarURCoord(int i) {return colorbarURCoord[i];}
	int getColorbarNumTics() {return numColorbarTics;}
	int getColorbarDigits() {return colorbarDigits;}
	int getColorbarFontsize() {return colorbarFontsize;}
		

	int getTimeAnnotType() {return timeAnnotType;}
	int getTimeAnnotTextSize() {return timeAnnotTextSize;}
	float getTimeAnnotCoord(int j){return timeAnnotCoords[j];}
	QColor getTimeAnnotColor(){return timeAnnotColor;}
	void setTimeAnnotTextSize(int size){timeAnnotTextSize = size;}
	void setTimeAnnotColor(QColor c) {timeAnnotColor = c;}
	void setTimeAnnotType(int t) {timeAnnotType = t;}
	void setTimeAnnotCoords(float crds[2]){
		timeAnnotCoords[0] = crds[0]; 
		timeAnnotCoords[1] = crds[1];
	}

	void setBackgroundColor(QColor& c) {DataStatus::getInstance()->setBackgroundColor(c);}
	void setColorbarBackgroundColor(QColor& c) {colorbarBackgroundColor = c;}
	void setRegionFrameColor(QColor& c) {DataStatus::getInstance()->setRegionFrameColor(c);}
	void setSubregionFrameColor(QColor& c) {DataStatus::getInstance()->setSubregionFrameColor(c);}
	void enableAxisArrows(bool enable) {axisArrowsEnabled = enable;}
	void enableAxisAnnotation(bool enable) {axisAnnotationEnabled = enable;}
	void enableColorbar(bool enable) {colorbarEnabled = enable;}
	void enableRegionFrame(bool enable) {DataStatus::getInstance()->enableRegionFrame(enable);}
	void enableSubregionFrame(bool enable) {DataStatus::getInstance()->enableSubregionFrame(enable);}
	
	void setAxisArrowCoord(int i, float val){axisArrowCoord[i] = val;}
	void setAxisOriginCoord(int i, double val){axisOriginCoord[i] = val;}
	void setNumTics(int i, int val) {numTics[i] = val;}
	void setTicDir(int i, int val) {ticDir[i] = val;}
	void setMinTic(int i, double val) {minTic[i] = val;}
	void setMaxTic(int i, double val) {maxTic[i] = val;}
	void setTicLength(int i, double val) {ticLength[i] = val;}
	void setLabelHeight(int h){labelHeight = h;}
	void setLabelDigits(int d) {labelDigits = d;}
	void setTicWidth(float w) {ticWidth = w;}
	void setAxisColor(QColor& c) {axisColor = c;}
	void setDisplacement(float val){displacement = val;}
	float getDisplacement() {return displacement;}
	void setColorbarLLCoord(int i, float crd) {colorbarLLCoord[i] = crd;}
	void setColorbarURCoord(int i, float crd) {colorbarURCoord[i] = crd;}
	void setColorbarNumTics(int i) {numColorbarTics = i;}
	bool colorbarIsDirty() {return colorbarDirty;}
	bool timeAnnotIsDirty() {return timeAnnotDirty;}
	void setColorbarDirty(bool val){colorbarDirty = val;}
	void setTimeAnnotDirty(bool val){timeAnnotDirty = val;}
	void setColorbarDigits(int ndigs) {colorbarDigits = ndigs;}
	void setColorbarFontsize(int fsize) {colorbarFontsize = fsize;}
	void setAxisLabelsDirty(bool val){axisLabelsDirty = val;}
	bool axisLabelsAreDirty(){return axisLabelsDirty;}


	bool mouseIsDown() {return mouseDownHere;}
	void setMouseDown(bool downUp) {mouseDownHere = downUp;}

	void setNumRenderers(int num) {numRenderers = num;}
	int getNumRenderers() { return numRenderers;}
	Params::ParamsBaseType getRendererType(int i) {return renderType[i];}

	Renderer* getRenderer(int i) {return renderer[i];}
	Renderer* getRenderer(RenderParams* p);
	
	//Renderers can be added early or late, using a "render Order" parameter.
	//The order is between 0 and 10; lower order gets rendered first.
	//Sorted renderers get sorted before each render
	//
	void insertSortedRenderer(RenderParams* p, Renderer* ren){insertRenderer(p, ren, 5);}
	void prependRenderer(RenderParams* p, Renderer* ren) {insertRenderer(p, ren, 0);}
	void appendRenderer(RenderParams* p, Renderer* ren){insertRenderer(p, ren, 10);}
	void insertRenderer(RenderParams* p, Renderer* ren, int order);
	bool removeRenderer(RenderParams* p);  //Return true if successful
	void removeAllRenderers();
	void sortRenderers(int timestep);  //Sort all the pri 0 renderers
	
	void removeDisabledRenderers();  //Remove renderers whose params are disabled
	//Find a renderParams in renderer list, if it exists:
	RenderParams* findARenderer(Params::ParamsBaseType renderertype);
	
	static int getCurrentMouseMode() {return currentMouseMode;}
	static void setCurrentMouseMode(int t){currentMouseMode = t;}
	//The glwindow keeps a copy of the params that are currently associated with the current
	//instance.  This needs to change during:
	//  -loading session
	//  -undo/redo
	//  -changing instance
	//	-reinit
	//	-new visualizer
	
	void setActiveViewpointParams(Params* p) {setActiveParams(p,Params::_viewpointParamsTag);}
	void setActiveRegionParams(Params* p) {
		setActiveParams(p,Params::_regionParamsTag);
		getManip(Params::_regionParamsTag)->setParams(p);
	}
	void setActiveAnimationParams(Params* p) {setActiveParams(p,Params::_animationParamsTag);}

	
	FlowParams* getActiveFlowParams() {return (FlowParams*)getActiveParams(Params::_flowParamsTag);}
	ProbeParams* getActiveProbeParams() {return (ProbeParams*)getActiveParams(Params::_probeParamsTag);}
	TwoDDataParams* getActiveTwoDDataParams() {return (TwoDDataParams*)getActiveParams(Params::_twoDDataParamsTag);}
	TwoDImageParams* getActiveTwoDImageParams() {return (TwoDImageParams*)getActiveParams(Params::_twoDImageParamsTag);}

	vector<Params*> currentParams;
	Params* getActiveParams(ParamsBase::ParamsBaseType pType) {return currentParams[pType];}
	Params* getActiveParams(const std::string& tag) {return currentParams[Params::GetTypeFromTag(tag)];}
	void setActiveParams(Params* p, ParamsBase::ParamsBaseType pType){
		currentParams[pType] = p;
	}
	void setActiveParams(Params* p, const std::string& tag){
		currentParams[Params::GetTypeFromTag(tag)] = p;
	}

	//The GLWindow keeps track of the renderers with an ordered list of them
	//as well as with a map from renderparams to renderer
	
	void mapRenderer(RenderParams* rp, Renderer* ren){rendererMapping[rp] = ren;}
	bool unmapRenderer(RenderParams* rp);
	
	//Determine the approximate size of a pixel in terms of viewer coordinates.
	float getPixelSize();
	bool viewerCoordsChanged() {return newViewerCoords;}
	void setViewerCoordsChanged(bool isNew) {newViewerCoords = isNew;}
	bool isCapturingImage() {return (capturingImage != 0);}
	bool isCapturingFlow() {return (capturingFlow);}
	bool isSingleCapturingImage() {return (capturingImage == 1);}
	void startImageCapture(QString& name, int startNum, bool isTif) {
		capturingImage = 2;
		captureNumImage = startNum;
		captureNameImage = name;
		newCaptureImage = true;
		capturingTif = isTif;
		previousFrameNum = -1;
		previousTimeStep = -1;
		update();
	}
	void startFlowCapture(QString& name) {
		capturingFlow = true;
		captureNameFlow = name;
		update();
	}
	void singleCaptureImage(QString& name){
		capturingImage = 1;
		captureNameImage = name;
		newCaptureImage = true;
		update();
	}
	bool captureIsNewImage() { return newCaptureImage;}
	
	void setCaptureNewImage(bool isNew){ newCaptureImage = isNew;}
	
	void stopImageCapture() {capturingImage = 0;}
	void stopFlowCapture() {capturingFlow = false;}
	//Routine is called at the end of rendering.  If capture is 1 or 2, it converts image
	//to jpeg and saves file.  If it ever encounters an error, it turns off capture.
	//If capture is 1 (single frame capture) it turns off capture.
	void doFrameCapture();
	QString& getFlowFilename(){return captureNameFlow;}
	

	TranslateStretchManip* getManip(const std::string& paramTag){
		int mode = getModeFromParams(ParamsBase::GetTypeFromTag(paramTag));
		return manipHolder[mode];
	}
	

	void setPreRenderCB(renderCBFcn f){preRenderCB = f;}
	void setPostRenderCB(renderCBFcn f){postRenderCB = f;}
	static int getJpegQuality();
	static void setJpegQuality(int qual);
	static bool depthPeelEnabled() {return depthPeeling;}
	static void enableDepthPeeling(bool val) {depthPeeling = val;}
	static bool getDefaultAxisArrowsEnabled(){return defaultAxisArrowsEnabled;}
	static void setDefaultAxisArrows(bool val){defaultAxisArrowsEnabled = val;}
	static bool getDefaultTerrainEnabled(){return defaultTerrainEnabled;}
	static bool getDefaultSpinAnimateEnabled(){return defaultSpinAnimateEnabled;}
	static void setDefaultShowTerrain(bool val){defaultTerrainEnabled = val;}
	static void setDefaultSpinAnimate(bool val){defaultSpinAnimateEnabled = val;}
	static void setDefaultPrefs();
	int getWindowNum() {return winNum;}
	
	//Static methods so that the vizwinmgr can tell the glwindow about
	//current active visualizer, and about region sharing
	static int getActiveWinNum() { return activeWindowNum;}
	static void setActiveWinNum(int winnum) {activeWindowNum = winnum;}
	bool windowIsActive(){return (winNum == activeWindowNum);}
	static bool activeWinSharesRegion() {return regionShareFlag;}
	static void setRegionShareFlag(bool regionIsShared){regionShareFlag = regionIsShared;}
	
	
	const GLdouble* getProjectionMatrix() { return projectionMatrix;}	
	
	void getNearFarClippingPlanes(GLfloat *nearplane, GLfloat *farplane) {
		*nearplane = nearDist; *farplane = farDist;
	}

	
	const GLint* getViewport() {return viewport;}

	enum OGLVendorType {
		UNKNOWN = 0,
		MESA,
		NVIDIA,
		ATI,
		INTEL
	};
	static OGLVendorType GetVendor();

	void startSpin(int renderMS);
	//Terminate spin, return true if successful
	bool stopSpin();
	bool spinning(){return isSpinning;}

	void clearRendererBypass(Params::ParamsBaseType t);
	//Following is intended to prevent conflicts between concurrent
	//opengl threads.  Probably it should be protected by a semaphore 
	//For now, leave it and see if problems occur.
	static bool isRendering(){return nowPainting;}
	void setValuesFromGui(ViewpointParams* vpparams);
	static void setSpinAnimation(bool on){spinAnimate = on;}
	static bool spinAnimationEnabled(){return spinAnimate;}
	QMutex renderMutex;  //prevent recursive rendering
	bool isControlled;
	//Depth peeling data
	GLint maxbuffers;
	GLuint fboA;
	GLuint fboB;
	GLuint currentBuffer;
	GLuint currentDepth;
	GLint currentLayer;
	GLuint depthA, depthB;
	GLuint *layers;
	int depthWidth, depthHeight, depthTexUnit;
	ShaderMgr* getShaderMgr() {return manager;}
	void depthPeelPaintEvent();
	void renderScene(float extents[6], float minFull[3], float maxFull[3], int timeStep);
	void regPaintEvent();
	bool peelInitialized;
	bool isDepthPeeling(){return depthPeeling;}
	
protected:
	
	QImage glTimeStampImage;
	SpinTimer *mySpinTimer;
	ShaderMgr *manager;
	//Mouse Mode tables.  Static since independent of window:
	static vector<ParamsBase::ParamsBaseType> paramsFromMode;
	static vector<int> manipFromMode;
	static vector<string> modeName;
	static map<ParamsBase::ParamsBaseType, int> modeFromParams;
	//There's a separate manipholder for each window
	vector<TranslateStretchManip*> manipHolder;
	//Register a manip, including an icon and text
	//Icon must be specified as an xpm or null
	//Manip type is : 0 for navigation (nothing) 1 for 3D translate/stretch, 2 for 2D translate/stretch, 3 for 3D rotate/stretch

	//Container for sorting renderer list:
	class RenderListElt {
		public:
		Renderer* ren;
		float camDist;
		Params::ParamsBaseType renType;
	};
	static bool renderPriority(RenderListElt* ren1, RenderListElt* ren2){
		return (ren1->camDist > ren2->camDist);
	}
	std::vector<QImage> axisLabels[3];
	
	bool axisLabelsDirty;

	static bool depthPeeling;
	int winNum;
	int previousTimeStep;
	int previousFrameNum;
	static int jpegQuality;
	//Following flag is set whenever there is mouse navigation, so that we can use 
	//the new viewer position
	//at the next rendering
	bool newViewerCoords;
	static int currentMouseMode;
	std::map<DirtyBitType,bool> vizDirtyBit; 
	Renderer* renderer[MAXNUMRENDERERS];
	Params::ParamsBaseType renderType[MAXNUMRENDERERS];
	int renderOrder[MAXNUMRENDERERS];

	//Map params to renderer for set/get dirty bits, etc:
	
	std::map<RenderParams*,Renderer*> rendererMapping;

	int numRenderers;
	//Picking helper functions, saved from last change in GL state.  These
	//Deal with the cube coordinates (known to the trackball)
	//openGL model matrix is kept in the viewpointparams, since
	//it may be shared (global)
	//
	GLdouble* getModelMatrix();
		
	GLfloat tcoord[2];
	
	GLfloat* setTexCrd(int i, int j);
	
	//These methods cannot be overridden, but the initialize and paint methods
	//call the corresponding Renderer methods.
	//
    void		initializeGL();
  
    void		resizeGL( int w, int h );

	//Following QT 4 guidance (see bubbles example), opengl painting is performed
	//in paintEvent(), so that we can paint nice text over the window.
	// GLWindow::paintGL() is not implemented.
	// The other changes include:
	// GL_MULTISAMPLE is enabled
	// GLWindow::updateGL() is replaced by GLWindow::update()
	// resizeGL() contents have been moved to setUpViewport(), which is also called 
	// from paintEvent().
	// setAutoFillBackground(false) is called in the GLWindow constructor

	void paintEvent(QPaintEvent* event);
#ifdef	Darwin
    bool event ( QEvent * e );
#endif

		
	void setUpViewport(int width, int height);
	//Methods to support drawing domain bounds, axes etc.
	//Set colors to use in domain-bound rendering:
	void setSubregionFrameColorFlt(const QColor& c);
	void setRegionFrameColorFlt(const QColor& c);
	//Draw the region bounds and frame it in full domain.
	//Arguments are in unit cube coordinates
	void renderDomainFrame(float* extents, float* minFull, float* maxFull);
	
	void drawSubregionBounds(float* extents);
	void drawAxisArrows(float* extents);
	void drawAxisTics(int tstep);
	void drawAxisLabels(int tstep);
	void buildAxisLabels(int tstep);
	
	void placeLights();
	
	//Helper functions for drawing region bounds:
	static float* cornerPoint(float* extents, int faceNum);
	// Faces of the cube are numbered 0..5 based on view from pos z axis:
	// back, front, bottom, top, left, right
	static bool faceIsVisible(float* extents, float* viewerCoords, int faceNum);

	
	//Render all the text; build textobjects if necessary.
	void renderText();
	float regionFrameColorFlt[3];
	float subregionFrameColorFlt[3];

	float	wCenter[3]; //World center coords
	float	maxDim;		//Max of x, y, z size in world coords
	bool perspective;	//perspective vs parallel coords;
	bool oldPerspective;
	//Indicate if the current render is different from previous,
	//Used for frame capture:
	bool renderNew;
	//Moved over from vizwin:
	int capturingImage;
	int captureNumImage;
	bool capturingFlow;
	bool capturingTif;
	//Flag to set indicating start of capture sequence.
	bool newCaptureImage;
	QString captureNameImage;
	QString captureNameFlow;
	//Set the following to force a call to resizeGL at the next call to
	//updateGL.
	bool needsResize;
	float farDist, nearDist;

	GLint viewport[4];
	GLdouble projectionMatrix[16];
	static bool nowPainting;

	//values in vizFeature
	QColor colorbarBackgroundColor;

	int colorbarParamsTypeId;

	int timeAnnotType;
	int timeAnnotTextSize;
	float timeAnnotCoords[2];
	QColor timeAnnotColor;
	
	bool axisArrowsEnabled;
	bool axisAnnotationEnabled;
	int colorbarDigits;
	int colorbarFontsize;
	bool colorbarEnabled;
	float axisArrowCoord[3];
	double axisOriginCoord[3];
	double minTic[3];
	double maxTic[3];
	double ticLength[3];
	int ticDir[3];
	int numTics[3];
	int labelHeight, labelDigits;
	float ticWidth;
	QColor axisColor;
	float displacement;

	float colorbarLLCoord[2];
	float colorbarURCoord[2];
	int numColorbarTics;
	bool colorbarDirty;
	bool timeAnnotDirty;
	bool mouseDownHere;
	bool latLonAnnot;


	renderCBFcn preRenderCB;
	renderCBFcn postRenderCB;

	static int activeWindowNum;
	//This flag is true if the active window is sharing the region.
	//If the current window is not active, it will still share the region, if
	//the region is shared, and the active region is shared.
	static bool regionShareFlag;
	static bool defaultTerrainEnabled;
	static bool defaultSpinAnimateEnabled;
	static bool spinAnimate;
	static bool defaultAxisArrowsEnabled;
	
	
	int axisLabelNums[3];

	//state to save during handle slide:
	// screen coords where mouse is pressed:
	float mouseDownPoint[2];
	// unit vector in direction of handle
	float handleProjVec[2];
	bool isSpinning;
	GLuint _timeStampTexid;

private:
	bool _readyToDraw;
	void makeWriter();
	
	map<Renderer*,vector<TextObject*> > textObjectMap;
	map<Renderer*, bool> textValidFlag;
	map< pair<Renderer*, int> , vector<float*>* > textCoordMap;
	
	
#endif //DOXYGEN_SKIP_THIS
	
};

};

#endif // GLWINDOW_H
