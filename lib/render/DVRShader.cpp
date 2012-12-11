//--DVRShader.cpp -----------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// A derived DVRTexture3d providing a OpenGL Shading Language shader for
// the color mapping. 
//
//----------------------------------------------------------------------------

#include "glutil.h"	// Must be included first!!!
#include <sstream>
#include <qgl.h>

#include "vapor/StretchedGrid.h"
#include "DVRShader.h"
#include "TextureBrick.h"
#include "ShaderProgram.h"
#include "BBox.h"
#include "params.h"

#include "Matrix3d.h"
#include "Point3d.h"

using namespace VAPoR;

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

const int volumeTextureTexUnit = 0;	// GL_TEXTURE0
const int colormapTexUnit = 1;			// GL_TEXTURE1
const int coordmapTexUnit = 2;			// GL_TEXTURE2

const int colormapTexName = 0;
const int colormapPITexName = 1;
const int coordmapTexName = 2;

const int coordmapTexWidth = 1024;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRShader::DVRShader(
	int precision, int nvars,
	ShaderMgr *shadermgr, int nthreads
) : DVRTexture3d(precision, nvars, nthreads),
  _lighting(false),
  _stretched(false),
  _colormap(NULL),
  _coordmap(NULL),
  _preintegration(false),
  _kd(0.0),
  _ka(0.0),
  _ks(0.0),
  _expS(0.0),
  _midx(0),
  _zidx(0),
  _initialized(false)
{
	MyBase::SetDiagMsg(
		"DVRShader::DVRShader( %d %d %d)", 
		precision, nvars, nthreads
	);

	_shadermgr = shadermgr;


  for (int i=0; i<3; i++) _pos[i] = 0.0;

  for (int i=0; i<4; i++) {
    _vdir[i] = 0.0;
    _vpos[i] = 0.0;
  }

  _xcoords.clear();
  _ycoords.clear();
  _zcoords.clear();
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRShader::~DVRShader() 
{
	MyBase::SetDiagMsg("DVRShader::~DVRShader()");

	if (_colormap) delete [] _colormap;
	_colormap = NULL;

	if (_coordmap) delete [] _coordmap;
	_coordmap = NULL;

	glDeleteTextures(3, _cmapid);
	printOpenGLError();

	_shadermgr->undefEffect(instanceName("default"));
	_shadermgr->undefEffect(instanceName("lighting"));
	_shadermgr->undefEffect(instanceName("preintegrated"));
	_shadermgr->undefEffect(instanceName("preintegrated+lighting"));
	printOpenGLError();

}



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRShader::GraphicsInit() 
{
	if (_initialized) return(0);

	glewInit();
	printOpenGLError();


	if (! _shadermgr->defineEffect("DVR", "", instanceName("default")))
		return(-1);

	if (! _shadermgr->defineEffect(
		"DVR", "LIGHTING;", instanceName("lighting")
	)) return(-1);
	if (! _shadermgr->defineEffect(
		"DVR", "PREINTEGRATED;", instanceName("preintegrated")
	)) return(-1);
	if (! _shadermgr->defineEffect(
		"DVR", "LIGHTING;PREINTEGRATED;", instanceName("preintegrated+lighting")
	)) return(-1);

  if (initTextures() < 0) return(-1);

  if (! _shadermgr->uploadEffectData(instanceName("preintegrated+lighting"), "volumeTexture", volumeTextureTexUnit)) return(-1);

  if (! _shadermgr->uploadEffectData(instanceName("preintegrated+lighting"), "colormap", colormapTexUnit)) return(-1);

  if (! _shadermgr->uploadEffectData(instanceName("preintegrated+lighting"), "coordmap", coordmapTexUnit)) return(-1);
	
  if (! _shadermgr->uploadEffectData(instanceName("preintegrated"), "colormap", colormapTexUnit)) return(-1);

  if (! _shadermgr->uploadEffectData(instanceName("preintegrated"), "volumeTexture", volumeTextureTexUnit)) return(-1);

  if (! _shadermgr->uploadEffectData(instanceName("preintegrated"), "coordmap", coordmapTexUnit)) return(-1);
	
  if (! _shadermgr->uploadEffectData(instanceName("lighting"), "colormap", colormapTexUnit)) return(-1);

  if (! _shadermgr->uploadEffectData(instanceName("lighting"), "volumeTexture", volumeTextureTexUnit)) return(-1);

  if (! _shadermgr->uploadEffectData(instanceName("lighting"), "coordmap", coordmapTexUnit)) return(-1);
	
  if (! _shadermgr->uploadEffectData(instanceName("default"), "colormap", colormapTexUnit)) return(-1);

  if (! _shadermgr->uploadEffectData(instanceName("default"), "volumeTexture", volumeTextureTexUnit)) return(-1);

  if (! _shadermgr->uploadEffectData(instanceName("default"), "coordmap", coordmapTexUnit)) return(-1);

  initShaderVariables();
  _initialized = true;
  return(0);

}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRShader::SetRegion(const RegularGrid *rg, const float range[2], int num) 
{ 
	MyBase::SetDiagMsg("SetRegion()");

	bool layered = ((dynamic_cast<const LayeredGrid *>(rg)) != NULL);
	bool missing = rg->HasMissingData();
	_stretched = ((dynamic_cast<const StretchedGrid *>(rg)) != NULL);

	//
	// Figure out which texture component (R,G,B,A) contains the missing data
	// indicator and/or the Z coordinate for layered data. N.B. if there
	// are only two components, the first is stored in R, the second in A. But
	// if there are three or four they're stored in RGB and RGBA, respectively.
	// The _?idx variables give the component offset and are also a boole
	// flag indicating whether the component is present (no, if zero)
	//
	_midx = 0;
	_zidx = 0;
	if (missing && layered) {
		_midx = 1;
		_zidx = 2;
	}
	else if (missing) {
		if (_nvars == 1) _midx = 3; // Alpha channel
		else _midx = 1;
		_zidx = 0;
	}
	else if (layered) {
		_midx = 0;
		if (_nvars == 1) _zidx = 3; // Alpha channel
		else _zidx = 1;
	}

	if (_stretched) {
		const StretchedGrid *sg = (dynamic_cast<const StretchedGrid *>(rg));
		sg->GetUserCoordinateMaps(_xcoords, _ycoords, _zcoords);
	}

	initShaderVariables();

	size_t dims[3];
	rg->GetDimensions(dims);

	_nx = dims[0]; _ny = dims[1]; _nz = dims[2];

	//
	// Set the geometry extents
	//
	return DVRTexture3d::SetRegion(rg, range, num);
}

//
// Generate a lookup table to peform an inverse mapping from user coordinates
// to texture coordinates
//
void DVRShader::_loadCoordMap(
	const vector <double> ucoords, 	// User coordinates
	int c0, int c1, 	// subset of user coordinates 
	float *coordmap, size_t coordmapsz, 	// lookup 
	int stride, int offset	// stride and offset between elements in coordmap
) {
	assert(c1-c0>=1);
	assert(c1<ucoords.size());

	// N.B. Texture coordinates in OpenGL run from 1/2N to 1-1/2N with
	// a sample spacing of 1/N, where N is the number of texels. There
	// are two different textures in play here: the texture containing
	// the users's data (containing c1-c0+1 texels), herein referred to as
	// the "data texure, and the texture
	// that will contain the lookup table (containing coordmapsz texels),
	// herein referred to as the lookup texture.
	//
	// The user coordinates define a sequence of connected line segments
	// with end points given by (t, ucoords[i])
	//

	// sample spacing between data texels, first and last texel coordinate
	// for both Y and X axis
	//
	double deltay_t = 1.0 / ((double) (c1-c0+1.0));
	double y_tmin = 0.5 * deltay_t;	
	double y_tmax = 1.0 - y_tmin;
	double tmin = y_tmin;
	double deltat = deltay_t;

	// Transform user coordinates of first line segment (first and second 
	// point) to texture coordinates in range 1/2N to 1-12N
	//
	int ii = c0;
	double y0 = (ucoords[ii]-ucoords[c0])/(ucoords[c1]-ucoords[c0]) *
		(y_tmax-y_tmin) + y_tmin;

	ii++;
	double y1 = (ucoords[ii]-ucoords[c0])/(ucoords[c1]-ucoords[c0]) *
		(y_tmax-y_tmin) + y_tmin;

	// Determine line equation for first line segment for *inverse* map
	// line slope and intercept: x = my + b
	//
	double m = deltay_t / (y1-y0);	
	double b = -m * y0 + tmin;

	// sample space and coordinate of first texel in lookup texture
	//
	double deltay_tprime = 1.0 / ((double) coordmapTexWidth);
	double y = 0.5 * deltay_tprime;

	// For each entry in lookup texel (coordmap) map normalized user
	// coordinate to texture coordinates
	//
	double t = tmin;
	for (int i=0; i<coordmapTexWidth; i++) {

		// Make sure y is in current line segment. If not, keep stepping to 
		// next line segment until we find one containing y
		//
		while (y>y1 && ii<=c1) {
			ii++;
			t += deltat;
			y0 = y1;
			y1 = (ucoords[ii]-ucoords[c0])/(ucoords[c1]-ucoords[c0]) *
				(y_tmax-y_tmin) + y_tmin;

			// Line equation for current line segment
			//
			m = deltay_t / (y1-y0);	
			b = -m * y0 + t;
		}
	
		coordmap[i*stride+offset] = y*m+b;
		y += deltay_tprime;
	}
}

void DVRShader::loadCoordMap(
	const TextureBrick *brick, size_t i0, size_t i1, size_t j0, size_t j1,
	size_t k0, size_t k1
) {
	if (! _stretched) return;

	if (_xcoords.size()>1) { 

		_loadCoordMap(_ycoords, i0, i1, _coordmap, coordmapTexWidth, 4, 0);

	}
	else {
		for (int i=0; i<coordmapTexWidth; i++) { 
			_coordmap[i*4+0] = (float) i / ((double) coordmapTexWidth-1.0);
		}
	}

	if (_ycoords.size()>1) {

		_loadCoordMap(_ycoords, j0, j1, _coordmap, coordmapTexWidth, 4, 1);

	} 
	else {
		for (int i=0; i<coordmapTexWidth; i++) { 
			_coordmap[i*4+1] = (float) i / ((double) coordmapTexWidth-1.0);
		}
	}

	if (_zcoords.size()>1) {

		_loadCoordMap(_zcoords, k0, k1, _coordmap, coordmapTexWidth, 4, 2);

	}
	else {
		for (int i=0; i<coordmapTexWidth; i++) { 
			_coordmap[i*4+2] = (float) i / ((double) coordmapTexWidth-1.0);
		}
	}

	glBindTexture(GL_TEXTURE_1D, _cmapid[coordmapTexName]);
	glTexSubImage1D(
		GL_TEXTURE_1D, 0, 0, coordmapTexWidth, GL_RGBA, GL_FLOAT,_coordmap
	);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::loadTexture(TextureBrick *brick)
{
  printOpenGLError();

  brick->load();
  printOpenGLError();
}


//----------------------------------------------------------------------------
// Render a single brick
//----------------------------------------------------------------------------
void DVRShader::renderBrick(const TextureBrick *brick,
                                         const Matrix3d &modelview,
                                         const Matrix3d &modelviewInverse)

{

    _shadermgr->uploadEffectData(
		getCurrentEffect(), "dimensions", 
		(float) brick->nx(), (float) brick->ny(), (float) brick->nz()
	);

	if (_stretched) {
		size_t x0 = brick->xoffset();
		size_t x1 = x0 + brick->nx() - 1;
		size_t y0 = brick->yoffset();
		size_t y1 = y0 + brick->ny() - 1;
		size_t z0 = brick->zoffset();
		size_t z1 = z0 + brick->nz() - 1;

		loadCoordMap(brick, x0, x1, y0, y1, z0, z1);
	}
    DVRTexture3d::drawViewAlignedSlices(brick, modelview, modelviewInverse);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRShader::Render()
{
  calculateSampling();

//#define NOSHADER
#ifndef	NOSHADER
  bool ok = _shadermgr->enableEffect(getCurrentEffect());		 
  if (! ok) return (-1);


  glPolygonMode(GL_FRONT, GL_FILL);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);

  if (_preintegration) {
    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE1);
    }
    else {
      glActiveTextureARB(GL_TEXTURE1_ARB);
    }
    glEnable(GL_TEXTURE_2D);  
    glBindTexture(GL_TEXTURE_2D, _cmapid[colormapPITexName]);
  }
  else {
    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE1);
    }
    else {
      glActiveTextureARB(GL_TEXTURE1_ARB);
    }
    glEnable(GL_TEXTURE_1D);
    glBindTexture(GL_TEXTURE_1D, _cmapid[colormapTexName]);
  }

  if (_stretched) {
    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE2);
    }
    else {
      glActiveTextureARB(GL_TEXTURE2_ARB);
    }
    glEnable(GL_TEXTURE_1D);
    glBindTexture(GL_TEXTURE_1D, _cmapid[coordmapTexName]);
  }

  if (GLEW_VERSION_2_0) {
    glActiveTexture(GL_TEXTURE0);
  }
  else {
    glActiveTextureARB(GL_TEXTURE0_ARB);
  }

  glEnable(GL_TEXTURE_3D);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  glDisable(GL_DITHER);

#endif

  renderBricks();

#ifndef	NOSHADER
  if (GLEW_VERSION_2_0) {

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, 0);
    glDisable(GL_TEXTURE_1D);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_1D, 0);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_1D);

    glActiveTexture(GL_TEXTURE0);
  } else {

    glActiveTextureARB(GL_TEXTURE2_ARB);
    glBindTexture(GL_TEXTURE_1D, 0);
    glDisable(GL_TEXTURE_1D);
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_1D, 0);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_1D);

    glActiveTextureARB(GL_TEXTURE0_ARB);
  }

  glBindTexture(GL_TEXTURE_3D, 0);
  glDisable(GL_TEXTURE_3D);

  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);

  _shadermgr->disableEffect();
#endif
  glFlush();
  return 0;
}

//----------------------------------------------------------------------------
// Set the color table used to render the volume without any opacity 
// correction.
//----------------------------------------------------------------------------
void DVRShader::SetCLUT(const float ctab[256][4]) 
{
  for (int i=0; i<256; i++)
  {
    _colormap[i*4+0] = ctab[i][0];
    _colormap[i*4+1] = ctab[i][1];
    _colormap[i*4+2] = ctab[i][2];
    _colormap[i*4+3] = ctab[i][3];
  }

  glBindTexture(GL_TEXTURE_1D, _cmapid[colormapTexName]);
  
  glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA,
                  GL_FLOAT, _colormap);

  glFlush();
}

//----------------------------------------------------------------------------
// Set the color table used to render the volume applying an opacity 
// correction.
//----------------------------------------------------------------------------
void DVRShader::SetOLUT(const float atab[256][4], const int numRefinements)
{
  //
  // Compute the sampling distance and rate
  //
  calculateSampling();

  if (_preintegration)
  {
    SetPreIntegrationTable(atab, numRefinements);
    return;
  }

  //
  // Calculate opacity correction. Delta is 1 for the ffineest refinement, 
  // multiplied by 2^n (the sampling distance)
  //
  double delta = 2.0/_samplingRate * (double)(1<<numRefinements);

  for(int i=0; i<256; i++) 
  {
    double opac = atab[i][3];

    opac = 1.0 - pow((1.0 - opac), delta);
  
    if (opac > 1.0)
    {
      opac = 1.0;
    }

    _colormap[i*4+0] = atab[i][0];
    _colormap[i*4+1] = atab[i][1];
    _colormap[i*4+2] = atab[i][2];
    _colormap[i*4+3] = opac;
  }

  glBindTexture(GL_TEXTURE_1D, _cmapid[colormapTexName]);
  
  glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA,
                  GL_FLOAT, _colormap);

  glFlush();
}

double clamp(double v, double min, double max)
{
  return MAX(MIN(v, max), min);
}

//----------------------------------------------------------------------------
// Set the pre-integrated color table used to render the volume applying an 
// opacity correction.
//----------------------------------------------------------------------------
void DVRShader::SetPreIntegrationTable(const float atab[256][4], 
                                       const int numRefinements)
{
  double factor;
  double ifunc[256][4];
  double r = 0;
  double g = 0;
  double b = 0;
  double a = 0;


  ifunc[0][0] = 0.0;
  ifunc[0][1] = 0.0;
  ifunc[0][2] = 0.0;
  ifunc[0][3] = 0.0;

  float opac[256];

  double delta = 2.0/_samplingRate * (double)(1<<numRefinements);

  opac[0] = MIN(1.0, 1.0 - pow((1.0 - atab[0][3]), delta));

  // 
  // Compute integral functions & opacity correction
  //

  for (int i=1; i<256; i++)
  {

    opac[i] = MIN(1.0, 1.0 - pow((1.0 - atab[i][3]), delta));

    a = 255.0*(opac[i-1]+opac[i])/2.0;

    // Opacity-weighted
    //ifunc[i][0] = ifunc[i-1][0] + (atab[i-1][0]+atab[i][0])/2.0*a;
    //ifunc[i][1] = ifunc[i-1][1] + (atab[i-1][1]+atab[i][1])/2.0*a;
    //ifunc[i][2] = ifunc[i-1][2] + (atab[i-1][2]+atab[i][2])/2.0*a;

    // No weighting
    ifunc[i][0] = ifunc[i-1][0] + (atab[i-1][0]+atab[i][0])/2.0*255;
    ifunc[i][1] = ifunc[i-1][1] + (atab[i-1][1]+atab[i][1])/2.0*255;
    ifunc[i][2] = ifunc[i-1][2] + (atab[i-1][2]+atab[i][2])/2.0*255;

    ifunc[i][3] = ifunc[i-1][3] + a;
  }    

  //
  // Compute lookup table
  //
  for (int sb=0; sb<256; sb++)
  {
    for (int sf=0; sf<256; sf++)
    {
      int smin = MIN(sb, sf);
      int smax = MAX(sb, sf);

      if (smin != smax)
      {
        factor = 1.0 / (double)(smax - smin);
        
        r = factor * (ifunc[smax][0] - ifunc[smin][0]);
        g = factor * (ifunc[smax][1] - ifunc[smin][1]);
        b = factor * (ifunc[smax][2] - ifunc[smin][2]);
        a = 256.0 * 
          (1.0 - expf(-(ifunc[smax][3] - ifunc[smin][3]) * factor / 255.0));
      }
      else
      {
        // Opacity-weighted
        //r = 255.0 * atab[smin][0] * atab[smin][3];
        //g = 255.0 * atab[smin][1] * atab[smin][3];
        //b = 255.0 * atab[smin][2] * atab[smin][3];

        // No weighting
        r = 255.0 * atab[smin][0];
        g = 255.0 * atab[smin][1];
        b = 255.0 * atab[smin][2];
        a = 256 * (1.0 - expf(-opac[smin]));

      }

      int index = (sf + sb*256) * 4;

      _colormap[index + 0] = clamp(r/255.0, 0.0, 1.0);
      _colormap[index + 1] = clamp(g/255.0, 0.0, 1.0);
      _colormap[index + 2] = clamp(b/255.0, 0.0, 1.0);
      _colormap[index + 3] = clamp(a/255.0, 0.0, 1.0);
    }
  }

  glBindTexture(GL_TEXTURE_2D, _cmapid[colormapPITexName]);
  
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA,
                  GL_FLOAT, _colormap);

  glFlush();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetView(const float *pos, const float *dir) 
{
  assert(dir);
  assert(pos);

  for (int i=0; i<4; i++) {
    _vdir[i] = dir[i];
    _vpos[i] = pos[i];
  }
  initShaderVariables();

}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetPreintegrationOnOff(int on) 
{
  _preintegration = on;

  initShaderVariables();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetLightingOnOff(int on) 
{
  _lighting = on;

  initShaderVariables();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetLightingCoeff(float kd, float ka, float ks, float expS)
{
	_kd = kd;
	_ka = ka;
	_ks = ks;
	_expS = expS;

	initShaderVariables();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetLightingLocation(const float *pos)
{
	_pos[0] = pos[0];
	_pos[1] = pos[1];
	_pos[2] = pos[2];

	initShaderVariables();
}


//----------------------------------------------------------------------------
// Initalize the textures (i.e., 3d volume texture and the 1D colormap texture)
//----------------------------------------------------------------------------
int DVRShader::initTextures()
{

  //
  // Setup the colormap texture
  //
  glGenTextures(3, _cmapid);

  _colormap = new float[256*256*4];
  for (int i=0; i<256; i++) { 
    _colormap[i*4+0] = (float) i / 255.0;
    _colormap[i*4+1] = (float) i / 255.0;
    _colormap[i*4+2] = (float) i / 255.0;
    _colormap[i*4+3] = 1.0;
  }
  
  //
  // Standard colormap
  //
  glBindTexture(GL_TEXTURE_1D, _cmapid[colormapTexName]);

  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA,
               GL_FLOAT, _colormap);

  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

  //
  // Pre-Integrated colormap
  //
  glBindTexture(GL_TEXTURE_2D, _cmapid[colormapPITexName]);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA,
               GL_FLOAT, _colormap);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


  //
  // Coordinate lookup map for stretched grids
  //
  _coordmap = new float[coordmapTexWidth*4];
  for (int i=0; i<coordmapTexWidth; i++) { 
    _coordmap[i*4+0] = (float) i / ((double)coordmapTexWidth-1.0);	// X
    _coordmap[i*4+1] = (float) i / ((double)coordmapTexWidth-1.0);	// Y
    _coordmap[i*4+2] = (float) i / ((double)coordmapTexWidth-1.0);	// Z
    _coordmap[i*4+3] = 1.0;
  }
  
  glBindTexture(GL_TEXTURE_1D, _cmapid[coordmapTexName]);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA16, coordmapTexWidth, 0, GL_RGBA,
               GL_FLOAT, _coordmap);

  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_1D, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFlush();
  if (printOpenGLError() != 0) return(-1);

  return(0);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::initShaderVariables()
{
  if (_preintegration)
  {
	  _shadermgr->uploadEffectData(getCurrentEffect(), "delta", _delta);
	  _shadermgr->uploadEffectData(getCurrentEffect(), "vdir", _vdir[0], _vdir[1], _vdir[2], _vdir[3]);
	  _shadermgr->uploadEffectData(getCurrentEffect(), "vpos", _vpos[0], _vpos[1], _vpos[2], _vpos[3]);
  }

  if (_lighting)
  {	  
	  _shadermgr->uploadEffectData(getCurrentEffect(), "kd", _kd);
	  _shadermgr->uploadEffectData(getCurrentEffect(), "ka", _ka);
	  _shadermgr->uploadEffectData(getCurrentEffect(), "ks", _ks);
	  _shadermgr->uploadEffectData(getCurrentEffect(), "expS", _expS);
	  _shadermgr->uploadEffectData(getCurrentEffect(), "lightDirection", _pos[0], _pos[1], _pos[2]);
  } 

  _shadermgr->uploadEffectData(getCurrentEffect(), "fast", (int) _renderFast);
  _shadermgr->uploadEffectData(getCurrentEffect(), "midx", (int) _midx);
  _shadermgr->uploadEffectData(getCurrentEffect(), "zidx", (int) _zidx);
  _shadermgr->uploadEffectData(getCurrentEffect(), "stretched", (int) _stretched);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool DVRShader::supported()
{
  return (ShaderProgram::supported() && GLEW_ARB_multitexture);
}



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
std::string DVRShader::getCurrentEffect()
{
	if (_preintegration && _lighting)
	{
		return instanceName("preintegrated+lighting");
	}
	
	if (_preintegration && !_lighting)
	{
		return instanceName("preintegrated");
	}
	
	if (!_preintegration && _lighting)
	{
		return instanceName("lighting");
	}
	
	return instanceName("default");
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::calculateSampling()
{
  DVRTexture3d::calculateSampling();
  if (_preintegration)
  {
	_shadermgr->uploadEffectData(getCurrentEffect(), "delta", _delta);
  }
}

//----------------------------------------------------------------------------
// Draw the proxy geometry for the brick. Overriden to setup the texture
// matrix that the shader can use to convert object coordinates to 
// texture coordinates. 
//----------------------------------------------------------------------------
void DVRShader::drawViewAlignedSlices(const TextureBrick *brick,
                                      const Matrix3d &modelview,
                                      const Matrix3d &modelviewInverse)
{
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  if (_preintegration)
  {
    Point3d vmax = brick->volumeMax();
    Point3d vmin = brick->volumeMin();
    Point3d tmax = brick->textureMax();
    Point3d tmin = brick->textureMin();

    Point3d scale((tmax.x - tmin.x) / (vmax.x - vmin.x),
                  (tmax.y - tmin.y) / (vmax.y - vmin.y),
                  (tmax.z - tmin.z) / (vmax.z - vmin.z));

    Point3d trans((tmin.x / scale.x) - vmin.x,
                  (tmin.y / scale.y) - vmin.y,
                  (tmin.z / scale.z) - vmin.z);

    glScaled(scale.x, scale.y, scale.z);
    glTranslated(trans.x, trans.y, trans.z);
  }

  glMatrixMode(GL_MODELVIEW);

  DVRTexture3d::drawViewAlignedSlices(brick, modelview, modelviewInverse);
}


string DVRShader::instanceName(string base) const {
        ostringstream oss;
        oss << base << "_" << this;
        return(oss.str());
}

