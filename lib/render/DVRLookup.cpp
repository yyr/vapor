//--DVRLookup.h --------------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// A derived DVRTexture3d providing a pixel map for the color lookup.
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

#include "DVRLookup.h"
#include "TextureBrick.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRLookup::DVRLookup(GLenum type, int nthreads) :
  DVRTexture3d(GL_RGBA, GL_COLOR_INDEX, type, nthreads),
  _colormap(NULL)
{

	switch (type) {
	case GL_UNSIGNED_BYTE:
	case GL_BYTE:
		_colormapsize = 256;
	break;
	case GL_UNSIGNED_SHORT:
	case GL_SHORT:
		_colormapsize = 256*2;
	break;
	case GL_UNSIGNED_INT:
	case GL_INT:
	case GL_FLOAT:
		_colormapsize = 256*4;
	break;
		default:
		assert(type == GL_UNSIGNED_BYTE);
	break;
	}

  
  if (GLEW_NV_fragment_program)
  {
    _maxTexture /= 4;
  }
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRLookup::~DVRLookup() 
{
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRLookup::GraphicsInit() 
{
  initColormap();

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRLookup::SetRegion(const RegularGrid *rg, const float range[2], int num)
{ 
  //
  // Construct the color mapping
  //
  glPixelMapfv(GL_PIXEL_MAP_I_TO_R, _colormapsize, _colormap+0*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_G, _colormapsize, _colormap+1*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_B, _colormapsize, _colormap+2*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_A, _colormapsize, _colormap+3*_colormapsize);
  glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

  //
  // Initialize the geometry extents & texture
  //
  return DVRTexture3d::SetRegion(rg, range, num);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRLookup::loadTexture(TextureBrick *brick)
{ 
  glBindTexture(GL_TEXTURE_3D, brick->handle());

  brick->load();

  printOpenGLError();
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRLookup::Render()

{	
  glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK, GL_LINE);

  glEnable(GL_TEXTURE_3D);
  glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
 
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
	
  calculateSampling();
  renderBricks();
	
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_3D);
  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

  return 0;
}

//----------------------------------------------------------------------------
// Set the color table used to render the volume without any opacity 
// correction.
//----------------------------------------------------------------------------
void DVRLookup::SetCLUT(const float ctab[256][4]) 
{
	for (int i=0; i<256; i++) {
		int len = _colormapsize / 256;
		for (int j=0; j<len; j++) {
			_colormap[i*len + j + (0*_colormapsize)] = ctab[i][0];
			_colormap[i*len + j + (1*_colormapsize)] = ctab[i][1];
			_colormap[i*len + j + (2*_colormapsize)] = ctab[i][2];
			_colormap[i*len + j + (3*_colormapsize)] = ctab[i][3];
		}
	}

  //
  // Construct the color mapping
  //
  glPixelMapfv(GL_PIXEL_MAP_I_TO_R, _colormapsize, _colormap+0*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_G, _colormapsize, _colormap+1*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_B, _colormapsize, _colormap+2*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_A, _colormapsize, _colormap+3*_colormapsize);
  glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

  //
  // Reload the 3D texture to pickup the colormap.
  // Is there a better (i.e., faster) way to do this?
  //
  for (int i=0; i<_bricks.size(); i++)
  {
    loadTexture(_bricks[i]);
  }

  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

  printOpenGLError();

  glFlush();
}

//----------------------------------------------------------------------------
// Set the color table used to render the volume applying an opacity 
// correction.
//----------------------------------------------------------------------------
void DVRLookup::SetOLUT(const float atab[256][4], const int numRefinements)
{
  //
  // Compute the sampling distance and rate
  //
  calculateSampling();

  //
  // Calculate opacity correction. Delta is 1 for the fineest refinement, 
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

	int len = _colormapsize / 256;
	for (int j=0; j<len; j++) {
		_colormap[i*len + j + (0*_colormapsize)] = atab[i][0];
		_colormap[i*len + j + (1*_colormapsize)] = atab[i][1];
		_colormap[i*len + j + (2*_colormapsize)] = atab[i][2];
		_colormap[i*len + j + (3*_colormapsize)] = opac;
	}
  }


  //
  // Construct the color mapping
  //
  glPixelMapfv(GL_PIXEL_MAP_I_TO_R, _colormapsize, _colormap+0*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_G, _colormapsize, _colormap+1*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_B, _colormapsize, _colormap+2*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_A, _colormapsize, _colormap+3*_colormapsize);
  glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

  //
  // Reload the 3D texture to pickup the colormap.
  // Is there a better (i.e., faster) way to do this?
  //
  for (int i=0; i<_bricks.size(); i++)
  {
    loadTexture(_bricks[i]);
  }

  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

  printOpenGLError();

  glFlush();
}


//----------------------------------------------------------------------------
// Initalize the the 1D colormap
//----------------------------------------------------------------------------
void DVRLookup::initColormap()
{
  //
  // Create the colormap table
  //
  _colormap = new float[_colormapsize*4];

  glPixelMapfv(GL_PIXEL_MAP_I_TO_R, _colormapsize, _colormap+0*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_G, _colormapsize, _colormap+1*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_B, _colormapsize, _colormap+2*_colormapsize);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_A, _colormapsize, _colormap+3*_colormapsize);

  glFlush();
}

