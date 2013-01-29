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

#include <vapor/RegularGrid.h>
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

    bool update(const RegularGrid *rg, int tex_size);

    const int* roi()       { return _roi; }
    const int* box()       { return _box; }
    const double* extents() { return _ext; }

  protected:

    int   _dim[3];
    int   _roi[6];
    int   _box[6];
    double _ext[6];
    int _tex_size;
  };

 public:


  DVRTexture3d(int precision, int nvars, int nthreads);
  virtual ~DVRTexture3d();

  virtual int SetRegion(const RegularGrid *rg, const float range[2],int num=1);

  virtual bool   GetRenderFast() const       { return _renderFast; }
  virtual void   SetRenderFast(bool fast)    { _renderFast = fast; }

  virtual void loadTexture(TextureBrick *brick) = 0;




  protected:

  //
  // Computes _slicePlaneNormal, _slicePoint, _samplingRate, _samples
  // and _delta
  //
  virtual void calculateSampling();

  virtual void drawViewAlignedSlices(const TextureBrick *brick,
                                     const Matrix3d &modelview,
                                     const Matrix3d &modelviewInverse);

  virtual void renderBrick(const TextureBrick *brick,
                                     const Matrix3d &modelview,
                                     const Matrix3d &modelviewInverse);

  void renderBricks();

  int  intersect(const Vect3d &sp, const Vect3d &spn, 
                 const BBox &volumeBox,  Vect3d verts[6],
                 const BBox &textureBox, Vect3d tverts[6],
                 const BBox &rotatedBox, Vect3d rverts[6]);
  
  void findVertexOrder(const Vect3d verts[6], int order[6], int degree);

  void buildBricks(const RegularGrid *rg, const float range[2], int num);
  void sortBricks(const Matrix3d &modelview);

  virtual void SetMaxTexture(int texsize);


protected:

  // data dimensions
  int    _nx;
  int    _ny;
  int    _nz;

  void   *_data;

  // brick dimensions

  bool   _renderFast;

  float  _delta;	// sample distance along Z in world coordinates
  float  _samples;
  float  _samplingRate;
  int    _minimumSamples;
  int    _minimumSamplesFast;
  int    _maxTexture;
  int    _maxBrickDim;
  Vect3d _slicePlaneNormal;
  Vect3d _slicePoint;
 
  // Texture bricks
  vector<TextureBrick*> _bricks;

  // Voxel extents
  Point3d _dmin;
  Point3d _dmax;

  // Data extents
  Point3d _vmin;
  Point3d _vmax;

  // Texture extents
  Point3d _tmin;
  Point3d _tmax;

  // Region state
  RegionState _lastRegion;

  // Texture internal precision
  int _precision;
  
  // num vars stored in a texture
  int _nvars;

  void SetMinimumSamples(int normal, int fast) {
	_minimumSamples = normal;
	_minimumSamplesFast = fast;
  };


};

};


#endif 
