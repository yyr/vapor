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
#include "ShaderProgram.h"
#include "BBox.h"
#include "glutil.h"
#include "params.h"

#include "Matrix3d.h"
#include "Point3d.h"
#include "shaders.h"

using namespace VAPoR;

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRShader::DVRShader(DataType_T type, int nthreads) :
  DVRTexture3d(type, nthreads),
 _colormap(NULL),
  _shader(NULL),
  _lighting(false),
  _preintegration(false),
  _kd(0.0),
  _ka(0.0),
  _ks(0.0),
  _expS(0.0)
{
  _shaders[DEFAULT]              = NULL;
  _shaders[LIGHT]                = NULL;
  _shaders[PRE_INTEGRATED]       = NULL;
  _shaders[PRE_INTEGRATED_LIGHT] = NULL;

  for (int i=0; i<3; i++) _pos[i] = 0.0;

  for (int i=0; i<4; i++) {
    _vdir[i] = 0.0;
    _vpos[i] = 0.0;
  }
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRShader::~DVRShader() 
{
  map <int, ShaderProgram *>::iterator pos;

  for(pos=_shaders.begin(); pos != _shaders.end(); ++pos) {
    if (pos->second) delete pos->second;
    pos->second = NULL;
  }

  if (_colormap) delete [] _colormap;
  _colormap = NULL;

  glDeleteTextures(2, _cmapid);

}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool DVRShader::createShader(ShaderType type, 
                             const char *vertexCommandLine,
                             const char *vertexSource,
                             const char *fragCommandLine,
                             const char *fragmentSource)
{
  _shaders[type] = new ShaderProgram();
  _shaders[type]->create();

  //
  // Vertex shader
  //
  if (Params::searchCmdLine(vertexCommandLine))
  {
    _shaders[type]->loadVertexShader(Params::parseCmdLine(vertexCommandLine));
  }
  else if (vertexSource)
  {
    _shaders[type]->loadVertexSource(vertexSource);
  }

  //
  // Fragment shader
  //
  if (Params::searchCmdLine(fragCommandLine))
  {
    _shaders[type]->loadFragmentShader(Params::parseCmdLine(fragCommandLine));
  }
  else if (fragmentSource)
  {
    _shaders[type]->loadFragmentSource(fragmentSource);
  }

  if (!_shaders[type]->compile())
  {
    return false;
  }

  //
  // Set up initial uniform values
  //
  _shaders[type]->enable();

  if (GLEW_VERSION_2_0)
  {
    glUniform1i(_shaders[type]->uniformLocation("colormap"), 1);
    glUniform1i(_shaders[type]->uniformLocation("volumeTexture"), 0);
  }
  else
  {
    glUniform1iARB(_shaders[type]->uniformLocation("colormap"), 1);
    glUniform1iARB(_shaders[type]->uniformLocation("volumeTexture"), 0);
  }

  _shaders[type]->disable();


  return true;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRShader::GraphicsInit() 
{
  glewInit();

  initTextures();

  //
  // Create, Load & Compile the default shader program
  //
  if (!createShader(DEFAULT, 
                    NULL, 
                    NULL, 
                    "--default-fragement-shader", 
                    fragment_shader_default))
  {
    return -1;
  }

  //
  // Create, Load & Compile the lighting shader program
  //
  if (!createShader(LIGHT,
                    "--lighting-vertex-shader",
                    vertex_shader_lighting,
                    "--lighting-fragment-shader",
                    fragment_shader_lighting))
  {
    return -1;
  }

  //
  // Create, Load & Compile the pre-integrated shader program
  //
  if (!createShader(PRE_INTEGRATED,
                    "--preintegrated-vertex-shader",
                    vertex_shader_preintegrated,
                    "--preintegrated-fragment-shader",
                    fragment_shader_preintegrated))
  {
    return -1;
  }

  //
  // Create, Load & Compile the lighted pre-integrated shader program
  //
  if (!createShader(PRE_INTEGRATED_LIGHT,
                    "--preintegrated-lighting-vertex-shader",
                    vertex_shader_preintegrated_lighting,
                    "--preintegrated-lighting-fragment-shader",
                    fragment_shader_preintegrated_lighting))
  {
    return -1;
  }
                    
  //
  // Set the current shader
  //
  _shader = _shaders[DEFAULT];

  initShaderVariables();

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
                         int level,
						 size_t fullHeight) 
{ 

  _nx = nx; _ny = ny; _nz = nz;
  _data = data;


  //
  // Set the geometry extents
  //
  return DVRTexture3d::SetRegion(data, nx, ny, nz, roi, extents, box, level, fullHeight);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::loadTexture(TextureBrick *brick)
{
  printOpenGLError();

  if (_type == GL_UNSIGNED_BYTE) {
    brick->load(GL_LUMINANCE8, GL_LUMINANCE);
  } else {
    brick->load(GL_LUMINANCE16, GL_LUMINANCE);
  }
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

  if (_preintegration) {
    glEnable(GL_TEXTURE_2D);  
    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE1);
    }
    else {
      glActiveTextureARB(GL_TEXTURE1_ARB);
    }
    glBindTexture(GL_TEXTURE_2D, _cmapid[1]);
  }
  else {
    glEnable(GL_TEXTURE_1D);
    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE1);
    }
    else {
      glActiveTextureARB(GL_TEXTURE1_ARB);
    }
    glBindTexture(GL_TEXTURE_1D, _cmapid[0]);
  }

  if (GLEW_VERSION_2_0) {
    glActiveTexture(GL_TEXTURE0);
  }
  else {
    glActiveTextureARB(GL_TEXTURE0_ARB);
  }

  glEnable(GL_TEXTURE_3D);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  glDisable(GL_DITHER);

  renderBricks();

  if (GLEW_VERSION_2_0) {

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_1D, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, 0);
  } else {

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_1D, 0);

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_3D, 0);
  }

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_3D);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_1D);

  if (_shader) _shader->disable();

  glFlush();
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

  glBindTexture(GL_TEXTURE_1D, _cmapid[0]);
  
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
  // Compute the sampling distance and rate
  //
  calculateSampling();

  if (_preintegration)
  {
    SetPreIntegrationTable(atab, numRefinements);
    return;
  }

  //
  // Calculate opacity correction. Delta is 1 for the ffineest refinement, 
  // multiplied by 2^n (the sampling distance)
  //
  double delta = 2.0/_samplingRate * (double)(1<<numRefinements);

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

  glBindTexture(GL_TEXTURE_1D, _cmapid[0]);
  
  glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA,
                  GL_FLOAT, _colormap);

  glFlush();
}

double clamp(double v, double min, double max)
{
  return MAX(MIN(v, max), min);
}

//----------------------------------------------------------------------------
// Set the pre-integrated color table used to render the volume applying an 
// opacity correction.
//----------------------------------------------------------------------------
void DVRShader::SetPreIntegrationTable(const float atab[256][4], 
                                       const int numRefinements)
{
  double factor;
  double ifunc[256][4];
  double r = 0;
  double g = 0;
  double b = 0;
  double a = 0;


  ifunc[0][0] = 0.0;
  ifunc[0][1] = 0.0;
  ifunc[0][2] = 0.0;
  ifunc[0][3] = 0.0;

  float opac[256];

  double delta = 2.0/_samplingRate * (double)(1<<numRefinements);

  opac[0] = MIN(1.0, 1.0 - pow((1.0 - atab[0][3]), delta));

  // 
  // Compute integral functions & opacity correction
  //

  for (int i=1; i<256; i++)
  {

    opac[i] = MIN(1.0, 1.0 - pow((1.0 - atab[i][3]), delta));

    a = 255.0*(opac[i-1]+opac[i])/2.0;

    // Opacity-weighted
    //ifunc[i][0] = ifunc[i-1][0] + (atab[i-1][0]+atab[i][0])/2.0*a;
    //ifunc[i][1] = ifunc[i-1][1] + (atab[i-1][1]+atab[i][1])/2.0*a;
    //ifunc[i][2] = ifunc[i-1][2] + (atab[i-1][2]+atab[i][2])/2.0*a;

    // No weighting
    ifunc[i][0] = ifunc[i-1][0] + (atab[i-1][0]+atab[i][0])/2.0*255;
    ifunc[i][1] = ifunc[i-1][1] + (atab[i-1][1]+atab[i][1])/2.0*255;
    ifunc[i][2] = ifunc[i-1][2] + (atab[i-1][2]+atab[i][2])/2.0*255;

    ifunc[i][3] = ifunc[i-1][3] + a;
  }    

  //
  // Compute lookup table
  //
  for (int sb=0; sb<256; sb++)
  {
    for (int sf=0; sf<256; sf++)
    {
      int smin = MIN(sb, sf);
      int smax = MAX(sb, sf);

      if (smin != smax)
      {
        factor = 1.0 / (double)(smax - smin);
        
        r = factor * (ifunc[smax][0] - ifunc[smin][0]);
        g = factor * (ifunc[smax][1] - ifunc[smin][1]);
        b = factor * (ifunc[smax][2] - ifunc[smin][2]);
        a = 256.0 * 
          (1.0 - expf(-(ifunc[smax][3] - ifunc[smin][3]) * factor / 255.0));
      }
      else
      {
        // Opacity-weighted
        //r = 255.0 * atab[smin][0] * atab[smin][3];
        //g = 255.0 * atab[smin][1] * atab[smin][3];
        //b = 255.0 * atab[smin][2] * atab[smin][3];

        // No weighting
        r = 255.0 * atab[smin][0];
        g = 255.0 * atab[smin][1];
        b = 255.0 * atab[smin][2];
        a = 256 * (1.0 - expf(-opac[smin]));

      }

      int index = (sf + sb*256) * 4;

      _colormap[index + 0] = clamp(r/255.0, 0.0, 1.0);
      _colormap[index + 1] = clamp(g/255.0, 0.0, 1.0);
      _colormap[index + 2] = clamp(b/255.0, 0.0, 1.0);
      _colormap[index + 3] = clamp(a/255.0, 0.0, 1.0);
    }
  }

  glBindTexture(GL_TEXTURE_2D, _cmapid[1]);
  
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA,
                  GL_FLOAT, _colormap);

  glFlush();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetView(const float *pos, const float *dir) 
{
  assert(dir);
  assert(pos);

  for (int i=0; i<4; i++) {
    _vdir[i] = dir[i];
    _vpos[i] = pos[i];
  }
  initShaderVariables();

}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetPreintegrationOnOff(int on) 
{
  _preintegration = on;

  _shader = shader(); assert(_shader);

  initShaderVariables();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetLightingOnOff(int on) 
{
  _lighting = on;

  _shader = shader(); assert(_shader);

  initShaderVariables();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetLightingCoeff(float kd, float ka, float ks, float expS)
{
	_kd = kd;
	_ka = ka;
	_ks = ks;
	_expS = expS;

	initShaderVariables();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::SetLightingLocation(const float *pos)
{
	_pos[0] = pos[0];
	_pos[1] = pos[1];
	_pos[2] = pos[2];

	initShaderVariables();
}


//----------------------------------------------------------------------------
// Initalize the textures (i.e., 3d volume texture and the 1D colormap texture)
//----------------------------------------------------------------------------
void DVRShader::initTextures()
{

  //
  // Setup the colormap texture
  //
  glGenTextures(2, _cmapid);

  _colormap = new float[256*256*4];
  
  //
  // Standard colormap
  //
  glBindTexture(GL_TEXTURE_1D, _cmapid[0]);

  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA,
               GL_FLOAT, _colormap);

  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

  //
  // Pre-Integrated colormap
  //
  glBindTexture(GL_TEXTURE_2D, _cmapid[1]);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA,
               GL_FLOAT, _colormap);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_1D, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glFlush();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::initShaderVariables()
{
  assert(_shader);

  _shader->enable();
  
  if (_preintegration)
  {
    if (GLEW_VERSION_2_0)
    {
      glUniform1f(_shader->uniformLocation("delta"), _delta);
      glUniform4f(
        _shader->uniformLocation("vdir"), 
        _vdir[0], _vdir[1], _vdir[2], _vdir[3]
      );
      glUniform4f(
        _shader->uniformLocation("vpos"), 
        _vpos[0], _vpos[1], _vpos[2], _vpos[3]
      );

    }
    else
    {
      glUniform1fARB(_shader->uniformLocation("delta"), _delta);
      glUniform4fARB(
        _shader->uniformLocation("vdir"), 
        _vdir[0], _vdir[1], _vdir[2], _vdir[3]
      );
      glUniform4fARB(
        _shader->uniformLocation("vpos"), 
        _vpos[0], _vpos[1], _vpos[2], _vpos[3]
      );
    }
  }

  if (_lighting)
  {
    if (GLEW_VERSION_2_0)
    {
      glUniform3f(_shader->uniformLocation("dimensions"), _nx, _ny, _nz);
      glUniform1f(_shader->uniformLocation("kd"), _kd);
      glUniform1f(_shader->uniformLocation("ka"), _ka);
      glUniform1f(_shader->uniformLocation("ks"), _ks);
      glUniform1f(_shader->uniformLocation("expS"), _expS);
      glUniform3f(
        _shader->uniformLocation("lightDirection"), _pos[0], _pos[1], _pos[2]
      );
    }
    else
    {
      glUniform3fARB(_shader->uniformLocation("dimensions"), _nx, _ny, _nz);
      glUniform1fARB(_shader->uniformLocation("kd"), _kd);
      glUniform1fARB(_shader->uniformLocation("ka"), _ka);
      glUniform1fARB(_shader->uniformLocation("ks"), _ks);
      glUniform1fARB(_shader->uniformLocation("expS"), _expS);
      glUniform3fARB(
        _shader->uniformLocation("lightDirection"), _pos[0], _pos[1], _pos[2]
      );
    }
  } 

  _shader->disable();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool DVRShader::supported()
{
  return (ShaderProgram::supported() && GLEW_ARB_multitexture);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ShaderProgram* DVRShader::shader()
{
  if (_preintegration && _lighting)
  {
    return _shaders[PRE_INTEGRATED_LIGHT];
  }
  
  if (_preintegration && !_lighting)
  {
    return _shaders[PRE_INTEGRATED];
  }
  
  if (!_preintegration && _lighting)
  {
    return _shaders[LIGHT];
  }
  
  return _shaders[DEFAULT];
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRShader::calculateSampling()
{
  DVRTexture3d::calculateSampling();

  if (_preintegration)
  {
    _shader->enable();

    if (GLEW_VERSION_2_0)
    {
      glUniform1f(_shader->uniformLocation("delta"), _delta);
    }
    else
    {
      glUniform1fARB(_shader->uniformLocation("delta"), _delta);
    }

    _shader->disable();
  }
}

//----------------------------------------------------------------------------
// Draw the proxy geometry for the brick. Overriden to setup the texture
// matrix that the shader can use to convert object coordinates to 
// texture coordinates. 
//----------------------------------------------------------------------------
void DVRShader::drawViewAlignedSlices(const TextureBrick *brick,
                                      const Matrix3d &modelview,
                                      const Matrix3d &modelviewInverse)
{
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  if (_preintegration)
  {
    Point3d vmax = brick->volumeMax();
    Point3d vmin = brick->volumeMin();
    Point3d tmax = brick->textureMax();
    Point3d tmin = brick->textureMin();

    Point3d scale((tmax.x - tmin.x) / (vmax.x - vmin.x),
                  (tmax.y - tmin.y) / (vmax.y - vmin.y),
                  (tmax.z - tmin.z) / (vmax.z - vmin.z));

    Point3d trans((tmin.x / scale.x) - vmin.x,
                  (tmin.y / scale.y) - vmin.y,
                  (tmin.z / scale.z) - vmin.z);

    glScaled(scale.x, scale.y, scale.z);
    glTranslated(trans.x, trans.y, trans.z);
  }

  glMatrixMode(GL_MODELVIEW);

  DVRTexture3d::drawViewAlignedSlices(brick, modelview, modelviewInverse);
}

