//-- TextureBrick.h ----------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Texture Brick class
//
//----------------------------------------------------------------------------

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "BBox.h"
#include "Point3d.h"

namespace VAPoR {

class TextureBrick
{
 public:
   
  TextureBrick();
  virtual ~TextureBrick();

  void bind() const     { glBindTexture(GL_TEXTURE_3D, _texid); }
  GLuint handle() const { return _texid; }

  void size(int nx, int ny, int nz) { _nx=nx; _ny=ny; _nz=nz; }

  int nx() const { return _nx; }
  int ny() const { return _ny; }
  int nz() const { return _nz; }

  void data(void *d) { _data = d; }
  void* data()       { return _data; }

  //
  // Accessors for the brick's data block extents
  //
  BBox dataBox() const    { return BBox(_dmin, _dmax); }

  Point3d dataMin() const { return _dmin; }
  Point3d dataMax() const { return _dmax; }

  void dataMin(float x, float y, float z);
  void dataMax(float x, float y, float z);

  void setDataBlock(int level, const int block[6]);

  //
  // Accessors for the brick's region of interest extents
  //
  BBox volumeBox() const  { return BBox(_vmin, _vmax); }

  Point3d volumeMin() const { return _vmin; }
  Point3d volumeMax() const { return _vmax; }

  void volumeMin(float x, float y, float z);
  void volumeMax(float x, float y, float z);

  void setROI(int level, const int roi[6]);

  //
  // Accessor for the brick's texture coordinates
  //
  BBox textureBox() const { return BBox(_tmin, _tmax); }

  Point3d textureMin() const { return _tmin; }
  Point3d textureMax() const { return _tmax; }

  void textureMin(float x, float y, float z); 
  void textureMax(float x, float y, float z); 

  //
  // Brick's center. Used for sorting. 
  //
  Point3d center() const { return _center; }
  void    center(const Point3d &center) { _center = center; }

  void invalidate();
  bool valid() const;

 protected:

  // Data size
  int _nx;
  int _ny;
  int _nz;

  // Brick center
  Point3d _center;

  // data extents
  Point3d _dmin;
  Point3d _dmax;

  // visible volume extents
  Point3d _vmin;
  Point3d _vmax;

  // Texture extents
  Point3d _tmin;
  Point3d _tmax;

  // GL texture handle
  GLuint _texid;

  // Data
  void *_data;
};

};
