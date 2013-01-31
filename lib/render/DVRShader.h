//--DVRShader.h -----------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// Volume rendering engine based on 3d-textured view aligned slices that
// uses OpenGL Shading Language. 
//
//----------------------------------------------------------------------------

#ifndef DVRShader_h
#define DVRShader_h

#include "DVRTexture3d.h"

#include "Vect3d.h"
#include "ShaderMgr.h"
#include <vector>
#include <map>
#include <QString>
namespace VAPoR {

  class BBox;
  class ShaderProgram;


class RENDER_API DVRShader : public DVRTexture3d
{

 public:


  DVRShader(int precision, int nvars, ShaderMgr *shadermgr, int nthreads);
  virtual ~DVRShader();

  virtual int GraphicsInit();
  
  virtual int SetRegion(const RegularGrid *rg, const float range[2], int num=0);

  virtual void loadTexture(TextureBrick *brick);

  virtual int Render();

  virtual int HasPreintegration() const { return true; };
  virtual int HasLighting() const { return true; };

  virtual void SetCLUT(const float ctab[256][4]);
  virtual void SetOLUT(const float ftab[256][4], const int numRefinenements);

  virtual void SetView(const float *pos, const float *dir);

  virtual void SetPreintegrationOnOff(int );
  virtual void SetPreIntegrationTable(const float tab[256][4], const int nR);

  virtual void SetLightingOnOff(int on);
  virtual void SetLightingCoeff(float kd, float ka, float ks, float expS);
  virtual void SetLightingLocation(const float *pos);

  static bool supported();

  virtual void calculateSampling();

protected:

  string instanceName(string base) const;

  int initTextures();
  virtual void initShaderVariables();

  virtual std::string getCurrentEffect();


  virtual void drawViewAlignedSlices(const TextureBrick *brick,
                                     const Matrix3d &modelview,
                                     const Matrix3d &modelviewInverse);

  virtual void renderBrick(const TextureBrick *brick,
                                     const Matrix3d &modelview,
                                     const Matrix3d &modelviewInverse);

  ShaderMgr *_shadermgr;

  bool            _lighting;

  void loadCoordMap(
	const TextureBrick *brick, size_t i0, size_t i1, size_t j0, size_t j1,
	size_t k0, size_t k1
  );

  bool _stretched;
  float _kd;
  float _ka;
  float _ks;
  float _expS;
  float _pos[3];
  int _midx;
  int _zidx;

private: 

  float          *_colormap;
  float          *_coordmap;
  ShaderProgram  *_shader;

  // voxel coordinates for stretched grids
  //
  vector <double> _xcoords, _ycoords, _zcoords;

  bool            _preintegration;

  GLuint _cmapid[4];

  float _vdir[4];
  float _vpos[4];

  void _loadCoordMap(
    const vector <double> ucoords, int c0, int c1,
    float *coordmap, size_t coordmapsz, int stride, int offset
  );


};

};


#endif 
