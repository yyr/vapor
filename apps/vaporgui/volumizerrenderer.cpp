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
	//Construct dvrvolumizer
	driver = create_driver("vz",1);
}
VolumizerRenderer::~VolumizerRenderer(){
	delete driver;
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
	size_t max_bdim[3];
	size_t min_bdim[3];
	float fullExtent[6];
	int data_roi[6];
	int i;
	//Nothing to do if there's no data source!
	if (!myDataMgr) return;
	if (!Session::getInstance()->renderReady()) return;
	int winNum = myVizWin->getWindowNum();
	RegionParams* myRegionParams = VizWinMgr::getInstance()->getRegionParams(winNum);
	AnimationParams* myAnimationParams = VizWinMgr::getInstance()->getAnimationParams(winNum);
	ViewpointParams* myViewpointParams = VizWinMgr::getInstance()->getViewpointParams(winNum);
	DvrParams* myDVRParams = VizWinMgr::getInstance()->getDvrParams(winNum);
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
		//Make sure slab has nonzero thickness (this can only
		//be a problem while the mouse is pressed):
		//
		if (min_dim[i] >= max_dim[i]){
			if (max_dim[i] < 1){
				max_dim[i] = 1;
				min_dim[i] = 0;
			}
			else min_dim[i] = max_dim[i] - 1;
		}
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

	
	
    myGLWindow->getTBall()->TrackballSetMatrix();
	//In regionMode, draw a grid:
	if(MainForm::getInstance()->getCurrentMouseMode() == Command::regionMode){
		renderDomainFrame(extents, minFull, maxFull);
	} 
	//This works around a volumizer/opengl bug!!!
	//If you issue a non-unit glColor before the volume rendering, it 
	//affects the subsequent volume rendering on Irix, but not on
	//Windows or Linux!
	glColor3f(1.f,1.f,1.f);	   
	
	
	//If there are new coords, get them from GL, send them to the gui
	if (myVizWin->viewerCoordsChanged()){ 
		myGLWindow->changeViewerFrame();
	}
	
	//Save the coord trans matrix, to pass to volumizer
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) matrix);
	
	if (myVizWin->dataRangeIsDirty()){
		myDataMgr->SetDataRange(myDVRParams->getVariableName(),
			myDVRParams->getCurrentDatarange());
		myVizWin->setDataRangeDirty(false);
	}
	// set up region. Only need to do this if the data
	// roi changes
	//
	if (myVizWin->regionIsDirty()) {

		//float xc, yc, zc; // center of volume
		//float hmaxdim;
		
		int	rc;
		
		
		int nx,ny,nz;
		
		//Make sure we are using the current regionParams
		
		
		//Do Region setup as in SetCurrentFile() (mdb.C):
		//

		
		
		void* data = (void*) myDataMgr->GetRegionUInt8(
				myAnimationParams->getCurrentFrameNumber(),
				myDVRParams->getVariableName(),
				numxforms,
				min_bdim,
				max_bdim,
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

	if (myVizWin->clutIsDirty()) {
		
		//Same table sets CLUT and OLUT
		//
		driver->SetCLUT(myDVRParams->getClut());
		driver->SetOLUT(myDVRParams->getClut());
		myVizWin->setClutDirty(false);
	}
	/* Attenuation is not supported yet...
	if (myDVRParams->attenuationIsDirty()) {
		driver->SetAmbient(myDVRParams->getAmbientAttenuation(),
			myDVRParams->getAmbientAttenuation(),myDVRParams->getAmbientAttenuation());
		driver->SetDiffuse(myDVRParams->getDiffuseAttenuation(),
			myDVRParams->getDiffuseAttenuation(),myDVRParams->getDiffuseAttenuation());
		driver->SetSpecular(myDVRParams->getSpecularAttenuation(),
			myDVRParams->getSpecularAttenuation(),myDVRParams->getSpecularAttenuation());
		myDVRParams->setAttenuationDirty(false);
	}
*/
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
		float camVec[3];
		ViewpointParams::worldToCube(myViewpointParams->getCameraPos(), camVec);
		//Obtain the face displacement in world coordinates,
		//Then normalize to unit cube coords:
		float disp = myRegionParams->getFaceDisplacement();
		disp /= ViewpointParams::getMaxCubeSide();
		int selectedFace = myRegionParams->getSelectedFaceNum();
		assert(selectedFace >= -1 && selectedFace < 6);
		renderRegionBounds(extents, selectedFace,
			camVec, disp);
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
/*
void VolumizerRenderer::renderRegionBounds(float* extents, int selectedFace, float* camPos, float faceDisplacement){
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

*/
#endif //VOLUMIZER

