//--DVRTexture3d.h -----------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// Abstract volume rendering engine based on 3d-textured view aligned slices. 
// Provides proxy geometry, color lookup should be provided by derived 
// classes. 
//
//----------------------------------------------------------------------------

#ifndef DVRTexture3d_h
#define DVRTexture3d_h

#include "DVRBase.h"
#include "Vect3d.h"
#include "Matrix3d.h"

#include <vector>
#include <map>

namespace VAPoR {

  class BBox;
  class TextureBrick;

class DVRTexture3d : public DVRBase 
{
  class CacheKey;

 public:


  DVRTexture3d(DataType_T type, int nthreads);
  virtual ~DVRTexture3d();

  virtual int SetRegion(void *data, 
                        int nx, int ny, int nz, 
                        const int data_roi[6],
                        const float extents[6],
                        const int data_box[6],
                        int level);

  virtual void loadTexture(TextureBrick *brick) = 0;

protected:

  void drawViewAlignedSlices(const TextureBrick *brick,
                             const Matrix3d &modelview,
                             const Matrix3d &modelviewInverse);
  void renderBricks();

  int  intersect(const Vect3d &sp, const Vect3d &spn, 
                 const BBox &volumeBox,  Vect3d verts[6],
                 const BBox &textureBox, Vect3d tverts[6],
                 const BBox &rotatedBox, Vect3d rverts[6]);
  
  void findVertexOrder(const Vect3d verts[6], int order[6], int degree);

  void buildBricks(int level, const int bbox[6], const int data_roi[6],
                   int nx, int ny, int nz);
  void sortBricks(const Matrix3d &modelview);

  TextureBrick* fetch(void *data, int nx, int ny, int nz);

  static long maxTextureSize(GLenum format);

protected:

  int    _nx;
  int    _ny;
  int    _nz;
  void   *_data;

  float  _delta; 
  long   _maxTexture;

  // Texture bricks
  vector<TextureBrick*> _bricks;

  // Data extents
  Point3d _vmin;
  Point3d _vmax;

  // Texture extents
  Point3d _tmin;
  Point3d _tmax;

 private:

  // Texture cache
  map<CacheKey, TextureBrick*> _textureCache;
  int _maxCacheSize;

  // map of maximum texture sizes
  static std::map<GLenum, int> _textureSizes;

  //--------------------------------
  // class DVRTexture3d::CacheKey
  //--------------------------------
  class CacheKey
  {
   public:
 
    CacheKey(void *data, int nx, int ny, int nz);
    virtual ~CacheKey();

    CacheKey(const CacheKey &key);
    CacheKey operator=(const CacheKey &key);

    // Equivalence
    friend int operator==(const DVRTexture3d::CacheKey &k1, 
                          const DVRTexture3d::CacheKey &k2)
    {
      return (k1._data == k2._data && 
              k1._nx == k2._nx &&
              k1._ny == k2._ny &&
              k1._nz == k2._nz);
    }

    friend int operator!=(const DVRTexture3d::CacheKey &k1, 
                          const DVRTexture3d::CacheKey &k2)
    {
      return !(operator==(k1, k2));
    }

    friend int operator<(const DVRTexture3d::CacheKey &k1, 
                         const DVRTexture3d::CacheKey &k2)
    {
      return k1._data < k2._data;
    }

  private:

    void *_data;
    int   _nx;
    int   _ny;
    int   _nz;
  };
};

};


#endif 
