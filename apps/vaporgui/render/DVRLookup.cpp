//--DVRLookup.h --------------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// A derived DVRTexture3d providing a pixel map for the color lookup.
//
//----------------------------------------------------------------------------

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <qgl.h>

#include "DVRLookup.h"
#include "messagereporter.h"
#include "glutil.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRLookup::DVRLookup(DataType_T type, int nthreads) :
  DVRTexture3d(type, nthreads),
  _data(NULL),
  _colormap(NULL),
  _texid(0),
  _nx(0),
  _ny(0),
  _nz(0)
{
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRLookup::~DVRLookup() 
{
  glDeleteTextures(1, &_texid);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRLookup::GraphicsInit() 
{
  initTextures();

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRLookup::SetRegion(void *data,
                            int nx, int ny, int nz,
                            const int data_roi[6],
                            const float extents[6]) 
{ 
  //
  // Initialize the geometry extents
  //
  DVRTexture3d::SetRegion(data, nx, ny, nz, data_roi, extents);

  //
  // Initialize the texture data
  //
  if (_nx != nx || _ny != ny || _nz != nz)
  {
    _data = data;
    _nx = nx; _ny = ny; _nz = nz;

    //
    // Construct the color mapping
    //
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, _colormap+0*256);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, _colormap+1*256);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, _colormap+2*256);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, _colormap+3*256);
    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, _nx, _ny, _nz, 0, 
                 GL_COLOR_INDEX, GL_UNSIGNED_BYTE, data);
  }
  else if (_data != data)
  {
    _data = data;

    //
    // Construct the color mapping
    //
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, _colormap+0*256);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, _colormap+1*256);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, _colormap+2*256);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, _colormap+3*256);
    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, _nx, _ny, _nz, GL_COLOR_INDEX,
                   GL_UNSIGNED_BYTE, data);  
  }

  printOpenGLError();

  glFlush();

  return 0;
}



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRLookup::Render(const float matrix[16])

{
  glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK, GL_LINE);

  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, _texid);
  glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
 
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  drawViewAlignedSlices();

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_3D);
  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRLookup::HasType(DataType_T type) 
{
  if (type == UINT8) 
    return(1);
  else 
    return(0);
}

//----------------------------------------------------------------------------
// Set the color table used to render the volume without any opacity 
// correction.
//----------------------------------------------------------------------------
void DVRLookup::SetCLUT(const float ctab[256][4]) 
{
  for (int i=0; i<256; i++)
  {
    _colormap[i+0*256] = ctab[i][0];
    _colormap[i+1*256] = ctab[i][1];
    _colormap[i+2*256] = ctab[i][2];
    _colormap[i+3*256] = ctab[i][3];
  }

  //
  // Construct the color mapping
  //
  glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, _colormap+0*256);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, _colormap+1*256);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, _colormap+2*256);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, _colormap+3*256);
  glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

  if (_data)
  {
    //
    // Reload the 3D texture to pickup the colormap.
    // Is there a better (i.e., faster) way to do this?
    //
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, _nx, _ny, _nz, GL_COLOR_INDEX,
                    GL_UNSIGNED_BYTE, _data);  
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
  // Calculate opacity correction. Delta is 1 for the fineest refinement, 
  // multiplied by 2^n (the sampling distance)
  //
  double delta = (double)(1<<numRefinements);

  for(int i=0; i<256; i++) 
  {
    double opac = atab[i][3];

    opac = 1.0 - pow((1.0 - opac), delta);
  
    if (opac > 1.0)
    {
      opac = 1.0;
    }

    _colormap[i+0*256] = atab[i][0];
    _colormap[i+1*256] = atab[i][1];
    _colormap[i+2*256] = atab[i][2];
    _colormap[i+3*256] = opac;
  }

  //
  // Construct the color mapping
  //
  glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, _colormap+0*256);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, _colormap+1*256);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, _colormap+2*256);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, _colormap+3*256);
  glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

  if (_data)
  {
    //
    // Reload the 3D texture to pickup the colormap.
    // Is there a better (i.e., faster) way to do this?
    //
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, _nx, _ny, _nz, GL_COLOR_INDEX,
                    GL_UNSIGNED_BYTE, _data);  
  }

  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

  printOpenGLError();

  glFlush();
}


//----------------------------------------------------------------------------
// Initalize the textures (i.e., 3d volume texture and the 1D colormap texture)
//----------------------------------------------------------------------------
void DVRLookup::initTextures()
{
  //
  // Setup the 3d texture
  //
  glGenTextures(1, &_texid);
  glBindTexture(GL_TEXTURE_3D, _texid);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  

  // Set texture border behavior
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  
  // Set texture interpolation method
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  //
  // Create the colormap table
  //
  _colormap = new float[256*4];

  glFlush();
}

