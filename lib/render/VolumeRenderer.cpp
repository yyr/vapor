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
#include <qapplication.h>
#include <qcursor.h>
#include <qstring.h>
#include <qstatusbar.h>

#include "vapor/errorcodes.h"
#include "VolumeRenderer.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"

#include "glwindow.h"

#include "DVRLookup.h"
#include "DVRShader.h"
#include "DVRSpherical.h"
#ifdef VOLUMIZER
#include "DVRVolumizer.h"
#endif
#include "DVRDebug.h"
#include "params.h"
#include "renderer.h"

#include "Stopwatch.h"


#include "glutil.h"
#include "dvrparams.h"
#include "vapor/MyBase.h"

using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VolumeRenderer::VolumeRenderer(GLWindow* glw, DvrParams::DvrType type, RenderParams* rp) 
  : Renderer(glw, rp),
    driver(NULL),
    _type(type),
    _frames(0),
    _seconds(0)
	
	
{
  //Construct dvrvolumizer
  driver = create_driver(type, 1);
  clutDirtyBit = false;
  datarangeDirtyBit = false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VolumeRenderer::~VolumeRenderer()
{
  delete driver;
  driver = NULL;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VolumeRenderer::initializeGL()
{
  myGLWindow->makeCurrent();
    
  if (driver->GraphicsInit() < 0) 
  {
	  Params::BailOut("OpenGL: Failure to initialize driver",__FILE__,__LINE__);
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
bool VolumeRenderer::hasLighting()
{
  if (driver)
  {
    return driver->HasLighting();
  }

  return false;
}


//----------------------------------------------------------------------------
// static
//----------------------------------------------------------------------------
bool VolumeRenderer::supported(DvrParams::DvrType type)
{
  switch (type)
  {
     case DvrParams::DVR_TEXTURE3D_LOOKUP:
     {
       return DVRLookup::supported();
     }

     case DvrParams::DVR_TEXTURE3D_SHADER:
     {
       return DVRShader::supported();
     }

     case DvrParams::DVR_SPHERICAL_SHADER:
     {
#    ifdef SPHERICAL_GRID
       return DVRSpherical::supported();
#    else
       return false;
#    endif
     }

     case DvrParams::DVR_VOLUMIZER:
     {
#    ifdef VOLUMIZER
       return true;
#    else
       return false;
#    endif
     }

     case DvrParams::DVR_DEBUG:
     {
#    ifdef DEBUG
       return true;
#    else
       return false;
#    endif
     }

     case DvrParams::DVR_STRETCHED_GRID:
     {
#    ifdef STRETCHEDGRID
          return true;
#    else
          return false;
#    endif
     }
	default:
	break;
  }

  return false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
DVRBase* VolumeRenderer::create_driver(DvrParams::DvrType dvrType, int)
{
  int argc = 0;
  char** argv = 0;
  DVRBase::DataType_T type = DVRBase::UINT8;
  DVRBase *driver = NULL;

  if (dvrType == DvrParams::DVR_TEXTURE3D_LOOKUP) 
  {
    driver = new DVRLookup(type, 1);
  }

  else if (dvrType == DvrParams::DVR_TEXTURE3D_SHADER)
  {
    driver = new DVRShader(type, 1);
  }

  else if (dvrType == DvrParams::DVR_SPHERICAL_SHADER)
  {
    driver = new DVRSpherical(type, 1);
  }

#ifdef VOLUMIZER
  else if (dvrType == DvrParams::DVR_VOLUMIZER)
  {
    driver = new DVRVolumizer(&argc, argv, type, 1);
  }
#endif

  else if (dvrType == DvrParams::DVR_DEBUG)
  {
    driver = new DVRDebug(&argc, argv, type, 1);
  }
  
#ifdef	DVR_STRETCHEDGRID
  else if (dvrType == DvrParams::DVR_STRETCHED_GRID)
  {
    driver = new DVRStretchedGrid(&argc, argv, type, nthreads);
  }
#endif
  else 
  {
    SetErrMsg("Invalid driver ");
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
  
  size_t max_dim[3];
  size_t min_dim[3];
  size_t max_bdim[3];
  size_t min_bdim[3];
  int data_roi[6];
  int datablock[6];
  int i;
  
  DataMgr* myDataMgr = DataStatus::getInstance()->getDataMgr();
  const Metadata* myMetadata = DataStatus::getInstance()->getCurrentMetadata();
	
  // Nothing to do if there's no data source!
  if (!myDataMgr) return;
	
  RegionParams* myRegionParams = myGLWindow->getActiveRegionParams();
  int timeStep = myGLWindow->getActiveAnimationParams()->getCurrentFrameNumber();
  
	
  DvrParams* myDVRParams = (DvrParams*)currentRenderParams;
  //This is no longer the case, because of multiple instancing:
  //assert(myDVRParams == myGLWindow->getActiveDvrParams());
  int varNum = myDVRParams->getSessionVarNum();
	
  
  //AN:  (2/10/05):  Calculate 'extents' to be the real coords in (0,1) that
  //the roi is mapped into.  First find the mapping of the full data array.  This
  //is mapped to the center of the cube with the largest dimension equal to 1.
  //Then determine what is the subvolume we are dealing with as a portion
  //of the full mapped data.
  int numxforms;

  if (myGLWindow->mouseIsDown()) 
  {
    numxforms = 0;
  }
  else numxforms = myDVRParams->getNumRefinements();

  //Whenever numxforms changes, we need to dirty the clut, since
  //that affects the opacity correction.
  if (numxforms != savedNumXForms)
  {
    setClutDirty();
    savedNumXForms = numxforms;
  }

  const size_t *bs = myMetadata->GetBlockSize();
  //qWarning(" calculating region bounds for next render");
  bool regionValid = myRegionParams->getAvailableVoxelCoords(numxforms, min_dim, max_dim, min_bdim, max_bdim, 
	  timeStep,&varNum, 1);
	
  if(!regionValid) {
	  const char* vname = DataStatus::getInstance()->getVariableName(varNum).c_str();
	  SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"Volume data unavailable for refinement level %d of variable %s, at current timestep", 
		  numxforms, vname);
	  return;
  }
  RegionParams::convertToBoxExtentsInCube(numxforms, min_dim, max_dim, extents);    
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
  if (regionValid&&(myGLWindow->regionIsDirty()
	  || datarangeIsDirty()||myGLWindow->dvrRegionIsNavigating()
	  || myGLWindow->animationIsDirty())) 
  {
    //Check if the region/resolution is too big:
	  int numMBs = RegionParams::getMBStorageNeeded(extents, extents+3, numxforms);
	  int cacheSize = DataStatus::getInstance()->getCacheMB();
	  if (numMBs > (int)(0.75*cacheSize)){
		  MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small for current region and resolution.\n%s",
			  "Lower the refinement level, reduce the region size, or increase the cache size.");
		  return;
	  }
    myGLWindow->setRenderNew();
    int	rc;
    int nx,ny,nz;
   
	//qWarning("Requesting region from dataMgr");
    //Turn off error callback, look for memory allocation problem.
    
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	const char* varname = (DataStatus::getInstance()->getVariableName(myDVRParams->getSessionVarNum()).c_str());
    void* data = 
      (void*) myDataMgr->GetRegionUInt8(
                                        timeStep,
                                        varname,
                                        numxforms,
                                        min_bdim,
                                        max_bdim,
                                        myDVRParams->getCurrentDatarange(),
                                        0 //Don't lock!
                                        );
    //Turn it back on:
    
	QApplication::restoreOverrideCursor();
	
    if (!data){
      SetErrMsg("Unable to obtain volume data\n");
      return;
    }

    datablock[0] = (int)(min_bdim[0]*bs[0]);
    datablock[1] = (int)(min_bdim[1]*bs[1]);
    datablock[2] = (int)(min_bdim[2]*bs[2]);
    datablock[3] = (int)((max_bdim[0]+1)*bs[0]-1);
    datablock[4] = (int)((max_bdim[1]+1)*bs[1]-1);
    datablock[5] = (int)((max_bdim[2]+1)*bs[2]-1);

    // make subregion origin (0,0,0)
    // Note that this doesn't affect the calc of nx,ny,nz.
    // Also, save original dims, will need them to find extents.
    for(i=0; i<3; i++) {
      while(min_bdim[i] > 0) {
        min_dim[i] -= bs[i]; max_dim[i] -= bs[i];
        min_bdim[i] -= 1; max_bdim[i] -= 1;
      }
    }
	
    nx = (int)((max_bdim[0] - min_bdim[0] + 1) * bs[0]);
    ny = (int)((max_bdim[1] - min_bdim[1] + 1) * bs[1]);
    nz = (int)((max_bdim[2] - min_bdim[2] + 1) * bs[2]);
	
    data_roi[0] = (int)min_dim[0];
    data_roi[1] = (int)min_dim[1];
    data_roi[2] = (int)min_dim[2];
    data_roi[3] = (int)max_dim[0];
    data_roi[4] = (int)max_dim[1];
    data_roi[5] = (int)max_dim[2];

	//qWarning("setting region in renderer");
    rc = driver->SetRegion(data,
                           nx, ny, nz,
                           data_roi, extents,
                           datablock, numxforms
                           );
    
    if (rc < 0) {
      fprintf(stderr, "Error in DVRVolume::SetRegion\n");
      fflush(stderr);
      
      return;
    }
	
	
  }
  
  if (clutIsDirty()) {
    myGLWindow->setRenderNew();
    //Same table sets CLUT and OLUT
    //

    if (_type == DvrParams::DVR_VOLUMIZER)
    {
      driver->SetCLUT(myDVRParams->getClut());
    }
	
    driver->SetOLUT(myDVRParams->getClut(), 
                    myMetadata->GetNumTransforms() - numxforms);
  }

  if (myGLWindow->lightingIsDirty())
  {
    ViewpointParams *vpParams = myGLWindow->getActiveViewpointParams();
    
    bool shading = myDVRParams->getLighting();

    driver->SetLightingOnOff(shading);

    if (shading)
    {
      driver->SetLightingCoeff(vpParams->getDiffuseCoeff(0),
                               vpParams->getAmbientCoeff(),
                               vpParams->getSpecularCoeff(0),
                               vpParams->getExponent());

      if (vpParams->getNumLights() >= 1)
      {
        driver->SetLightingLocation(vpParams->getLightDirection(0));
      }
    }
    
    // Attenuation is not supported yet...
    // if (myDVRParams->attenuationIsDirty()) 
    // {
    //   driver->SetAmbient(myDVRParams->getAmbientAttenuation(),
    //                      myDVRParams->getAmbientAttenuation(),
    //                      myDVRParams->getAmbientAttenuation());
    //   driver->SetDiffuse(myDVRParams->getDiffuseAttenuation(),
    //                      myDVRParams->getDiffuseAttenuation(),
    //                      myDVRParams->getDiffuseAttenuation());
    //   driver->SetSpecular(myDVRParams->getSpecularAttenuation(),
    //                       myDVRParams->getSpecularAttenuation(),
    //                       myDVRParams->getSpecularAttenuation());
    //   myDVRParams->setAttenuationDirty(false);
    //
  }
  // Modelview matrix
  //
  glMatrixMode(GL_MODELVIEW);

  if (_type == DvrParams::DVR_VOLUMIZER)
  {
    glLoadIdentity();
  }
  
  //Make the z-buffer read-only for the volume data
  glDepthMask(GL_FALSE);
  //qWarning("Starting render");
  if (driver->Render((GLfloat *) matrix ) < 0){
    SetErrMsg("Unable to Render");
    return;
  }
  //qWarning("Render done");
  
  //Colorbar is rendered with DVR renderer:
  if(myGLWindow->colorbarIsEnabled()){
    //Now go to default 2D window
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
	
    renderColorscale(myGLWindow->colorbarIsDirty()||clutIsDirty()||datarangeIsDirty());
	
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
  }
  
  clutDirtyBit = false;
  datarangeDirtyBit = false;
  
  myGLWindow->setDvrRegionNavigating(false);
  myGLWindow->setLightingDirty(false);
}


void VolumeRenderer::DrawVoxelWindow(unsigned fast)
{
#ifdef BENCHMARKING

  static Stopwatch frameTimer;

  frameTimer.reset();

  do
  {
    _frames++;
    
    DrawVoxelScene(fast);
    glFinish();

  } while (frameTimer.read() == 0.0);

  _seconds += frameTimer.read();

  QString msg = QString("%1 fps").arg(_frames/_seconds);

  emit statusMessage(msg);
    
#elif defined PROFILING

  static Stopwatch frameTimer;

  frameTimer.reset();

  _frames++;

  DrawVoxelScene(fast);
  glFinish();

  _seconds += frameTimer.read();

  if (_frames % 100 == 0)
  {
    if (_seconds != 0)
    {
      QString msg = QString("%1 fps").arg(_frames/_seconds);

      emit statusMessage(msg);
    }

    _frames  = 0;
    _seconds = 0;
  }

#else

  DrawVoxelScene(fast);

#endif
}
