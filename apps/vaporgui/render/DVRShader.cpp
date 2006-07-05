//--DVRShader.cpp -----------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// A derived DVRTexture3d providing a OpenGL Shading Language shader for
// the color mapping. 
//
//----------------------------------------------------------------------------

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <qgl.h>

#include "DVRShader.h"
#include "TextureBrick.h"
#include "messagereporter.h"

#include "ShaderProgram.h"
#include "BBox.h"
#include "glutil.h"

#include "Matrix3d.h"
#include "Point3d.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRShader::DVRShader(DataType_T type, int nthreads) :
  DVRTexture3d(type, nthreads),
  _data(NULL),
  _colormap(NULL),
  _shader(NULL),
  _texid(0),
  _cmapid(0)
{
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRShader::~DVRShader() 
{
  delete _shader;
  _shader = NULL;

  delete [] _colormap;
  _colormap = NULL;

  glDeleteTextures(1, &_texid);
  glDeleteTextures(1, &_cmapid);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRShader::GraphicsInit() 
{
  glewInit();

  initTextures();

  //
  // Create, Load & Compile the shader program
  //
  static char fragment_shader[] =
    "uniform sampler1D colormap;\n"
    "uniform sampler3D volumeTexture;\n"
    "//--------------------------------------------------------------------\n"
    "// Fragment shader main\n"
    "//--------------------------------------------------------------------\n"
    "void main(void)\n"
    "{\n"
    "vec4 intensity = vec4 (texture3D(volumeTexture, gl_TexCoord[0].xyz));\n"
    "vec4 color     = vec4 (texture1D(colormap, intensity.x));\n"
    "gl_FragColor = color;\n"
    "}\n";

  _shader = new ShaderProgram();
  _shader->create();
  _shader->loadFragmentSource(fragment_shader);

  if (!_shader->compile())
  {
    return -1;
  }

  //
  // Set up initial uniform values
  //
  _shader->enable();

  if (GLEW_VERSION_2_0)
  {
    glUniform1i(_shader->uniformLocation("colormap"), 1);
    glUniform1i(_shader->uniformLocation("volumeTexture"), 0);
  }
  else
  {
    glUniform1iARB(_shader->uniformLocation("colormap"), 1);
    glUniform1iARB(_shader->uniformLocation("volumeTexture"), 0);
  }
  _shader->disable();

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRShader::SetRegion(void *data,
                         int nx, int ny, int nz,
                         const int roi[6],
                         const float extents[6],
                         const int box[6],
                         int level) 
{ 
  //
  // Set the texture data
  //
  if (_nx != nx || _ny != ny || _nz != nz)
  {
    _nx = nx; _ny = ny; _nz = nz;
    _data = data;
    //
    // TBD -- debugging shader conditionals. 
    //
    //if (_shader)
    //{
    //  _shader->enable();
    //  glUniform3f(_shader->uniformLocation("dimensions"), nx, ny, nz);
    //  _shader->disable();
    //}
  }

  //
  // Set the geometry extents
  //
  return DVRTexture3d::SetRegion(data, nx, ny, nz, roi, extents, box, level);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::loadTexture(TextureBrick *brick)
{
  printOpenGLError();

  glActiveTexture(GL_TEXTURE0);

  brick->load(GL_LUMINANCE8, GL_LUMINANCE);

  printOpenGLError();
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRShader::Render(const float matrix[16])
{
  if (_shader) _shader->enable();

  glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK, GL_LINE);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, _texid);

  glEnable(GL_TEXTURE_1D);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  glDisable(GL_DITHER);

  renderBricks();

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_3D);
  glDisable(GL_TEXTURE_1D);

  if (_shader) _shader->disable();

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRShader::HasType(DataType_T type) 
{
  if (type == UINT8) 
    return(1);
  else 
    return(0);
}

//----------------------------------------------------------------------------
// Set the color table used to render the volume without any opacity 
// correction.
//----------------------------------------------------------------------------
void DVRShader::SetCLUT(const float ctab[256][4]) 
{
  for (int i=0; i<256; i++)
  {
    _colormap[i*4+0] = ctab[i][0];
    _colormap[i*4+1] = ctab[i][1];
    _colormap[i*4+2] = ctab[i][2];
    _colormap[i*4+3] = ctab[i][3];
  }

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _cmapid);
  
  glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA,
                  GL_FLOAT, _colormap);

  glFlush();
}

//----------------------------------------------------------------------------
// Set the color table used to render the volume applying an opacity 
// correction.
//----------------------------------------------------------------------------
void DVRShader::SetOLUT(const float atab[256][4], const int numRefinements)
{
  //
  // Calculate opacity correction. Delta is 1 for the ffineest refinement, 
  // multiplied by 2^n (the sampling distance)
  //
  double delta = (double)(1<<numRefinements);

  for(int i=0; i<256; i++) 
  {
    double opac = atab[i][3];

    opac = 1.0 - pow((1.0 - opac), delta);
  
    if (opac > 1.0)
    {
      opac = 1.0;
    }

    _colormap[i*4+0] = atab[i][0];
    _colormap[i*4+1] = atab[i][1];
    _colormap[i*4+2] = atab[i][2];
    _colormap[i*4+3] = opac;
  }

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _cmapid);
  
  glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA,
                  GL_FLOAT, _colormap);

  glFlush();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetLightingOnOff(int on) 
{
  //if (_shader)
  //{
  //  _shader->enable();
  //  glUniform1b(_shader->uniformLocation("shading"), on);
  //  _shader->enable();
  //}
    
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetLightingCoeff(float kd, float ka, float ks, float expS)
{
  //if (_shader)
  //{
  //  _shader->enable();
  //  glUniform1f(_shader->uniformLocation("kd"), kd);
  //  glUniform1f(_shader->uniformLocation("ka"), ka);
  //  glUniform1f(_shader->uniformLocation("ks"), ks);
  //  glUniform1f(_shader->uniformLocation("expS"), expS);
  //  _shader->enable();
  //}
}


//----------------------------------------------------------------------------
// Initalize the textures (i.e., 3d volume texture and the 1D colormap texture)
//----------------------------------------------------------------------------
void DVRShader::initTextures()
{
  //
  // Setup the colormap texture
  //
  glGenTextures(1, &_cmapid);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _cmapid);

  _colormap = new float[256*4];
  
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA,
               GL_FLOAT, _colormap);

  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

  glFlush();
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool DVRShader::supported()
{
  return (ShaderProgram::supported() && GLEW_ARB_multitexture);
}
