//--DVRLookup.h -----------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// A derived DVRTexture3d providing a pixel map for the color lookup.
//
//----------------------------------------------------------------------------

#ifndef DVRLookup_h
#define DVRLookup_h

#include "DVRTexture3d.h"

#include "Vect3d.h"

#include <vector>

namespace VAPoR {

  class BBox;

class DVRLookup : public DVRTexture3d
{
 public:


  DVRLookup(DataType_T type, int nthreads);
  virtual ~DVRLookup();

  virtual int GraphicsInit();
  
  virtual int SetRegion(void *data, 
                        int nx, int ny, int nz, 
                        const int data_roi[6],
                        const float extents[6]);

  virtual int Render(const float matrix[16]);

  virtual int HasType(DataType_T type);

  virtual void SetCLUT(const float ctab[256][4]);
  virtual void SetOLUT(const float ftab[256][4], const int numRefinenements);

  static bool supported() { return true; }

protected:

  virtual void initTextures();

 
protected:

  void             *_data;
  float            *_colormap;

  GLuint _texid;

  int    _nx;
  int    _ny;
  int    _nz;
};

};


#endif 
