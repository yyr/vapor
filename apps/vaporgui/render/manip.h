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

#define HANDLE_DIAMETER 0.05f

namespace VAPoR {
class VizWin;
class Params;
class Manip {

public:
	Manip(VizWin* win) {myVizWin = win;}
	~Manip(){}
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
	static const float highlightColor[4];
	static const float faceColor[4];
	int mouseDownPosition[2];
	float mouseDownIntersect[3];
	
	void getBoxVertices(float vertices[8][3]);
	Params* myParams;
	VizWin* myVizWin;
	
	//general utility function for drawing axis-aligned cubes.
	//Should be in cube coords
	void drawCubeFaces(float* extents, bool isSelected);
	//Draw all the faces of the Box, apply translations and rotations 
	//according to current drag state
	virtual void drawBoxFaces(int highlightedFace);
	float dragDistance;
	int selectedHandle;
};

//Subclass that handles translation manip.  Should work
//with RegionParams, FlowParams, ProbeParams
class TranslateManip : public Manip {
public:
	TranslateManip(VizWin* win, Params*p); 
	virtual void render();
	//Identify the handle and face if the mouse is over handle:
	virtual int mouseIsOverHandle(float screenCoords[2], float* boxExtents, int* faceNum);
	
	virtual void mouseRelease(float screenCoords[2]);
	virtual int draggingHandle() {return selectedHandle;}
	void captureMouseDown(int handleNum, int faceNum, float* camPos, float* dirVec);
	void slideHandle(int handleNum, float movedRay[3]);

protected:
	float subregionFrameColor[3];
	float unselectedFaceColor[4];
	float faceSelectionColor[4];
	float initialSelectionRay[3];
	
	
	
	//Utility to determine the point in 3D where the mouse was clicked.
	//FaceNum assumes coord-plane-aligned cubic handles.
	//faceNum = handleNum for region and flow params
	//Intersection is performed in world coords
	bool rayHandleIntersect(float ray[3], float cameraPos[3], int handleNum, int faceNum, float intersect[3]);
	void makeHandleExtents(int handleNum, float* handleExtents, int octant, float* extents);
	void makeHandleExtentsInCube(int handleNum, float* handleExtents, int octant, float* extents);
	
	void drawCubeFaces(float* handleExtents, bool isSelected);
	void drawHandleConnector(int handleNum, float* handleExtents, float* extents);
	int makeHandleFaces(int handleNum, float handle[8][3], int octant, float* boxExtents);
	
};
};


#endif //MANIP_H
