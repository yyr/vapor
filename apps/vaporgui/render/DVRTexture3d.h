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

#include <vector>

namespace VAPoR {

  class BBox;

class DVRTexture3d : public DVRBase 
{
 public:


  DVRTexture3d(DataType_T type, int nthreads);
  virtual ~DVRTexture3d();

  virtual int SetRegion(void *data, 
                        int nx, int ny, int nz, 
                        const int data_roi[6],
                        const float extents[6]);

protected:

  void drawViewAlignedSlices();

  int  intersect(const Vect3d &sp, const Vect3d &spn, 
                 const BBox &volumeBox,  Vect3d verts[6],
                 const BBox &textureBox, Vect3d tverts[6],
                 const BBox &rotatedBox, Vect3d rverts[6]);
  
  void findVertexOrder(const Vect3d verts[6], int order[6], int degree);
 
protected:

  float  _delta; 

  // Data extents
  Point3d _vmin;
  Point3d _vmax;

  // Texture extents
  Point3d _tmin;
  Point3d _tmax;
};

};


#endif 
