//--DVRSpherical.cpp ----------------------------------------------------------
//   
//                   Copyright (C)  2004
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
// Kenny Gruchalla
// National Center for Atmospheric Research
// PO 3000, Boulder, Colorado
//
//
// A 3d texture-based direct volume renderer for spherical grids.
//
// NOTE: This code is currently in a rough proof-of-concept state. It is not 
//       intended for any analytical use. There are known sampling issues with
//       the current approach, and generally much kludginess about.
//
//----------------------------------------------------------------------------

#include <GL/glew.h>
#ifdef Darwin
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <qgl.h>

#include "DVRSpherical.h"
#include "TextureBrick.h"
#include "ShaderProgram.h"
#include "BBox.h"
#include "params.h"

#include "datastatus.h"

#include "Matrix3d.h"
#include "Point3d.h"

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

using namespace VAPoR;

const int volumeTextureTexUnit = 0; // GL_TEXTURE0
const int colormapTexUnit = 1;          // GL_TEXTURE1


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRSpherical::DVRSpherical(
	int precision, int nvars, ShaderMgr *shadermgr,
	int nthreads
) : DVRShader(precision, nvars, shadermgr, nthreads),
  _nr(0),
  _shellWidth(1.0),
  _permutation(3),
  _clip(3)
{
	_shadermgr = shadermgr;
	
	_permutation[0] = 0;
	_permutation[1] = 1;
	_permutation[2] = 2;
	_clip[0] = false;
	_clip[1] = false;
	_clip[2] = false;
	for (int i=0; i<3; i++) {
		_extentsSP[i] = 0.0;
		_extentsSP[i+3] = 1.0;
	}
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRSpherical::~DVRSpherical() 
{	 
	_shadermgr->undefEffect(instanceName("default"));
	_shadermgr->undefEffect(instanceName("lighting"));

}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRSpherical::GraphicsInit() 
{
	glewInit();
	
	if (initTextures() < 0) return(-1);

    if (! _shadermgr->defineEffect("SphericalDVR", "", instanceName("default")))
        return(-1);

	printOpenGLError();

	return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRSpherical::SetRegionSpherical(
	const SphericalGrid *rg, const float range[2], int num,
	const std::vector<long> &permutation, const std::vector<bool> &clip
) { 
  assert(permutation.size() == 3 && clip.size() == 3);

  for(int i=0; i<permutation.size(); i++)
  {
    _permutation[i] = permutation[i];
    _clip[i] = clip[i];
  }
 
  if (_maxTexture == 0) {
    _maxTexture = TextureBrick::maxTextureSize(rg, _precision, _nvars);
  }

  if (_lastRegion.update(rg, _maxTexture))
  {

    size_t dims[3];
    rg->GetDimensions(dims);

    _nx = dims[0];
    _ny = dims[1];
    _nz = dims[2];

	//
	// Extents in spherical coordinates
	//
    double extentsS[6];
    rg->RegularGrid::GetUserExtents(extentsS);

	permute(_permutation, _extentsSP, extentsS[0], extentsS[1], extentsS[2]);
	permute(_permutation, _extentsSP+3, extentsS[3], extentsS[4], extentsS[5]);
	_r0 = _extentsSP[2];
	_r1 = _extentsSP[5];
    
    //
    // Extents in Cartesian coordinates
    //
    double extentsC[6];
    rg->GetUserExtents(extentsC);

    _vmin.x = extentsC[0];
    _vmin.y = extentsC[1];
    _vmin.z = extentsC[2];
    _vmax.x = extentsC[3];
    _vmax.y = extentsC[4];
    _vmax.z = extentsC[5];
    
    _delta = fabs(_vmin.z-_vmax.z) / (2.0*_nz); 
    
    for (int i=0; i<_bricks.size(); i++)
    {
      delete _bricks[i];
      _bricks[i] = NULL;
    }
    
    _bricks.clear();
    
    if (_nx <= _maxTexture && _ny <= _maxTexture && _nz <= _maxTexture)
    {
      //
      // The data will fit completely within texture memory, so no bricking is 
      // needed. We can save a few cycles by setting up the single brick here,
      // rather than calling buildBricks(...). 
      //
      _bricks.push_back(new TextureBrick(rg, _precision, _nvars));
      
      _bricks[0]->dataMin(extentsC[0], extentsC[1], extentsC[2]);
      _bricks[0]->dataMax(extentsC[3], extentsC[4], extentsC[5]);
      _bricks[0]->volumeMin(extentsC[0], extentsC[1], extentsC[2]);
      _bricks[0]->volumeMax(extentsC[3], extentsC[4], extentsC[5]);

      _bricks[0]->textureMin(0, 0, 0);
      _bricks[0]->textureMax(1, 1, 1);
      
      _bricks[0]->fill(rg, range, num);
      
      if (num == _nvars - 1) loadTexture(_bricks[0]);
    } 
    else
    {
      //
      // The data will not fit completely within texture memory, need to 
      // subdivide into multiple bricks.
      //

      // TBD 
      // buildBricks(level, data_box, data_roi, nx, ny, nz);
      SetErrMsg(VAPOR_WARNING, 
                "Bricking is currently unsupported for spherical rendering");
	  
    }

  }
  else
  {
    //
    // Only the data has changed; therefore, we'll refill and reuse the
    // the existing bricks (texture objects).
    //
    for (int i=0; i<_bricks.size(); i++)
    {
      _bricks[i]->refill(rg, range, num);
      if (num == _nvars - 1) loadTexture(_bricks[i]);
    }
  }

  return 0;
}

int DVRSpherical::Render()
{
	MyBase::SetDiagMsg("DVRSpherical::Render()");

    calculateSampling();

	int rc = DVRShader::Render();

	return(rc);
}


//----------------------------------------------------------------------------
// Draw the proxy geometry optmized to a spherical shell
//----------------------------------------------------------------------------
void DVRSpherical::drawViewAlignedSlices(const TextureBrick *brick,
                                         const Matrix3d &modelview,
                                         const Matrix3d &modelviewInverse)
{

  //
  //  
  //
  BBox volumeBox  = brick->volumeBox();
  BBox textureBox = brick->textureBox();
  BBox rotatedBox(volumeBox);
  
  //
  // transform the volume into world coordinates
  //
  rotatedBox.transform(modelview);

  //
  // Calculate the slice plane normal (i.e., the view plane normal transformed
  // into model space). 
  //
  // slicePlaneNormal = modelviewInverse * viewPlaneNormal; 
  //                                       (0,0,1);
  //
  Vect3d slicePlaneNormal(modelviewInverse(2,0), 
                          modelviewInverse(2,1),
                          modelviewInverse(2,2)); 
  //Vect3d slicePlaneNormal(0,0,1);
  slicePlaneNormal.unitize();

  //
  // Calculate the distance between slices
  //
  Vect3d sliceDelta = slicePlaneNormal * _delta;

  //
  // Define the slice view aligned slice plane. The plane will be positioned
  // one delta inside the outer radius of the shell.
  //
  Vect3d center(Point3d(0,0,0), volumeBox.center());
  Vect3d slicePoint = center - (_r1 * slicePlaneNormal) + sliceDelta;

  Vect3d sliceOrtho;

  if (slicePlaneNormal.dot(Vect3d(1,1,0)) != 0.0)
  {
    sliceOrtho = slicePlaneNormal.cross(Vect3d(1,1,0));
  }
  else
  {
    sliceOrtho = slicePlaneNormal.cross(Vect3d(0,1,0));
  }

  sliceOrtho.unitize();

  //
  // Calculate edge intersections between the plane and the spherical shell
  //

  //
  // TBD -- This code creates proxy geometry for the full spherical shell,
  // ignoring the brick's extents. This won't work once we support spherical
  // bricking. Also, when less than a full region has been selected, this
  // code superflously samples empty space. The shader handles that; however,
  // it still incurs the expensive coordinate transform. This should be 
  // fixed.
  //

  for(int i = 0 ; i <= _samples; i++)
  {
    float d = (center - slicePoint).mag();

    if (_r1 > d)
    {
      float rg0 = 0.0;
      float rg1 = sqrt(_r1*_r1 - d*d);

      if (_r0 > d)
      {
        rg0 = sqrt(_r0*_r0 - d*d);
      }

      //
      // Draw donut (shell-plane intersection) and texture map it
      //
      glBegin(GL_QUAD_STRIP); 
      {
        for (float theta=-M_PI/20.0; theta<=2.0*M_PI; theta+=M_PI/20.0)
        {
          Vect3d v = (cos(theta)*sliceOrtho +
                      sin(theta)*slicePlaneNormal.cross(sliceOrtho));

          v.unitize();

          Vect3d v0 = slicePoint + v * rg0;
          Vect3d v1 = slicePoint + v * rg1;
          Vect3d t0 = v0 / (2*_r1) + 0.5;
          Vect3d t1 = v1 / (2*_r1) + 0.5;

          glTexCoord3f(t0.x(), t0.y(), t0.z());          
          glVertex3f(v0.x(), v0.y(), v0.z());

          glTexCoord3f(t1.x(), t1.y(), t1.z());          
          glVertex3f(v1.x(), v1.y(), v1.z());

        }
      }
      glEnd();
    }

    //
    // increment the slice plane by the slice distance
    //
    slicePoint += sliceDelta;    
  }

  glFlush();
}

void DVRSpherical::renderBrick(
	const TextureBrick *brick, const Matrix3d &modelview, 
	const Matrix3d &modelviewInverse
) {

//#define NOSHADER
#ifndef NOSHADER
	initShaderVariables();

	bool ok = _shadermgr->enableEffect(getCurrentEffect());
	if (! ok) return;
#endif

	DVRSpherical::drawViewAlignedSlices(brick, modelview, modelviewInverse);

#ifndef NOSHADER
	_shadermgr->disableEffect();
#endif

}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool DVRSpherical::supported()
{
	return (ShaderProgram::supported() && GLEW_ARB_multitexture);
}

//----------------------------------------------------------------------------
// Calculate the sampling distance
//----------------------------------------------------------------------------
void DVRSpherical::calculateSampling()
{

	//
	// Get the modelview matrix and its inverse
	//
	Matrix3d modelview;   
	Matrix3d modelviewInverse;
	
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview.data());  
	modelview.inverse(modelviewInverse);
	
	BBox volumeBox(_vmin, _vmax);
	
	volumeBox.transform(modelview);
	
	// 
	// Calculate the the minimum and maximum z-position of the rotated bounding
	// boxes. Equivalent to but quicker than:
	//
	// Vect3d maxv(0, 0, rotatedBox.maxZ().z); 
	// Vect3d minv(0, 0, rotatedBox.minZ().z); 
	// maxv = modelviewInverse * maxd;
	// minv = modelviewInverse * mind;
	//
	Vect3d maxv(modelviewInverse(2,0)*volumeBox.maxZ().z,
				modelviewInverse(2,1)*volumeBox.maxZ().z,
				modelviewInverse(2,2)*volumeBox.maxZ().z);
	Vect3d minv(modelviewInverse(2,0)*volumeBox.minZ().z,
				modelviewInverse(2,1)*volumeBox.minZ().z,
				modelviewInverse(2,2)*volumeBox.minZ().z);
	
	if (_renderFast)
	{
		_samples = MAX(_nx, MAX(_ny, _nz));
	}
	else
	{
		_samples = _nr * ((maxv - minv).mag() / _shellWidth);
	}

	// Ugh. Need a more accurate calculation of sampling rate
	//
	_samples = 256;
	
	_delta = (maxv - minv).mag() / _samples; 
	//_samplingRate = (1.0/(float)(_level+1))*(_shellWidth / _delta) / (float)_nr;
	
	// TBD -- The sampling rate & opacity correction delta are not quite right 
	// for spherical geometry. We need to work through how the spherical 
	// transform is warping the voxels and effecting the sampling. 
	// 
	
	printOpenGLError();
}

//----------------------------------------------------------------------------
// Rearrange the x, y, and z components according to the permutation matrix
//----------------------------------------------------------------------------
void DVRSpherical::permute(const vector<long>& permutation,
                           float result[3], float x, float y, float z)
{
	result[permutation[0]] = x;
	result[permutation[1]] = y;
	result[permutation[2]] = z;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRSpherical::initShaderVariables()
{
	string effect = getCurrentEffect();
    
    float tmpv[3];
    
    permute(_permutation, tmpv, _nx, _ny, _nz);
    
    _nr = (int)tmpv[2];
    
    permute(_permutation, tmpv, 0.0, 0.0, 0.0);
	_shadermgr->uploadEffectData(
		effect, "tmin" ,(float)tmpv[0], (float)tmpv[1], (float)tmpv[2]
	);

    permute(_permutation, tmpv, 1.0, 1.0, 1.0);
	_shadermgr->uploadEffectData(
		effect, "tmax" ,(float)tmpv[0], (float)tmpv[1], (float)tmpv[2]
	);
    
	_shadermgr->uploadEffectData(
		effect, "dmin", (_extentsSP[0] + 180.0) / 360.0,
		(_extentsSP[1] + 90.0) / 180.0, (float)_extentsSP[2]
	);
    _shellWidth = _extentsSP[2];
    
  	_shadermgr->uploadEffectData(
		effect, "dmax", (_extentsSP[3] + 180.0) / 360.0,
		(_extentsSP[4] + 90.0) / 180.0, (float)_extentsSP[5]
	);
    assert(_extentsSP[5]);
    _shellWidth = (_extentsSP[5] - _shellWidth)/(2.0*_extentsSP[5]);
	
	_shadermgr->uploadEffectData(
		effect, "permutation" ,
		(float)_permutation[0], (float)_permutation[1], (float)_permutation[2]
	);
	_shadermgr->uploadEffectData(
		effect, "clip" , 
		(int)_clip[_permutation[0]], (int)_clip[_permutation[1]]
	);

	_shadermgr->uploadEffectData(effect, "colormap",colormapTexUnit);
	_shadermgr->uploadEffectData(effect, "volumeTexture", volumeTextureTexUnit);
	
	printOpenGLError();
}
