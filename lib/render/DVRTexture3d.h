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

class RENDER_API DVRTexture3d : public DVRBase 
{

  //----------------------------------
  // sub class DVRTexture3d::RegionState
  //----------------------------------
  class RegionState
  {

  public:

    RegionState();
    virtual ~RegionState() {};

    bool update(int nx, int ny, int nz, 
                const int roi[6], const int box[6], const float extents[6]);
  protected:

    int   _dim[3];
    int   _roi[6];
    int   _box[6];
    float _ext[6];
  };

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

  static int maxTextureSize(GLenum format);

protected:

  // data dimensions
  int    _nx;
  int    _ny;
  int    _nz;

  void   *_data;

  // brick dimensions
  int    _bx;  
  int    _by;
  int    _bz;

  float  _delta; 
  int    _maxTexture;
  int    _maxBrickDim;
 
  // Texture bricks
  vector<TextureBrick*> _bricks;

  // Data extents
  Point3d _vmin;
  Point3d _vmax;

  // Texture extents
  Point3d _tmin;
  Point3d _tmax;

  // Region state
  RegionState _lastRegion;

};

};


#endif 
