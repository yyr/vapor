//--DVRTexture3d.h -----------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// Volume rendering engine based on 3d-textured view aligned slices. 
//
//----------------------------------------------------------------------------

#ifndef DVRTexture3d_h
#define DVRTexture3d_h

#include "DVRBase.h"

#include "Vect3d.h"

#include <vector>

namespace VAPoR {

  class BBox;
  class ShaderProgram;


class DVRTexture3d : public DVRBase 
{
 public:


  DVRTexture3d(int *argc, char **argv, DataType_T type, int nthreads);
  virtual ~DVRTexture3d();

  virtual int GraphicsInit();
  
  virtual int SetRegion(void *data, 
                        int nx, int ny, int nz, 
                        const int data_roi[6],
                        const float extents[6]);

  virtual int Render(const float matrix[16]);

  virtual int HasType(DataType_T type);

  virtual void SetCLUT(const float ctab[256][4]);
  virtual void SetOLUT(const float ftab[256][4], const int numRefinenements);

  virtual void SetLightingOnOff(int on);
  virtual void SetLightingCoeff(float kd, float ka, float ks, float expS);

protected:

  void initTextures();

  void drawViewAlignedSlices();

  int  intersect(const Vect3d &sp, const Vect3d &spn, 
                 const BBox &volumeBox,  Vect3d verts[6],
                 const BBox &textureBox, Vect3d tverts[6],
                 const BBox &rotatedBox, Vect3d rverts[6]);
  
  void findVertexOrder(const Vect3d verts[6], int order[6], int degree);
 
private:

  void             *_data;
  ShaderProgram    *_shader;
  float            *_colormap;
  float             _delta; 

  GLuint _texid;
  GLuint _cmapid;

  int    _nx;
  int    _ny;
  int    _nz;

  // Data extents
  Point3d _vmin;
  Point3d _vmax;

  // Texture extents
  Point3d _tmin;
  Point3d _tmax;
};

};


#endif 
