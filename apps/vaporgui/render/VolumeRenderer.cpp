//-- VolumeRender.cpp --------------------------------------------------------
//   
//                   Copyright (C)  2004
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
//----------------------------------------------------------------------------
//
//      File:           VolumeRenderer.h
//
//      Author:         Alan Norton
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           October 2004
//
//      Description:  Implementation of VolumeRenderer class
//
// Code was extracted from mdb, setup, vox in order to do simple
// volume rendering with volumizer.
//
//
// Original Author:
//      Ken Chin-Purcell (ken@ahpcrc.umn.edu)
//      Army High Performance Computing Research Center (AHPCRC)
//      University of Minnesota
//
//----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifndef WIN32
#include <unistd.h>
#endif

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>

#include <qgl.h>
#include <qmessagebox.h>

#include "VolumeRenderer.h"
#include "dvrparams.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"
#include "vizwin.h"
#include "vizwinmgr.h"
#include "glwindow.h"
#include "DVRTexture3d.h"
#include "DVRVolumizer.h"
#include "DVRDebug.h"
#include "renderer.h"
#include "animationcontroller.h"
#include "messagereporter.h"
#include "Stopwatch.h"

#include "mainform.h"
#include "command.h"
#include "session.h"
#include "glutil.h"

using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VolumeRenderer::VolumeRenderer(VizWin* vw) : Renderer(vw) 
{
  //Construct dvrvolumizer
  driver = create_driver("t3d",1);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VolumeRenderer::~VolumeRenderer()
{
  delete driver;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VolumeRenderer::initializeGL()
{
  myGLWindow->makeCurrent();
    
  if (driver->GraphicsInit() < 0) 
  {
    BailOut("OpenGL: Failure to initialize driver",__FILE__,__LINE__);
  }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VolumeRenderer::paintGL()
{
  DrawVoxelWindow(false);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
DVRBase* VolumeRenderer::create_driver(const char *name, int)
{
  int* argc = 0;
  char** argv = 0;
  DVRBase::DataType_T type = DVRBase::UINT8;
  DVRBase *driver = NULL;

  if (strcmp(name, "t3d") == 0) 
  {
    driver = new DVRTexture3d(argc, argv, type, 1);
  }

#ifdef VOLUMIZER
  else if (strcmp(name, "vz") == 0) 
  {
    driver = new DVRVolumizer(argc, argv, type, 1);
  }
#endif

  else if (strcmp(name, "debug") == 0)
  {
    driver = new DVRDebug(argc, argv, type, 1);
  }
  
#ifdef	STRETCHEDGRID
  else if (strcmp(name, "sg") == 0) 
  {
    driver = new DVRStretchedGrid(argc, argv, type, nthreads);
  }
#endif
  else 
  {
    qWarning("Invalid driver ");
    return NULL;
  }

  if (driver->GetErrCode() != 0) 
  { 
    MessageReporter::errorMsg("volume renderer error: %s\n", 
                              driver->GetErrMsg());
    driver->SetErrCode(0);
    return NULL;
  }

  return(driver);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VolumeRenderer::DrawVoxelScene(unsigned /*fast*/)
{
  float matrix[16];
  
  static float extents[6];
  static float minFull[3], maxFull[3];
  int max_dim[3];
  int min_dim[3];
  size_t max_bdim[3];
  size_t min_bdim[3];
  int data_roi[6];
  int i;
  
  DataMgr* myDataMgr = Session::getInstance()->getDataMgr();
  const Metadata* myMetadata = Session::getInstance()->getCurrentMetadata();
	
  // Nothing to do if there's no data source!
  if (!myDataMgr) return;
	
  int winNum = myVizWin->getWindowNum();
  RegionParams* myRegionParams = VizWinMgr::getInstance()->getRegionParams(winNum);
  AnimationParams* myAnimationParams = VizWinMgr::getInstance()->getAnimationParams(winNum);
	
  DvrParams* myDVRParams = VizWinMgr::getInstance()->getDvrParams(winNum);
	
  
  //AN:  (2/10/05):  Calculate 'extents' to be the real coords in (0,1) that
  //the roi is mapped into.  First find the mapping of the full data array.  This
  //is mapped to the center of the cube with the largest dimension equal to 1.
  //Then determine what is the subvolume we are dealing with as a portion
  //of the full mapped data.
  int numxforms;

  if (myVizWin->mouseIsDown()) 
  {
    numxforms = myRegionParams->getMinNumTrans();
  }
  else numxforms = myRegionParams->getNumTrans();

  //Whenever numxforms changes, we need to dirty the clut, since
  //that affects the opacity correction.
  if (numxforms != savedNumXForms)
  {
    myVizWin->setClutDirty(true);
    savedNumXForms = numxforms;
  }

  const size_t *bs = myMetadata->GetBlockSize();
		
  //Note that this function will be called again by the renderer
  myRegionParams->calcRegionExtents(min_dim, max_dim, min_bdim, max_bdim, numxforms, minFull, maxFull, extents);
  
  
  //Make the depth buffer writable
  glDepthMask(GL_TRUE);
  //and readable
  glEnable(GL_DEPTH_TEST);
  
  
  //This works around a volumizer/opengl bug!!!
  //If you issue a non-unit glColor before the volume rendering, it 
  //affects the subsequent volume rendering on Irix, but not on
  //Windows or Linux!
  glColor3f(1.f,1.f,1.f);	   
  
  
  //Save the coord trans matrix, to pass to volumizer
  glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) matrix);
  
  
  // set up region. Only need to do this if the data
  // roi changes, or if the datarange has changed.
  //
  if (myVizWin->regionIsDirty()|| myVizWin->dataRangeIsDirty()||myVizWin->regionIsNavigating()) 
  {
    
    myGLWindow->setRenderNew();
    int	rc;
    int nx,ny,nz;
    
    //Turn off error callback, look for memory allocation problem.
    Session::pauseErrorCallback();
	
    void* data = 
      (void*) myDataMgr->GetRegionUInt8(
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
        min_dim[i] -= bs[i]; max_dim[i] -= bs[i];
        min_bdim[i] -= 1; max_bdim[i] -= 1;
      }
    }
	
    nx = (max_bdim[0] - min_bdim[0] + 1) * bs[0];
    ny = (max_bdim[1] - min_bdim[1] + 1) * bs[1];
    nz = (max_bdim[2] - min_bdim[2] + 1) * bs[2];
	
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
      fprintf(stderr, "Error in DVRVolume::SetRegion\n");
      fflush(stderr);
      
      return;
    }
	
	
  }
  
  if (myVizWin->clutIsDirty()) {
    myGLWindow->setRenderNew();
    //Same table sets CLUT and OLUT
    //
    //driver->SetCLUT(myDVRParams->getClut());
    driver->SetOLUT(myDVRParams->getClut(), 
                    myRegionParams->getMaxNumTrans()-numxforms);

    bool shading = myDVRParams->getLighting();

    driver->SetLightingOnOff(shading);

    if (shading)
    {
      driver->SetLightingCoeff(myDVRParams->getDiffuseCoeff(),
                               myDVRParams->getAmbientCoeff(),
                               myDVRParams->getSpecularCoeff(),
                               myDVRParams->getExponent());
    }
    
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

#ifdef VOLUMIZER
  if (dynamic_cast<DVRVolumizer*>(driver))
  {
    glLoadIdentity();
  }
#endif

  //Make the z-buffer read-only for the volume data
  glDepthMask(GL_FALSE);
  if (driver->Render((GLfloat *) matrix ) < 0){
    MessageReporter::errorMsg("%s","Unable to Render");
    return;
  }
  
  //Colorbar is rendered with DVR renderer:
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
  
  myVizWin->setClutDirty(false);
  myVizWin->setDataRangeDirty(false);
  myVizWin->setRegionNavigating(false);
}


void VolumeRenderer::DrawVoxelWindow(unsigned fast)
{
#ifdef DEBUG_FPS

  static int frames = 0;
  static float seconds = 0;
  static Stopwatch frameTimer;

  frameTimer.reset();

  frames++;

  DrawVoxelScene(fast);

  seconds += frameTimer.read();

  if (frames % 100 == 0)
  {
    cout << (float)frames/seconds << " fps" << endl;
    frames=0;
    seconds=0;
  }

#else

  DrawVoxelScene(fast);

#endif
}
