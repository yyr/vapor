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
#include <gl.h>
#include <glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <qgl.h>

#include "DVRSpherical.h"
#include "TextureBrick.h"
#include "ShaderProgram.h"
#include "BBox.h"
#include "glutil.h"
#include "params.h"

#include "datastatus.h"
#include "renderer.h"

#include "Matrix3d.h"
#include "Point3d.h"
#include "shaders.h"

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRSpherical::DVRSpherical(
						   GLint internalFormat, GLenum format, GLenum type, int nthreads, Renderer* ren
						   ) : DVRShader(internalFormat, format, type, nthreads, ren),
_nr(0),
_shellWidth(1.0),
_permutation(3),
_clip(3)
{
	/*  _shaders[DEFAULT]        = NULL;
	 _shaders[LIGHT]          = NULL;
	 _shaders[PRE_INTEGRATED] = NULL;*/
	
	_permutation[0] = 0;
	_permutation[1] = 1;
	_permutation[2] = 2;
	_clip[0] = false;
	_clip[1] = false;
	_clip[2] = false;
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRSpherical::~DVRSpherical() 
{	 
	 delete [] _colormap;
	 _colormap = NULL;
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRSpherical::GraphicsInit() 
{
	glewInit();
	
	if (initTextures() < 0) return(-1);
	
	GLWindow::manager->uploadEffectData("sphericalDefault", "colormap",  1);
	GLWindow::manager->uploadEffectData("sphericalDefault", "volumeTexture",  0);
	GLWindow::manager->uploadEffectData("sphericalLighting", "colormap",  1);
	GLWindow::manager->uploadEffectData("sphericalLighting", "volumeTexture",  0);
	_effect = "sphericalDefault";
	printOpenGLError();
	return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRSpherical::SetRegionSpherical(void *data,
                                     int nx, int ny, int nz,
                                     const int data_roi[6],
                                     const float extents[6],
                                     const int data_box[6],
                                     int level,
									 
                                     const std::vector<long> &permutation,
                                     const std::vector<bool> &clip)
{ 
	assert(permutation.size() == 3 && clip.size() == 3);
	
	for(int i=0; i<permutation.size(); i++)
	{
		_permutation[i] = permutation[i];
		_clip[i] = clip[i];
	}
	
	//
	// Set the texture data
	//
	if (_nx != nx || _ny != ny || _nz != nz)
	{
		_data = data;
	}
	
	//
	// Set the geometry extents
	//
	_data = data;
	
	if (_lastRegion.update(nx, ny, nz, data_roi, data_box, extents, _maxTexture))
	{
		_level = level;
		
		if (_nx != nx || _ny != ny || _nz != nz)
		{
			_nx = _bx = nextPowerOf2(nx); 
			_ny = _by = nextPowerOf2(ny); 
			_nz = _bz = nextPowerOf2(nz);
		}
		
		//
		// Hard-code the volume extents to the unit cube 
		//
		_vmin.x = 0;//extents[0];
		_vmin.y = 0;//extents[1];
		_vmin.z = 0;//extents[2];
		_vmax.x = 1;//extents[3];
		_vmax.y = 1;//extents[4];
		_vmax.z = 1;//extents[5];
		
		_delta = fabs(_vmin.z-_vmax.z) / (2.0*nz); 
		
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
			_bricks.push_back(new TextureBrick(_internalFormat, _format, _type));
			
			_bricks[0]->volumeMin(0, 0, 0);
			_bricks[0]->volumeMax(1, 1, 1);
			
			_bricks[0]->textureMin(0, 0, 0);
			_bricks[0]->textureMax(1, 1, 1);
			
			_bricks[0]->fill((GLubyte*)data, nx, ny, nz);
			
			loadTexture(_bricks[0]);
		} 
		else
		{
			//
			// The data will not fit completely within texture memory, need to 
			// subdivide into multiple bricks.
			//
			
			// TBD 
			// buildBricks(level, data_box, data_roi, nx, ny, nz);
			myRenderer->setAllBypass(true);
			SetErrMsg(VAPOR_WARNING, 
					  "Bricking is currently unsupported for spherical rendering");
			
		}
		
		initShaderVariables();
		
		calculateSampling();
	}
	else
	{
		//
		// Only the data has changed; therefore, we'll refill and reuse the
		// the existing bricks (texture objects).
		//
		for (int i=0; i<_bricks.size(); i++)
		{
			_bricks[i]->refill((GLubyte*)data);
		}
	}
	
	printOpenGLError();
	return 0;
}

//----------------------------------------------------------------------------
// Draw the proxy geometry optmized to a spherical shell
//----------------------------------------------------------------------------
void DVRSpherical::drawViewAlignedSlices(const TextureBrick *brick,
                                         const Matrix3d &modelview,
                                         const Matrix3d &modelviewInverse)
{
	float tmpv[3];
	const float *extents = _lastRegion.extents();
    
	permute(_permutation, tmpv, extents[0], extents[1], extents[2]);
	float r0 = tmpv[2]/2.0; // inner shell radius
	
	permute(_permutation, tmpv, extents[3], extents[4], extents[5]);
	float r1 = tmpv[2]/2.0; // outer shell raduis
	
	printOpenGLError();
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
	
	printOpenGLError();
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
	
	printOpenGLError();
	//
	// Calculate the distance between slices
	//
	Vect3d sliceDelta = slicePlaneNormal * _delta;
	
	//
	// Define the slice view aligned slice plane. The plane will be positioned
	// one delta inside the outer radius of the shell.
	//
	Vect3d center(Point3d(0,0,0), volumeBox.center());
	Vect3d slicePoint = center - (r1 * slicePlaneNormal) + sliceDelta;
	
	Vect3d sliceOrtho;
	
	printOpenGLError();
	if (slicePlaneNormal.dot(Vect3d(1,1,0)) != 0.0)
	{
		sliceOrtho = slicePlaneNormal.cross(Vect3d(1,1,0));
	}
	else
	{
		sliceOrtho = slicePlaneNormal.cross(Vect3d(0,1,0));
	}
	
	sliceOrtho.unitize();
	
	printOpenGLError();
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
	printOpenGLError();
	for(int i = 0 ; i <= _samples; i++)
	{ 
		float d = (center - slicePoint).mag();
		printOpenGLError();
		
		if (r1 > d)
		{
			float rg0 = 0.0;
			float rg1 = sqrt(r1*r1 - d*d);
			
			if (r0 > d)
			{
				rg0 = sqrt(r0*r0 - d*d);
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
					
					glTexCoord3f(v0.x(), v0.y(), v0.z());          
					glVertex3f(v0.x(), v0.y(), v0.z());
					
					glTexCoord3f(v1.x(), v1.y(), v1.z());          
					glVertex3f(v1.x(), v1.y(), v1.z());
					
				}
			}
			glEnd();
			printOpenGLError();
		}
		
		//
		// increment the slice plane by the slice distance
		//
		slicePoint += sliceDelta;    
		printOpenGLError();
	}
	
	printOpenGLError();
	glFlush();
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
	
	_delta = (maxv - minv).mag() / _samples; 
	_samplingRate = (1.0/(float)(_level+1))*(_shellWidth / _delta) / (float)_nr;
	
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
	
	const int *data_roi  = _lastRegion.roi();
	const float *extents = _lastRegion.extents();
	
	if (_lighting){  
		GLWindow::manager->uploadEffectData(getCurrentEffect(), "dimensions", (float) _nx, (float)_ny, (float)_nz);
		GLWindow::manager->uploadEffectData(getCurrentEffect(),"kd", _kd);
		GLWindow::manager->uploadEffectData(getCurrentEffect(), "ka", _ka);
		GLWindow::manager->uploadEffectData(getCurrentEffect(), "ks", _ks);
		GLWindow::manager->uploadEffectData(getCurrentEffect(), "expS", _expS);
		GLWindow::manager->uploadEffectData(getCurrentEffect(), "lightDirection", (float)_pos[0], (float)_pos[1],(float) _pos[2]);
	}
	
    
    float tmpv[3];
    
    permute(_permutation, tmpv, _nx, _ny, _nz);
    
    _nr = (int)tmpv[2];
    
    permute(_permutation, tmpv, 
            (float)data_roi[0]/(_nx-1),
            (float)data_roi[1]/(_ny-1), 
            (float)data_roi[2]/(_nz-1));
	
	GLWindow::manager->uploadEffectData(getCurrentEffect(), "tmin" ,(float)tmpv[0], (float)tmpv[1], (float)tmpv[2]);
    permute(_permutation, tmpv,
            (float)data_roi[3]/(_nx-1), 
            (float)data_roi[4]/(_ny-1), 
            (float)data_roi[5]/(_nz-1));
	
	GLWindow::manager->uploadEffectData(getCurrentEffect(), "tmax" ,(float)tmpv[0], (float)tmpv[1], (float)tmpv[2]);
    
    permute(_permutation, tmpv, extents[0], extents[1], extents[2]);
	
	GLWindow::manager->uploadEffectData(getCurrentEffect(), "dmin" ,(float)(tmpv[0] + 180.0) / 360.0, (float)(tmpv[1] + 90.0) / 180.0, (float)tmpv[2]);
    _shellWidth = tmpv[2];
    
    permute(_permutation, tmpv, extents[3], extents[4], extents[5]);
    
  	GLWindow::manager->uploadEffectData(getCurrentEffect(), "dmax" ,(float)(tmpv[0] + 180.0) / 360.0, (float)(tmpv[1] + 90.0) / 180.0, (float)tmpv[2]);  
    assert(tmpv[2]);
    _shellWidth = (tmpv[2] - _shellWidth)/(2.0*tmpv[2]);
	
	GLWindow::manager->uploadEffectData(getCurrentEffect(), "permutation" , (float)_permutation[0], (float)_permutation[1], (float)_permutation[2]);
	GLWindow::manager->uploadEffectData(getCurrentEffect(), "clip" , (int)_clip[_permutation[0]], (int)_clip[_permutation[1]]);
	
	printOpenGLError();
}

std::string DVRSpherical::getCurrentEffect()
{
	if (_lighting) {
		_effect = "sphericalLighting";
		return _effect;
	}
	else {
		_effect = "sphericalDefault";
		return _effect;
	}
	
}
