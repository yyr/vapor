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

  ShaderProgram* shader();
  virtual std::string getCurrentEffect();


  virtual void drawViewAlignedSlices(const TextureBrick *brick,
                                     const Matrix3d &modelview,
                                     const Matrix3d &modelviewInverse);

  float          *_colormap;
  ShaderProgram  *_shader;
  map <int, ShaderProgram *> _shaders;

  bool            _lighting;
  bool            _preintegration;

  GLuint _cmapid[2];

  float _kd;
  float _ka;
  float _ks;
  float _expS;
  float _pos[3];
  float _vdir[4];
  float _vpos[4];
  int _midx;
  int _zidx;
  ShaderMgr *_shadermgr;

};

};


#endif 
