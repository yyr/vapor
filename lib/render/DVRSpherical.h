//--DVRSpherical.h ------------------------------------------------------------
//   
//                   Copyright (C)  2004
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
// Kenny Gruchalla
// National Center for Atmospheric Research
// PO 3000, Boulder, Colorado
//
//
// A 3d texture-based direct volume renderer for spherical grids.
//
// NOTE: This code is currently in a rough proof-of-concept state. It is not 
//       intended for any analytical use. There are known sampling issues with
//       the current approach, and generally much kludginess about.
//
//----------------------------------------------------------------------------


#ifndef DVRSpherical_h
#define DVRSpherical_h

#include "DVRShader.h"

#include "Vect3d.h"

#include <vector>
#include "assert.h"
namespace VAPoR {

  class BBox;
  class ShaderProgram;


class RENDER_API DVRSpherical : public DVRShader
{
 public:


  DVRSpherical(DataType_T type, int nthreads);
  virtual ~DVRSpherical();

  virtual int GraphicsInit();
  
  virtual int SetRegion(void *data, 
                        int nx, int ny, int nz, 
                        const int data_roi[6],
                        const float extents[6],
                        const int data_box[6],
                        int level,
                        size_t fullHeight) { assert(0); return 0; }

  virtual int SetRegionSpherical(void *data, 
                                 int nx, int ny, int nz, 
                                 const int data_roi[6],
                                 const float extents[6],
                                 const int data_box[6],
                                 int level,
                                 size_t fullHeight,
                                 const std::vector<long> &permutation,
                                 const std::vector<bool> &clipping);

  virtual int HasLighting() const { return false; };
  virtual int HasPreintegration() const { return false; };

  static bool supported();

 protected:

  virtual void calculateSampling();

  void permute(const vector<long>& permutation,
               float result[3], float x, float y, float z);

 protected:

  static char spherical_shader_default[];

  int   _nr;
  float _shellWidth;
  int   _level;
};

};


#endif 
