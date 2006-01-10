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

#include "manip.h"
#include "params.h"
#include "viewpointparams.h"
#include "vizwin.h"
#include "glwindow.h"
#include "glutil.h"
using namespace VAPoR;
const float Manip::faceSelectionColor[4] = {0.8f,0.8f,0.0f,0.8f};
const float Manip::unselectedFaceColor[4] = {0.8f,0.2f,0.0f,0.8f};
TranslateManip::TranslateManip(VizWin* win, Params* p) : Manip(win) {
	QColor& c = myVizWin->getSubregionFrameColor();
	subregionFrameColor[0]= (float)c.red()/255.;
	subregionFrameColor[1]= (float)c.green()/255.;
	subregionFrameColor[2]= (float)c.blue()/255.;

	setParams(p);
	selectedHandle = -1;
}
//Following code should be invoked during OpenGL rendering.
//Assumes coords are already setup.
//This renders the box and the associated handles.
//If a handle is selected, it is highlighted, and the box is slid in the handle
//direction according to the current translation.
//This rendering takes place in cube coords
//The extents argument give the full domain coordinate extents in the unit cube
void TranslateManip::render(){
	if (!myVizWin || !myParams) return;
	float camVec[3];
	float extents[6];
	//Calculate the box extents, and the viewer position, in the unit cube,
	//Without any rotation applied:
	myParams->calcBoxExtentsInCube(extents);
	ViewpointParams* myViewpointParams = VizWinMgr::getInstance()->getViewpointParams(myVizWin->getWindowNum());
	ViewpointParams::worldToCube(myViewpointParams->getCameraPos(), camVec);

	//Set the handleSize, in cube coords:
	handleSizeInCube = myVizWin->getPixelSize()*(float)HANDLE_DIAMETER/ViewpointParams::getMaxCubeSide();
	
	//Color depends on which item selected. (reg color vs highlight color)
	//Selected item is rendered at current offset
	//This will issue gl calls to render 6 cubes and 3 lines through them (to the center of the 
	//specified region).  If one is selected that line (and both cubes) are given
	//the highlight color
	
	//Determine the octant based on camera relative to box center:
	/*int octant = 0;

	for (int axis = 0; axis < 3; axis++){
		if (camVec[axis] > 0.5f*(extents[axis]+extents[axis+3])) octant |= 1<<axis;
	}*/
	//Now generate each handle and render it.  Order is not important
	float handleExtents[6];
	for (int handleNum = 0; handleNum < 6; handleNum++){
		makeHandleExtentsInCube(handleNum, handleExtents, 0/*octant*/, extents);
		if (selectedHandle >= 0){
			//displace handleExtents appropriately
			int axis = (selectedHandle < 3) ? (2-selectedHandle) : (selectedHandle -3);
			handleExtents[axis] += dragDistance;
			handleExtents[axis+3] += dragDistance;
		}
		drawCubeFaces(handleExtents, (handleNum == selectedHandle));
		drawHandleConnector(handleNum, handleExtents, extents);
	}
	//Then render the full box, unhighlighted and displaced
	drawBoxFaces(-1);
	
}
// Determine if the mouse is over any of the six handles.
// Test first the 3 handles in front, then the object, then the three in back.
// Return the handle num if we are over one, or -1 if not.
// If the mouse is over a handle, the value of faceNum indicates the face number of
// the side that is intersected.
// The boxExtents are the extents in the unit cube.
// Does not take into account drag distance, because the mouse is just being clicked.
//
int TranslateManip::
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
	ViewpointParams* myViewpointParams = VizWinMgr::getInstance()->getViewpointParams(myVizWin->getWindowNum());
	ViewpointParams::worldToCube(myViewpointParams->getCameraPos(), camPos);
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
		if((face = myVizWin->getGLWindow()->pointIsOnBox(handle, screenCoords)) >= 0){
			*faceNum = face; return handleNum;
		}
	}
	
	//Check main box.  If it is on the box, we are done
	/*  Instead, allow users to pick "through" the box:
	getBoxVertices(handle);
	if ((myVizWin->getGLWindow()->pointIsOnBox(handle,screenCoords)) >= 0) return -1;
	*/

	//Then check backHandles
	for (int sortNum = 3; sortNum < 6; sortNum++){
		handleNum = makeHandleFaces(sortNum, handle, octant, boxExtents);
		if((face = myVizWin->getGLWindow()->pointIsOnBox(handle, screenCoords)) >= 0){
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
int TranslateManip::
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
void TranslateManip::
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
void TranslateManip::
makeHandleExtents(int sortPosition, float handleExtents[6], int octant, float boxExtents[6]){
	//Identify the axis this handle is on:
	int axis = (sortPosition<3) ? (2-sortPosition) : (sortPosition-3);
	int newPosition = sortPosition;
	if ((octant>>axis)&1) newPosition = 5 - sortPosition;
	
	//Now create the cube associated with newPosition.  It's just the handle translated
	//up or down in the direction associated with newPosition
	float worldHandleDiameter = handleSizeInCube*ViewpointParams::getMaxCubeSide();
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
void TranslateManip::drawCubeFaces(float* extents, bool isSelected){
	
	glLineWidth( 2.0 );
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT, GL_FILL);
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
//Draw a line connecting the specified handle to the box center.
//Highlight both ends of the line to the selected handle
//Note that handleExtents have already been displaced.
void TranslateManip::drawHandleConnector(int handleNum, float* handleExtents, float* boxExtents){
	//Determine the side of the handle and the side of the box that is connected:
	int axis = (handleNum <3) ? (2-handleNum) : (handleNum-3) ;
	bool posSide = (handleNum > 2);
	float handleDisp[3], boxDisp[3];
	//Every handle gets a line from its inside face to the center of the box.
	for (int i = 0; i< 3; i++){
		//determine the box displacement, based on dimension:
		if ((i == (2-selectedHandle)) || (i ==(selectedHandle -3)))
			boxDisp[i] = dragDistance;
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
//Draw the main box, just rendering the lines.
//Take into account the rotation angles, current drag state.
//the highlightedFace is not the same as the selectedFace!!
//
void TranslateManip::drawBoxFaces(int highlightedFace){
	float corners[8][3];
	
	myParams->calcBoxCorners(corners);
	//Now the corners need to be put into the unit cube, and displaced appropriately
	//This will need to change when we drag faces.
	for (int cor = 0; cor < 8; cor++){
		ViewpointParams::worldToCube(corners[cor],corners[cor]);
		if (selectedHandle >= 0) {
			int axis = (selectedHandle < 3) ? (2-selectedHandle):(selectedHandle-3);
			corners[cor][axis] += dragDistance;
		}
	}
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
	
	//Draw a translucent rectangle at the middle.
	//If the probe is enabled, will apply the probe texture to the rectangle
	//Note that we are assuming that the TranslateManip is associated with a probe!
	//For generality, the getProbeTexture method needs to be pushed up to 
	//Params class (so other params can use this manip).
	unsigned char* probeTex = ((ProbeParams*)myParams)->getProbeTexture();
	if (probeTex){
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, probeTex);
		//Do write to the z buffer
		glDepthMask(GL_TRUE);
	} else {
		//Don't write to the z-buffer, so won't obscure stuff behind that shows up later
		glDepthMask(GL_FALSE);
		glColor4f(.8f,.8f,0.f,0.2f);
	}
	glBegin(GL_QUADS);
	glTexCoord2f(0.f,0.f); glVertex3fv(midCorners[0]);
	glTexCoord2f(0.f, 1.f); glVertex3fv(midCorners[2]);
	glTexCoord2f(1.f,1.f); glVertex3fv(midCorners[3]);
	glTexCoord2f(1.f, 0.f); glVertex3fv(midCorners[1]);
	
	glEnd();
	glFlush();
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	if (probeTex) glDisable(GL_TEXTURE_2D);
	
	
}

//Obtain the vertices of the box, in the unit cube, put them into an array
void Manip::
getBoxVertices(float vertices[8][3]){
	float extents[6];
	getParams()->calcBoxExtentsInCube(extents);
	for (int vert = 0; vert< 8; vert++){
		for (int i = 0; i< 3; i++){
			if ((vert>>i)&1) vertices[vert][i] = extents[i+3];
			else vertices[vert][i] = extents[i];
		}
	}
	
}

void TranslateManip::
mouseRelease(float /*screenCoords*/[2]){
	//Need to commit to latest drag position
	//Are we dragging?
	if (selectedHandle >= 0){
		float boxMin[3], boxMax[3];
		int axis = (selectedHandle <3) ? (2-selectedHandle): (selectedHandle-3);
		//Convert dragDistance to world coords:
		float dist = dragDistance*ViewpointParams::getMaxCubeSide();
		myParams->getBox(boxMin,boxMax);
		boxMin[axis]+=dist;
		boxMax[axis]+=dist;
		myParams->setBox(boxMin, boxMax);
		myParams->captureMouseUp();
	}
	dragDistance = 0.f;
	selectedHandle = -1;
}
//Note: This is performed in world coordinates!
void TranslateManip::
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
	
}
//Determine intersection (in world coords) of ray with handle
bool TranslateManip::
rayHandleIntersect(float ray[3], float cameraPos[3], int handleNum, int faceNum, float intersect[3]){

	double val;
	float handleExtents[6];
	float boxExtents[6];
	myParams->calcBoxExtents(boxExtents);
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
	
//Slide the handle based on mouse move from previous capture.  
//Requires new direction vector associated with current mouse position
//The new face position requires finding the planar displacement such that 
//the ray (in the scene) associated with the new mouse position is as near
//as possible to the line projected from the original mouse position in the
//direction of planar motion.
//Everything is done in WORLD coords.
//

void TranslateManip::
slideHandle(int handleNum, float movedRay[3]){
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
	if (denom != 0.f)
		dragDistance = -vdot(q,r)/denom;
	
	
	//Make sure the displacement is OK.  Not allowed to
	//slide box center out of full domain box.
	//Do this calculation in world coords
	float boxExtents[6];
	float* extents = Session::getInstance()->getExtents();
	myParams->calcBoxExtents(boxExtents);
	float boxCenter = 0.5f*(boxExtents[coord]+boxExtents[coord+3]);
	if (dragDistance + boxCenter < extents[coord]) {
		dragDistance = extents[coord] - boxCenter;
	}
	if (dragDistance + boxCenter > extents[coord+3]){
		dragDistance = extents[coord+3] - boxCenter;
	} 

	//now convert to cube coords:
	dragDistance /= ViewpointParams::getMaxCubeSide();

	
}


TranslateStretchManip::TranslateStretchManip(VizWin* win, Params* p) : TranslateManip(win,p) {
	isStretching = false;
}
//The following is like the render() method of the TranslateManip; however,
//If it is stretching, it only moves the one handle that is doing the stretching
void TranslateStretchManip::render(){
	if (!myVizWin || !myParams) return;
	float camVec[3];
	float extents[6];
	//Calculate the box extents, and the viewer position, in the unit cube,
	//Without any rotation applied:
	myParams->calcBoxExtentsInCube(extents);
	ViewpointParams* myViewpointParams = VizWinMgr::getInstance()->getViewpointParams(myVizWin->getWindowNum());
	ViewpointParams::worldToCube(myViewpointParams->getCameraPos(), camVec);

	//Set the handleSize, in cube coords:
	handleSizeInCube = myVizWin->getPixelSize()*(float)HANDLE_DIAMETER/ViewpointParams::getMaxCubeSide();
	
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
	drawBoxFaces(-1);
	
}
//Draw the main box, just rendering the lines.
//the highlightedFace is not the same as the selectedFace!!
//
void TranslateStretchManip::drawBoxFaces(int highlightedFace){
	float corners[8][3];
	
	myParams->calcBoxCorners(corners);
	//Now the corners need to be put into the unit cube, and displaced appropriately
	//Either displace just half the corners or do the opposite ones as well.
	for (int cor = 0; cor < 8; cor++){
		ViewpointParams::worldToCube(corners[cor],corners[cor]);
		if (selectedHandle >= 0) {
			int axis = (selectedHandle < 3) ? (2-selectedHandle):(selectedHandle-3);
			//The corners associated with a handle are as follows:
			//If the handle is on the low end, i.e. selectedHandle < 3, then
			// for axis == 0, handles on corners  1,3,5,7 (cor&1 is 1)
			// for axis == 1, handles on corners  0,1,4,5 (cor&2 is 0)
			// for axis == 2, handles on corners  4,5,6,7 (cor&4 is 4) 
			if (isStretching) {
				if (axis != 1){
					if (((cor>>axis)&1) && selectedHandle >= 3) continue;
					if (!((cor>>axis)&1) && selectedHandle < 3) continue;
				} else { //axis == 1
					if (((cor>>axis)&1) && selectedHandle < 3) continue;
					if (!((cor>>axis)&1) && selectedHandle >= 3) continue;
				}
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
	if (selectedHandle >= 0){
		float boxMin[3], boxMax[3];
		int axis = (selectedHandle <3) ? (2-selectedHandle): (selectedHandle-3);
		//Convert dragDistance to world coords:
		float dist = dragDistance*ViewpointParams::getMaxCubeSide();
		myParams->getBox(boxMin,boxMax);
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

		myParams->setBox(boxMin, boxMax);
		myParams->captureMouseUp();
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
}

//Slide the handle based on mouse move from previous capture.  
//Requires new direction vector associated with current mouse position
//The new face position requires finding the planar displacement such that 
//the ray (in the scene) associated with the new mouse position is as near
//as possible to the line projected from the original mouse position in the
//direction of planar motion.
//Everything is done in WORLD coords.
//

void TranslateStretchManip::
slideHandle(int handleNum, float movedRay[3]){
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
	if (denom != 0.f)
		dragDistance = -vdot(q,r)/denom;
	
	
	//Make sure the displacement is OK.  Not allowed to
	//Slide box out of full domain box.
	//If stretching, not allowed to push face through opposite face.
	
	//Do this calculation in world coords
	float boxExtents[6];
	float* extents = Session::getInstance()->getExtents();
	myParams->calcBoxExtents(boxExtents);
	
	if (isStretching){ //don't push through opposite face ..
		//Depends on whether we are pushing the "low" or "high" handle
		//E.g., The low handle is limited by the low end of the extents, and the
		//big end of the box
		if (handleNum < 3 ){ 
			if(dragDistance + boxExtents[coord] > boxExtents[coord+3]) {
				dragDistance = boxExtents[coord+3] - boxExtents[coord];
			}
			if(dragDistance + boxExtents[coord] < extents[coord]) {
				dragDistance = extents[coord] - boxExtents[coord];
			}
		} else {//Moving "high" handle:
			if (dragDistance + boxExtents[coord+3] < boxExtents[coord]) {
				dragDistance = boxExtents[coord] - boxExtents[coord+3];
			}
			if (dragDistance + boxExtents[coord+3] > extents[coord+3]) {
				dragDistance = extents[coord+3]-boxExtents[coord+3];
			}
		}
	} else { //sliding, not stretching
		//Don't push the box out of the full region extents:
		if (dragDistance + boxExtents[coord] < extents[coord]) {
			dragDistance = extents[coord] - boxExtents[coord];
		}
		if (dragDistance + boxExtents[coord+3] > extents[coord+3]) {
			dragDistance = extents[coord+3] - boxExtents[coord+3];
		}
	}
	//now convert to cube coords:
	dragDistance /= ViewpointParams::getMaxCubeSide();

}

//Draw a line connecting the specified handle to the box center.
//Highlight both ends of the line to the selected handle
//Note that handleExtents have already been displaced.
//This is like the TranslateManip version, except that if isStretching is true,
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