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

#include <GL/glew.h>
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

#include "vapor/MetadataSpherical.h"
#include "VolumeRenderer.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"
#include "transferfunction.h"

#include "glwindow.h"

#include "DVRLookup.h"
#include "DVRShader.h"
#include "DVRRayCaster.h"
#include "DVRRayCaster2Var.h"
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
#include "ParamsIso.h"
#include "vapor/MyBase.h"

// Floating-point equivalience comparison
#define EPSILON 5.960464478e-8
#define FLTEQ(a,b) (fabs(a-b) < fabs(a*EPSILON) && fabs(a-b) < fabs(b*EPSILON))

using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VolumeRenderer::VolumeRenderer(GLWindow* glw, DvrParams::DvrType type, RenderParams* rp, string name) 
  : Renderer(glw, rp, name),
    _driver(NULL),
    _type(type),
    _frames(0),
    _seconds(0),
	_userTextureSize(0),
	_userTextureSizeIsSet(false)
	
	
{

  //Construct dvrvolumizer
  _driver = create_driver(type, 1);
  
  datarangeDirtyBit = false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VolumeRenderer::~VolumeRenderer()
{
  if (_driver) delete _driver;
  _driver = NULL;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void VolumeRenderer::initializeGL()
{
  myGLWindow->makeCurrent();
    
  if (_driver->GraphicsInit() < 0) 
  {
	  setAllBypass(true);
	  MyBase::SetErrMsg(VAPOR_ERROR_DRIVER_FAILURE,"Failure to initialize OpenGL renderer.  Possible remedies:\n %s %s %s",
		  "   Specify a different Renderer Type,\n",
		  "   Install newer graphics drivers on your system, or\n",
		  "   Install one of the recommended graphics cards."
		  );
	  
      initialized = false;
  } else
	initialized = true;
  return;
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
  if (_driver)
  {
    return _driver->HasLighting();
  }

  return false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool VolumeRenderer::hasPreintegration()
{
  if (_driver)
  {
    return _driver->HasPreintegration();
  }

  return false;
}


//----------------------------------------------------------------------------
// static
//----------------------------------------------------------------------------
bool VolumeRenderer::supported(DvrParams::DvrType type)
{
  bool spherical = DataStatus::getInstance()->sphericalTransform();

  if (spherical && type != DvrParams::DVR_SPHERICAL_SHADER)
  {
    return false;
  }

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
       return DVRSpherical::supported() && spherical;
#    else
       return false;
#    endif
     }

     case DvrParams::DVR_RAY_CASTER:
     {
       return DVRRayCaster::supported();
     }

     case DvrParams::DVR_RAY_CASTER_2_VAR:
     {
       return DVRRayCaster2Var::supported();
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
  DVRBase *driver = NULL;
  GLenum format = GL_LUMINANCE;

  if (dvrType == DvrParams::DVR_TEXTURE3D_LOOKUP) 
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    _voxelType = rp->getNumBits() == 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    driver = new DVRLookup(_voxelType, 1, this);
  }
  else if (dvrType == DvrParams::DVR_TEXTURE3D_SHADER)
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    _voxelType = rp->getNumBits() == 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    GLint internalFormat = rp->getNumBits() == 8 ? GL_LUMINANCE8 : GL_LUMINANCE16;
    driver = new DVRShader(internalFormat, format, _voxelType, 1, this);
  }
  else if (dvrType == DvrParams::DVR_SPHERICAL_SHADER)
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    _voxelType = rp->getNumBits() == 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    GLint internalFormat = rp->getNumBits() == 8 ? GL_LUMINANCE8 : GL_LUMINANCE16;
    driver = new DVRSpherical(internalFormat, format, _voxelType, 1, this);
  }
  else if (dvrType == DvrParams::DVR_RAY_CASTER)
  {
	ParamsIso *rp = (ParamsIso *) currentRenderParams;
    _voxelType = rp->GetNumBits() == 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    GLint internalFormat = rp->GetNumBits() == 8 ? GL_LUMINANCE8 : GL_LUMINANCE16;
    driver = new DVRRayCaster(internalFormat, format, _voxelType, 1,this);
  }
  else if (dvrType == DvrParams::DVR_RAY_CASTER_2_VAR)
  {
    GLenum format = GL_LUMINANCE_ALPHA;
	ParamsIso *rp = (ParamsIso *) currentRenderParams;
    _voxelType = rp->GetNumBits() == 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    GLint internalFormat = rp->GetNumBits() == 8 ? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE16_ALPHA16;
    driver = new DVRRayCaster2Var(internalFormat, format, _voxelType, 1,this);
  }

#ifdef VOLUMIZER
  else if (dvrType == DvrParams::DVR_VOLUMIZER)
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    _voxelType = rp->getNumBits() == 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    driver = new DVRVolumizer(&argc, argv, _voxelType, 1, this);
  }
#endif

  else if (dvrType == DvrParams::DVR_DEBUG)
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    _voxelType = rp->getNumBits() == 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    driver = new DVRDebug(&argc, argv, 1, this);
  }
  
#ifdef	DVR_STRETCHEDGRID
  else if (dvrType == DvrParams::DVR_STRETCHED_GRID)
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    _voxelType = rp->getNumBits() == 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    driver = new DVRStretchedGrid(&argc, argv, _voxelType, nthreads, this);
  }
#endif
  else 
  {
	  setAllBypass(true);
	  MyBase::SetErrMsg(VAPOR_ERROR_DVR,"Invalid driver ");
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
	static float padded_extents[6];
	  
	size_t max_dim[3];
	size_t min_dim[3];
	size_t max_pad_dim[3];
	size_t min_pad_dim[3];
	size_t max_bdim[3];
	size_t min_bdim[3];
	int data_roi[6];
	int datablock[6];
	int i;


	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();

	// Nothing to do if there's no data source!
	if (!dataMgr) return;
	
	RegionParams* myRegionParams = myGLWindow->getActiveRegionParams();
	int timeStep = myGLWindow->getActiveAnimationParams()->getCurrentFrameNumber();

	//If we have partial bypass, the data is only available at low res, so exit if
	//the mouse is not down:
	if (currentRenderParams->doBypass(timeStep) && !myGLWindow->mouseIsDown() && !myGLWindow->spinning()) 
		return;

	  
	ViewpointParams *vpParams = myGLWindow->getActiveViewpointParams();  
	DataStatus* ds = DataStatus::getInstance();
		
	
	int varNum = currentRenderParams->getSessionVarNum();

	if (myGLWindow->viewportIsDirty()) {
		const GLint* viewport = myGLWindow->getViewport();
		_driver->Resize(viewport[2], viewport[3]);
	}
	//If the extents are varying, we need to change the clipping planes whenever
	//the time step changes.
	if (myGLWindow->projMatrixIsDirty()||
		(myGLWindow->animationIsDirty() && myRegionParams->extentsAreVarying())) {
		GLfloat nearplane, farplane;
		myGLWindow->getNearFarClippingPlanes(&nearplane, &farplane);
		_driver->SetNearFar(nearplane, farplane);
		
		_driver->calculateSampling();
		//set clut dirty to force recalc of sampling rate
		setClutDirty();
	}
	
  
  //AN:  (2/10/05):  Calculate 'extents' to be the real coords in (0,1) that
  //the roi is mapped into.  First find the mapping of the full data array.  This
  //is mapped to the center of the cube with the largest dimension equal to 1.
  //Then determine what is the subvolume we are dealing with as a portion
  //of the full mapped data.
	int numxforms;

	if (myGLWindow->mouseIsDown() || myGLWindow->spinning()) 
	{
		numxforms = Min(currentRenderParams->getNumRefinements(),
			DataStatus::getInteractiveRefinementLevel());
		if (numxforms < currentRenderParams->getNumRefinements()){
			_driver->SetRenderFast(true);

			// Need update sampling rate & opacity correction 
			setClutDirty(); 
			myGLWindow->setDvrRegionNavigating(true);
		}
	} else {

		numxforms = currentRenderParams->getNumRefinements();
	    
		if (_driver->GetRenderFast())
		{
			_driver->SetRenderFast(false);

			// Need update sampling rate & opacity correction
			setClutDirty();
			//And force rerender
			myGLWindow->setDvrRegionNavigating(true);
		}
	}

	//Whenever numxforms changes, we need to dirty the clut, since
	//that affects the opacity correction.
	if (numxforms != savedNumXForms)
	{
		setClutDirty();
		savedNumXForms = numxforms;
	}

	const size_t *bs = dataMgr->GetBlockSize();

	  
	//Loop if user accepts lower resolution:
	
	int availRefLevel = myRegionParams->getAvailableVoxelCoords(numxforms, min_dim, max_dim, min_bdim, max_bdim, 
		timeStep,&varNum, 1);

	if(availRefLevel < 0) {
		//The data is not available at the required ref level.
		//Set full bypass if the interactiveRefinementLevel is not available:
		//Set partial bypass if interactive level is available
		//Provide an error message if bypass was not already set:
		bool doMsg = true;
		if ((DataStatus::getInstance()->maxXFormPresent(varNum, timeStep)) < DataStatus::getInteractiveRefinementLevel())
			setBypass(timeStep);
		else {
			doMsg = !doBypass(timeStep);
			setPartialBypass(timeStep);
		}
		if (doMsg) {
			const char* varname = ds->getVariableName(varNum).c_str();
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,
				"Data unavailable for variable %s\n at refinement level %d and current time step",
				varname, numxforms);
		}
		return;
	}

	if (availRefLevel < numxforms){
		numxforms = availRefLevel;
		setClutDirty();
		savedNumXForms = numxforms;
	}
	RegionParams::convertToStretchedBoxExtentsInCube(numxforms, min_dim, max_dim, extents);    
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	//and readable
	glEnable(GL_DEPTH_TEST);
	  
	
	//If you issue a non-unit glColor before the volume rendering, it 
	//affects the subsequent volume rendering on Irix, but not on
	//Windows or Linux!
	glColor3f(1.f,1.f,1.f);	   
	  
	  
	//Save the coord trans matrix, to pass to volumizer
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) matrix);
	  
	bool userTextureSizeIsSet = DataStatus::getInstance()->textureSizeIsSpecified();
	int userTextureSize = DataStatus::getInstance()->getTextureSize();
	bool forceReload = false;

	if ((userTextureSizeIsSet != _userTextureSizeIsSet) || 
		(userTextureSize != _userTextureSize)) { 

		_userTextureSize = userTextureSize;
		_userTextureSizeIsSet = userTextureSizeIsSet;
		forceReload = true;

		if (_userTextureSizeIsSet) {
			_driver->SetMaxTexture(_userTextureSize);
		}
		else {
			_driver->SetMaxTexture(0);
		}
	}
			
  
	// set up region. Only need to do this if the data
	// roi changes, or if the datarange has changed.
	//
	if (myGLWindow->regionIsDirty() || forceReload
		|| datarangeIsDirty() || myGLWindow->dvrRegionIsNavigating()
		|| myGLWindow->animationIsDirty()) 
	{
		//Check if the region/resolution is too big:
		int numMBs = RegionParams::getMBStorageNeeded(extents, extents+3, numxforms);
		int cacheSize = DataStatus::getInstance()->getCacheMB();
		if (numMBs > (int)(0.75*cacheSize)){
			setAllBypass(true);
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small \nfor current region and resolution.\n%s",
				"Lower the refinement level, reduce region size,\nor increase the cache size.");
			return;
		}
		myGLWindow->setRenderNew();
		int	rc;
		int nx,ny,nz;
	   
		//qWarning("Requesting region from dataMgr");
		//Turn off error callback, look for memory allocation problem.
		
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		const char* varname = (DataStatus::getInstance()->getVariableName(currentRenderParams->getSessionVarNum()).c_str());


		void* data = _getRegion(
			dataMgr, currentRenderParams, myRegionParams, timeStep,
			varname, numxforms, min_bdim, max_bdim
		);

		//Turn it back on:
	    
		QApplication::restoreOverrideCursor();
		
		if (!data){
			//Only set bypass if the data is not available at the interactiveRefLevel
			//Otherwise set partial bypass
			if (numxforms <= DataStatus::getInteractiveRefinementLevel())
				setBypass(timeStep);
			else setPartialBypass(timeStep);
			if (ds->warnIfDataMissing()){
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Volume data unavailable\nfor refinement level %d\nof variable %s, at current timestep.", 
					numxforms, varname);
			}
			ds->setDataMissing(timeStep, numxforms, currentRenderParams->getSessionVarNum());
			setBypass(timeStep);
			return;
		}

		datablock[0] = (int)(min_bdim[0]*bs[0]);
		datablock[1] = (int)(min_bdim[1]*bs[1]);
		datablock[2] = (int)(min_bdim[2]*bs[2]);
		datablock[3] = (int)((max_bdim[0]+1)*bs[0]-1);
		datablock[4] = (int)((max_bdim[1]+1)*bs[1]-1);
		datablock[5] = (int)((max_bdim[2]+1)*bs[2]-1);

		//
		// Calculate the extents of the volume when it's padded by a single
		// voxel.
		//    
		min_pad_dim[0] = min_dim[0] + 1; 
		min_pad_dim[1] = min_dim[1] + 1; 
		min_pad_dim[2] = min_dim[2] + 1;
		max_pad_dim[0] = max_dim[0] - 1; 
		max_pad_dim[1] = max_dim[1] - 1; 
		max_pad_dim[2] = max_dim[2] - 1;

		RegionParams::convertToStretchedBoxExtentsInCube(numxforms, 
												min_pad_dim, max_pad_dim, 
												padded_extents);
								
	   
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

		if (_type == DvrParams::DVR_SPHERICAL_SHADER)
		{
			vector<bool> clip(3, false);
			const vector<long> &periodic = dataMgr->GetPeriodicBoundary();

			data_roi[0] = (int)min_dim[0];
			data_roi[1] = (int)min_dim[1];
			data_roi[2] = (int)min_dim[2];
			data_roi[3] = (int)max_dim[0];
			data_roi[4] = (int)max_dim[1];
			data_roi[5] = (int)max_dim[2];

			float* exts = myRegionParams->getRegionExtents(timeStep);
			for (int k = 0; k< 6; k++) extents[k] = exts[k];

			clip[0] = !(periodic[0] &&
						FLTEQ(extents[0], dataMgr->GetExtents()[0]) &&
						FLTEQ(extents[3], dataMgr->GetExtents()[3]));
			clip[1] = !(periodic[1] && 
						FLTEQ(extents[1], dataMgr->GetExtents()[1]) &&
						FLTEQ(extents[4], dataMgr->GetExtents()[4]));
			clip[2] = !(periodic[2] && 
						FLTEQ(extents[2], dataMgr->GetExtents()[2]) &&
						FLTEQ(extents[5], dataMgr->GetExtents()[5]));

			rc = _driver->SetRegionSpherical(data,
											nx, ny, nz,
											data_roi, 
											extents, 
											datablock, 
											numxforms,
											dataMgr->GetGridPermutation(),
											clip);
		}
		else
		{
			//
			// Pad the roi with a single voxel in each axis. This ensures
			// data will be available for gradient calculations. 
			//
			data_roi[0] = (int)min_dim[0]+1;
			data_roi[1] = (int)min_dim[1]+1;
			data_roi[2] = (int)min_dim[2]+1;
			data_roi[3] = (int)max_dim[0]-1;
			data_roi[4] = (int)max_dim[1]-1;
			data_roi[5] = (int)max_dim[2]-1;
		      
			rc = _driver->SetRegion(data,
									nx, ny, nz,
									data_roi, padded_extents,
									datablock, numxforms
									);
		}
	    
		if (rc < 0) {
			setBypass(timeStep);
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, "Error in DVRVolume::SetRegion");
			return;
		}
		
		
	}

	_updateDriverRenderParamsSpec(currentRenderParams);

	// Update the DVR's view
	//
	_driver->SetView(vpParams->getCameraPos(), vpParams->getViewDir());

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
	if (_driver->Render((GLfloat *) matrix ) < 0){
		setBypass(timeStep);
		SetErrMsg(VAPOR_ERROR_DVR, "Unable to volume render");
		return;
	}
	//qWarning("Render done");
	  
	//Colorbar is rendered with DVR renderer:
	if(myGLWindow->colorbarIsEnabled() &&
		currentRenderParams->GetParamsBaseTypeId() == Params::GetTypeFromTag(Params::_dvrParamsTag)){
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
	  
	clearClutDirty();
	clearDatarangeDirty();
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

void *VolumeRenderer::_getRegion(
	DataMgr *data_mgr, RenderParams *rp, RegionParams *reg_params,
	size_t ts, const char *varname, int numxforms, 
	const size_t min[3], const size_t max[3]
) {
	void *data;

	if (_voxelType == GL_UNSIGNED_BYTE) {
		data =  data_mgr->GetRegionUInt8(
			ts, varname, numxforms, -1, min, max,
			rp->getCurrentDatarange(),
			0 // Don't lock!
		);
	}
	else {
		data = data_mgr->GetRegionUInt16(
			ts, varname, numxforms, -1, min, max,
			rp->getCurrentDatarange(),
			0 // Don't lock!
		);
	}
	return(data);
}


void VolumeRenderer::_updateDriverRenderParamsSpec(
	RenderParams *rp
) {
	DvrParams *myDVRParams = (DvrParams *) rp;

	if (clutIsDirty()) {
		DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();

		myGLWindow->setRenderNew();

		bool preint = myDVRParams->getPreIntegration();

		_driver->SetPreintegrationOnOff(preint);

		if (_type == DvrParams::DVR_VOLUMIZER) {
			_driver->SetCLUT(myDVRParams->getClut());
		}

		_driver->SetOLUT(myDVRParams->getClut(), 
		dataMgr->GetNumTransforms() - savedNumXForms);
	}
	
	
	if (myGLWindow->lightingIsDirty()) {
		bool shading = myDVRParams->getLighting();

		_driver->SetLightingOnOff(shading);

		if (shading) {
			  ViewpointParams *vpParams = myGLWindow->getActiveViewpointParams();  
			_driver->SetLightingCoeff(
				vpParams->getDiffuseCoeff(0), vpParams->getAmbientCoeff(),
				vpParams->getSpecularCoeff(0), vpParams->getExponent()
			);

			if (vpParams->getNumLights() >= 1) {
				_driver->SetLightingLocation(vpParams->getLightDirection(0));
			}
		}

		// Attenuation is not supported yet...
		// if (myDVRParams->attenuationIsDirty()) 
		// {
		//   _driver->SetAmbient(myDVRParams->getAmbientAttenuation(),
		//                      myDVRParams->getAmbientAttenuation(),
		//                      myDVRParams->getAmbientAttenuation());
		//   _driver->SetDiffuse(myDVRParams->getDiffuseAttenuation(),
		//                      myDVRParams->getDiffuseAttenuation(),
		//                      myDVRParams->getDiffuseAttenuation());
		//   _driver->SetSpecular(myDVRParams->getSpecularAttenuation(),
		//                       myDVRParams->getSpecularAttenuation(),
		//                       myDVRParams->getSpecularAttenuation());
		//   myDVRParams->setAttenuationDirty(false);
		//}
	}
}
