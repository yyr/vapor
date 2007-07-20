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

#include <vector>

namespace VAPoR {

  class BBox;
  class ShaderProgram;


class RENDER_API DVRShader : public DVRTexture3d
{

 public:


  DVRShader(DataType_T type, int nthreads);
  virtual ~DVRShader();

  virtual int GraphicsInit();
  
  virtual int SetRegion(void *data, 
                        int nx, int ny, int nz, 
                        const int data_roi[6],
                        const float extents[6],
                        const int data_box[6],
                        int level,
						size_t fullHeight);

  virtual void loadTexture(TextureBrick *brick);

  virtual int Render(const float matrix[16]);

  virtual int HasType(DataType_T type);
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

protected:

  enum ShaderType
  {
    DEFAULT = 0,
    LIGHT,
    PRE_INTEGRATED,
    PRE_INTEGRATED_LIGHT
  };

  void initTextures();
  void initShaderVariables();

  bool createShader(ShaderType,
                    const char *vertexCommandLine,
                    const char *vertexSource,
                    const char *fragCommandLine,
                    const char *fragmentSource);

  ShaderProgram* shader();

  virtual void calculateSampling();

  virtual void drawViewAlignedSlices(const TextureBrick *brick,
                                     const Matrix3d &modelview,
                                     const Matrix3d &modelviewInverse);

protected:

  float          *_colormap;
  ShaderProgram  *_shader;
  ShaderProgram  *_shaders[4];

  bool            _lighting;
  bool            _preintegration;

  GLuint _texid;
  GLuint _cmapid[2];

  int    _nx;
  int    _ny;
  int    _nz;
};

};


#endif 
