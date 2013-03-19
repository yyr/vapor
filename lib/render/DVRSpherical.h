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

#include "glutil.h"	// Must be included first!!!
#include "DVRShader.h"
#include "vapor/SphericalGrid.h"

#include "Vect3d.h"

#include "ShaderMgr.h"
#include <vector>
#include "assert.h"
namespace VAPoR {

  class BBox;
  class ShaderProgram;

class RENDER_API DVRSpherical : public DVRShader
{
 public:


  DVRSpherical(int precision, int nvars, ShaderMgr *shadermgr, int nthreads);
  virtual ~DVRSpherical();

  virtual int GraphicsInit();
  
  virtual int SetRegion(
	const RegularGrid *rg, const float range[2], int num
  ) { assert(0); return 0; }

  virtual int SetRegionSpherical(const SphericalGrid *sg,
								const float range[2], int num,
                                 const std::vector<long> &permutation,
                                 const std::vector<bool> &clipping);

  virtual int HasLighting() const { return true; };
  virtual int HasPreintegration() const { return false; };

  virtual int Render();

  static bool supported();

 protected:

  virtual void drawViewAlignedSlices(const TextureBrick *brick,
                                     const Matrix3d &modelview,
                                     const Matrix3d &modelviewInverse);

  virtual void renderBrick(
	const TextureBrick *brick, 
	const Matrix3d &modelview,
	const Matrix3d &modelviewInverse
  );
 

  virtual void initShaderVariables();

  virtual void calculateSampling();

  void permute(const vector<long>& permutation,
               float result[3], float x, float y, float z);

 private:

  static char spherical_shader_default[];
  static char spherical_shader_lighting[];

  int   _nr;
  float _shellWidth;
  int   _level;
  float _r0, _r1; // inner and outer shell radius
  float _extentsSP[6]; // spherical extents, permuted to long, lat, radius order

  std::vector<long> _permutation;
  std::vector<bool> _clip;
  bool _initialized;
};

};


#endif 
