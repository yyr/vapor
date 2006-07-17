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
  enum // Shaders
  {
    DEFAULT = 0,
    LIGHT
  };

 public:


  DVRShader(DataType_T type, int nthreads);
  virtual ~DVRShader();

  virtual int GraphicsInit();
  
  virtual int SetRegion(void *data, 
                        int nx, int ny, int nz, 
                        const int data_roi[6],
                        const float extents[6],
                        const int data_box[6],
                        int level);

  virtual void loadTexture(TextureBrick *brick);

  virtual int Render(const float matrix[16]);

  virtual int HasType(DataType_T type);

  virtual void SetCLUT(const float ctab[256][4]);
  virtual void SetOLUT(const float ftab[256][4], const int numRefinenements);
  virtual void SetLightingOnOff(int on);
  virtual void SetLightingCoeff(float kd, float ka, float ks, float expS);
  virtual void SetLightingLocation(const float *pos);

  static bool supported();

protected:

  void initTextures();

private:

  void           *_data;
  float          *_colormap;
  ShaderProgram  *_shader;
  ShaderProgram  *_shaders[2];

  GLuint _texid;
  GLuint _cmapid;
  
  int    _nx;
  int    _ny;
  int    _nz;
};

};


#endif 
