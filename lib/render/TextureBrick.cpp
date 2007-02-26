//-- TextureBrick.cpp --------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Texture Brick class
//
//----------------------------------------------------------------------------

#include "TextureBrick.h"
#include "regionparams.h"
#include "glutil.h"

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
TextureBrick::TextureBrick() :
  _nx(0),
  _ny(0),
  _nz(0),
  _xoffset(0),
  _yoffset(0),
  _zoffset(0),
  _center(0,0,0),
  _dmin(0,0,0),
  _dmax(0,0,0),
  _vmin(0,0,0),
  _vmax(0,0,0),
  _tmin(0,0,0),
  _tmax(0,0,0),
  _texid(0),
  _format(GL_LUMINANCE),
  _data(NULL),
  _haveOwnership(false),
  _dnx(0),
  _dny(0),
  _dnz(0)
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

  if (_haveOwnership)
  {
    delete [] _data;
  }

  _data = NULL;
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
void TextureBrick::setDataBlock(int level, const int box[6], const int block[6])
{
  float extents[6];
  size_t min[3] = {block[0]+box[0], block[1]+box[1], block[2]+box[2]};
  size_t max[3] = {block[3]+box[0], block[4]+box[1], block[5]+box[2]};

  RegionParams::convertToBoxExtentsInCube(level, min, max, extents);

  _dmin.x = extents[0]; 
  _dmin.y = extents[1]; 
  _dmin.z = extents[2]; 
  _dmax.x = extents[3]; 
  _dmax.y = extents[4]; 
  _dmax.z = extents[5]; 

  _center.x = _dmin.x + (_dmax.x - _dmin.x) / 2.0;
  _center.y = _dmin.y + (_dmax.y - _dmin.y) / 2.0;
  _center.z = _dmin.z + (_dmax.z - _dmin.z) / 2.0;
}

//----------------------------------------------------------------------------
// Set the extents of the brick's roi from the voxel coordinates.
//----------------------------------------------------------------------------
void TextureBrick::setROI(int level, const int box[6], const int roi[6])
{
  float extents[6];
  size_t min[3] = {roi[0]+box[0], roi[1]+box[1], roi[2]+box[2]};
  size_t max[3] = {roi[3]+box[0], roi[4]+box[1], roi[5]+box[2]};

  RegionParams::convertToBoxExtentsInCube(level, min, max, extents);

  _vmin.x = extents[0]; 
  _vmin.y = extents[1]; 
  _vmin.z = extents[2]; 
  _vmax.x = extents[3]; 
  _vmax.y = extents[4]; 
  _vmax.z = extents[5]; 
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
// Determine if this brick is valid.
//----------------------------------------------------------------------------
bool TextureBrick::valid() const
{
  return (_tmax.x > _tmin.x && _tmax.y > _tmin.y && _tmax.z > _tmin.z);
}


//----------------------------------------------------------------------------
// Load data into the texture object
//----------------------------------------------------------------------------
void TextureBrick::load(GLint internalFormat, GLint format)
{
  _format = format;

  glBindTexture(GL_TEXTURE_3D, _texid);

// 
// If we "own" the data, it's stored in texture bricks that are
// guaranteed to be powers-of-two (dimensioned by _nx,_ny,_nz). 
// Similary true, if we don't own the data but the data dimensions match
// the "next power of two dimensions" (_nx,_ny, _nz).  N.B. In the
// former case, the bricks need only be partially filled with valid data.
//
// else, the data volume pointed to by _data are now a power-of-two
// and will need to be download as a subimage.
//
  if (_haveOwnership || (_dnx==_nx && _dny==_ny) && (_dnz==_nz)) 
  {
    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat,
                 _nx, _ny, _nz, 0, _format, GL_UNSIGNED_BYTE, _data);
  }
  else
  {
    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat,
                 _nx, _ny, _nz, 0, _format, GL_UNSIGNED_BYTE, NULL);

    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
                    _dnx, _dny, _dnz, _format, GL_UNSIGNED_BYTE, _data);
  }
}

//----------------------------------------------------------------------------
// Reload data into the texture object.
//----------------------------------------------------------------------------
void TextureBrick::reload()
{
  glBindTexture(GL_TEXTURE_3D, _texid);

  if (_haveOwnership || (_dnx==_nx && _dny==_ny) && (_dnz==_nz)) 
  {
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, _nx, _ny, _nz, 
                    _format, GL_UNSIGNED_BYTE, _data);
  }
  else
  {
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, _dnx, _dny, _dnz, 
                    _format, GL_UNSIGNED_BYTE, _data);
  }
}


//----------------------------------------------------------------------------
// Fill the texture brick. This method is used to deep copy a subset of the 
// data into the brick. (Used when bricking a volume).
//----------------------------------------------------------------------------
void TextureBrick::fill(GLubyte *data, 
                        int bx, int by, int bz,
                        int nx, int ny, int nz, 
                        int xoffset, int yoffset, int zoffset)
{ 

  _nx = bx;
  _ny = by;
  _nz = bz;

  _dnx = nx;
  _dny = ny;
  _dnz = nz;

  _xoffset = xoffset;
  _yoffset = yoffset;
  _zoffset = zoffset;

  if (_haveOwnership)
  {
    delete [] _data;
  }

  _haveOwnership = true;
  _data = new GLubyte[_nx*_ny*_nz];

  //
  // Copy over the data
  //
  for (int z=0; z<bz && z+zoffset<=_dnz; z++)
  {
    for (int y=0; y<by && y+yoffset<=_dny; y++)
    {
      int di = (xoffset +
                MIN(y+yoffset, _dny-1)*_dnx + 
                MIN(z+zoffset, _dnz-1)*_dnx*_dny);

      int ti = y*_nx + z*_nx*_ny;

      memcpy(&_data[ti], &data[di], MIN(bx, _dnx-_xoffset)*sizeof(GLubyte));
    }
  }
}

//----------------------------------------------------------------------------
// Points the brick to a contigous block of memory. Used when a volume will
// fit wholly into graphics memory, and bricking is not needed (i.e., there
// is only one brick, this one). 
//----------------------------------------------------------------------------
void TextureBrick::fill(GLubyte *data, int nx, int ny, int nz)
{ 
  _dnx = nx;
  _dny = ny;
  _dnz = nz;

  _nx = nextPowerOf2(nx);
  _ny = nextPowerOf2(ny);
  _nz = nextPowerOf2(nz);

  _xoffset = 0;
  _yoffset = 0;
  _zoffset = 0;

  if (_haveOwnership)
  {
    delete [] _data;
  }

  _haveOwnership = false;
  _data = data;
}

//----------------------------------------------------------------------------
// Update the brick's data.
//----------------------------------------------------------------------------
void TextureBrick::refill(GLubyte *data)
{ 
  if (_haveOwnership)
  {
    //
    // Copy over the data
    //
    for (int z=0; z<_nz && z+_zoffset<=_dnz; z++)
    {
      for (int y=0; y<_ny && y+_yoffset<=_dny; y++)
      {
        int di = (_xoffset +
                  MIN(y+_yoffset, _dny-1)*_dnx + 
                  MIN(z+_zoffset, _dnz-1)*_dnx*_dny);
        
        int ti = y*_nx + z*_nx*_ny;
        
        memcpy(&_data[ti], &data[di], MIN(_nx, _dnx-_xoffset)*sizeof(GLubyte));
      }
    }
  }
  else
  {
    _data = data;
  }

  reload();
}
