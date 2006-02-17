//-- TextureBrick.cpp --------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Texture Brick class
//
//----------------------------------------------------------------------------

#include "TextureBrick.h"
#include "session.h"
#include "glutil.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
TextureBrick::TextureBrick() :
  _nx(0),
  _ny(0),
  _nz(0),
  _center(0,0,0),
  _dmin(0,0,0),
  _dmax(0,0,0),
  _vmin(0,0,0),
  _vmax(0,0,0),
  _tmin(0,0,0),
  _tmax(0,0,0),
  _texid(0),
  _data(NULL)
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
  printOpenGLError();
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
TextureBrick::~TextureBrick()
{
  glDeleteTextures(1, &_texid);
}

//----------------------------------------------------------------------------
// Set the minimum extent of the brick's data block
//----------------------------------------------------------------------------
void TextureBrick::dataMin(float x, float y, float z) 
{ 
  _dmin.x = x; 
  _dmin.y = y; 
  _dmin.z = z; 
}

//----------------------------------------------------------------------------
// Set the maximum extent of the brick's data block
//----------------------------------------------------------------------------
void TextureBrick::dataMax(float x, float y, float z) 
{ 
  _dmax.x = x; 
  _dmax.y = y; 
  _dmax.z = z; 
}

//----------------------------------------------------------------------------
// Set the extents of the brick's data block from the voxel coordinates.
//----------------------------------------------------------------------------
void TextureBrick::setDataBlock(int level, const int block[6])
{
  double extents[3];
  size_t min[3] = {block[0], block[1], block[2]};
  size_t max[3] = {block[3], block[4], block[5]};

  Session::getInstance()->mapVoxelToUserCoords(level, min, extents);
  _dmin.x = extents[0]; 
  _dmin.y = extents[1]; 
  _dmin.z = extents[2]; 

  Session::getInstance()->mapVoxelToUserCoords(level, max, extents);
  _dmax.x = extents[0]; 
  _dmax.y = extents[1]; 
  _dmax.z = extents[2]; 

  _center.x = _dmin.x + (_dmax.x - _dmin.x) / 2.0;
  _center.y = _dmin.y + (_dmax.y - _dmin.y) / 2.0;
  _center.z = _dmin.z + (_dmax.z - _dmin.z) / 2.0;
}

//----------------------------------------------------------------------------
// Set the extents of the brick's roi from the voxel coordinates.
//----------------------------------------------------------------------------
void TextureBrick::setROI(int level, const int roi[6])
{
  double extents[3];
  size_t min[3] = {roi[0], roi[1], roi[2]};
  size_t max[3] = {roi[3], roi[4], roi[5]};

  Session::getInstance()->mapVoxelToUserCoords(level, min, extents);
  _vmin.x = extents[0]; 
  _vmin.y = extents[1]; 
  _vmin.z = extents[2]; 

  Session::getInstance()->mapVoxelToUserCoords(level, max, extents);
  _vmax.x = extents[0]; 
  _vmax.y = extents[1]; 
  _vmax.z = extents[2]; 
}


//----------------------------------------------------------------------------
// Set the minimum extent of the brick's roi
//----------------------------------------------------------------------------
void TextureBrick::volumeMin(float x, float y, float z) 
{ 
  _vmin.x = x; 
  _vmin.y = y; 
  _vmin.z = z; 
}

//----------------------------------------------------------------------------
// Set the maximum extent of the brick's roi
//----------------------------------------------------------------------------
void TextureBrick::volumeMax(float x, float y, float z) 
{ 
  _vmax.x = x; 
  _vmax.y = y; 
  _vmax.z = z; 
}

//----------------------------------------------------------------------------
// Set the brick's minimum texture coordinate
//----------------------------------------------------------------------------
void TextureBrick::textureMin(float x, float y, float z) 
{ 
  _tmin.x = x; 
  _tmin.y = y; 
  _tmin.z = z; 
}

//----------------------------------------------------------------------------
// Set the brick's maximum texture coordinate
//----------------------------------------------------------------------------
void TextureBrick::textureMax(float x, float y, float z) 
{ 
  _tmax.x = x; 
  _tmax.y = y; 
  _tmax.z = z; 
}

//----------------------------------------------------------------------------
// Invalidate the brick. An invalid brick will be ignored by the renderer.
//----------------------------------------------------------------------------
void TextureBrick::invalidate()
{
  _nx = 0;
  _ny = 0;
  _nz = 0;
  _dmin = Point3d(0,0,0);
  _dmax = Point3d(0,0,0);
  _vmin = Point3d(0,0,0);
  _vmax = Point3d(0,0,0);
  _tmin = Point3d(0,0,0);
  _tmax = Point3d(0,0,0);
  _data = NULL;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool TextureBrick::valid() const
{
  return (_tmax.x > _tmin.x && _tmax.y > _tmin.y && _tmax.z > _tmin.z);
}

