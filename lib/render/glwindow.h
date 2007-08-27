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
#include "qcolor.h"
#include "params.h"
#include "manip.h"
#include "vapor/MyBase.h"
#include "common.h"

//No more than 10 renderers in a window:
//Eventually this may be dynamic.
#define MAXNUMRENDERERS 20
//Following factor accentuates the terrain changes by pointing the
//normals more away from the vertical
#define ELEVATION_GRID_ACCENT 20.f

namespace VAPoR {

typedef bool (*renderCBFcn)(int winnum, bool newCoords);
class ViewpointParams;
class RegionParams;
class DvrParams;
class ParamsIso;
class AnimationParams;
class FlowParams;
class ProbeParams;
class Renderer;
class TranslateStretchManip;
class TranslateRotateManip;
class FlowRenderer;
class VolumeRenderer;

class RENDER_API GLWindow : public MyBase, public QGLWidget
{

public:

    GLWindow( const QGLFormat& fmt, QWidget* parent, const char* name, int winnum);
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
	//Determine a unit direction vector associated with a pixel.  Uses OpenGL screencoords
	// I.e. y = 0 at bottom.  Returns false on failure.
	//
	bool pixelToVector(int x, int y, const float cameraPos[3], float dirVec[3]);

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
	
	void setRegionDirty(bool isDirty){ setDirtyBit(RegionBit,isDirty);}
	void setDvrRegionNavigating(bool isDirty){ setDirtyBit(DvrRegionBit,isDirty);}
	void setLightingDirty(bool isDirty) {setDirtyBit(LightingBit,isDirty);}



	//Get/set methods for vizfeatures
	QColor& getBackgroundColor() {return backgroundColor;}
	QColor& getRegionFrameColor() {return regionFrameColor;}
	QColor& getSubregionFrameColor() {return subregionFrameColor;}
	QColor& getColorbarBackgroundColor() {return colorbarBackgroundColor;}
	bool axesAreEnabled() {return axesEnabled;}
	bool colorbarIsEnabled() {return colorbarEnabled;}
	bool regionFrameIsEnabled() {return regionFrameEnabled;}
	bool subregionFrameIsEnabled() {return subregionFrameEnabled;}
	float getAxisCoord(int i){return axisCoord[i];}
	float getColorbarLLCoord(int i) {return colorbarLLCoord[i];}
	float getColorbarURCoord(int i) {return colorbarURCoord[i];}
	int getColorbarNumTics() {return numColorbarTics;}
	QColor& getElevGridColor() {return elevColor;}
	
	bool elevGridRenderingEnabled() {return renderElevGrid;}
	void setBackgroundColor(QColor& c) {backgroundColor = c;}
	void setColorbarBackgroundColor(QColor& c) {colorbarBackgroundColor = c;}
	void setRegionFrameColor(QColor& c) {regionFrameColor = c;}
	void setSubregionFrameColor(QColor& c) {subregionFrameColor = c;}
	void enableAxes(bool enable) {axesEnabled = enable;}
	void enableColorbar(bool enable) {colorbarEnabled = enable;}
	void enableRegionFrame(bool enable) {regionFrameEnabled = enable;}
	void enableSubregionFrame(bool enable) {subregionFrameEnabled = enable;}
	void setElevGridColor(QColor& c) {elevColor = c;}
	void setElevGridRefinementLevel(int lev) {elevGridRefLevel = lev;}
	void enableElevGridRendering(bool val) {renderElevGrid = val;}
	void setAxisCoord(int i, float val){axisCoord[i] = val;}
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
	Params::ParamType getRendererType(int i) {return renderType[i];}

	Renderer* getRenderer(int i) {return renderer[i];}
	Renderer* getRenderer(RenderParams* p);
	
	//Renderers can be added early or late, using a "render Order" parameter.
	//The order is between 0 and 10; lower order gets rendered first.
	//
	void prependRenderer(RenderParams* p, Renderer* ren) {insertRenderer(p, ren, 0);}
	void appendRenderer(RenderParams* p, Renderer* ren){insertRenderer(p, ren, 10);}
	void insertRenderer(RenderParams* p, Renderer* ren, int order);
	bool removeRenderer(RenderParams* p);  //Return true if successful
	//Find a renderParams in renderer list, if it exists:
	RenderParams* findARenderer(Params::ParamType renderertype);
	
	static mouseModeType getCurrentMouseMode() {return currentMouseMode;}
	static void setCurrentMouseMode(mouseModeType t){currentMouseMode = t;}
	//The glwindow keeps a copy of the params that are currently associated with the current
	//instance.  This needs to change during:
	//  -loading session
	//  -undo/redo
	//  -changing instance
	//	-reinit
	//	-new visualizer
	void setActiveParams(Params* p, Params::ParamType t);
	void setActiveViewpointParams(ViewpointParams* p) {currentViewpointParams = p;}
	void setActiveRegionParams(RegionParams* p) {
		currentRegionParams = p; 
		if(myRegionManip)myRegionManip->setParams((Params*)currentRegionParams);
	}
	void setActiveAnimationParams(AnimationParams* p) {currentAnimationParams = p;}
	void setActiveDvrParams(DvrParams* p) {currentDvrParams = p; }
	void setActiveIsoParams(ParamsIso* p) {currentIsoParams = p; }
	void setActiveFlowParams(FlowParams* p) {
		currentFlowParams = p; myFlowManip->setParams((Params*)currentFlowParams);
	}
	void setActiveProbeParams(ProbeParams* p) {
		currentProbeParams = p; myProbeManip->setParams((Params*)currentProbeParams);
	}
	
	ViewpointParams* getActiveViewpointParams() {return currentViewpointParams;}
	RegionParams* getActiveRegionParams() {return currentRegionParams;}
	AnimationParams* getActiveAnimationParams() {return currentAnimationParams;}
	DvrParams* getActiveDvrParams() {return currentDvrParams;}
	ParamsIso* getActiveIsoParams() {return currentIsoParams;}
	FlowParams* getActiveFlowParams() {return currentFlowParams;}
	ProbeParams* getActiveProbeParams() {return currentProbeParams;}

	//The GLWindow keeps track of the renderers with an ordered list of them
	//as well as with a map from renderparams to renderer
	
	void mapRenderer(RenderParams* rp, Renderer* ren){rendererMapping[rp] = ren;}
	bool unmapRenderer(RenderParams* rp);
	
	//Determine the approximate size of a pixel in terms of viewer coordinates.
	float getPixelSize();
	bool viewerCoordsChanged() {return newViewerCoords;}
	void setViewerCoordsChanged(bool isNew) {newViewerCoords = isNew;}
	bool isCapturing() {return (capturing != 0);}
	bool isSingleCapturing() {return (capturing == 1);}
	void startCapture(QString& name, int startNum) {
		capturing = 2;
		captureNum = startNum;
		captureName = name;
		newCapture = true;
		updateGL();
	}
	void singleCapture(QString& name){
		capturing = 1;
		captureName = name;
		newCapture = true;
		updateGL();
	}
	bool captureIsNew() { return newCapture;}
	void setCaptureNew(bool isNew){ newCapture = isNew;}
	void stopCapture() {capturing = 0;}
	//Routine is called at the end of rendering.  If capture is 1 or 2, it converts image
	//to jpeg and saves file.  If it ever encounters an error, it turns off capture.
	//If capture is 1 (single frame capture) it turns off capture.
	void doFrameCapture();

	TranslateRotateManip* getProbeManip() {return myProbeManip;}
	TranslateStretchManip* getFlowManip() {return myFlowManip;}
	TranslateStretchManip* getRegionManip() {return myRegionManip;}

	void setPreRenderCB(renderCBFcn f){preRenderCB = f;}
	void setPostRenderCB(renderCBFcn f){postRenderCB = f;}
	static int getJpegQuality();
	static void setJpegQuality(int qual);
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
	bool projMatrixIsDirty() {return 1;}
	void getNearFarClippingPlanes(GLfloat *nearplane, GLfloat *farplane) {
		*nearplane = nearDist; *farplane = farDist;
	}

	bool viewportIsDirty() {return 1;}
	const GLint* getViewport() {return viewport;}

protected:
	int winNum;
	static int jpegQuality;
	//Following flag is set whenever there is mouse navigation, so that we can use 
	//the new viewer position
	//at the next rendering
	bool newViewerCoords;
	static mouseModeType currentMouseMode;
	std::map<DirtyBitType,bool> vizDirtyBit; 
	Renderer* renderer[MAXNUMRENDERERS];
	Params::ParamType renderType[MAXNUMRENDERERS];
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
		

	 
	//These methods cannot be overridden, but the initialize and paint methods
	//call the corresponding Renderer methods.
	//
    void		initializeGL();
    void		paintGL();
    void		resizeGL( int w, int h );

	//Methods to support drawing domain bounds, axes etc.
	//Set colors to use in domain-bound rendering:
	void setSubregionFrameColorFlt(QColor& c);
	void setRegionFrameColorFlt(QColor& c);
	//Draw the region bounds and frame it in full domain.
	//Arguments are in unit cube coordinates
	void renderDomainFrame(float* extents, float* minFull, float* maxFull);
	void renderRegionBounds(float* extents, int selectedFace, float faceDisplacement);
	
	void drawSubregionBounds(float* extents);
	void drawAxes(float* extents);
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


	float regionFrameColorFlt[3];
	float subregionFrameColorFlt[3];

	//Cached elevation grid (one for each time step)
	int maxXElev, maxYElev;
	float** elevVert, **elevNorm;
	int numElevTimesteps;
	//Additional parameters used for elev grid:
	QColor elevColor;
	int elevGridRefLevel;
	bool renderElevGrid;

	float	wCenter[3]; //World center coords
	float	maxDim;		//Max of x, y, z size in world coords
	bool perspective;	//perspective vs parallel coords;
	bool oldPerspective;
	//Indicate if the current render is different from previous,
	//Used for frame capture:
	bool renderNew;
	//Moved over from vizwin:
	int capturing;
	int captureNum;
	//Flag to set indicating start of capture sequence.
	bool newCapture;
	QString captureName;
	//Set the following to force a call to resizeGL at the next call to
	//updateGL.
	bool needsResize;
	float farDist, nearDist;

	GLint viewport[4];
	GLdouble projectionMatrix[16];
	bool nowPainting;

	//Manip stuff:
	TranslateRotateManip* myProbeManip;
	TranslateStretchManip* myFlowManip;
	TranslateStretchManip* myRegionManip;

	//values in vizFeature
	QColor backgroundColor;
	QColor regionFrameColor;
	QColor subregionFrameColor;
	QColor colorbarBackgroundColor;
	
	bool axesEnabled;
	bool regionFrameEnabled;
	bool subregionFrameEnabled;
	bool colorbarEnabled;
	float axisCoord[3];
	float colorbarLLCoord[2];
	float colorbarURCoord[2];
	int numColorbarTics;
	bool colorbarDirty;
	bool mouseDownHere;

	ViewpointParams* currentViewpointParams;
	RegionParams* currentRegionParams;
	AnimationParams* currentAnimationParams;
	FlowParams* currentFlowParams;
	DvrParams* currentDvrParams;
	ParamsIso* currentIsoParams;
	ProbeParams* currentProbeParams;

	renderCBFcn preRenderCB;
	renderCBFcn postRenderCB;

	static int activeWindowNum;
	//This flag is true if the active window is sharing the region.
	//If the current window is not active, it will still share the region, if
	//the region is shared, and the active region is shared.
	static bool regionShareFlag;

	
};
};

#endif // GLWINDOW_H
