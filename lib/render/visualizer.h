//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		Visualizer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2013
//
//	Description:  Definition of Visualizer class: 
//	A Visualizer object is associated with each visualization window.  It contains
//  information required for rendering, performs
//	navigation and resize, and defers drawing to the viz window's list of 
//	registered renderers.

#ifndef Visualizer_H
#define Visualizer_H

#include <map>

#include <jpeglib.h>

#include "params.h"

#include <vapor/MyBase.h>
#include <vapor/common.h>
#include "datastatus.h"


namespace VAPoR {
typedef bool (*renderCBFcn)(int winnum, bool newCoords);

class ViewpointParams;
class RegionParams;
class AnimationParams;
class Renderer;



//! \class Visualizer
//! \brief A class for performing OpenGL rendering in  VAPOR Window
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!
//! 
//! The Visualizer class is an OpenGL rendering window.  There is one instance for 
//! each of the VAPOR visualizers.  The Visualizer is embedded in a VizWin instance.
//! It performs basic setup for OpenGL rendering, renders incidental geometry,
//! and calls all the enabled VAPOR Renderer instances.

class RENDER_API Visualizer : public MyBase
{
public:
   
	
	//! Method that returns the ViewpointParams that is active in this window.
	//! \retval ViewpointParams* current active ViewpointParams
	ViewpointParams* getActiveViewpointParams() {
		return (ViewpointParams*)Params::GetCurrentParamsInstance(Params::GetTypeFromTag(Params::_viewpointParamsTag),winNum);
	}

	//! Method that returns the RegionParams that is active in this window.
	//! \retval RegionParams* current active RegionParams
	RegionParams* getActiveRegionParams() {
		return (RegionParams*)Params::GetCurrentParamsInstance(Params::GetTypeFromTag(Params::_regionParamsTag),winNum);
	}

	//! Method that returns the AnimationParams that is active in this window.
	//! \retval AnimationParams* current active AnimationParams
	AnimationParams* getActiveAnimationParams() {
		return (AnimationParams*)Params::GetCurrentParamsInstance(Params::GetTypeFromTag(Params::_animationParamsTag),winNum);
	}


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

	//! Method to initialize GL rendering.  Must  be called from a GL context.
	void	initializeGL();
	//Following QT 4 guidance (see bubbles example), opengl painting is performed
	//in paintEvent(), so that we can paint nice text over the window.
	// Visualizer::paintGL() is not implemented.
	// The other changes include:
	// GL_MULTISAMPLE is enabled
	// Visualizer::updateGL() is replaced by Visualizer::update()
	// resizeGL() contents have been moved to setUpViewport(), which is also called 
	// from paintEvent().
	// setAutoFillBackground(false) is called in the Visualizer constructor

	void paintEvent(bool force);

	void resizeGL( int w, int h );

#ifndef DOXYGEN_SKIP_THIS
	 Visualizer( int winnum);
    ~Visualizer();
	

	//Enum describes various mouse modes:
	enum mouseModeType {
		unknownMode=1000,
		navigateMode=0,
		regionMode=1,
		probeMode=3,
		twoDDataMode=4,
		twoDImageMode=5,
		rakeMode=2,
		barbMode=6
	};

	//Reset the GL perspective, so that the near and far clipping planes are wide enough
	//to view twice the entire region from the current camera position.  If the camera is
	//inside the doubled region, region, make the near clipping plane 1% of the region size.

	void resetView(ViewpointParams* vpParams);
	
	//Test if the screen projection of a 3D quad encloses a point on the screen.
	//The 4 corners of the quad must be specified in counter-clockwise order
	//as viewed from the outside (pickable side) of the quad.  Should be moved to Manip.cpp
	//
	bool pointIsOnQuad(float cor1[3],float cor2[3],float cor3[3],float cor4[3], float pickPt[2]);

	//Determine if a point is over (and not inside) a box, specified by 8 3-D coords.
	//the first four corners are the counter-clockwise (from outside) vertices of one face,
	//the last four are the corresponding back vertices, clockwise from outside
	//Returns index of face (0..5), or -1 if not on Box
	//This is used by Manips, should be moved there.
	//
	int pointIsOnBox(float corners[8][3], float pickPt[2]);

	//Project a 3D point (in cube coord system) to window coords.
	//Return true if in front of camera.  Used by pointIsOnQuad, as well as in building Axis labels.
	//
	bool projectPointToWin(float cubeCoords[3], float winCoords[2]);

	// Project the current mouse coordinates to a line in screen space.
	// The line starts at the mouseDownPosition, and points in the
	// direction resulting from projecting to the screen the axis 
	// associated with the dragHandle.  Returns false on error.
	// Invoked during mouseMoveEvent, uses values of mouseDownPoint, handleProjVec, mouseDownHere

	bool projectPointToLine(float mouseCoords[2], float projCoords[2]);

	//Params argument is the params that owns the manip
	bool startHandleSlide(float mouseCoords[2], int handleNum, Params* p);
	
	//Determine a unit direction vector associated with a pixel.  Uses OpenGL screencoords
	// I.e. y = 0 at bottom.  Returns false on failure.  Used during mouseMoveEvent.
	//
	bool pixelToVector(float winCoords[2], const float cameraPos[3], float dirVec[3]);

	//Get the current image in the front buffer;
	bool getPixelData(unsigned char* data);

	void draw3DCursor(const float position[3]);

	//Following needed for manip rendering:
	bool mouseIsDown() {return mouseDownHere;}
	void setMouseDown(bool downUp) {mouseDownHere = downUp;}

	Renderer* getRenderer(int i) {return renderer[i];}
	int getNumRenderers(){return renderer.size();}
	Renderer* getRenderer(RenderParams* p);
	
	//Renderers can be added early or late, using a "render Order" parameter.
	//The order is between 0 and 10; lower order gets rendered first.
	//Sorted renderers get sorted before each render
	//
	int insertSortedRenderer(RenderParams* p, Renderer* ren){return insertRenderer(p, ren, 5);}
	int prependRenderer(RenderParams* p, Renderer* ren) {return insertRenderer(p, ren, 0);}
	int appendRenderer(RenderParams* p, Renderer* ren){return insertRenderer(p, ren, 10);}
	int insertRenderer(RenderParams* p, Renderer* ren, int order);
	bool removeRenderer(RenderParams* p);  //Return true if successful
	void removeAllRenderers();
	void sortRenderers(int timestep);  //Sort all the pri 0 renderers
	
	void removeDisabledRenderers();  //Remove renderers whose params are disabled

	//Find a renderParams in renderer list, if it exists.   Used to determine
	//that only one DVR is active.
	RenderParams* findARenderer(Params::ParamsBaseType renderertype);
	
	static int getCurrentMouseMode() {return currentMouseMode;}
	static void setCurrentMouseMode(int t){currentMouseMode = t;}
	//The Visualizer keeps a copy of the params that are currently associated with the current
	//instance.  This needs to change during:
	//  -loading session
	//  -undo/redo
	//  -changing instance
	//	-reinit
	//	-new visualizer
	

	
	//The Visualizer keeps track of the renderers with an ordered list of them
	//as well as with a map from renderparams to renderer
	
	void mapRenderer(RenderParams* rp, Renderer* ren){rendererMapping[rp] = ren;}
	bool unmapRenderer(RenderParams* rp);
	
	//Determine the approximate size of a pixel in terms of viewer coordinates.
	float getPixelSize();
	

	//Routine is called at the end of rendering.  If capture is 1 or 2, it converts image
	//to jpeg and saves file.  If it ever encounters an error, it turns off capture.
	//If capture is 1 (single frame capture) it turns off capture.
	void doFrameCapture();
	
	/*TranslateStretchManip* getManip(const std::string& paramTag){
		int mode = getModeFromParams(ParamsBase::GetTypeFromTag(paramTag));
		return manipHolder[mode];
	}*/
	

	void setPreRenderCB(renderCBFcn f){preRenderCB = f;}
	void setPostRenderCB(renderCBFcn f){postRenderCB = f;}
	
	static bool getDefaultAxisArrowsEnabled(){return defaultAxisArrowsEnabled;}
	static void setDefaultAxisArrows(bool val){defaultAxisArrowsEnabled = val;}
	static bool getDefaultTerrainEnabled(){return defaultTerrainEnabled;}
	
	static void setDefaultShowTerrain(bool val){defaultTerrainEnabled = val;}
	
	static void setDefaultPrefs();
	int getWindowNum() {return winNum;}
	
	//Static methods so that the vizwinmgr can tell the Visualizer about
	//current active visualizer, and about region sharing
	static int getActiveWinNum() { return activeWindowNum;}
	static void setActiveWinNum(int winnum) {activeWindowNum = winnum;}
	bool windowIsActive(){return (winNum == activeWindowNum);}
	static bool activeWinSharesRegion() {return regionShareFlag;}
	static void setRegionShareFlag(bool regionIsShared){regionShareFlag = regionIsShared;}
	const double* getProjectionMatrix() { return projectionMatrix;}	
	void getNearFarClippingPlanes(float *nearplane, float *farplane) {
		*nearplane = nearDist; *farplane = farDist;
	}

	const int* getViewport() {return viewport;}

	enum OGLVendorType {
		UNKNOWN = 0,
		MESA,
		NVIDIA,
		ATI,
		INTEL
	};
	static OGLVendorType GetVendor();
	void clearRendererBypass(Params::ParamsBaseType t);
	
	void setValuesFromGui(ViewpointParams* vpparams);
	
	bool isControlled;
	
	void renderScene(float extents[6], float minFull[3], float maxFull[3], int timeStep);
	void regPaintEvent();
	
	
	double* getModelViewMatrix();
	void setModelViewMatrix(const double mtx[16]);
		
	
protected:
	
	
	//Mouse Mode tables.  Static since independent of window:
	static vector<ParamsBase::ParamsBaseType> paramsFromMode;
	static vector<int> manipFromMode;
	static vector<string> modeName;
	static map<ParamsBase::ParamsBaseType, int> modeFromParams;
	//There's a separate manipholder for each window
	//vector<TranslateStretchManip*> manipHolder;
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
	
	int winNum;
	int width, height;
	int previousTimeStep;
	int previousFrameNum;
	static int jpegQuality;
	
	static int currentMouseMode;
	
	vector<Renderer*> renderer;
	vector<Params::ParamsBaseType> renderType;
	vector<int>renderOrder;

	//Map params to renderer
	
	std::map<RenderParams*,Renderer*> rendererMapping;

	float tcoord[2];
	
	float* setTexCrd(int i, int j);
    
	void setMatrixFromFrame(double* posvec, double* dirvec, double* upvec, double* centerRot, double mvmtx[16]);
	void setUpViewport(int width, int height);
	//Methods to support drawing domain bounds, axes etc.
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

	float regionFrameColorFlt[3];
	float subregionFrameColorFlt[3];
	
	
	//Set the following to force a call to resizeGL at the next call to
	//updateGL.
	bool needsResize;
	float farDist, nearDist;

	int viewport[4];
	double projectionMatrix[16];
	static bool nowPainting;
	
	float displacement;

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

	
#endif //DOXYGEN_SKIP_THIS
	
};

};

#endif // Visualizer_H
