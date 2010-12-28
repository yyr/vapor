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
#include <qgl.h>
#include <map>
#include "trackball.h"
#include <qthread.h>
#include <QMutex>
#include "qcolor.h"
#include "qlabel.h"
#include "params.h"
#include "manip.h"
#include <vapor/MyBase.h>
#include <vapor/common.h>
#include <vaporinternal/jpegapi.h>
#include "datastatus.h"

//No more than 20 renderers in a window:
//Eventually this may be dynamic.
#define MAXNUMRENDERERS 20
//Following factor accentuates the terrain changes by pointing the
//normals more away from the vertical
#define ELEVATION_GRID_ACCENT 20.f
class QLabel;
class QThread;

namespace VAPoR {
typedef bool (*renderCBFcn)(int winnum, bool newCoords);
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
class SpinThread;


class RENDER_API GLWindow : public MyBase, public QGLWidget
{
public:
	typedef void (*ErrMsgReleaseCB_T)(void);
    GLWindow( QGLFormat& fmt, QWidget* parent, int winnum);
    ~GLWindow();
	Trackball* myTBall;
	
	Trackball* getTBall() {return myTBall;}
	void setPerspective(bool isPersp) {perspective = isPersp;}
	bool getPerspective() {return perspective;}

	//Enum describes various mouse modes:
	enum mouseModeType {
		unknownMode,
		navigateMode,
		regionMode,
		probeMode,
		twoDDataMode,
		twoDImageMode,
		rakeMode,
		lightMode
	};

	//Reset the GL perspective, so that the near and far clipping planes are wide enough
	//to view twice the entire region from the current camera position.  If the camera is
	//inside the doubled region, region, make the near clipping plane 1% of the region size.

	void resetView(RegionParams* rparams, ViewpointParams* vpParams);
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
	bool projectPointToWin(float cubeCoords[3], float winCoords[2]);

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

	void setDirtyBit(DirtyBitType t, bool val);
	bool vizIsDirty(DirtyBitType t);
	bool regionIsDirty() {return vizIsDirty(RegionBit);}
	bool dvrRegionIsNavigating(){return vizIsDirty(DvrRegionBit);}
	
	bool lightingIsDirty() {return vizIsDirty(LightingBit);}
	bool animationIsDirty() {return vizIsDirty(AnimationBit);}
	bool projMatrixIsDirty() {return vizIsDirty(ProjMatrixBit);}
	bool viewportIsDirty() {return vizIsDirty(ViewportBit);}
	
	void setRegionDirty(bool isDirty){ setDirtyBit(RegionBit,isDirty);}
	void setDvrRegionNavigating(bool isDirty){ setDirtyBit(DvrRegionBit,isDirty);}
	void setLightingDirty(bool isDirty) {setDirtyBit(LightingBit,isDirty);}



	//Get/set methods for vizfeatures
	QColor getBackgroundColor() {return DataStatus::getInstance()->getBackgroundColor();}
	QColor getRegionFrameColor() {return DataStatus::getInstance()->getRegionFrameColor();}
	QColor getSubregionFrameColor() {return DataStatus::getInstance()->getSubregionFrameColor();}
	QColor& getColorbarBackgroundColor() {return colorbarBackgroundColor;}
	bool axisArrowsAreEnabled() {return axisArrowsEnabled;}
	bool axisAnnotationIsEnabled() {return axisAnnotationEnabled;}
	bool colorbarIsEnabled() {return colorbarEnabled;}
	bool regionFrameIsEnabled() {return DataStatus::getInstance()->regionFrameIsEnabled();}
	bool subregionFrameIsEnabled() {return DataStatus::getInstance()->subregionFrameIsEnabled();}
	float getAxisArrowCoord(int i){return axisArrowCoord[i];}
	float getAxisOriginCoord(int i){return axisOriginCoord[i];}
	float getMinTic(int i){return minTic[i];}
	float getMaxTic(int i){return maxTic[i];}
	float getTicLength(int i){return ticLength[i];}
	int getNumTics(int i){return numTics[i];}
	int getTicDir(int i){return ticDir[i];}
	int getLabelHeight() {return labelHeight;}
	int getLabelDigits() {return labelDigits;}
	float getTicWidth(){return ticWidth;}
	QColor& getAxisColor() {return axisColor;}
	float getColorbarLLCoord(int i) {return colorbarLLCoord[i];}
	float getColorbarURCoord(int i) {return colorbarURCoord[i];}
	int getColorbarNumTics() {return numColorbarTics;}
	QColor& getElevGridColor() {return elevColor;}
	

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

	
	bool elevGridRenderingEnabled() {return renderElevGrid;}
	void setBackgroundColor(QColor& c) {DataStatus::getInstance()->setBackgroundColor(c);}
	void setColorbarBackgroundColor(QColor& c) {colorbarBackgroundColor = c;}
	void setRegionFrameColor(QColor& c) {DataStatus::getInstance()->setRegionFrameColor(c);}
	void setSubregionFrameColor(QColor& c) {DataStatus::getInstance()->setSubregionFrameColor(c);}
	void enableAxisArrows(bool enable) {axisArrowsEnabled = enable;}
	void enableAxisAnnotation(bool enable) {axisAnnotationEnabled = enable;}
	void enableColorbar(bool enable) {colorbarEnabled = enable;}
	void enableRegionFrame(bool enable) {DataStatus::getInstance()->enableRegionFrame(enable);}
	void enableSubregionFrame(bool enable) {DataStatus::getInstance()->enableSubregionFrame(enable);}
	void setElevGridColor(QColor& c) {elevColor = c;}
	void setElevGridRefinementLevel(int lev) {elevGridRefLevel = lev;}
	void enableElevGridRendering(bool val) {renderElevGrid = val;}
	void setAxisArrowCoord(int i, float val){axisArrowCoord[i] = val;}
	void setAxisOriginCoord(int i, float val){axisOriginCoord[i] = val;}
	void setNumTics(int i, int val) {numTics[i] = val;}
	void setTicDir(int i, int val) {ticDir[i] = val;}
	void setMinTic(int i, float val) {minTic[i] = val;}
	void setMaxTic(int i, float val) {maxTic[i] = val;}
	void setTicLength(int i, float val) {ticLength[i] = val;}
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
	void setColorbarDirty(bool val){colorbarDirty = val;}

	int getElevGridRefinementLevel() {return elevGridRefLevel;}

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
	
	static mouseModeType getCurrentMouseMode() {return currentMouseMode;}
	static void setCurrentMouseMode(mouseModeType t){currentMouseMode = t;}
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
		if(myRegionManip)myRegionManip->setParams(p);
	}
	void setActiveAnimationParams(Params* p) {setActiveParams(p,Params::_animationParamsTag);}
	void setActiveDvrParams(Params* p) {setActiveParams(p,Params::_dvrParamsTag); }
	void setActiveIsoParams(Params* p) {setActiveParams(p,Params::_isoParamsTag); }
	void setActiveFlowParams(Params* p) {
		setActiveParams(p,Params::_flowParamsTag); 
		myFlowManip->setParams(p);
	}
	void setActiveProbeParams(Params* p) {
		setActiveParams(p,Params::_probeParamsTag);
		myProbeManip->setParams(p);
	}
	void setActiveTwoDDataParams(Params* p) {
		setActiveParams(p,Params::_twoDDataParamsTag); 
		myTwoDDataManip->setParams(p);
	}
	void setActiveTwoDImageParams(Params* p) {
		setActiveParams(p,Params::_twoDImageParamsTag); 
		myTwoDImageManip->setParams(p);
	}
	ViewpointParams* getActiveViewpointParams() {return (ViewpointParams*)getActiveParams(Params::_viewpointParamsTag);}
	RegionParams* getActiveRegionParams() {return (RegionParams*)getActiveParams(Params::_regionParamsTag);}
	AnimationParams* getActiveAnimationParams() {return (AnimationParams*)getActiveParams(Params::_animationParamsTag);}
	DvrParams* getActiveDvrParams() {return (DvrParams*)getActiveParams(Params::_dvrParamsTag);}
	
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
	void startImageCapture(QString& name, int startNum) {
		capturingImage = 2;
		captureNumImage = startNum;
		captureNameImage = name;
		newCaptureImage = true;
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
	

	TranslateRotateManip* getProbeManip() {return myProbeManip;}
	TranslateStretchManip* getTwoDDataManip() {return myTwoDDataManip;}
	TranslateStretchManip* getTwoDImageManip() {return myTwoDImageManip;}
	TranslateStretchManip* getFlowManip() {return myFlowManip;}
	TranslateStretchManip* getRegionManip() {return myRegionManip;}

	void setPreRenderCB(renderCBFcn f){preRenderCB = f;}
	void setPostRenderCB(renderCBFcn f){postRenderCB = f;}
	static int getJpegQuality();
	static void setJpegQuality(int qual);
	static bool getDefaultAxisArrowsEnabled(){return defaultAxisArrowsEnabled;}
	static void setDefaultAxisArrows(bool val){defaultAxisArrowsEnabled = val;}
	static bool getDefaultTerrainEnabled(){return defaultTerrainEnabled;}
	static void setDefaultShowTerrain(bool val){defaultTerrainEnabled = val;}
	static void setDefaultPrefs();
	int getWindowNum() {return winNum;}
	
	//Static methods so that the vizwinmgr can tell the glwindow about
	//current active visualizer, and about region sharing
	static int getActiveWinNum() { return activeWindowNum;}
	static void setActiveWinNum(int winnum) {activeWindowNum = winnum;}
	bool windowIsActive(){return (winNum == activeWindowNum);}
	static bool activeWinSharesRegion() {return regionShareFlag;}
	static void setRegionShareFlag(bool regionIsShared){regionShareFlag = regionIsShared;}
	void invalidateElevGrid();
	
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
	
protected:
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
	QMutex renderMutex;  //prevent recursive rendering
	int winNum;
	int previousTimeStep;
	static int jpegQuality;
	//Following flag is set whenever there is mouse navigation, so that we can use 
	//the new viewer position
	//at the next rendering
	bool newViewerCoords;
	static mouseModeType currentMouseMode;
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
	GLuint _elevTexid;
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
		
	void setUpViewport(int width, int height);
	//Methods to support drawing domain bounds, axes etc.
	//Set colors to use in domain-bound rendering:
	void setSubregionFrameColorFlt(const QColor& c);
	void setRegionFrameColorFlt(const QColor& c);
	//Draw the region bounds and frame it in full domain.
	//Arguments are in unit cube coordinates
	void renderDomainFrame(float* extents, float* minFull, float* maxFull);
	void renderRegionBounds(float* extents, int selectedFace, float faceDisplacement);
	
	void drawSubregionBounds(float* extents);
	void drawAxisArrows(float* extents);
	void drawAxisTics();
	void drawAxisLabels();
	void drawTimeAnnotation();
	
	void drawElevationGrid(size_t timestep);
	void placeLights();
	bool rebuildElevationGrid(size_t timestep);
	void calcElevGridNormals(size_t timestep);
	//Helper functions for drawing region bounds:
	static float* cornerPoint(float* extents, int faceNum);
	// Faces of the cube are numbered 0..5 based on view from pos z axis:
	// back, front, bottom, top, left, right
	static bool faceIsVisible(float* extents, float* viewerCoords, int faceNum);
	void drawRegionFace(float* extents, int faceNum, bool isSelected);
	void drawRegionFaceLines(float* extents, int selectedFace);
	void drawProbeFace(float* corners, int faceNum, bool isSelected);
	void drawTwoDFace(float* corners, int faceNum, bool isSelected);


	float regionFrameColorFlt[3];
	float subregionFrameColorFlt[3];

	//Cached elevation grid sizes (one for each time step)
	int *maxXElev, *maxYElev;
	float** elevVert, **elevNorm;
	//constants used in fitting texture to domain:
	float *xfactr, *xbeg, *yfactr, *ybeg;
	//current values used in tex coord mapping:
	float xfct, yfct, xbg, ybg;
	int mxx, mxy;

	int numElevTimesteps;
	//Additional parameters used for elev grid:
	QColor elevColor;
	int elevGridRefLevel;
	bool renderElevGrid;
	
	int elevGridWidth, elevGridHeight;

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

	//Manip stuff:
	TranslateRotateManip* myProbeManip;
	TranslateStretchManip* myTwoDDataManip;
	TranslateStretchManip* myTwoDImageManip;
	TranslateStretchManip* myFlowManip;
	TranslateStretchManip* myRegionManip;

	//values in vizFeature
	QColor colorbarBackgroundColor;

	int timeAnnotType;
	int timeAnnotTextSize;
	float timeAnnotCoords[2];
	QColor timeAnnotColor;
	
	bool axisArrowsEnabled;
	bool axisAnnotationEnabled;
	
	bool colorbarEnabled;
	float axisArrowCoord[3];
	float axisOriginCoord[3];
	float minTic[3];
	float maxTic[3];
	float ticLength[3];
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
	bool mouseDownHere;



	renderCBFcn preRenderCB;
	renderCBFcn postRenderCB;

	static int activeWindowNum;
	//This flag is true if the active window is sharing the region.
	//If the current window is not active, it will still share the region, if
	//the region is shared, and the active region is shared.
	static bool regionShareFlag;
	static bool defaultTerrainEnabled;
	static bool defaultAxisArrowsEnabled;

	int axisLabelNums[3];

	//state to save during handle slide:
	// screen coords where mouse is pressed:
	float mouseDownPoint[2];
	// unit vector in direction of handle
	float handleProjVec[2];

	SpinThread* spinThread;
	bool isSpinning;
	QTime* spinTimer;

	
};
class SpinThread : public QThread{
public:
	SpinThread(GLWindow* w, int renderMS) : QThread() {
		spinningWindow = w;
		renderTime = renderMS;
	}
	void run();
	void setWindow(GLWindow* w) { spinningWindow = w;}
	void finish() { wait(4*renderTime);}
private:
	GLWindow* spinningWindow;
	int renderTime;
};

};

#endif // GLWINDOW_H
