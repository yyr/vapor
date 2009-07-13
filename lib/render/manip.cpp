//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		manip.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Implements the Manip class and some of its subclasses
//TOGO:
//#include "vizwinmgr.h"
//#include "probeeventrouter.h"

#include "manip.h"
#include "params.h"
#include "probeparams.h"
#include "viewpointparams.h"
#include "animationparams.h"
#include "glwindow.h"
#include "glutil.h"
#include "datastatus.h"
using namespace VAPoR;
const float Manip::faceSelectionColor[4] = {0.8f,0.8f,0.0f,0.8f};
const float Manip::unselectedFaceColor[4] = {0.8f,0.2f,0.0f,0.8f};
// Manip Methods
//Obtain the vertices of the box, in the unit cube, put them into an array
//

//Methods for TranslateStretchManip:
//


TranslateStretchManip::TranslateStretchManip(GLWindow* win, Params* p) : Manip(win) {
	const QColor& c = DataStatus::getSubregionFrameColor();
	subregionFrameColor[0]= (float)c.red()/255.;
	subregionFrameColor[1]= (float)c.green()/255.;
	subregionFrameColor[2]= (float)c.blue()/255.;

	setParams(p);
	selectedHandle = -1;
	isStretching = false;
	tempRotation = 0.f;
	tempRotAxis = -1;
}

// Determine if the mouse is over any of the six handles.
// Test first the 3 handles in front, then the object, then the three in back.
// Return the handle num if we are over one, or -1 if not.
// If the mouse is over a handle, the value of faceNum indicates the face number of
// the side that is intersected.
// The boxExtents are the extents in the unit cube.
// Does not take into account drag distance, because the mouse is just being clicked.
//
int TranslateStretchManip::
mouseIsOverHandle(float screenCoords[2], float* boxExtents, int* faceNum){
	//Determine if the mouse is over any of the six handles.
	//Test first the 3 handles in front, then the object, then the three in back.
	//The specified getHandle methods must return boxes with prescribed alignment
	// (ctr clockwise around front (+Z) starting at -X, -Y, then clockwise
	// around back (-Z) starting at -x ,-y.
	float handle[8][3];
	
	float camPos[3];
	
	//Get the camera position in cube coords.  This is needed to determine
	//which handles are in front of the box.
	ViewpointParams* myViewpointParams = myGLWin->getActiveViewpointParams();
	ViewpointParams::worldToStretchedCube(myViewpointParams->getCameraPos(), camPos);
	//qWarning(" mouseHandleTest: campos is %f %f %f",camPos[0],camPos[1],camPos[2]);
	//Determine the octant based on camera relative to box center:
	int octant = 0;
	//face identifies the faceNumber (for intersection calc) if there is
	int face, handleNum;
	for (int axis = 0; axis < 3; axis++){
		if (camPos[axis] > 0.5f*(boxExtents[axis]+boxExtents[axis+3])) octant |= 1<<axis;
	}
	for (int sortNum = 0; sortNum < 3; sortNum++){
		handleNum = makeHandleFaces(sortNum, handle, octant, boxExtents);
		if((face = myGLWin->pointIsOnBox(handle, screenCoords)) >= 0){
			*faceNum = face; return handleNum;
		}
	}
	
	
	//Then check backHandles
	for (int sortNum = 3; sortNum < 6; sortNum++){
		handleNum = makeHandleFaces(sortNum, handle, octant, boxExtents);
		if((face = myGLWin->pointIsOnBox(handle, screenCoords)) >= 0){
			*faceNum = face; return handleNum;
		}
	}
	return -1;
}
//Construct one of 6 cube-handles.  The first 3 (0,1,2) are those in front of the probe 
// the octant (between 0 and 7) is a binary representation of the viewer relative to
// the probe center.  In binary, this value is ZYX, where Z is 1 if the viewer is above,
// 0 if the viewer is below, similarly for Y and X.  The handles are numbered from 0 to 5,
// in the order -X, +X, -Y, +Y, -Z, +Z.  
// 
// We provide the handle in the appropriate sort order, based on the octant
// in which the viewer is located.  The sort position of a handle is either
// N or (5-N) where N is the order sorted on x,y,z axes.  It is 5-N if the
// corresponding bit in the octant is 1.  Octant is between 0 and 7
// with a bit for each axis (Z,Y,X order).
// Return value is the absolute handle index, which may differ from the sort order.
// If you want the faces in absolute order, just specify octant = 0
//
// We make the handles in cube coords, since rendering and picking use that system.
// This requires that we have boxCenter and boxWidth in cube coords.
// HANDLE_DIAMETER is measured in pixels
// These can be calculated from the boxRegion 
int TranslateStretchManip::
makeHandleFaces(int sortPosition, float handle[8][3], int octant, float boxRegion[6]){
	//Identify the axis this handle is on:
	int axis = (sortPosition<3) ? (2-sortPosition) : (sortPosition-3);
	int newPosition = sortPosition;
	if ((octant>>axis) & 1) newPosition = 5 - sortPosition;
	
	//Now create the cube associated with newPosition.  It's just the handle translated
	//in the direction associated with newPosition
	float translateSign = (newPosition > 2) ? 1.f : -1.f;
	for (int vertex = 0; vertex < 8; vertex ++){
		for (int coord = 0; coord<3; coord++){
			//Obtain the coordinate of unit cube corner.  It's either +0.5 or -0.5
			//multiplied by the handle diameter, then translated along the 
			//specific axis corresponding to 
			float fltCoord = (((float)((vertex>>coord)&1) -0.5f)*handleSizeInCube);
			//First offset it from the probeCenter:
			fltCoord += 0.5f*(boxRegion[coord+3] + boxRegion[coord]);
			//Displace all the c - coords of this handle if this handle is on the c-axis
			if (coord == axis){
				float boxWidth = (boxRegion[coord+3] - boxRegion[coord]);
				//Note we are putting the handle 2 diameters from the box edge
				fltCoord += translateSign*(boxWidth*0.5f + 2.f*handleSizeInCube);
			}
			handle[vertex][coord] = fltCoord;
		}
	}
	return newPosition;
}
//Find the handle extents using the boxExtents.
//All coordinates are in the unit cube.
//Set the octant to be 0 if the sortPosition is just the 
//handleNum
//If this is on the same axis as the selected handle it is displaced by dragDistance
//
void TranslateStretchManip::
makeHandleExtentsInCube(int sortPosition, float handleExtents[6], int octant, float boxExtents[6]){
	//Identify the axis this handle is on:
	int axis = (sortPosition<3) ? (2-sortPosition) : (sortPosition-3);
	int newPosition = sortPosition;
	if ((octant>>axis)&1) newPosition = 5 - sortPosition;
	
	//Now create the cube associated with newPosition.  It's just the handle translated
	//up or down in the direction associated with newPosition
	
	for (int coord = 0; coord<3; coord++){
		//Start at the box center position
		handleExtents[coord] = .5f*(-handleSizeInCube +(boxExtents[coord+3] + boxExtents[coord]));
		handleExtents[coord+3] = .5f*(handleSizeInCube +(boxExtents[coord+3] + boxExtents[coord]));
		
		if (coord == axis){//Translate up or down along this axis
			//The translation is 2 handles + .5 box thickness
			float boxWidth = (boxExtents[coord+3] - boxExtents[coord]);
			if (newPosition < 3){ //"low" handles are shifted down in the coord:
				handleExtents[coord] -= (boxWidth*0.5f + 2.f*handleSizeInCube);
				handleExtents[coord+3] -= (boxWidth*0.5f + 2.f*handleSizeInCube);
			} else {
				handleExtents[coord]+= (boxWidth*0.5f + 2.f*handleSizeInCube);
				handleExtents[coord+3]+= (boxWidth*0.5f + 2.f*handleSizeInCube);
			}
		}
	}
	return;
}
//Find the handle extents using the boxExtents in world coords
//Set the octant to be 0 if the sortPosition is just the 
//handleNum
//If this is on the same axis as the selected handle it is displaced by dragDistance
//

void TranslateStretchManip::
makeHandleExtents(int sortPosition, float handleExtents[6], int octant, float boxExtents[6]){
	//Identify the axis this handle is on:
	int axis = (sortPosition<3) ? (2-sortPosition) : (sortPosition-3);
	int newPosition = sortPosition;
	if ((octant>>axis)&1) newPosition = 5 - sortPosition;
	
	//Now create the cube associated with newPosition.  It's just the handle translated
	//up or down in the direction associated with newPosition
	float worldHandleDiameter = handleSizeInCube;
	for (int coord = 0; coord<3; coord++){
		//Start at the box center position
		handleExtents[coord] = .5f*(-worldHandleDiameter +(boxExtents[coord+3] + boxExtents[coord]));
		handleExtents[coord+3] = .5f*(worldHandleDiameter +(boxExtents[coord+3] + boxExtents[coord]));
		
		if (coord == axis){//Translate up or down along this axis
			//The translation is 2 handles + .5 box thickness
			float boxWidth = (boxExtents[coord+3] - boxExtents[coord]);
			if (newPosition < 3){ //"low" handles are shifted down in the coord:
				handleExtents[coord] -= (boxWidth*0.5f + 2.f*worldHandleDiameter);
				handleExtents[coord+3] -= (boxWidth*0.5f + 2.f*worldHandleDiameter);
			} else {
				handleExtents[coord]+= (boxWidth*0.5f + 2.f*worldHandleDiameter);
				handleExtents[coord+3]+= (boxWidth*0.5f + 2.f*worldHandleDiameter);
			}
		}
	}
	return;
}

//Draw all the faces of a cube with specified extents.
//Currently just used for handles.
void TranslateStretchManip::drawCubeFaces(float* extents, bool isSelected){
	
	glLineWidth( 2.0 );
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (isSelected) glColor4fv(faceSelectionColor);
	else glColor4fv(unselectedFaceColor);
	
	
//Do left (x=0)
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[2]);
	glEnd();
	
//do right 
	if (isSelected) glColor4fv(faceSelectionColor);
	else glColor4fv(unselectedFaceColor);
	glBegin(GL_QUADS);
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glEnd();
	
	
//top
	if (isSelected) glColor4fv(faceSelectionColor);
	else glColor4fv(unselectedFaceColor);
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[4], extents[2]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	glEnd();
	
//bottom
	if (isSelected) glColor4fv(faceSelectionColor);
	else glColor4fv(unselectedFaceColor);
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[3], extents[1], extents[2]);
	glEnd();
	
	//back
	if (isSelected) glColor4fv(faceSelectionColor);
	else glColor4fv(unselectedFaceColor);
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glVertex3f(extents[0], extents[4], extents[2]);
	glEnd();
	
	//do the front:
	//
	if (isSelected) glColor4fv(faceSelectionColor);
	else glColor4fv(unselectedFaceColor);
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	glEnd();
	
			
	glColor4f(1,1,1,1);
	glDisable(GL_BLEND);
}



//Note: This is performed in world coordinates!

//Determine intersection (in world coords!) of ray with handle
bool TranslateStretchManip::
rayHandleIntersect(float ray[3], float cameraPos[3], int handleNum, int faceNum, float intersect[3]){

	double val;
	float handleExtents[6];
	float boxExtents[6];
	int timestep = myGLWin->getActiveAnimationParams()->getCurrentFrameNumber();
	myParams->calcBoxExtents(boxExtents,timestep);
	makeHandleExtents(handleNum, handleExtents, 0, boxExtents);
	int coord;
	
	switch (faceNum){
		case(2): //back; z = zmin
			val = handleExtents[2];
			coord = 2;
			break;
		case(3): //front; z = zmax
			val = handleExtents[5];
			coord = 2;
			break;
		case(1): //bot; y = min
			val = handleExtents[1];
			coord = 1;
			break;
		case(4): //top; y = max
			val = handleExtents[4];
			coord = 1;
			break;
		case(0): //left; x = min
			val = handleExtents[0];
			coord = 0;
			break;
		case(5): //right; x = max
			val = handleExtents[3];
			coord = 0;
			break;
		default:
			return false;
	}
	if (ray[coord] == 0.0) return false;
	float param = (val - cameraPos[coord])/ray[coord];
	for (int i = 0; i<3; i++){
		intersect[i] = cameraPos[i]+param*ray[i];
	}
	return true;
}
	

//Renders handles and box
//If it is stretching, it only moves the one handle that is doing the stretching
void TranslateStretchManip::render(){
	if (!myGLWin || !myParams) return;
	float camVec[3];
	float extents[6];
	//Calculate the box extents, and the viewer position, in the unit cube,
	//Without any rotation applied:
	int timestep = myGLWin->getActiveAnimationParams()->getCurrentFrameNumber();
	myParams->calcStretchedBoxExtentsInCube(extents, timestep);
	ViewpointParams* myViewpointParams = myGLWin->getActiveViewpointParams();
	ViewpointParams::worldToStretchedCube(myViewpointParams->getCameraPos(), camVec);

	//Set the handleSize, in cube coords:
	handleSizeInCube = myGLWin->getPixelSize()*(float)HANDLE_DIAMETER/myViewpointParams->getMaxStretchedCubeSide();
	
	//Color depends on which item selected. (reg color vs highlight color)
	//Selected item is rendered at current offset
	//This will issue gl calls to render 6 cubes and 3 lines through them (to the center of the 
	//specified region).  If one is selected that line (and both cubes) are given
	//the highlight color
	
	
	//Now generate each handle and render it.  Order is not important
	float handleExtents[6];
	for (int handleNum = 0; handleNum < 6; handleNum++){
		makeHandleExtentsInCube(handleNum, handleExtents, 0/*octant*/, extents);
		if (selectedHandle >= 0){
			int axis = (selectedHandle < 3) ? (2-selectedHandle) : (selectedHandle -3);
			//displace handleExtents appropriately
			if (isStretching){
				//modify the extents for the one grabbed handle
				if ( selectedHandle == handleNum){ 
					handleExtents[axis] += dragDistance;
					handleExtents[axis+3] += dragDistance;
				} //and make the handles on the non-grabbed axes move half as far:
				else if (handleNum != (5-selectedHandle)){
					handleExtents[axis] += 0.5f*dragDistance;
					handleExtents[axis+3] += 0.5f*dragDistance;
				}
			} else {
				handleExtents[axis] += dragDistance;
				handleExtents[axis+3] += dragDistance;
			}
		}
		drawCubeFaces(handleExtents, (handleNum == selectedHandle));
		drawHandleConnector(handleNum, handleExtents, extents);
	}
	//Then render the full box, unhighlighted and displaced
	drawBoxFaces();
	
}
//Draw the main box, just rendering the lines.
//the highlightedFace is not the same as the selectedFace!!
//

void TranslateStretchManip::drawBoxFaces(){
	float corners[8][3];
	int timestep = myGLWin->getActiveAnimationParams()->getCurrentFrameNumber();
	myParams->calcBoxCorners(corners, 0.f, timestep);
	
	//Now the corners need to be put into the unit cube, and displaced appropriately
	//Either displace just half the corners or do the opposite ones as well.
	for (int cor = 0; cor < 8; cor++){
		ViewpointParams::worldToStretchedCube(corners[cor],corners[cor]);
		if (selectedHandle >= 0) {
			int axis = (selectedHandle < 3) ? (2-selectedHandle):(selectedHandle-3);
			//The corners associated with a handle are as follows:
			//If the handle is on the low end, i.e. selectedHandle < 3, then
			// for axis == 0, handles on corners  1,3,5,7 (cor&1 is 1)
			// for axis == 1, handles on corners  0,1,4,5 (cor&2 is 0)
			// for axis == 2, handles on corners  4,5,6,7 (cor&4 is 4) 
			//HMMM That's wrong.  The same expression works for x,y, and z
			// for axis == 0, handles on corners 0,2,4,6
			// for axis == 2, handles on corners 0,1,2,3
			//This fixes a bug (2/7/07)
			if (isStretching) {
				if (((cor>>axis)&1) && selectedHandle < 3) continue;
				if (!((cor>>axis)&1) && selectedHandle >= 3) continue;
			}
			corners[cor][axis] += dragDistance;
		}
	}
	
	
	//Now render the edges:
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glLineWidth( 2.0 );
	glColor3f(1.f,0.f,0.f);
	glBegin(GL_LINES);
	glVertex3fv(corners[0]);
	glVertex3fv(corners[1]);
	glVertex3fv(corners[0]);
	glVertex3fv(corners[2]);
	glVertex3fv(corners[0]);
	glVertex3fv(corners[4]);

	glVertex3fv(corners[1]);
	glVertex3fv(corners[3]);
	glVertex3fv(corners[1]);
	glVertex3fv(corners[5]);

	glVertex3fv(corners[2]);
	glVertex3fv(corners[3]);
	glVertex3fv(corners[2]);
	glVertex3fv(corners[6]);

	glVertex3fv(corners[3]);
	glVertex3fv(corners[7]);

	glVertex3fv(corners[4]);
	glVertex3fv(corners[5]);
	glVertex3fv(corners[4]);
	glVertex3fv(corners[6]);

	glVertex3fv(corners[5]);
	glVertex3fv(corners[7]);

	glVertex3fv(corners[6]);
	glVertex3fv(corners[7]);
	glEnd();
	
	
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	
}



void TranslateStretchManip::
mouseRelease(float /*screenCoords*/[2]){
	//Need to commit to latest drag position
	//Are we dragging?
	int timestep = myGLWin->getActiveAnimationParams()->getCurrentFrameNumber();
	if (selectedHandle >= 0){
		float boxMin[3], boxMax[3];
		int axis = (selectedHandle <3) ? (2-selectedHandle): (selectedHandle-3);
		//Convert dragDistance to world coords:
		float dist = dragDistance*ViewpointParams::getMaxStretchedCubeSide();
		myParams->getStretchedBox(boxMin,boxMax,timestep);
		//Check if we are stretching.  If so, only move coords associated with
		//handle:
		if (isStretching){
			//boxMin gets changed for nearHandle, boxMax for farHandle
			if (selectedHandle < 3)
				boxMin[axis]+=dist;
			else 
				boxMax[axis]+=dist;
		} else {
			boxMin[axis]+=dist;
			boxMax[axis]+=dist;
		}

		myParams->setStretchedBox(boxMin, boxMax, timestep);
	}
	dragDistance = 0.f;
	selectedHandle = -1;
}

//Note: This is performed in world coordinates!
void TranslateStretchManip::
captureMouseDown(int handleNum, int faceNum, float* camPos, float* dirVec, int buttonNum){
	//Grab a probe handle
	//qWarning(" click direction vector: %f %f %f", dirVec[0],dirVec[1],dirVec[2]);
	selectedHandle = handleNum;
	dragDistance = 0.f;
	//Calculate intersection of ray with specified plane
	if (!rayHandleIntersect(dirVec, camPos, handleNum, faceNum, initialSelectionRay))
		selectedHandle = -1;
	//The selection ray is the vector from the camera to the intersection point,
	//So subtract the camera position
	vsub(initialSelectionRay, camPos, initialSelectionRay);

	//qWarning(" initial selection ray: %f %f %f", initialSelectionRay[0],initialSelectionRay[1],initialSelectionRay[2]);
	if (buttonNum > 1) isStretching = true; 
	else isStretching = false;
	//Reset any active rotation
	tempRotation = 0.f;
	tempRotAxis = -1;
}

//Slide the handle based on mouse move from previous capture.  
//Requires new direction vector associated with current mouse position
//The new face position requires finding the planar displacement such that 
//the ray (in the scene) associated with the new mouse position is as near
//as possible to the line projected from the original mouse position in the
//direction of planar motion.
//Initially calc done in  WORLD coords,
//Converted to stretched world coords for bounds testing,
//then final displacement is in cube coords.
//

void TranslateStretchManip::
slideHandle(int handleNum, float movedRay[3], bool constrain){
	float normalVector[3] = {0.f,0.f,0.f};
	float q[3], r[3], w[3];
	assert(handleNum >= 0);
	int coord = (handleNum < 3) ? (2-handleNum):(handleNum-3);
	
	normalVector[coord] = 1.f;
	//qWarning(" Moved Ray %f %f %f", movedRay[0],movedRay[1],movedRay[2]);
	//Calculate W:
	vcopy(movedRay, w);
	vnormal(w);
	float scaleFactor = 1.f/vdot(w,normalVector);
	//Calculate q:
	vmult(w, scaleFactor, q);
	vsub(q, normalVector, q);
	
	//Calculate R:
	scaleFactor *= vdot(initialSelectionRay, normalVector);
	vmult(w, scaleFactor, r);
	vsub(r, initialSelectionRay, r);

	float denom = vdot(q,q);
	dragDistance = 0.f;
	//convert to stretched world coords.
	if (denom != 0.f){
		dragDistance = -vdot(q,r)/denom;
		dragDistance *= (DataStatus::getInstance()->getStretchFactors())[coord];
		//qWarning("drag dist, stretch factor: %g %g\n",dragDistance, (DataStatus::getInstance()->getStretchFactors())[coord]);
	}
	
	
	
	//Make sure the displacement is OK.  Not allowed to
	//Slide box out of full domain box.
	//If stretching, not allowed to push face through opposite face.
	
	//Do this calculation in stretched world coords
	float boxExtents[6];
	const float* extents = DataStatus::getInstance()->getStretchedExtents();
	int timestep = myGLWin->getActiveAnimationParams()->getCurrentFrameNumber();
	myParams->calcStretchedBoxExtents(boxExtents, timestep);
	
	if (isStretching){ //don't push through opposite face ..
		//Depends on whether we are pushing the "low" or "high" handle
		//E.g., The low handle is limited by the low end of the extents, and the
		//big end of the box
		if (handleNum < 3 ){ 
			if(dragDistance + boxExtents[coord] > boxExtents[coord+3]) {
				dragDistance = boxExtents[coord+3] - boxExtents[coord];
			}
			if(constrain && (dragDistance + boxExtents[coord] < extents[coord])) {
				dragDistance = extents[coord] - boxExtents[coord];
			}
		} else {//Moving "high" handle:
			if (dragDistance + boxExtents[coord+3] < boxExtents[coord]) {
				dragDistance = boxExtents[coord] - boxExtents[coord+3];
			}
			if (constrain&&(dragDistance + boxExtents[coord+3] > extents[coord+3])) {
				dragDistance = extents[coord+3]-boxExtents[coord+3];
			}
		}
	} else if (constrain){ //sliding, not stretching
		//Don't push the box out of the full region extents:
		
		if (dragDistance + boxExtents[coord] < extents[coord]) {
			dragDistance = extents[coord] - boxExtents[coord];
		}
		if (dragDistance + boxExtents[coord+3] > extents[coord+3]) {
			dragDistance = extents[coord+3] - boxExtents[coord+3];
		}
		
	}
	//now convert from stretched world to cube coords:
	dragDistance /= ViewpointParams::getMaxStretchedCubeSide();

}

//Draw a line connecting the specified handle to the box center.
//Highlight both ends of the line to the selected handle
//Note that handleExtents have already been displaced.
//if isStretching is true,
//the box center only moves half as far.
void TranslateStretchManip::drawHandleConnector(int handleNum, float* handleExtents, float* boxExtents){
	//Determine the side of the handle and the side of the box that is connected:
	int axis = (handleNum <3) ? (2-handleNum) : (handleNum-3) ;
	bool posSide = (handleNum > 2);
	float handleDisp[3], boxDisp[3];
	//Every handle gets a line from its inside face to the center of the box.
	for (int i = 0; i< 3; i++){
		//determine the box displacement, based on dimension:
		if ((i == (2-selectedHandle)) || (i ==(selectedHandle -3))){
			if (isStretching) boxDisp[i] = 0.5f*dragDistance;
			else boxDisp[i] = dragDistance;
		}
		else boxDisp[i] = 0.f;
		//The line to the handle is displaced according to what axis the
		//handle is on (independent of the dragDistance)
		//
		if (i==axis){
			if (posSide) {//displace to lower (in-) side of handle
				handleDisp[i] = -0.5f*(handleExtents[i+3] - handleExtents[i]);
			}
			else {//displace to upper (out-) side of handle
				handleDisp[i] = 0.5f*(handleExtents[i+3] - handleExtents[i]);
			}
		}
		else {//don't displace other coordinates
			handleDisp[i] = 0.f;
		}
		
	}
	glLineWidth( 2.0 );
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT, GL_FILL);
	if ((handleNum == selectedHandle) || (handleNum ==(5-selectedHandle))) glColor4fv(faceSelectionColor);
	else glColor4fv(unselectedFaceColor);
	glBegin(GL_LINES);
	glVertex3f(0.5f*(handleExtents[3]+handleExtents[0])+handleDisp[0],0.5f*(handleExtents[4]+handleExtents[1])+handleDisp[1],0.5f*(handleExtents[5]+handleExtents[2])+handleDisp[2]);
	glVertex3f(0.5f*(boxExtents[3]+boxExtents[0])+boxDisp[0],0.5f*(boxExtents[4]+boxExtents[1])+boxDisp[1],0.5f*(boxExtents[5]+boxExtents[2])+boxDisp[2]);
	glEnd();
	glDisable(GL_BLEND);
}



//Subclass constructor.  Allows rotated cube to be translated and stretched
//
TranslateRotateManip::TranslateRotateManip(GLWindow* w, Params* p) : TranslateStretchManip(w,p){
	
}
void TranslateRotateManip::drawBoxFaces(){
	float corners[8][3];
	Permuter* myPermuter = 0;
	if (isStretching) myPermuter = new Permuter(myParams->getTheta(),myParams->getPhi(),myParams->getPsi());
	
	myParams->calcBoxCorners(corners, 0.f, -1, tempRotation, tempRotAxis);
	//Now the corners need to be put into the unit cube, and displaced appropriately
	
	//Either displace just half the corners (when stretching) or do the opposite ones as well.
	for (int cor = 0; cor < 8; cor++){
		ViewpointParams::worldToStretchedCube(corners[cor],corners[cor]);
		if (selectedHandle >= 0) {
			int axis = (selectedHandle < 3) ? (2-selectedHandle):(selectedHandle-3);
			//The corners associated with a handle are as follows:
			//If the handle is on the low end, i.e. selectedHandle < 3, then
			// for axis == 0, handles on corners  0,2,4,6 (cor&1 is 0)
			// for axis == 1, handles on corners  0,1,4,5 (cor&2 is 0)
			// for axis == 2, handles on corners  0,1,2,3 (cor&4 is 0) 
			
			if (isStretching) {
				//Based on the angles (phi theta psi) the user is grabbing 
				//a rotated side of the cube. These vertices slide with the mouse.
				//rotate the selected handle by theta, phi, psi to find the side that corresponds to
				//the corners that need to be moved
				int newHandle; 
				if (selectedHandle<3) newHandle = myPermuter->permute(selectedHandle-3);
				else newHandle = myPermuter->permute(selectedHandle - 2);
				if (newHandle < 0) newHandle +=3;
				else newHandle +=2;
				int axis2 = (newHandle < 3) ? (2-newHandle):(newHandle-3);
				
				if (((cor>>axis2)&1) && newHandle < 3) continue;
				if (!((cor>>axis2)&1) && newHandle >= 3) continue;
			
			}
			corners[cor][axis] += dragDistance;
		}
	}
	if (myPermuter) delete myPermuter;
	//Then the faces need to be rendered
	//Use the same face ordering as was used for picking (mouseOver())
	
	//determine the corners of the textured plane.
	//the front corners are numbered 4 more than the back.
	//Average the front and back to get the middle:
	//
	float midCorners[4][3];
	for (int i = 0; i<4; i++){
		for(int j=0; j<3; j++){
			midCorners[i][j] = 0.5f*(corners[i][j]+corners[i+4][j]);
		}
	}
	//Now render the edges:
	
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);
	glLineWidth( 2.0 );
	glColor3f(1.f,0.f,0.f);
	glBegin(GL_LINES);
	glVertex3fv(corners[0]);
	glVertex3fv(corners[1]);
	glVertex3fv(corners[0]);
	glVertex3fv(corners[2]);
	glVertex3fv(corners[0]);
	glVertex3fv(corners[4]);

	glVertex3fv(corners[1]);
	glVertex3fv(corners[3]);
	glVertex3fv(corners[1]);
	glVertex3fv(corners[5]);

	glVertex3fv(corners[2]);
	glVertex3fv(corners[3]);
	glVertex3fv(corners[2]);
	glVertex3fv(corners[6]);

	glVertex3fv(corners[3]);
	glVertex3fv(corners[7]);

	glVertex3fv(corners[4]);
	glVertex3fv(corners[5]);
	glVertex3fv(corners[4]);
	glVertex3fv(corners[6]);

	glVertex3fv(corners[5]);
	glVertex3fv(corners[7]);

	glVertex3fv(corners[6]);
	glVertex3fv(corners[7]);

	
	//Now do the middle:
	glVertex3fv(midCorners[0]);
	glVertex3fv(midCorners[1]);

	glVertex3fv(midCorners[0]);
	glVertex3fv(midCorners[2]);

	glVertex3fv(midCorners[2]);
	glVertex3fv(midCorners[3]);

	glVertex3fv(midCorners[3]);
	glVertex3fv(midCorners[1]);
	glEnd();

	
	
	
	glFlush();
	//glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	
	
	
}

//Slide the handle based on mouse move from previous capture.  
//Requires new direction vector associated with current mouse position
//The new face position requires finding the planar displacement such that 
//the ray (in the scene) associated with the new mouse position is as near
//as possible to the line projected from the original mouse position in the
//direction of planar motion.
//Everything is done in WORLD coords.
//

void TranslateRotateManip::
slideHandle(int handleNum, float movedRay[3], bool constrain){
	float normalVector[3] = {0.f,0.f,0.f};
	float q[3], r[3], w[3];
	assert(handleNum >= 0);
	int coord = (handleNum < 3) ? (2-handleNum):(handleNum-3);
	
	normalVector[coord] = 1.f;
	//qWarning(" Moved Ray %f %f %f", movedRay[0],movedRay[1],movedRay[2]);
	//Calculate W:
	vcopy(movedRay, w);
	vnormal(w);
	float scaleFactor = 1.f/vdot(w,normalVector);
	//Calculate q:
	vmult(w, scaleFactor, q);
	vsub(q, normalVector, q);
	
	//Calculate R:
	scaleFactor *= vdot(initialSelectionRay, normalVector);
	vmult(w, scaleFactor, r);
	vsub(r, initialSelectionRay, r);

	float denom = vdot(q,q);
	dragDistance = 0.f;
	//Convert the drag distance to stretched world coords
	if (denom != 0.f){
		dragDistance = -vdot(q,r)/denom;
		dragDistance *= (DataStatus::getInstance()->getStretchFactors())[coord];
		//qWarning("drag dist, stretch factor: %g %g\n",dragDistance, (DataStatus::getInstance()->getStretchFactors())[coord]);
	}
	
	
	//Make sure the displacement is OK.  
	//Not allowed to
	//slide or stretch box center out of full domain box.
	//Do this calculation in stretched world coords
	float boxExtents[6];
	//const float* extents = DataStatus::getInstance()->getStretchedExtents();
	myParams->calcStretchedBoxExtents(boxExtents, -1);
	//float boxCenter = 0.5f*(boxExtents[coord]+boxExtents[coord+3]);
	if (isStretching){ //don't push through opposite face ..
		dragDistance = constrainStretch(dragDistance);
	} else { //sliding, not stretching
		//Previous constraint: Don't slide the center out of the full domain:
		/*  Constraint removed...
		if (dragDistance + boxCenter < extents[coord]) {
			dragDistance = extents[coord] - boxCenter;
		}
		if (dragDistance + boxCenter > extents[coord+3]){
			dragDistance = extents[coord+3] - boxCenter;
		} 
		*/
	}

	//now convert from stretched world to cube coords:
	dragDistance /= ViewpointParams::getMaxStretchedCubeSide();

	
}
//Following code should be invoked during OpenGL rendering.
//Assumes coords are already setup.
//This renders the box and the associated handles.
//Takes into account theta/phi rotations
//If a handle is selected, it is highlighted, and the box is slid in the handle
//direction according to the current translation.
//This rendering takes place in cube coords
//The extents argument give the full domain coordinate extents in the unit cube
void TranslateRotateManip::render(){
	if (!myGLWin || !myParams) return;
	float camVec[3];
	float extents[6];
	//Calculate the box extents, and the viewer position, in the unit cube,
	//With any rotation applied:
	
	myParams->calcContainingStretchedBoxExtentsInCube(extents);
	ViewpointParams* myViewpointParams = myGLWin->getActiveViewpointParams();
	ViewpointParams::worldToStretchedCube(myViewpointParams->getCameraPos(), camVec);

	//Set the handleSize, in cube coords:
	handleSizeInCube = myGLWin->getPixelSize()*(float)HANDLE_DIAMETER/myViewpointParams->getMaxStretchedCubeSide();
	
	//Color depends on which item selected. (reg color vs highlight color)
	//Selected item is rendered at current offset
	//This will issue gl calls to render 6 cubes and 3 lines through them (to the center of the 
	//specified region).  If one is selected that line (and both cubes) are given
	//the highlight color
	
	
	//Now generate each handle and render it.  Order is not important
	float handleExtents[6];
	for (int handleNum = 0; handleNum < 6; handleNum++){
		makeHandleExtentsInCube(handleNum, handleExtents, 0/*octant*/, extents);
		if (selectedHandle >= 0){
			//displace handleExtents appropriately
			int axis = (selectedHandle < 3) ? (2-selectedHandle) : (selectedHandle -3);
			//displace handleExtents appropriately
			if (isStretching){
				//modify the extents for the one grabbed handle
				if ( selectedHandle == handleNum){ 
					handleExtents[axis] += dragDistance;
					handleExtents[axis+3] += dragDistance;
				} //and make the handles on the non-grabbed axes move half as far:
				else if (handleNum != (5-selectedHandle)){
					handleExtents[axis] += 0.5f*dragDistance;
					handleExtents[axis+3] += 0.5f*dragDistance;
				}
			} else {
				handleExtents[axis] += dragDistance;
				handleExtents[axis+3] += dragDistance;
			}
		}
		drawCubeFaces(handleExtents, (handleNum == selectedHandle));
		drawHandleConnector(handleNum, handleExtents, extents);
	}
	//Then render the full box, unhighlighted and displaced
	drawBoxFaces();
	
}
void TranslateRotateManip::
mouseRelease(float /*screenCoords*/[2]){
	//Need to commit to latest drag position
	Permuter* myPermuter = 0;
	//Are we dragging?
	if (selectedHandle >= 0){
		float boxMin[3], boxMax[3];
		int axis = (selectedHandle <3) ? (2-selectedHandle): (selectedHandle-3);
		//Convert dragDistance to world coords:
		float dist = dragDistance*ViewpointParams::getMaxStretchedCubeSide();
		myParams->getStretchedBox(boxMin,boxMax,-1);
		//Check if we are stretching.  If so, need to decide what
		//coords are associated with handle.  Only those are to be
		//translated.
		if (isStretching){
			myPermuter = new Permuter(myParams->getTheta(),myParams->getPhi(),myParams->getPsi());
			//Based on the angles (phi and theta) the user is grabbing 
			//a rotated side of the cube. These vertices slide with the mouse.
			//rotate the selected handle by theta, phi to find the side that corresponds to
			//the corners that need to be moved
			int newHandle; //This corresponds to the side that was grabbed
			if (selectedHandle<3) newHandle = myPermuter->permute(selectedHandle-3);
			else newHandle = myPermuter->permute(selectedHandle - 2);
			if (newHandle < 0) newHandle +=3;
			else newHandle +=2;
			//And axis2 is the axis of the grabbed side
			int axis2 = (newHandle < 3) ? (2-newHandle):(newHandle-3);
				
			//Along axis, need to move center by half of distance.
			//boxMin gets changed for nearHandle, boxMax for farHandle
			//Need to also stretch box, since the rotation was about the middle:
			//Note that if axis2 is axis then we only change the max or the min
			boxMin[axis] += 0.5f*dist;
			boxMax[axis] += 0.5f*dist;
			//We need to stretch the size along axis2, without changing the center;
			//However this stretch is affected by the relative stretch factors of 
			//axis2 and axis
			const float* stretch = DataStatus::getInstance()->getStretchFactors();
			float dist2 = dist*stretch[axis2]/stretch[axis];
			if (selectedHandle < 3){
				boxMin[axis2] += 0.5f*dist2;
				boxMax[axis2] -= 0.5f*dist2;
			}
			else {
				boxMax[axis2] += 0.5f*dist2;
				boxMin[axis2] -= 0.5f*dist2;
			}
			
		} else {
			boxMin[axis]+=dist;
			boxMax[axis]+=dist;
		}

		myParams->setStretchedBox(boxMin, boxMax, -1);
	}
	dragDistance = 0.f;
	selectedHandle = -1;
}
//Determine the right-mouse drag constraint based on
//requiring that the resulting box will have all its min coords less than
//its max coords.
float TranslateRotateManip::constrainStretch(float currentDist){
	float dist = currentDist/ViewpointParams::getMaxStretchedCubeSide();
	float boxMin[3],boxMax[3];
	myParams->getStretchedBox(boxMin,boxMax,-1);
	
	Permuter* myPermuter = new Permuter(myParams->getTheta(),myParams->getPhi(),myParams->getPsi());
	//Based on the angles (phi and theta) the user is grabbing 
	//a rotated side of the cube. These vertices slide with the mouse.
	//rotate the selected handle by theta, phi to find the side that corresponds to
	//the corners that need to be moved
	int newHandle; //This corresponds to the side that was grabbed
	if (selectedHandle<3) newHandle = myPermuter->permute(selectedHandle-3);
	else newHandle = myPermuter->permute(selectedHandle - 2);
	if (newHandle < 0) newHandle +=3;
	else newHandle +=2;
	// axis2 is the axis of the grabbed side before rotation
	int axis2 = (newHandle < 3) ? (2-newHandle):(newHandle-3);
	// axis 1 is the axis in the scene
	int axis1 = (selectedHandle < 3) ? (2 - selectedHandle):(selectedHandle -3);
	//Don't drag the z-axis if it's planar:
	if (axis2 == 2){
		ProbeParams* pParams = (ProbeParams*) myParams;
		if (pParams->isPlanar()) {
			delete myPermuter;
			return 0.f;
		}
	}
	const float* strFacs = DataStatus::getInstance()->getStretchFactors();
	float corrFactor = ViewpointParams::getMaxStretchedCubeSide()*strFacs[axis2]/strFacs[axis1]; 
	
	if (selectedHandle < 3){
			if (dist*corrFactor > (boxMax[axis2]-boxMin[axis2])) 
				dist = (boxMax[axis2]-boxMin[axis2])/corrFactor;
	}
	else {
			if (dist*corrFactor < (boxMin[axis2]-boxMax[axis2])) 
				dist = (boxMin[axis2]-boxMax[axis2])/corrFactor;
	}
	delete myPermuter;
	
	return (dist*ViewpointParams::getMaxStretchedCubeSide());
}
TranslateRotateManip::Permuter::Permuter(float theta, float phi, float psi){
	//Find the nearest multiple of 90 degrees > 0
	theta += 44.f;
	phi += 44.f;
	psi += 44.f;
	while(theta < 0.f) theta += 360.f;
	while(phi < 0.f) phi += 360.f;
	while(psi < 0.f) psi += 360.f;
	//Then convert to right angles between 0 and 3:
	thetaRot = (int)(theta/90.f);
	thetaRot = thetaRot%4;
	phiRot = (int)(phi/90.f);
	phiRot = phiRot%4;
	psiRot = (int)(psi/90.f);
	psiRot = psiRot%4;
}
int TranslateRotateManip::Permuter::permute(int i){
	//first do the theta permutation:
	switch (thetaRot){
		case 1: // 1->-2, -2->-1, -1 -> 2, 2 ->1
			if (abs(i) == 1) i = -2*i;
			else if (abs(i) == 2) i = i/2;
			break;
		case 2: 
			if (abs(i) < 3) i = -i;
			break;
		case 3: // 1->2, 2->-1, -1 -> -2, -2 ->1
			if (abs(i) == 1) i = 2*i;
			else if (abs(i) == 2) i = -i/2;
			break;
		default:
			break;
	}
	
	//then do the phi permutation:
	switch (phiRot){
		case 1: // 1->3, 3->-1, -1 -> -3, -3 ->1
			if (abs(i) == 1) i = 3*i;
			else if (abs(i) == 3) i = -i/3;
			break;
		case 2: 
			if (abs(i) != 2) i = -i;
			break;
		case 3: // 1->3, 3->-1, -1 -> -3, -3 ->1
			if (abs(i) == 1) i = -3*i;
			else if (abs(i) == 3) i = i/3;
			break;
		default:
			break;
	}
	//finally do the psi permutation:
	switch (psiRot){
		case 1: // 1->-2, -2->-1, -1 -> 2, 2 ->1
			if (abs(i) == 1) i = -2*i;
			else if (abs(i) == 2) i = i/2;
			break;
		case 2: 
			if (abs(i) < 3) i = -i;
			break;
		case 3: // 1->2, 2->-1, -1 -> -2, -2 ->1
			if (abs(i) == 1) i = 2*i;
			else if (abs(i) == 2) i = -i/2;
			break;
		default:
			break;
	}
	return i;
}
 
