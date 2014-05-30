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
#include "mousemodeparams.h"

namespace VAPoR {
typedef bool (*renderCBFcn)(int winnum, bool newCoords);

class ViewpointParams;
class RegionParams;
class AnimationParams;
class Renderer;
class Trackball;
class TranslateStretchManip;

//! \class Visualizer
//! \ingroup Public
//! \brief A class for performing OpenGL rendering in VAPOR GUI Window
//! \author Alan Norton
//! \version 3.0
//! \date    October 2013
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
	ViewpointParams* getActiveViewpointParams();
	//! Method that returns the RegionParams that is active in this window.
	//! \retval RegionParams* current active RegionParams
	RegionParams* getActiveRegionParams(); 

	//! Method that returns the AnimationParams that is active in this window.
	//! \retval AnimationParams* current active AnimationParams
	AnimationParams* getActiveAnimationParams();
	
	//! Method to initialize GL rendering.  Must  be called from a GL context.
	void	initializeGL();

	//! Obtain the current trackball (either shared or local) based on current viewpoint params
	Trackball* GetTrackball();

	//! Set/clear a flag indicating that the trackball has changed the viewpoint.
	//! This implies that the new viewpoint needs to be saved at render time.
	//! \param[in] bool flag indicating whether the values have changed
	void SetTrackballCoordsChanged(bool val){
		tBallChanged = val;
	}
	//! Set/clear a flag indicating that the viewpoint has changed since last render
	//! This implies that the trackball needs to use the new viewpoint.
	//! \param[in] bool flag indicating whether the viewpoint has changed.
	void SetViewpointChanged(bool val){
		vpChanged = val;
	}
	//! Static method to set flags when a shared viewpoint has changed.
	//! Results in all the visualizers that share the viewpoint getting their 
	//! vpChanged flag being set.
	
	static void SetSharedViewpointChanged();

	//Following QT 4 guidance (see bubbles example), opengl painting is performed
	//in paintEvent(), so that we can paint nice text over the window.
	// Visualizer::paintGL() is not implemented.
	// The other changes include:
	// GL_MULTISAMPLE is enabled
	// Visualizer::updateGL() is replaced by Visualizer::update()
	// resizeGL() contents have been moved to setUpViewport(), which is also called 
	// from paintEvent().
	// setAutoFillBackground(false) is called in the Visualizer constructor

	int paintEvent(bool force);

	void resizeGL( int w, int h );

	//! Identify the visualizer index associated with this visualizer
	//! \retval visualizer index;
	int getWindowNum() {return winNum;}

#ifndef DOXYGEN_SKIP_THIS
	 Visualizer( int winnum);
    ~Visualizer();
	

	//Reset the GL perspective, so that the near and far clipping planes are wide enough
	//to view twice the entire region from the current camera position.  If the camera is
	//inside the doubled region, region, make the near clipping plane 1% of the region size.

	void resetView(ViewpointParams* vpParams);

	//! Project a 3D point (in cube coord system) to window coords.
	//! Return true if in front of camera.  Used by pointIsOnQuad, as well as in building Axis labels.
	//
	bool projectPointToWin(double cubeCoords[3], float winCoords[2]);

	//! Project the current mouse coordinates to a line in screen space.
	//! The line starts at the mouseDownPosition, and points in the
	//! direction resulting from projecting to the screen the axis 
	//! associated with the dragHandle.  Returns false on error.
	//! Invoked during mouseMoveEvent, uses values of mouseDownPoint, handleProjVec, mouseDownHere

	bool projectPointToLine(float mouseCoords[2], float projCoords[2]);
	//Test if the screen projection of a 3D quad encloses a point on the screen.
	//The 4 corners of the quad must be specified in counter-clockwise order
	//as viewed from the outside (pickable side) of the quad.  
	//
	bool pointIsOnQuad(double cor1[3],double cor2[3],double cor3[3],double cor4[3], float pickPt[2]);

	//Determine if a point is over (and not inside) a box, specified by 8 3-D coords.
	//the first four corners are the counter-clockwise (from outside) vertices of one face,
	//the last four are the corresponding back vertices, clockwise from outside
	//Returns index of face (0..5), or -1 if not on Box
	//
	int pointIsOnBox(double corners[8][3], float pickPt[2]);


	//Params argument is the params that owns the manip
	bool startHandleSlide(float mouseCoords[2], int handleNum, Params* p);
	
	//Determine a unit direction vector associated with a pixel.  Uses OpenGL screencoords
	// I.e. y = 0 at bottom.  Returns false on failure.  Used during mouseMoveEvent.
	//
	bool pixelToVector(float winCoords[2], const vector<double> cameraPos, double dirVec[3]);

	//Get the current image in the front buffer;
	bool getPixelData(unsigned char* data);

	void draw3DCursor(const double position[3]);

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
	
	TranslateStretchManip* getManip(const std::string& paramTag){
		int mode = MouseModeParams::getModeFromParams(ParamsBase::GetTypeFromTag(paramTag));
		return manipHolder[mode];
	}
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
	double getPixelSize();
	
	//Routine is called at the end of rendering.  If capture is 1 or 2, it converts image
	//to jpeg and saves file.  If it ever encounters an error, it turns off capture.
	//If capture is 1 (single frame capture) it turns off capture.
	void doFrameCapture();

	void setPreRenderCB(renderCBFcn f){preRenderCB = f;}
	void setPostRenderCB(renderCBFcn f){postRenderCB = f;}
	
	static bool getDefaultAxisArrowsEnabled(){return defaultAxisArrowsEnabled;}
	static void setDefaultAxisArrows(bool val){defaultAxisArrowsEnabled = val;}
	static bool getDefaultTerrainEnabled(){return defaultTerrainEnabled;}
	
	static void setDefaultShowTerrain(bool val){defaultTerrainEnabled = val;}
	

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
	
	bool isControlled;
	
	
	double* getModelViewMatrix();
	void setModelViewMatrix(const double mtx[16]);
		
	
protected:
	
	static Trackball* globalTrackball;
	Trackball* localTrackball;
	bool tBallChanged;
	//Save the current GL modelview matrix in the viewpoint params
	void saveGLMatrix(int timestep, ViewpointParams*);

	
	
	//There's a separate manipholder for each window
	vector<TranslateStretchManip*> manipHolder;

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

	void setUpViewport(int width, int height);
	//Methods to support drawing domain bounds, axes etc.
	//Draw the region bounds and frame it in full domain.
	//Arguments are in unit cube coordinates
	void renderDomainFrame(const double* extents,const double* minFull,const double* maxFull);
	
	void placeLights();
	
	float regionFrameColorFlt[3];
	
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

	//flag indicating change to viewpoint
	bool vpChanged;

	
#endif //DOXYGEN_SKIP_THIS
	
};

};

#endif // Visualizer_H
