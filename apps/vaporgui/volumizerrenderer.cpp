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
#include "session.h"
#include "glutil.h"

using namespace VAPoR;
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
	int data_roi[6];

	//Nothing to do if there's no data source!
	if (!myDataMgr) return;
	int winNum = myVizWin->getWindowNum();
	//Tell the animation we are starting.  If it returns false, we are not
	//being monitored by the animation controller
	bool isControlled = AnimationController::getInstance()->beginRendering(winNum);
    // Move to trackball view of scene  
	glPushMatrix();

	glLoadIdentity();
	glTranslatef(.5,.5,.5);
    myGLWindow->getTBall()->TrackballSetMatrix();
	/*
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) matrix);
	fprintf(stderr, "after trackballsetmatrix, modelMatrix is: \n %f %f %f %f \n %f %f %f %f \n %f %f %f %f \n %f %f %f %f ",
		matrix[0], matrix[1],matrix[2],matrix[3],
		matrix[4], matrix[5],matrix[6],matrix[7],
		matrix[8], matrix[9],matrix[10],matrix[11],
		matrix[12], matrix[13],matrix[14],matrix[15]);*/
	//If there are new coords, get them from GL, send them to the gui
	if (myVizWin->newViewerCoords){ 
		myGLWindow->changeViewerFrame();
	}
	
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) matrix);
	/*
	fprintf(stderr, "after changeViewerframe, modelMatrix is: \n %f %f %f %f \n %f %f %f %f \n %f %f %f %f \n %f %f %f %f ",
		matrix[0], matrix[1],matrix[2],matrix[3],
		matrix[4], matrix[5],matrix[6],matrix[7],
		matrix[8], matrix[9],matrix[10],matrix[11],
		matrix[12], matrix[13],matrix[14],matrix[15]);*/
	//
	// set up view transform. Really only need to do this if the data
	// roi changes
	//
	if (myVizWin->regionIsDirty()|| myDVRParams->datarangeIsDirty()) {

		//float xc, yc, zc; // center of volume
		//float hmaxdim;
		
		int	rc;
		int max_dim[3];
		int min_dim[3];
		int max_bdim[3];
		int min_bdim[3];
		int nx,ny,nz;
		int min_subdim[3];
		int max_subdim[3];

		//Make sure we are using the current regionParams
		
		myRegionParams = myVizWin->getWinMgr()->getRegionParams(winNum);
		AnimationParams* myAnimationParams = myVizWin->getWinMgr()->getAnimationParams(winNum);
		//Do Region setup as in SetCurrentFile() (mdb.C):
		//Set up parameters needed to specify region:
		//level is flevels -stride + 1:
		//= (maxstride -1) - (stride) + 1
		int numxforms;
		if (myVizWin->mouseIsDown()) numxforms = myRegionParams->getMaxNumTrans();
		else numxforms = myRegionParams->getNumTrans();
		int bs;
		bs = myMetadata->GetBlockSize();
		int i;
		for(i=0; i<3; i++) {
			int	s = numxforms;

			min_dim[i] = (int) ((float) (myRegionParams->getCenterPosition(i) >> s) - 0.5 
				- (((myRegionParams->getRegionSize(i) >> s) / 2.0)-1.0));
			max_dim[i] = (int) ((float) (myRegionParams->getCenterPosition(i) >> s) - 0.5 
				+ (((myRegionParams->getRegionSize(i) >> s) / 2.0)));
		

			min_bdim[i] = min_dim[i] / bs;
			max_bdim[i] = max_dim[i] / bs;
		}
/*
		int* center = myRegionParams->getCenterPos();
		int* dim = myRegionParams->getRegionSize();
		fprintf(stderr, "index == %d\n", myRegionParams->getCurrentTimestep());
		fprintf(stderr, "level == %d\n", level);
		fprintf(stderr, "flevels == %d\n", myRegionParams->getMaxStride()-1);
		fprintf(stderr, "stride == %d\n", myRegionParams->getStride());
		fprintf(stderr, "bs == %d\n", bs);
		fprintf(stderr, "dim == %d %d %d\n", dim[0], dim[1], dim[2]);
		fprintf(stderr, "center == %d %d %d\n", center[0], center[1], center[2]);
		fprintf(stderr, "min_dim == %d %d %d\n", min_dim[0], min_dim[1], min_dim[2]);
		fprintf(stderr, "max_dim == %d %d %d\n", max_dim[0], max_dim[1], max_dim[2]);

		*/
	
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

		//AN:  (2/10/05):  Calculate 'extents' to be the real coords in (0,1) that
		//the roi is mapped into.  First find the mapping of the full data array.  This
		//is mapped to the center of the cube with the largest dimension equal to 1.
		//Then determine what is the subvolume we are dealing with as a portion
		//of the full mapped data.
		float fullExtent[6];
		for (i = 0; i< 3; i++){
			fullExtent[i] = myRegionParams->getFullDataExtent(i);
			fullExtent[i+3] = myRegionParams->getFullDataExtent(i+3);
		}
		float maxCoordRange = Max(fullExtent[3]-fullExtent[0],Max( fullExtent[4]-fullExtent[1], fullExtent[5]-fullExtent[2]));
		//calculate the geometric extents of the dimensions in the unit cube:
		
		for (i = 0; i<3; i++) {
			int dim = (myRegionParams->getFullSize(i)>>numxforms) -1;
			assert (dim>= max_dim[i]);
			float extentRatio = (fullExtent[i+3]-fullExtent[i])/maxCoordRange;
			float minFull = 0.5f*(1.f - extentRatio);
			float maxFull = 0.5f*(1.f + extentRatio);
			extents[i] = minFull + ((float)min_dim[i]/(float)dim)*(maxFull-minFull);
			extents[i+3] = minFull + ((float)max_dim[i]/(float)dim)*(maxFull-minFull);
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
		min_subdim[0] = min_dim[0];
		min_subdim[1] = min_dim[1];
		min_subdim[2] = min_dim[2];
		max_subdim[0] = max_dim[0];
		max_subdim[1] = max_dim[1];
		max_subdim[2] = max_dim[2];
		//End of SetCurrentFile().


		//Now use the above values in calculating roi, extents, in order 
		//to call setRegion on driver:
		data_roi[0] = min_subdim[0];
		data_roi[1] = min_subdim[1];
		data_roi[2] = min_subdim[2];
		data_roi[3] = max_subdim[0];
		data_roi[4] = max_subdim[1];
		data_roi[5] = max_subdim[2];

	

		
	
		glMatrixMode(GL_MODELVIEW);
		//Get the matrix from gl, will send it to DVR Volumizer to use for its own purposes.
		
		glGetIntegerv(GL_DRAW_BUFFER, (GLint *) &buffer);
		glDrawBuffer(GL_BACK);
		glClearDepth(1);
		//Make background color black (0,0,0,0)
		//glClearColor(0.f,0.f,0.f,0.f);
		
		glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
		glDrawBuffer(buffer);
		
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

	if (driver->Render(
		(GLfloat *) matrix ) < 0){
		qWarning("Error calling Render()");
	}

	//myGLWindow->swapBuffers();

    glPopMatrix();
	if (isControlled) AnimationController::getInstance()->endRendering(winNum);
}


void VolumizerRenderer::
DrawVoxelWindow(unsigned fast)
{
	myGLWindow->makeCurrent();
	DrawVoxelScene(fast);
}

#endif //VOLUMIZER

