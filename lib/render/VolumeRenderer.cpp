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

#include <vapor/MetadataSpherical.h>
#include "VolumeRenderer.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"
#include "transferfunction.h"

#include "glwindow.h"

//#include "DVRLookup.h"
#include "DVRShader.h"
#include "DVRRayCaster.h"
#include "DVRSpherical.h"
#include "DVRDebug.h"
#include "params.h"
#include "renderer.h"

#include "Stopwatch.h"


#include "glutil.h"
#include "dvrparams.h"
#include "ParamsIso.h"
#include <vapor/MyBase.h>

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
	if (_colorbarTexid) glDeleteTextures(1,&_colorbarTexid);
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
	glGenTextures(1,&_colorbarTexid);
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
 //      return DVRLookup::supported();
     }

     case DvrParams::DVR_TEXTURE3D_SHADER:
     {
       return DVRShader::supported();
     }

     case DvrParams::DVR_SPHERICAL_SHADER:
     {
       return DVRSpherical::supported() && spherical;
     }

     case DvrParams::DVR_RAY_CASTER:
     {
       return DVRRayCaster::supported();
     }

     case DvrParams::DVR_RAY_CASTER_2_VAR:
     {
       return DVRRayCaster::supported();
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

#ifdef	DEAD
  if (dvrType == DvrParams::DVR_TEXTURE3D_LOOKUP) 
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    _voxelType = rp->getNumBits() == 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    driver = new DVRLookup(_voxelType, 1);
  }
  else 
#endif

  if (dvrType == DvrParams::DVR_TEXTURE3D_SHADER)
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    driver = new DVRShader(rp->getNumBits(), 1, myGLWindow->getShaderMgr(), 1);
  }
  else if (dvrType == DvrParams::DVR_SPHERICAL_SHADER)
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    driver = new DVRSpherical(rp->getNumBits(), 1, myGLWindow->getShaderMgr(), 1);
  }
  else if (dvrType == DvrParams::DVR_RAY_CASTER)
  {
	ParamsIso *rp = (ParamsIso *) currentRenderParams;
    driver = new DVRRayCaster(rp->GetNumBits(), 1, myGLWindow->getShaderMgr(), 1);
  }
  else if (dvrType == DvrParams::DVR_RAY_CASTER_2_VAR)
  {
	ParamsIso *rp = (ParamsIso *) currentRenderParams;
    driver = new DVRRayCaster(rp->GetNumBits(), 2, myGLWindow->getShaderMgr(), 1);
  }
  else if (dvrType == DvrParams::DVR_DEBUG)
  {
	DvrParams *rp = (DvrParams *) currentRenderParams;
    driver = new DVRDebug(&argc, argv, 1);
  }
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
void VolumeRenderer::DrawVoxelScene(unsigned fast)
{
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

//cerr << "transforming everything to unit box coords :-(\n";
	float sceneScaleFactor = 1.f/ViewpointParams::getMaxStretchedCubeSide();
	glScalef(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);
	float* transVec = ViewpointParams::getMinStretchedCubeCoords();
	glTranslatef(-transVec[0],-transVec[1], -transVec[2]);

//vector <double> gextents = dataMgr->GetExtents(timeStep);
//glTranslatef(
//gextents[0]-lextents[0],
//gextents[1]-lextents[1],
//gextents[2]-lextents[2]
//);
//cout << "TRANSLATE " << 
//gextents[0]-lextents[0] << " " <<
//gextents[1]-lextents[1] << " " <<
//gextents[2]-lextents[2] << " " << endl;


	const float* scales = DataStatus::getInstance()->getStretchFactors();
	glScalef(scales[0], scales[1], scales[2]);

			
	
	int varNum = currentRenderParams->getSessionVarNum();

	if (myGLWindow->vizIsDirty(ViewportBit)) {
		const GLint* viewport = myGLWindow->getViewport();
		_driver->Resize(viewport[2], viewport[3]);
	}
  
  //AN:  (2/10/05):  Calculate 'extents' to be the real coords in (0,1) that
  //the roi is mapped into.  First find the mapping of the full data array.  This
  //is mapped to the center of the cube with the largest dimension equal to 1.
  //Then determine what is the subvolume we are dealing with as a portion
  //of the full mapped data.
	int reflevel, lod;
	lod = currentRenderParams->GetCompressionLevel();
	if (ds->useLowerAccuracy())
		lod = Min(lod,ds->maxLODPresent3D(varNum, timeStep));
	
	if (myGLWindow->mouseIsDown() || myGLWindow->spinning()) 
	{
		reflevel = Min(currentRenderParams->GetRefinementLevel(),
            DataStatus::getInteractiveRefinementLevel());

		if (! _driver->GetRenderFast()) {
			_driver->SetRenderFast(true);

			// Need update sampling rate & opacity correction 
			setClutDirty(); 
			myGLWindow->setDirtyBit(NavigatingBit,true);
		}
	} else {

		reflevel = currentRenderParams->GetRefinementLevel();
		if (_driver->GetRenderFast())
		{
			_driver->SetRenderFast(false);

			// Need update sampling rate & opacity correction
			setClutDirty();
			//And force rerender
			myGLWindow->setDirtyBit(NavigatingBit,true);
		}
	}

	//Whenever reflevel changes, we need to dirty the clut, since
	//that affects the opacity correction.
	if (reflevel != savedNumXForms)
	{
		setClutDirty();
		savedNumXForms = reflevel;
	}
	size_t bs[3];
	dataMgr->GetBlockSize(bs,reflevel);

	  
	//Loop if user accepts lower resolution:
	
	int availRefLevel = myRegionParams->getAvailableVoxelCoords(reflevel, min_dim, max_dim, min_bdim, max_bdim, 
		timeStep,&varNum, 1);

	if(availRefLevel < 0) {
		//The data is not available at the required ref level.
		//Set full bypass if the interactiveRefinementLevel is not available:
		//Set partial bypass if interactive level is available
		//Provide an error message if bypass was not already set:
		bool doMsg = true;
		if ((DataStatus::getInstance()->maxXFormPresent3D(varNum, timeStep)) < DataStatus::getInteractiveRefinementLevel())
			setBypass(timeStep);
		else {
			doMsg = !doBypass(timeStep);
			setPartialBypass(timeStep);
		}
		if (doMsg) {
			string varname = ds->getVariableName3D(varNum);
			MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,
				"Data unavailable for variable %s\n at refinement level %d and current time step",
				varname.c_str(), reflevel);
		}
		return;
	}

	if (availRefLevel < reflevel){
		reflevel = availRefLevel;
		setClutDirty();
		savedNumXForms = reflevel;
	}
	RegionParams::convertToStretchedBoxExtentsInCube(reflevel, min_dim, max_dim, extents);    
	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	//and readable
	glEnable(GL_DEPTH_TEST);
	  
	
	//If you issue a non-unit glColor before the volume rendering, it 
	//affects the subsequent volume rendering on Irix, but not on
	//Windows or Linux!
	glColor3f(1.f,1.f,1.f);	   
	  
	  
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
	if (myGLWindow->vizIsDirty(RegionBit) || forceReload
		|| datarangeIsDirty() || myGLWindow->vizIsDirty(NavigatingBit)
		|| myGLWindow->vizIsDirty(AnimationBit)) 
	{
		//Check if the region/resolution is too big:
		int numMBs = RegionParams::getMBStorageNeeded(extents, extents+3, reflevel);
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
		string varname = (DataStatus::getInstance()->getVariableName3D(currentRenderParams->getSessionVarNum()));

		rc = _updateRegion(
			dataMgr, currentRenderParams, myRegionParams,
			timeStep, varname, reflevel, lod, min_dim, max_dim
		);
			
		//Turn it back on:
	    
		QApplication::restoreOverrideCursor();
		
		if (rc<0){
			//Only set bypass if the data is not available at the interactiveRefLevel
			//Otherwise set partial bypass
			if (reflevel <= DataStatus::getInteractiveRefinementLevel())
				setBypass(timeStep);
			else setPartialBypass(timeStep);
			MyBase::SetErrCode(0);
			if (ds->warnIfDataMissing()){
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Volume data unavailable\nfor refinement level %d\nof variable %s, at current timestep.", 
					reflevel, varname.c_str());
			}
			ds->setDataMissing3D(timeStep, reflevel, lod, currentRenderParams->getSessionVarNum());
			setBypass(timeStep);
			dataMgr->SetErrCode(0);
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

	//Make the z-buffer read-only for the volume data
	glDepthMask(GL_FALSE);
	//qWarning("Starting render");
	if (_driver->Render() < 0){
		setBypass(timeStep);
		SetErrMsg(VAPOR_ERROR_DVR, "Unable to volume render");
		return;
	}
	//qWarning("Render done");
	  
	//Colorbar is rendered with DVR renderer:
	/*if(myGLWindow->colorbarIsEnabled() &&
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
	  */
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

int	VolumeRenderer::_updateRegion(
	DataMgr *dataMgr, RenderParams *rp, RegionParams *regp,
	size_t ts, string varname, int reflevel, int lod,
	const size_t min[3], const size_t max[3]
) {

	RegularGrid *rg = dataMgr->GetGrid(
			ts, varname, reflevel, lod, min, max
	);
	if (!rg) return(-1);

	int rc = 0;
	if (_type == DvrParams::DVR_SPHERICAL_SHADER)
	{
		DVRSpherical *driver = dynamic_cast<DVRSpherical *>(_driver);
		const SphericalGrid *sg = dynamic_cast<const SphericalGrid *>(rg);
		assert(driver != NULL);
		assert(sg != NULL);

		vector<bool> clip(3, false);
		const vector<long> &periodic = dataMgr->GetPeriodicBoundary();

		double dbexts[6];
		double extents[6];
		regp->GetBox()->GetExtents(dbexts, ts);
		for (int i = 0; i<6; i++) extents[i] = dbexts[i];

		clip[0] = !(periodic[0] &&
					FLTEQ(extents[0], dataMgr->GetExtents()[0]) &&
					FLTEQ(extents[3], dataMgr->GetExtents()[3]));
		clip[1] = !(periodic[1] && 
					FLTEQ(extents[1], dataMgr->GetExtents()[1]) &&
					FLTEQ(extents[4], dataMgr->GetExtents()[4]));
		clip[2] = !(periodic[2] && 
					FLTEQ(extents[2], dataMgr->GetExtents()[2]) &&
					FLTEQ(extents[5], dataMgr->GetExtents()[5]));

		rc = driver->SetRegionSpherical(sg, rp->getCurrentDatarange(), 0,
										dataMgr->GetGridPermutation(),
										clip);
	} else
	{
		rc = _driver->SetRegion(rg, rp->getCurrentDatarange(), 0);
	}

	delete rg;

	return(rc);
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

		_driver->SetOLUT(myDVRParams->getClut(), 
		dataMgr->GetNumTransforms() - savedNumXForms);
	}
	
	
	if (myGLWindow->vizIsDirty(LightingBit)) {
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
