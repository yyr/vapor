//-- TextureBrick.h ----------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Texture Brick class
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

#include <iostream>

#include <vapor/RegularGrid.h>
#include <vapor/LayeredGrid.h>
#include "BBox.h"
#include "Point3d.h"

namespace VAPoR {

class RENDER_API TextureBrick
{
 public:
   
  //
  // precision : num bits for internal texture representation (8 or 16)
  // nvars : number of variables to be stored (1..4)
  //
  // N.B. the total number of components that can be stored is 4. One
  // component is needed for each variable, one component is needed if 
  // rg contains missing data, and one component is needed if rg is a
  // LayeredGrid.
  //
  TextureBrick(const RegularGrid *rg, int precision, int nvars);
  virtual ~TextureBrick();

  void bind() const     { glBindTexture(GL_TEXTURE_3D, _texid); }
  GLuint handle() const { return _texid; }

  void size(int bx, int by, int bz) { _bx=bx; _by=by; _bz=bz; }

  int nx() const { return _bx; }
  int ny() const { return _by; }
  int nz() const { return _bz; }

  int xoffset() const { return _xoffset; }
  int yoffset() const { return _yoffset; }
  int zoffset() const { return _zoffset; }

  size_t ncomp() const { return _ncomp; }
  size_t szcomp() const { return _szcomp; }
  int nvars() const { return _nvars; }

  void* data() { return _data; }

  void load();
  void reload();

  void copytex(
	const RegularGrid *rg, unsigned char *data, const float range[2],
	int bx, int by, int bz,
	int dnx, int dny, int dnz,
	int xoffset=0, int yoffset=0, int zoffset=0
  );
  void copytex_fast(
	const RegularGrid *rg, unsigned char *data, const float range[2],
	int bx, int by, int bz,
	int dnx, int dny, int dnz,
	int xoffset=0, int yoffset=0, int zoffset=0
  );

  void fill(const RegularGrid *rg, 
			const float range[2], int num,
            int bx, int by, int bz,
            int dnx, int dny, int dnz,
            int xoffset=0, int yoffset=0, int zoffset=0
  );

  void fill(
	const RegularGrid *rg, const float range[2], int num
  );

  void refill(
	const RegularGrid *rg, const float range[2], int num
  );


  //
  // Accessors for the brick's data block extents
  //
  BBox dataBox() const    { return BBox(_dmin, _dmax); }

  Point3d dataMin() const { return _dmin; }
  Point3d dataMax() const { return _dmax; }

  void dataMin(float x, float y, float z);
  void dataMax(float x, float y, float z);

  void setDataBlock(int level, const int box[6], const int block[6]);

  //
  // Accessors for the brick's region of interest extents
  //
  BBox volumeBox() const  { return BBox(_vmin, _vmax); }

  Point3d volumeMin() const { return _vmin; }
  Point3d volumeMax() const { return _vmax; }

  void volumeMin(float x, float y, float z);
  void volumeMax(float x, float y, float z);

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
  Point3d center() const ;

  void invalidate();
  bool valid() const;

  //----------------------------------------------------------------------------
  // Determine and return the maximum texture size (in bytes).
  //----------------------------------------------------------------------------
  static int maxTextureSize(
    const RegularGrid *rg, int precision, int nvars
  );


  //
  // Friend functions
  //
  friend std::ostream &operator<<(std::ostream &o, const TextureBrick &b)
  {
    o << "Brick " << b.handle() << std::endl
      << " " << b.nx() << "x" << b.ny() << "x" << b.nz() << std::endl
      << " data extents " << b.dataMin() 
      << "              " << b.dataMax()
      << " roi extents  " << b.volumeMin()
      << "              " << b.volumeMax()
      << " tex extents  " << b.textureMin()
      << "              " << b.textureMax()
      << " center       " << b.center() << std::endl;
    
    return o;
  }

 protected:

  // Brick size (power-of-2)
  int _bx;
  int _by;
  int _bz;

  // Brick's voxel offset into data
  int _xoffset;
  int _yoffset;
  int _zoffset;

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

  // Texture internal data format
  GLint _internalFormat;

  // Texture data format
  GLenum _format;

  // Texture data type
  GLenum _type;

  // number of texture components (1,2,3,4)
  size_t _ncomp;

  // size of texture components in bytes
  size_t _szcomp;

  // Data
  GLubyte *_data;
  size_t   _texSzBytes;

  // Data size (maybe non-power-2 < brick size)
  int _dnx;
  int _dny;
  int _dnz;

  bool _missing;	// Grid has missing data
  bool _layered;	// grid is of type LayeredGrid
  int _nvars;		// # of vars stored in a texture.

private:
  static int _configure(
    const RegularGrid *rg, int precision, int nvars,
    GLenum &format, GLint &internalFormat, size_t &ncomp, GLenum &type,
    bool &missing, bool &layered
  );

};

};
