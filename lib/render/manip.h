//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		manip.h 
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Defines the pure virtual Manip class
//		Subclasses of this class provide in-scene manipulators
//		for positioning and setting properties of objects
//		in the scene.
#ifndef MANIP_H
#define MANIP_H
//Handle diameter in pixels:
#define HANDLE_DIAMETER 15
#include "common.h"
namespace VAPoR {

class GLWindow;
class Params;
class RENDER_API Manip {

public:
	Manip(GLWindow* win) {myGLWin = win;}
	virtual ~Manip(){}
	virtual void render()= 0;
	//The manip gets its dimensions from the Params.
	//This is useful for DvrParams, FlowParams, ProbeParams.
	//Expected to support translation, resizing, and rotation
	//Each manip has several geometric objects (e.g. cube faces).
	// The manips are used to handle mouse events in the VizWin.
	//When the mouse is clicked, we check to see if it if clicked over a manip handle.
	//If so:  The world coord of the position is saved
	//			The handle num is remembered (highlighted in future rendering)
	//When the mouse if moved, and if the manip is dragging, then the new
	//			mouse position is converted to a displacement.  The displacement is
	//			applied to the box geometry in the params.
	//When the mouse is released, the displacement is again applied to the box in the params.
	//			Then the params is notified to clean up (for undo, redo) and to make the
	//			data dirty.  The highlighting is turned off, other flags are reset.
	Params* getParams() {return myParams;}
	void setParams(Params* p) {myParams = p;}
	// Determine which handle (if any) is under mouse 
	virtual int mouseIsOverHandle(float screenCoords[2], float* boxExtents, int* faceNum) = 0;
	virtual void mouseRelease(float screenCoords[2]) = 0;
	virtual int draggingHandle() = 0;
	
protected:
	static const float faceSelectionColor[4];
	static const float unselectedFaceColor[4];
	int mouseDownPosition[2];
	float mouseDownIntersect[3];
	
	void getBoxVertices(float vertices[8][3]);
	Params* myParams;
	GLWindow* myGLWin;
	
	//general utility function for drawing axis-aligned cubes.
	//Should be in cube coords
	void drawCubeFaces(float* extents, bool isSelected);
	
	float dragDistance;
	int selectedHandle;
};

//Subclass that handles translation manip.  Works 
//with ProbeParams
//Subclass that handles both translation and stretch .  Works 
//with RegionParams and FlowParams (rake).
//When you slide a handle with the right mouse it stretches the region
class RENDER_API TranslateStretchManip : public Manip {
public:
	TranslateStretchManip(GLWindow* win, Params*p); 
	virtual ~TranslateStretchManip(){}
	virtual void render();
	
	
	//Identify the handle and face if the mouse is over handle:
	int mouseIsOverHandle(float screenCoords[2], float* boxExtents, int* faceNum);
	
	virtual void mouseRelease(float screenCoords[2]);
	virtual int draggingHandle() {return selectedHandle;}
	virtual void captureMouseDown(int handleNum, int faceNum, float* camPos, float* dirVec, int buttonNum);
	virtual void slideHandle(int handleNum, float movedRay[3]);
	

protected:
	
	//Draw all the faces of the Box, apply translations and rotations 
	//according to current drag state.  doesn't take into account rotation and mid-plane
	virtual void drawBoxFaces();
	void drawHandleConnector(int handleNum, float* handleExtents, float* extents);

	
	//Utility to determine the point in 3D where the mouse was clicked.
	//FaceNum assumes coord-plane-aligned cubic handles.
	//faceNum = handleNum for region and flow params
	//Intersection is performed in world coords
	bool rayHandleIntersect(float ray[3], float cameraPos[3], int handleNum, int faceNum, float intersect[3]);
	void makeHandleExtents(int handleNum, float* handleExtents, int octant, float* extents);
	void makeHandleExtentsInCube(int handleNum, float* handleExtents, int octant, float* extents);
	
	void drawCubeFaces(float* handleExtents, bool isSelected);
	
	int makeHandleFaces(int handleNum, float handle[8][3], int octant, float* boxExtents);

	bool isStretching;
	float handleSizeInCube;
	float subregionFrameColor[3];
	float initialSelectionRay[3];
	//Following only used by rotating manip subclasses:
	float tempRotation;
	int tempRotAxis;
	
};
//Subclass that allows the box to be rotated, and for it to be slid outside
//the full domain bounds.
class RENDER_API TranslateRotateManip : public TranslateStretchManip {
public:
	TranslateRotateManip(GLWindow* w, Params* p);
	virtual ~TranslateRotateManip(){}
	virtual void render();
	virtual void slideHandle(int handleNum, float movedRay[3]);
	virtual void mouseRelease(float screenCoords[2]);
	//While dragging the thumbwheel call this.
	//When drag is over, set rot to 0.f
	void setTempRotation(float rot, int axis) {
		tempRotation = rot; tempRotAxis = axis;
	}
protected:
	virtual void drawBoxFaces();
	float constrainStretch(float currentDist);
	//utility class for handling permutations resulting from rotations of mult of 90 degrees:
	class Permuter {
	public:
		Permuter(float theta, float phi);
		int permute(int i);
	private:
		int thetaRot; 
		int phiRot;
	};

};


};


#endif //MANIP_H
