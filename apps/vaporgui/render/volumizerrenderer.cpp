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
#include <qmessagebox.h>
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
#include "messagereporter.h"

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
		BailOut("OpenGL: Failure to initialize driver",__FILE__,__LINE__);
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
		MessageReporter::errorMsg(
		"volumizer renderer error: %s\n", driver->GetErrMsg());
		driver->SetErrCode(0);
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
	int data_roi[6];
	int i;
	//If we are doing the first capture of a sequence then set the
	//newRender flag to true, whether or not it's a real new render.
	//Then turn off the flag, subsequent renderings will only be captured
	//if they really are new.
	//
	bool newRender = myVizWin->captureIsNew();
	myVizWin->setCaptureNew(false);
	if (!Session::getInstance()->renderReady()) return;
	DataMgr* myDataMgr = Session::getInstance()->getDataMgr();
	const Metadata* myMetadata = Session::getInstance()->getCurrentMetadata();
	//Nothing to do if there's no data source!
	if (!myDataMgr) return;
	
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
	if (myVizWin->mouseIsDown()) {
		numxforms = myRegionParams->getMaxNumTrans();
	}
	else numxforms = myRegionParams->getNumTrans();
	//Whenever numxforms changes, we need to dirty the clut, since
	//that affects the opacity correction.
	if (numxforms != savedNumXForms){
		myVizWin->setClutDirty(true);
		savedNumXForms = numxforms;
	}

		
	int bs = myMetadata->GetBlockSize();
		
	myRegionParams->calcRegionExtents(min_dim, max_dim, min_bdim, max_bdim, numxforms, minFull, maxFull, extents);
	
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
	if(myVizWin->regionFrameIsEnabled()|| MainForm::getInstance()->getCurrentMouseMode() == Command::regionMode){
		renderDomainFrame(extents, minFull, maxFull);
	} 
	if(myVizWin->subregionFrameIsEnabled()&& !(MainForm::getInstance()->getCurrentMouseMode() == Command::regionMode)){
		drawSubregionBounds(extents);
	} 
	if (myVizWin->axesAreEnabled()) drawAxes(extents);
	//This works around a volumizer/opengl bug!!!
	//If you issue a non-unit glColor before the volume rendering, it 
	//affects the subsequent volume rendering on Irix, but not on
	//Windows or Linux!
	glColor3f(1.f,1.f,1.f);	   
	
	
	//If there are new coords, get them from GL, send them to the gui
	if (myVizWin->viewerCoordsChanged()){ 
		newRender = true;
		myGLWindow->changeViewerFrame();
	}
	
	//Save the coord trans matrix, to pass to volumizer
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) matrix);
	
	
	// set up region. Only need to do this if the data
	// roi changes, or if the datarange has changed.
	//
	if (myVizWin->regionIsDirty()|| myVizWin->dataRangeIsDirty()) {

		newRender = true;
		
		int	rc;
		
		int nx,ny,nz;
		
		//Do Region setup as in SetCurrentFile() (mdb.C):
		//

		//Turn off error callback, look for memory allocation problem.
		Session::pauseErrorCallback();
		
		void* data = (void*) myDataMgr->GetRegionUInt8(
				myAnimationParams->getCurrentFrameNumber(),
				myDVRParams->getVariableName(),
				numxforms,
				min_bdim,
				max_bdim,
				myDVRParams->getCurrentDatarange(),
				0 //Don't lock!
			);
		//Turn it back on:
		Session::resumeErrorCallback();
		if (!data){
			int errCode = myDataMgr->GetErrCode();
			const char* msg = myDataMgr->GetErrMsg();
			MessageReporter::errorMsg("Unable to obtain volume data\n Datamanager Error code %d\n %s",
				errCode, msg);
			myDataMgr->SetErrCode(0);
			return;
		}
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
		newRender = true;
		//Same table sets CLUT and OLUT
		//
		driver->SetCLUT(myDVRParams->getClut());
		driver->SetOLUT(myDVRParams->getClut(), 
			myRegionParams->getMaxNumTrans()-numxforms);
		
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
	if (driver->Render((GLfloat *) matrix ) < 0){
		MessageReporter::errorMsg("%s","Unable to Render");
		return;
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
	if(myVizWin->colorbarIsEnabled()){
		//Now go to default 2D window
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		
		renderColorscale(myVizWin->colorbarIsDirty()||myVizWin->clutIsDirty()||myVizWin->dataRangeIsDirty());
	
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
	myVizWin->setClutDirty(false);
	myVizWin->setDataRangeDirty(false);
	//Capture the image, if not navigating:
	if (newRender && !myVizWin->mouseIsDown()) myVizWin->doFrameCapture();
	if (isControlled) AnimationController::getInstance()->endRendering(winNum);
}


void VolumizerRenderer::
DrawVoxelWindow(unsigned fast)
{
	myGLWindow->makeCurrent();
	DrawVoxelScene(fast);
}

#endif //VOLUMIZER

