//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		renderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Implements the Renderer class.
//		A pure virtual class that is implemented for each renderer.
//		Methods are called by the glwindow class as needed.
//

#include "renderer.h"
#include "vapor/DataMgr.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "vizwinmgr.h"
#include "mainform.h"
#include "session.h"
#include "vizwin.h"
#include "glutil.h"

using namespace VAPoR;

Renderer::Renderer( VizWin* vw )
{
	//Establish the data sources for the rendering:
	//
	myVizWin = vw;
    myGLWindow = vw->getGLWindow();
	VizWinMgr* vwm = VizWinMgr::getInstance();
	int winNum = vw->getWindowNum();
	myViewpointParams = vwm->getViewpointParams(winNum);
	myRegionParams = vwm->getRegionParams(winNum);
	myDataMgr = Session::getInstance()->getDataMgr();
	myMetadata = Session::getInstance()->getCurrentMetadata();

}
//Issue OpenGL commands to draw a grid of lines of the full domain.
//Grid resolution is up to 4x4x4
//
void Renderer::renderDomainFrame(float* extents, float* minFull, float* maxFull){

	int i; 
	int numLines[3];
	float regionSize[3], fullSize[3];
	/*
	//Determine how many lines in each dimension to render.  Find the smallest size 
	//of any of the current region dimensions.  See what power-of-two fraction of the full 
	//cube is as small as that size in that dimension.  Then subdivide all dimensions 
	//in that fraction.float minSize = 1.e30f;
	
	int logDim[3], numLines[3];
	
	int minDim;
	float minSize = 1.e30f;
	
	for (i = 0; i<3; i++){
		regionSize[i] = extents[i+3]-extents[i];;
		fullSize[i] = maxFull[i] - minFull[i];
		if (minSize > regionSize[i]) {
			minSize = regionSize[i];
			minDim = i;
		}
	}
	if (regionSize[minDim]< fullSize[minDim]*.01){
		int foo = 3;
	}
	float sizeFraction = (fullSize[minDim])/minSize;
	//Find the integer log, base 2 of this:
	
	logDim[minDim] = ILog2((int)(sizeFraction+0.5f));
	//The log in the other dimensions will be about the same unless the
	//full region is substantially different in that dimension
	for (i = 0; i<3; i++){
		if (i!= minDim){
			float sizeRatio =	fullSize[i]/fullSize[minDim];
			if (sizeRatio > 1.f)
				logDim[i] = logDim[minDim]+ ILog2((int)sizeRatio);
			else 
				logDim[i] = logDim[minDim]- ILog2((int)(1.f/sizeRatio));
		}
		numLines[i] = 1<<logDim[i];
		//Limit to no more than 2-way grid lines
		if (numLines[i]>2) numLines[i] = 2;
	}
	*/
	//Instead:  either have 2 or 1 lines in each dimension.  2 if the size is < half
	for (i = 0; i<3; i++){
		regionSize[i] = extents[i+3]-extents[i];;
		fullSize[i] = maxFull[i] - minFull[i];
		if (regionSize[i] < fullSize[i]*.5) numLines[i] = 2;
		else numLines[i] = 1;
	}
	

	glColor3f(1.f,1.f,1.f);	   
    glLineWidth( 2.0 );
	//Now draw the lines.  Divide each dimension into numLines[dim] sections.
	
	int x,y,z;
	//Do the lines in each z-plane
	//Turn on writing to the z-buffer
	glDepthMask(GL_TRUE);
	
	glBegin( GL_LINES );
	for (z = 0; z<=numLines[2]; z++){
		float zCrd = minFull[2] + ((float)z/(float)numLines[2])*fullSize[2];
		//Draw lines in x-direction for each y
		for (y = 0; y<=numLines[1]; y++){
			float yCrd = minFull[1] + ((float)y/(float)numLines[1])*fullSize[1];
			
			glVertex3f(  minFull[0],  yCrd, zCrd );   
			glVertex3f( maxFull[0],  yCrd, zCrd );
			
		}
		//Draw lines in y-direction for each x
		for (x = 0; x<=numLines[0]; x++){
			float xCrd = minFull[0] + ((float)x/(float)numLines[0])*fullSize[0];
			
			glVertex3f(  xCrd, minFull[1], zCrd );   
			glVertex3f( xCrd, maxFull[1], zCrd );
			
		}
	}
	//Do the lines in each y-plane
	
	for (y = 0; y<=numLines[1]; y++){
		float yCrd = minFull[1] + ((float)y/(float)numLines[1])*fullSize[1];
		//Draw lines in x direction for each z
		for (z = 0; z<=numLines[2]; z++){
			float zCrd = minFull[2] + ((float)z/(float)numLines[2])*fullSize[2];
			
			glVertex3f(  minFull[0],  yCrd, zCrd );   
			glVertex3f( maxFull[0],  yCrd, zCrd );
			
		}
		//Draw lines in z direction for each x
		for (x = 0; x<=numLines[0]; x++){
			float xCrd = minFull[0] + ((float)x/(float)numLines[0])*fullSize[0];
		
			glVertex3f(  xCrd, yCrd, minFull[2] );   
			glVertex3f( xCrd, yCrd, maxFull[2]);
			
		}
	}
	
	//Do the lines in each x-plane
	for (x = 0; x<=numLines[0]; x++){
		float xCrd = minFull[0] + ((float)x/(float)numLines[0])*fullSize[0];
		//Draw lines in y direction for each z
		for (z = 0; z<=numLines[2]; z++){
			float zCrd = minFull[2] + ((float)z/(float)numLines[2])*fullSize[2];
			
			glVertex3f(  xCrd, minFull[1], zCrd );   
			glVertex3f( xCrd, maxFull[1], zCrd );
			
		}
		//Draw lines in z direction for each y
		for (y = 0; y<=numLines[1]; y++){
			float yCrd = minFull[1] + ((float)y/(float)numLines[1])*fullSize[1];
			
			glVertex3f(  xCrd, yCrd, minFull[2] );   
			glVertex3f( xCrd, yCrd, maxFull[2]);
			
		}
	}
	glEnd();//GL_LINES
	
	

}
// This method draws the faces of the region-cube.
// The surface of the cube is drawn partially transparent. 
// This is drawn after the cube is drawn.
// If a face is selected, it is drawn yellow
// The z-buffer will continue to be
// read-only, but is left on so that the grid lines will continue to be visible.
// Faces of the cube are numbered 0..5 based on view from pos z axis:
// back, front, bottom, top, left, right
// selectedFace is -1 if nothing selected
//	
// The viewer direction determines which faces are rendered.  If a coordinate
// of the viewDirection is positive (resp., negative), 
// then the back side (resp front side) of the corresponding cube side is rendered
void Renderer::renderRegionBounds(float* extents, int selectedFace, float* camPos, float faceDisplacement){
	//Copy the extents so they can be stretched
	int i;
	float cpExtents[6];
	int stretchCrd = -1;
	//Determine which coord direction is being stretched:
	if (selectedFace >= 0) {
		stretchCrd = (5-selectedFace)/2;
		if (selectedFace%2) stretchCrd +=3;
	}
	for (i = 0; i< 6; i++) {
		cpExtents[i] = extents[i];
		//Stretch the "extents" associated with selected face
		if(i==stretchCrd) cpExtents[i] += faceDisplacement;
	}
	for (i = 0; i< 6; i++){
		if (faceIsVisible(extents, camPos, i)){
			drawRegionFace(cpExtents, i, (i==selectedFace));
		}
	}
}

float* Renderer::cornerPoint(float* extents, int faceNum){
	if(faceNum&1) return extents+3;
	else return extents;
}
bool Renderer::faceIsVisible(float* extents, float* viewerCoords, int faceNum){
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
void Renderer::drawRegionFace(float* extents, int faceNum, bool isSelected){
	
	glPolygonMode(GL_FRONT, GL_FILL);
	if (isSelected)
			glColor4f(.8f,.8f,0.f,.3f);
		else 
			glColor4f(.8f,.8f,.8f,.2f);
	switch (faceNum){
		case 4://Do left (x=0)
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glColor3f(1,0,0);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			return;
		
		case 5:
		//do right 
			glBegin(GL_QUADS);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			glColor3f(1,0,0);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			return;
		case(3)://top
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glColor3f(1,0,0);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			return;
		case(2)://bottom
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glEnd();
			glColor3f(1,0,0);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glEnd();
			return;
	
		case(0):
			//back
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glColor3f(1,0,0);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			return;
		case(1):
			//do the front:
			//
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glColor3f(1,0,0);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			return;
		default: 
			return;
	}
}


