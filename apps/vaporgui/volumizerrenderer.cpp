//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		volumizerrenderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:  Implementation of VolumizerRenderer class	

// Code extracted from mdb, setup, vox in order to do simple
// volume rendering with volumizer
//

/* Original Author:
 *	Ken Chin-Purcell (ken@ahpcrc.umn.edu)
 *	Army High Performance Computing Research Center (AHPCRC)
 *	University of Minnesota
 *
 */
#ifdef VOLUMIZER

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifndef WIN32
#include <unistd.h>
#endif

#include <sys/types.h>
//#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include <qgl.h>
#include "volumizerrenderer.h"
#include "dvrparams.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"
#include "vizwin.h"
#include "vizwinmgr.h"
#include "glwindow.h"
#include "DVRVolumizer.h"
#include "DVRDebug.h"
#include "renderer.h"
#include "animationcontroller.h"

#include "mainform.h"
#include "command.h"
#include "session.h"
#include "glutil.h"

using namespace VAPoR;
using namespace VetsUtil;
//Most of the one-time setup from setupBob is performed in session,
//such as construction of DataMgr
//Gui settings are done in param constructors
//This constructor just creates the volumizer driver
//
VolumizerRenderer::VolumizerRenderer(VizWin* vw) : Renderer(vw) {
	myDVRParams = vw->getWinMgr()->getDvrParams(vw->getWindowNum());
	//Construct dvrvolumizer
	driver = create_driver("vz",1);
}



void VolumizerRenderer::
initializeGL(){
	//copied from VoxInitCB(Widget w, XtPointer closure, XtPointer callData)
	
	myGLWindow->makeCurrent();
    
    if (driver->GraphicsInit() < 0) {
		qWarning("Failure to initialize driver");
		
    }
    
}

void VolumizerRenderer::paintGL(){
	DrawVoxelWindow(false);
}



DVRBase* VolumizerRenderer::
create_driver(
	const char	*name,
	int	//nthreads
) 
{
	int* argc = 0;
	char** argv = 0;
	DVRBase::DataType_T	type = DVRBase::UINT8;

	
	DVRBase	*driver = NULL;


#ifdef	VOLUMIZER
	if (strcmp(name, "vz") == 0) {
		driver = new DVRVolumizer(argc, argv, type, 1);
		//driver = new DVRDebug(argc, argv, type, 1);
	}
#endif



#ifdef	STRETCHEDGRID
	else if (strcmp(name, "sg") == 0) {
		driver = new DVRStretchedGrid(argc, argv, type, nthreads);
	}
#endif
	else {
		qWarning("Invalid driver ");
		return NULL;
	}

	if (driver->GetErrCode() != 0) { 
		qWarning("DVRBase::DVRBase() : %s\n", driver->GetErrMsg());
		return NULL;
	}

	return(driver);
}



void VolumizerRenderer::
DrawVoxelScene(unsigned /*fast*/)
{
	float matrix[16];
    GLenum	buffer;
	static float extents[6];
	static float minFull[3], maxFull[3];
	int max_dim[3];
	int min_dim[3];
	int max_bdim[3];
	int min_bdim[3];
	float fullExtent[6];
	int data_roi[6];
	int i;
	//Nothing to do if there's no data source!
	if (!myDataMgr) return;
	if (!Session::getInstance()->renderReady()) return;
	int winNum = myVizWin->getWindowNum();
	//Tell the animation we are starting.  If it returns false, we are not
	//being monitored by the animation controller
	bool isControlled = AnimationController::getInstance()->beginRendering(winNum);

	//AN:  (2/10/05):  Calculate 'extents' to be the real coords in (0,1) that
	//the roi is mapped into.  First find the mapping of the full data array.  This
	//is mapped to the center of the cube with the largest dimension equal to 1.
	//Then determine what is the subvolume we are dealing with as a portion
	//of the full mapped data.
	int numxforms;
	if (myVizWin->mouseIsDown()) numxforms = myRegionParams->getMaxNumTrans();
	else numxforms = myRegionParams->getNumTrans();
		
	int bs = myMetadata->GetBlockSize();
		
	for(i=0; i<3; i++) {
		int	s = numxforms;

		min_dim[i] = (int) ((float) (myRegionParams->getCenterPosition(i) >> s) - 0.5 
			- (((myRegionParams->getRegionSize(i) >> s) / 2.0)-1.0));
		max_dim[i] = (int) ((float) (myRegionParams->getCenterPosition(i) >> s) - 0.5 
			+ (((myRegionParams->getRegionSize(i) >> s) / 2.0)));
		min_bdim[i] = min_dim[i] / bs;
		max_bdim[i] = max_dim[i] / bs;
	}
	
	for (i = 0; i< 3; i++){
		fullExtent[i] = myRegionParams->getFullDataExtent(i);
		fullExtent[i+3] = myRegionParams->getFullDataExtent(i+3);
	}
	float maxCoordRange = Max(fullExtent[3]-fullExtent[0],Max( fullExtent[4]-fullExtent[1], fullExtent[5]-fullExtent[2]));
	//calculate the geometric extents of the dimensions in the unit cube:
	//fit the full region adjacent to the coordinate planes.
	for (i = 0; i<3; i++) {
		int dim = (myRegionParams->getFullSize(i)>>numxforms) -1;
		assert (dim>= max_dim[i]);
		float extentRatio = (fullExtent[i+3]-fullExtent[i])/maxCoordRange;
		minFull[i] = 0.f;
		maxFull[i] = extentRatio;
		//minFull[i] = 0.5f*(1.f - extentRatio);
		//maxFull[i] = 0.5f*(1.f + extentRatio);
		extents[i] = minFull[i] + ((float)min_dim[i]/(float)dim)*(maxFull[i]-minFull[i]);
		extents[i+3] = minFull[i] + ((float)max_dim[i]/(float)dim)*(maxFull[i]-minFull[i]);
	}
    // Move to trackball view of scene  
	glPushMatrix();

	glLoadIdentity();

	glGetIntegerv(GL_DRAW_BUFFER, (GLint *) &buffer);
	//Clear out in preparation for rendering
	glDrawBuffer(GL_BACK);
	glClearDepth(1);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDrawBuffer(buffer);

	
	//glTranslatef(.5,.5,.5);
    myGLWindow->getTBall()->TrackballSetMatrix();
	//In regionMode, draw a grid:
	if(MainForm::getInstance()->getCurrentMouseMode() == Command::regionMode){
		renderDomainFrame(extents, minFull, maxFull);
	}
	
	//If there are new coords, get them from GL, send them to the gui
	if (myVizWin->newViewerCoords){ 
		myGLWindow->changeViewerFrame();
	}
	
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) matrix);
	
	
	// set up view transform. Really only need to do this if the data
	// roi changes
	//
	if (myVizWin->regionIsDirty()|| myDVRParams->datarangeIsDirty()) {

		//float xc, yc, zc; // center of volume
		//float hmaxdim;
		
		int	rc;
		
		
		int nx,ny,nz;
		
		//Make sure we are using the current regionParams
		
		myRegionParams = myVizWin->getWinMgr()->getRegionParams(winNum);
		AnimationParams* myAnimationParams = myVizWin->getWinMgr()->getAnimationParams(winNum);
		//Do Region setup as in SetCurrentFile() (mdb.C):
		//

		if (myDVRParams->datarangeIsDirty()){
			myDataMgr->SetDataRange(myDVRParams->getVariableName(),
				myDVRParams->getCurrentDatarange());
			myDVRParams->setDatarangeDirty(false);
		}
		
		void* data = (void*) myDataMgr->GetRegionUInt8(
				myAnimationParams->getCurrentFrameNumber(),
				myDVRParams->getVariableName(),
				numxforms,
				(const size_t*)min_bdim,
				(const size_t*)max_bdim,
				0 //Don't lock!
			);

		// make subregion origin (0,0,0)
		// Note that this doesn't affect the calc of nx,ny,nz.
		// Also, save original dims, will need them to find extents.
		
		for(i=0; i<3; i++) {
			while(min_bdim[i] > 0) {
				min_dim[i] -= bs; max_dim[i] -= bs;
				min_bdim[i] -= 1; max_bdim[i] -= 1;
			}
		}
		
		nx = (max_bdim[0] - min_bdim[0] + 1) * bs;
		ny = (max_bdim[1] - min_bdim[1] + 1) * bs;
		nz = (max_bdim[2] - min_bdim[2] + 1) * bs;
		
		data_roi[0] = min_dim[0];
		data_roi[1] = min_dim[1];
		data_roi[2] = min_dim[2];
		data_roi[3] = max_dim[0];
		data_roi[4] = max_dim[1];
		data_roi[5] = max_dim[2];

		
		rc = driver->SetRegion(data,
			nx, ny, nz,
			data_roi, extents
		);

		if (rc < 0) {
			fprintf(stderr, "Error in DVRVolumizer::SetRegion\n");
			fflush(stderr);
			
			return;
		}
		
		myVizWin->setRegionDirty(false);
	}

	if (myDVRParams->clutIsDirty()) {
		
		//Same table sets CLUT and OLUT
		//
		driver->SetCLUT(myDVRParams->getClut());
		driver->SetOLUT(myDVRParams->getClut());
		myDVRParams->setClutDirty(false);
	}
	
	if (myDVRParams->attenuationIsDirty()) {
		driver->SetAmbient(myDVRParams->getAmbientAttenuation(),
			myDVRParams->getAmbientAttenuation(),myDVRParams->getAmbientAttenuation());
		driver->SetDiffuse(myDVRParams->getDiffuseAttenuation(),
			myDVRParams->getDiffuseAttenuation(),myDVRParams->getDiffuseAttenuation());
		driver->SetSpecular(myDVRParams->getSpecularAttenuation(),
			myDVRParams->getSpecularAttenuation(),myDVRParams->getSpecularAttenuation());
		myDVRParams->setAttenuationDirty(false);
	}

	// Modelview matrix
	//
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//Make the z-buffer read-only for the volume data
	glDepthMask(GL_FALSE);
	if (driver->Render(
		(GLfloat *) matrix ) < 0){
		qWarning("Error calling Render()");
	}

	//Finally render the region geometry, if in region mode
	if(MainForm::getInstance()->getCurrentMouseMode() == Command::regionMode){
		renderRegionBounds(extents);
	}

    glPopMatrix();
	if (isControlled) AnimationController::getInstance()->endRendering(winNum);
}


void VolumizerRenderer::
DrawVoxelWindow(unsigned fast)
{
	myGLWindow->makeCurrent();
	DrawVoxelScene(fast);
}
//OpenGL commands to draw a grid of lines of the full domain.
//Grid resolution is fine enough to show location of selected subregion
void VolumizerRenderer::renderDomainFrame(float* extents, float* minFull, float* maxFull){

	//Recalculate these things temporarily:

	float fullExtent[6];
	int i;
	for (i = 0; i< 3; i++){
		fullExtent[i] = myRegionParams->getFullDataExtent(i);
		fullExtent[i+3] = myRegionParams->getFullDataExtent(i+3);
	}
	
	//Determine how many lines in each dimension to render.  Find the smallest size 
	//of any of the current region dimensions.  See what power-of-two fraction of the full 
	//cube is as small as that size in that dimension.  Then subdivide all dimensions 
	//in that fraction.float minSize = 1.e30f;
	float regionSize[3], fullSize[3];
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
		//Limit to no more than 4-way grid lines
		if (numLines[i]>4) numLines[i] = 4;
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
//The "front" surface of the cube is drawn partially transparent, 
//with  handles to drag the planes.  This is drawn after the cube is drawn.
// The z-buffer can continue to be
//read-only, but leave it on so that the grid lines will continue to be visible.
//A face of the cube is "front" if the camera is in that direction from the
//cube center.
void VolumizerRenderer::renderRegionBounds(float* extents){
	
	glColor4f(.8f,.8f,.8f,.2f);
	float sideCoord;
	if (VizWinMgr::getInstance()->cameraBeyondRegionCenter(0, myVizWin->getWindowNum())){
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		sideCoord = extents[3];
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		sideCoord = extents[0];
	}
	//draw region end, y and z varying:
	glPolygonMode(GL_FRONT, GL_FILL);
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[2]);
	glEnd();
	//draw region end, y and z varying:
	glBegin(GL_QUADS);
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glEnd();
/*
	if (VizWinMgr::getInstance()->cameraBeyondRegionCenter(1, myVizWin->getWindowNum())){
		glPolygonMode(GL_FRONT, GL_FILL);
		sideCoord = extents[4];
	}
	else {
		glPolygonMode(GL_FRONT, GL_FILL);
		sideCoord = extents[1];
	}
	*/
	//draw region top/bottom, x and z varying:
	glPolygonMode(GL_FRONT, GL_FILL);
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[4], extents[2]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	
	
	glEnd();
	//draw region top/bottom, x and z varying:
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[3], extents[1], extents[2]);
	glEnd();

	if (VizWinMgr::getInstance()->cameraBeyondRegionCenter(2, myVizWin->getWindowNum())){
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		sideCoord = extents[5];
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		sideCoord = extents[2];
	}
	
	//draw region end, x and y varying:
	glPolygonMode(GL_FRONT, GL_FILL);
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glVertex3f(extents[0], extents[4], extents[2]);
	glEnd();
	//make the front more obvious!
	//glColor4f(.8f,.8f,.3f,0.7f);
	glPolygonMode(GL_FRONT, GL_FILL);
	glBegin(GL_QUADS);
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//Draw the  8 lines in solid yellow:
	glColor3f(1.f,1.f,0.f);
	glBegin(GL_LINES);
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[3], extents[1], extents[5]);
	
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[3], extents[4], extents[5]);
	
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[2]);
	
	glVertex3f(extents[3], extents[4], extents[2]);
	glVertex3f(extents[0], extents[4], extents[2]);
	
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[4], extents[2]);
	
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[0], extents[4], extents[2]);
	
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[0], extents[1], extents[5]);
	
	glVertex3f(extents[0], extents[4], extents[2]);
	glVertex3f(extents[0], extents[4], extents[5]);
	
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[5]);
	
	glVertex3f(extents[3], extents[4], extents[2]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glEnd();
}

#endif //VOLUMIZER

