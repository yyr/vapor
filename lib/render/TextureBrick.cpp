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

#define DEFAULT_MAX_TEXTURE 512

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
TextureBrick::TextureBrick(const RegularGrid *rg, int precision, int nvars) :
  _bx(0),
  _by(0),
  _bz(0),
  _xoffset(0),
  _yoffset(0),
  _zoffset(0),
  _dmin(0,0,0),
  _dmax(0,0,0),
  _vmin(0,0,0),
  _vmax(0,0,0),
  _tmin(0,0,0),
  _tmax(0,0,0),
  _texid(0),
  _data(NULL),
  _texSzBytes(0),
  _dnx(0),
  _dny(0),
  _dnz(0),
  _missing(false),
  _layered(false),
  _nvars(nvars)

{

  _configure(
    rg, precision, nvars,
	_format, _internalFormat, _ncomp, _type, _missing, _layered
  );
	
  if (_type == GL_UNSIGNED_BYTE) {
    _szcomp = 1;
  }
  else {
    _szcomp = 2;
  }
	
  //
  // Setup the 3d texture
  // 
  // Does this do anything? Should be called just prior to loading
  // texture I think (inside ::load())
  //
  glGenTextures(1, &_texid);
  glBindTexture(GL_TEXTURE_3D, _texid);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  
  //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

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

  if (_data) delete [] _data;

  printOpenGLError();
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

Point3d TextureBrick::center() const {
  Point3d center;

  center.x = _dmin.x + (_dmax.x - _dmin.x) / 2.0;
  center.y = _dmin.y + (_dmax.y - _dmin.y) / 2.0;
  center.z = _dmin.z + (_dmax.z - _dmin.z) / 2.0;
  return(center);
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
  _bx = 0;
  _by = 0;
  _bz = 0;
  _dmin = Point3d(0,0,0);
  _dmax = Point3d(0,0,0);
  _vmin = Point3d(0,0,0);
  _vmax = Point3d(0,0,0);
  _tmin = Point3d(0,0,0);
  _tmax = Point3d(0,0,0);
  if (_data) delete [] _data;
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
void TextureBrick::load()
{

  glBindTexture(GL_TEXTURE_3D, _texid);

// 
// If we "own" the data, it's stored in texture bricks that are
// guaranteed to be powers-of-two (dimensioned by _bx,_by,_bz). 
// Similary true, if we don't own the data but the data dimensions match
// the "next power of two dimensions" (_bx,_by, _bz).  N.B. In the
// former case, the bricks need only be partially filled with valid data.
//
// else, the data volume pointed to by _data are now a power-of-two
// and will need to be download as a subimage.
//

  glBindTexture(GL_TEXTURE_3D, _texid);
  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glTexImage3D(GL_TEXTURE_3D, 0, _internalFormat,
                 _bx, _by, _bz, 0, _format, _type, _data);
}

//----------------------------------------------------------------------------
// Reload data into the texture object.
//----------------------------------------------------------------------------
void TextureBrick::reload()
{
  glBindTexture(GL_TEXTURE_3D, _texid);
  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glTexImage3D(GL_TEXTURE_3D, 0, _internalFormat,
                 _bx, _by, _bz, 0, _format, _type, _data);
}

void TextureBrick::copytex_fast(
	const RegularGrid *rg, unsigned char *data, const float range[2], 
	int bx, int by, int bz, int dnx, int dny, int dnz,
	int xoffset, int yoffset, int zoffset
) {


  size_t dims[3];
  rg->GetDimensions(dims);

	//
	// Copy over the data
	//
	float v;
	unsigned int qv;
	unsigned char *ucptr = data;
	size_t step = _szcomp * _ncomp;

	if (xoffset == 0 && yoffset == 0 && zoffset == 0 && 
		dims[0]<=bx && dims[1]<=by && dims[2]<= bz
	) {
		// Fast copy with iterator

		RegularGrid::Iterator itr;

		// kludge - no const_iterator
		RegularGrid *rg_const = (RegularGrid *) rg;	
		if (_szcomp == 1) {
			for (itr = rg_const->begin(); itr!=rg_const->end(); ++itr) {
				v = *itr;

				if (v<range[0]) qv=0;
				else if (v>range[1]) qv=255;
				else qv = (unsigned int) rint((v-range[0])/(range[1]-range[0]) * 255);

				ucptr[0] = qv;
				ucptr+=step;
			}
		} else if (_szcomp == 2) {
			for (itr = rg_const->begin(); itr!=rg_const->end(); ++itr) {
				v = *itr;

				if (v<range[0]) qv=0;
				else if (v>range[1]) qv=65535;
				else qv = (unsigned int) rint((v-range[0])/(range[1]-range[0]) * 65535);

				ucptr[0] = (unsigned char) (qv & 0xff);
				ucptr[1] = (unsigned char) ((qv >> 8) & 0xff);

				ucptr+=step;
			}
		}
	}
	else {

		if (_szcomp == 1) {
			for (int z=0; z<bz && z<dnz; z++) {
			for (int y=0; y<by && y<dny; y++) {
			ucptr = data+z*bx*by*step + y*bx*step;
			for (int x=0; x<bx && x<dnx; x++) {
				v = rg->AccessIJK(x+xoffset,y+yoffset,z+zoffset);

				if (v<range[0]) qv=0;
				else if (v>range[1]) qv=255;
				else qv = (unsigned int) rint((v-range[0])/(range[1]-range[0]) * 255);

				ucptr[0] = qv;
				ucptr += step;
			}
			}
			}
		}
		else if (_szcomp == 2) {
			for (int z=0; z<bz && z<dnz; z++) {
			for (int y=0; y<by && y<dny; y++) {
			ucptr = data+z*bx*by*step + y*bx*step;
			for (int x=0; x<bx && x<dnx; x++) {
				v = rg->AccessIJK(x+xoffset,y+yoffset,z+zoffset);

				if (v<range[0]) qv=0;
				else if (v>range[1]) qv=65535;
				else qv = (unsigned int) rint((v-range[0])/(range[1]-range[0]) * 65535);

				ucptr[0] = (unsigned char) (qv & 0xff);
				ucptr[1] = (unsigned char) ((qv >> 8) & 0xff);
				ucptr+=step;
			}
			}
			}
		}
	}
}

void TextureBrick::copytex(
	const RegularGrid *rg, unsigned char *data, const float range[2], 
	int bx, int by, int bz, int dnx, int dny, int dnz,
	int xoffset, int yoffset, int zoffset
) {

  size_t dims[3];
  rg->GetDimensions(dims);

float myrange[2];
rg->GetRange(myrange);
cout << "RG Range : " << myrange[0] << " " << myrange[1] << endl;

	//
	// Copy over the data
	//
	float v;
	unsigned int qv;
	unsigned char *ucptr = data;
	size_t step = _szcomp * _ncomp;
	double x_f, y_f, z_f;

	double extents[6];
	rg->GetUserExtents(extents);
	float mv = 0;
	if (_missing) mv = rg->GetMissingValue();

	if (xoffset == 0 && yoffset == 0 && zoffset == 0 && 
		dims[0]<=bx && dims[1]<=by && dims[2]<= bz
	) {
		size_t x = 0;
		size_t y = 0;
		size_t z = 0;
    
		// Fast copy with iterator

		RegularGrid::Iterator itr;

		// kludge - no const_iterator
		RegularGrid *rg_const = (RegularGrid *) rg;	
		if (_szcomp == 1) {
			for (itr = rg_const->begin(); itr!=rg_const->end(); ++itr) {
				v = *itr;
				int c = 0;

				if (v<range[0]) qv=0;
				else if (v>range[1]) qv=255;
				else qv = (unsigned int) rint((v-range[0])/(range[1]-range[0]) * 255);

				ucptr[c++] = qv;

				if (_missing) {
					if (v == mv ) ucptr[c++] = 255;
					else ucptr[c++] = 0;
				}
				if (_layered) {
					(void) rg->GetUserCoordinates(x,y,z,&x_f, &y_f, &z_f);
					qv = (unsigned int) rint(
						(z_f-extents[2])/(extents[5]-extents[2]) * 255
					);
					ucptr[c++] = qv;
				}
				ucptr+=step;

				x++;
				if (x>=dnx) {
					x = 0;
					y++;
				}
				if (y>=dny) {
					y = 0;
					z++;
				}
			}
		} else if (_szcomp == 2) {
			for (itr = rg_const->begin(); itr!=rg_const->end(); ++itr) {
				int c = 0;
				v = *itr;

				if (v<range[0]) qv=0;
				else if (v>range[1]) qv=65535;
				else qv = (unsigned int) rint((v-range[0])/(range[1]-range[0]) * 65535);

				ucptr[c++] = (unsigned char) (qv & 0xff);
				ucptr[c++] = (unsigned char) ((qv >> 8) & 0xff);

				if (_missing) {
					if (v == mv ) {
						ucptr[c++] = (unsigned char) 0xff;
						ucptr[c++] = (unsigned char) 0xff;
					}
					else {
						ucptr[c++] = 0;
						ucptr[c++] = 0;
					}
				}
				if (_layered) {
					(void) rg->GetUserCoordinates(x,y,z,&x_f, &y_f, &z_f);
					qv = (unsigned int) rint(
						(z_f-extents[2])/(extents[5]-extents[2]) * 65535
					);
					ucptr[c++] = (unsigned char) (qv & 0xff);
					ucptr[c++] = (unsigned char) ((qv >> 8) & 0xff);
				}

				ucptr+=step;

				x++;
				if (x>=dnx) {
					x = 0;
					y++;
				}
				if (y>=dny) {
					y = 0;
					z++;
				}
			}
		}
	}
	else {

		if (_szcomp == 1) {
			for (int z=0; z<bz && z<dnz; z++) {
			for (int y=0; y<by && y<dny; y++) {
			ucptr = data+z*bx*by*step + y*bx*step;
			for (int x=0; x<bx && x<dnx; x++) {
				int c = 0;
				v = rg->AccessIJK(x+xoffset,y+yoffset,z+zoffset);

				if (v<range[0]) qv=0;
				else if (v>range[1]) qv=255;
				else qv = (unsigned int) rint((v-range[0])/(range[1]-range[0]) * 255);

				ucptr[c++] = qv;
				if (_missing) {
					if (v == mv ) ucptr[c++] = 255;
					else ucptr[c++] = 0;
				}
				if (_layered) {
					(void) rg->GetUserCoordinates(x,y,z,&x_f, &y_f, &z_f);
					qv = (unsigned int) rint(
						(z_f-extents[2])/(extents[5]-extents[2]) * 255
					);
					ucptr[c++] = qv;
				}
				ucptr += step;
			}
			}
			}
		}
		else if (_szcomp == 2) {
			for (int z=0; z<bz && z<dnz; z++) {
			for (int y=0; y<by && y<dny; y++) {
			ucptr = data+z*bx*by*step + y*bx*step;
			for (int x=0; x<bx && x<dnx; x++) {
				int c = 0;
				v = rg->AccessIJK(x+xoffset,y+yoffset,z+zoffset);

				if (v<range[0]) qv=0;
				else if (v>range[1]) qv=65535;
				else qv = (unsigned int) rint((v-range[0])/(range[1]-range[0]) * 65535);

				ucptr[c++] = (unsigned char) (qv & 0xff);
				ucptr[c++] = (unsigned char) ((qv >> 8) & 0xff);
				if (_missing) {
					if (v == mv ) {
						ucptr[c++] = (unsigned char) 0xff;
						ucptr[c++] = (unsigned char) 0xff;
					}
					else {
						ucptr[c++] = 0;
						ucptr[c++] = 0;
					}
				}
				if (_layered) {
					(void) rg->GetUserCoordinates(x,y,z,&x_f, &y_f, &z_f);
					qv = (unsigned int) rint(
						(z_f-extents[2])/(extents[5]-extents[2]) * 65535
					);
					ucptr[c++] = (unsigned char) (qv & 0xff);
					ucptr[c++] = (unsigned char) ((qv >> 8) & 0xff);
				}
				ucptr+=step;
			}
			}
			}
		}
	}
}



//----------------------------------------------------------------------------
// Fill the texture brick. This method is used to deep copy a subset of the 
// data into the brick. (Used when bricking a volume).
//----------------------------------------------------------------------------
void TextureBrick::fill(const RegularGrid *rg, 
						const float range[2], int num,
                        int bx, int by, int bz,
						int dnx, int dny, int dnz,
                        int xoffset, int yoffset, int zoffset
) { 
  assert(num < _nvars);

  _bx = bx;
  _by = by;
  _bz = bz;

  _dnx = dnx;
  _dny = dny;
  _dnz = dnz;

  _xoffset = xoffset;
  _yoffset = yoffset;
  _zoffset = zoffset;

  size_t size = _ncomp*_szcomp;
  if (_texSzBytes < _bx*_by*_bz*size) {
      assert(num == 0);
	  if (_data) delete [] _data;
	  _data = new GLubyte[_bx*_by*_bz*size];
      _texSzBytes = _bx*_by*_bz*size;
  }
  assert(_data != NULL);

  //
  // Copy over the data
  //
  if ((! _missing  && ! _layered) || num != 0) {
    //
    // Only variable # 0 contains useable missing or layered data info.
    // 
    size_t offset = num;
	if (_missing) offset++;
	if (_layered) offset++;
	offset *= _szcomp;
	
    copytex_fast(
	  rg, _data + offset, range, _bx, _by, _bz, _dnx, _dny, _dnz,
	  _xoffset, _yoffset, _zoffset
    );
  }
  else {
    copytex(
	  rg, _data, range, _bx, _by, _bz, _dnx, _dny, _dnz,
	  _xoffset, _yoffset, _zoffset
    );
  }

}

//----------------------------------------------------------------------------
// Points the brick to a contigous block of memory. Used when a volume will
// fit wholly into graphics memory, and bricking is not needed (i.e., there
// is only one brick, this one). 
//----------------------------------------------------------------------------
void TextureBrick::fill(
	const RegularGrid *rg, const float range[2], int num
) { 
  size_t dims[3];
  rg->GetDimensions(dims);

  _dnx = dims[0];
  _dny = dims[1];
  _dnz = dims[2];

  _bx = dims[0];
  _by = dims[1];
  _bz = dims[2];

  _xoffset = 0;
  _yoffset = 0;
  _zoffset = 0;

  size_t size = _ncomp*_szcomp;
  if (_texSzBytes < _bx*_by*_bz*size) {
	  if (_data) delete [] _data;
	  _data = new GLubyte[_bx*_by*_bz*size];
      _texSzBytes = _bx*_by*_bz*size;
  }

  //
  // Copy over the data
  //
  if ((! _missing  && ! _layered) || num != 0) {
    //
    // Only variable # 0 contains useable missing or layered data info.
    // 
    size_t offset = num;
	if (_missing) offset++;
	if (_layered) offset++;
	offset *= _szcomp;
	
    copytex_fast(
	  rg, _data + offset, range, _bx, _by, _bz, _dnx, _dny, _dnz,
	  _xoffset, _yoffset, _zoffset
    );
  }
  else {
    copytex(
	  rg, _data, range, _bx, _by, _bz, _dnx, _dny, _dnz,
	  _xoffset, _yoffset, _zoffset
    );
  }

}

//----------------------------------------------------------------------------
// Update the brick's data.
//----------------------------------------------------------------------------
void TextureBrick::refill(
	const RegularGrid *rg, const float range[2], int num
) { 
  //
  // Copy over the data
  //
  if ((! _missing  && ! _layered) || num != 0) {
    //
    // Only variable # 0 contains useable missing or layered data info.
    // 
    size_t offset = num;
	if (_missing) offset++;
	if (_layered) offset++;
	offset *= _szcomp;

    copytex_fast(
	  rg, _data + offset, range, _bx, _by, _bz, _dnx, _dny, _dnz,
	  _xoffset, _yoffset, _zoffset
    );
  }
  else {
    copytex(
	  rg, _data, range, _bx, _by, _bz, _dnx, _dny, _dnz,
	  _xoffset, _yoffset, _zoffset
    );
  }
}



//----------------------------------------------------------------------------
// Determine and return the maximum texture size (in bytes). 
//----------------------------------------------------------------------------
int TextureBrick::maxTextureSize(
  const RegularGrid *rg, int precision, int nvars
) {

  const char *s = (const char *) glGetString(GL_VENDOR);
  if (! s) return(128);
  string glvendor;
  for(int i=0; i<strlen(s); i++) {
    glvendor.append(1, (char) toupper(s[i]));
  }

  if ((glvendor.find("INTEL") != string::npos) || 
	  (glvendor.find("SGI") != string::npos)) {
		
    return(128);
  }

  GLenum format, type;
  GLint internalFormat;
  size_t ncomp;
  bool missing, layered;

  int rc = _configure(
    rg, precision, nvars,
	format, internalFormat, ncomp, type, missing, layered
  );
  if (rc < 0) return(128);


  // Upper limit on texture size - maximum value returned by this 
  // function. 
  //
  // N.B. The texture proxy method of determining maximum texture
  // size supported by the card has not been reliable with nVidia
  // drivers, in some instances returning values larger than what the
  // card will actually support.
  //

  if (GLEW_ATI_fragment_shader)
  {
    return (256);
  }

  for (int i = 16; i < 2*DEFAULT_MAX_TEXTURE; i*=2)
  {
    glTexImage3D(GL_PROXY_TEXTURE_3D, 0, internalFormat, i, i, i, 0,
                 format, type, NULL);

    GLint width;
    GLint height;
    GLint depth;
    
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0,
                             GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0,
                             GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0,
                             GL_TEXTURE_DEPTH, &depth);

    if (width == 0 || height == 0 || depth == 0)
    {
      i /= 2;

      return i;
    }
  }

  return DEFAULT_MAX_TEXTURE;

}

int TextureBrick::_configure(
	const RegularGrid *rg, int precision, int nvars,
	GLenum &format, GLint &internalFormat, size_t &ncomp, GLenum &type, 
	bool &missing, bool &layered
	
) {
  missing = false;
  layered = false;

  if (! (precision == 8 || precision == 16)) return(-1);

  ncomp = nvars; 
  if (rg->HasMissingData()) {
    ncomp++;
    missing = true;
  }
  if (dynamic_cast<const LayeredGrid *>(rg)) {
    ncomp++;
    layered = true;
  }
  if (! (ncomp >= 1 && ncomp <= 4)) return(-1);

  if (precision == 8) {
    type = GL_UNSIGNED_BYTE;
  }
  else {
    type = GL_UNSIGNED_SHORT;
  }

  switch (ncomp) {
  case 1:
    format = GL_LUMINANCE;
    if (type == GL_UNSIGNED_BYTE) {
      internalFormat = GL_LUMINANCE8;
    }
    else {
      internalFormat = GL_LUMINANCE16;
    }
  break;
  case 2:
    format = GL_LUMINANCE_ALPHA;
    if (type == GL_UNSIGNED_BYTE) {
      internalFormat = GL_LUMINANCE8_ALPHA8;
    }
    else {
      internalFormat = GL_LUMINANCE16_ALPHA16;
    }
  break;
  case 3:
    format = GL_RGB;
    if (type == GL_UNSIGNED_BYTE) {
      internalFormat = GL_RGB8;
    }
    else {
      internalFormat = GL_RGB16;
    }
  break;
  case 4:
    format = GL_RGBA;
    if (type == GL_UNSIGNED_BYTE) {
      internalFormat = GL_RGBA8;
    }
    else {
      internalFormat = GL_RGBA16;
    }
  break;
  default:
    return(-1);
  }
  return(0);
}
