//--DVRSpherical.cpp ----------------------------------------------------------
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

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <qgl.h>

#include "DVRSpherical.h"
#include "TextureBrick.h"
#include "ShaderProgram.h"
#include "BBox.h"
#include "glutil.h"
#include "params.h"
#include "vapor/errorcodes.h"
#include "datastatus.h"

#include "Matrix3d.h"
#include "Point3d.h"
#include "shaders.h"

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRSpherical::DVRSpherical(DataType_T type, int nthreads) :
  DVRShader(type, nthreads),
  _nr(0),
  _shellWidth(1.0),
  _permutation(3),
  _clip(3)
{
  _shaders[DEFAULT]        = NULL;
  _shaders[LIGHT]          = NULL;
  _shaders[PRE_INTEGRATED] = NULL;

  _permutation[0] = 0;
  _permutation[1] = 1;
  _permutation[2] = 2;

  _clip[0] = false;
  _clip[1] = false;
  _clip[2] = false;
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRSpherical::~DVRSpherical() 
{
  delete _shaders[DEFAULT];
  _shaders[DEFAULT] = NULL;

  delete _shaders[LIGHT];
  _shaders[LIGHT] = NULL;

  delete [] _colormap;
  _colormap = NULL;
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRSpherical::GraphicsInit() 
{
  glewInit();

  initTextures();

  //
  // Create, Load & Compile the default shader program
  //
  if (!createShader(DEFAULT, 
                    NULL, 
                    NULL, 
                    "--spherical", 
                    spherical_shader_default))
  {
    return -1;
  }

  //
  // Create, Load & Compile the lighting shader program
  //
  if (!createShader(LIGHT,
                    "--spherical_lighting-vertex-shader",
                    vertex_shader_lighting,
                    "--spherical_lighting-fragment-shader",
                    spherical_shader_lighting))
  {
    return -1;
  }

  //
  // Set up initial uniform values
  //
  _shaders[DEFAULT]->enable();

  if (GLEW_VERSION_2_0)
  {
    glUniform1i(_shaders[DEFAULT]->uniformLocation("colormap"), 1);
    glUniform1i(_shaders[DEFAULT]->uniformLocation("volumeTexture"), 0);
  }
  else
  {
    glUniform1iARB(_shaders[DEFAULT]->uniformLocation("colormap"), 1);
    glUniform1iARB(_shaders[DEFAULT]->uniformLocation("volumeTexture"), 0);
  }

  _shaders[DEFAULT]->disable();



  //
  // Create, Load & Compile the lighting shader program
  //
  // TBD ...

  //
  // Set the current shader
  //
  _shader = _shaders[DEFAULT];

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRSpherical::SetRegionSpherical(void *data,
                                     int nx, int ny, int nz,
                                     const int data_roi[6],
                                     const float extents[6],
                                     const int data_box[6],
                                     int level,
                                     size_t /*fullHeight*/,
                                     const std::vector<long> &permutation,
                                     const std::vector<bool> &clip)
{ 
  assert(permutation.size() == 3 && clip.size() == 3);

  for(int i=0; i<permutation.size(); i++)
  {
    _permutation[i] = permutation[i];
    _clip[i] = clip[i];
  }
 
  //
  // Set the texture data
  //
  if (_nx != nx || _ny != ny || _nz != nz)
  {
    _data = data;
  }

  //
  // Set the geometry extents
  //
  _data = data;

  if (_lastRegion.update(nx, ny, nz, data_roi, data_box, extents))
  {
    _level = level;

    if (_nx != nx || _ny != ny || _nz != nz)
    {
      _nx = _bx = nextPowerOf2(nx); 
      _ny = _by = nextPowerOf2(ny); 
      _nz = _bz = nextPowerOf2(nz);
    }
    
    //
    // Hard-code the volume extents to the unit cube 
    //
    _vmin.x = 0;//extents[0];
    _vmin.y = 0;//extents[1];
    _vmin.z = 0;//extents[2];
    _vmax.x = 1;//extents[3];
    _vmax.y = 1;//extents[4];
    _vmax.z = 1;//extents[5];
    
    _delta = fabs(_vmin.z-_vmax.z) / (2.0*nz); 
    
    for (int i=0; i<_bricks.size(); i++)
    {
      delete _bricks[i];
      _bricks[i] = NULL;
    }
    
    _bricks.clear();
    
    if (_nx <= _maxTexture && _ny <= _maxTexture && _nz <= _maxTexture)
    {
      //
      // The data will fit completely within texture memory, so no bricking is 
      // needed. We can save a few cycles by setting up the single brick here,
      // rather than calling buildBricks(...). 
      //
      _bricks.push_back(new TextureBrick(_type));
      
      _bricks[0]->volumeMin(0, 0, 0);
      _bricks[0]->volumeMax(1, 1, 1);

      _bricks[0]->textureMin(0, 0, 0);
      _bricks[0]->textureMax(1, 1, 1);
      
      _bricks[0]->fill((GLubyte*)data, nx, ny, nz);
      
      loadTexture(_bricks[0]);
    } 
    else
    {
      //
      // The data will not fit completely within texture memory, need to 
      // subdivide into multiple bricks.
      //

      // TBD 
      // buildBricks(level, data_box, data_roi, nx, ny, nz);

      SetErrMsg(VAPOR_WARNING, 
                "Bricking is currently unsupported for spherical rendering");
    }

    initShaderVariables();

    calculateSampling();
  }
  else
  {
    //
    // Only the data has changed; therefore, we'll refill and reuse the
    // the existing bricks (texture objects).
    //
    for (int i=0; i<_bricks.size(); i++)
    {
      _bricks[i]->refill((GLubyte*)data);
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool DVRSpherical::supported()
{
  return (ShaderProgram::supported() && GLEW_ARB_multitexture);
}

//----------------------------------------------------------------------------
// Calculate the sampling distance
//----------------------------------------------------------------------------
void DVRSpherical::calculateSampling()
{
  //
  // Get the modelview matrix and its inverse
  //
  Matrix3d modelview;   
  Matrix3d modelviewInverse;

  glGetFloatv(GL_MODELVIEW_MATRIX, modelview.data());  
  modelview.inverse(modelviewInverse);

  BBox volumeBox(_vmin, _vmax);

  volumeBox.transform(modelview);

  // 
  // Calculate the the minimum and maximum z-position of the rotated bounding
  // boxes. Equivalent to but quicker than:
  //
  // Vect3d maxv(0, 0, rotatedBox.maxZ().z); 
  // Vect3d minv(0, 0, rotatedBox.minZ().z); 
  // maxv = modelviewInverse * maxd;
  // minv = modelviewInverse * mind;
  //
  Vect3d maxv(modelviewInverse(2,0)*volumeBox.maxZ().z,
              modelviewInverse(2,1)*volumeBox.maxZ().z,
              modelviewInverse(2,2)*volumeBox.maxZ().z);
  Vect3d minv(modelviewInverse(2,0)*volumeBox.minZ().z,
              modelviewInverse(2,1)*volumeBox.minZ().z,
              modelviewInverse(2,2)*volumeBox.minZ().z);

  if (_renderFast)
  {
    _samples = MAX(_nx, MAX(_ny, _nz));
  }
  else
  {
    _samples = _nr * ((maxv - minv).mag() / _shellWidth);
  }
  
  _delta = (maxv - minv).mag() / _samples; 
  _samplingRate = (1.0/(float)(_level+1))*(_shellWidth / _delta) / (float)_nr;
  
  // TBD -- The sampling rate & opacity correction delta are not quite right 
  // for spherical geometry. We need to work through how the spherical 
  // transform is warping the voxels and effecting the sampling. 
  // 
}

//----------------------------------------------------------------------------
// Rearrange the x, y, and z components according to the permutation matrix
//----------------------------------------------------------------------------
void DVRSpherical::permute(const vector<long>& permutation,
                           float result[3], float x, float y, float z)
{
  result[permutation[0]] = x;
  result[permutation[1]] = y;
  result[permutation[2]] = z;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRSpherical::initShaderVariables()
{
  assert(_shader);
  _shader->enable();

  const int *data_roi  = _lastRegion.roi();
  const float *extents = _lastRegion.extents();

  if (GLEW_VERSION_2_0)
  {
  
    if (_lighting)
    {
      glUniform3f(_shader->uniformLocation("dimensions"), _nx, _ny, _nz);
      glUniform1f(_shader->uniformLocation("kd"), _kd);
      glUniform1f(_shader->uniformLocation("ka"), _ka);
      glUniform1f(_shader->uniformLocation("ks"), _ks);
      glUniform1f(_shader->uniformLocation("expS"), _expS);
      glUniform3f(_shader->uniformLocation("lightDirection"), 
                  _pos[0], _pos[1], _pos[2]);
    } 
    
    float tmpv[3];
    
    permute(_permutation, tmpv, _nx, _ny, _nz);
    
    _nr = (int)tmpv[2];
    
    permute(_permutation, tmpv, 
            (float)data_roi[0]/(_nx-1),
            (float)data_roi[1]/(_ny-1), 
            (float)data_roi[2]/(_nz-1));
    
    glUniform3f(_shader->uniformLocation("tmin"), 
                tmpv[0], tmpv[1], tmpv[2]);
      
    permute(_permutation, tmpv,
            (float)data_roi[3]/(_nx-1), 
            (float)data_roi[4]/(_ny-1), 
            (float)data_roi[5]/(_nz-1));
    
    glUniform3f(_shader->uniformLocation("tmax"), 
                tmpv[0], tmpv[1], tmpv[2]);
    
    permute(_permutation, tmpv, extents[0], extents[1], extents[2]);
    
    glUniform3f(_shader->uniformLocation("dmin"), 
                (tmpv[0] + 180.0) / 360.0,
                (tmpv[1] + 90.0) / 180.0,
                tmpv[2]);
    
    _shellWidth = tmpv[2];
    
    permute(_permutation, tmpv, extents[3], extents[4], extents[5]);
    
    glUniform3f(_shader->uniformLocation("dmax"), 
                (tmpv[0] + 180.0) / 360.0,
                (tmpv[1] + 90.0) / 180.0,
                tmpv[2]);
    
    assert(tmpv[2]);
    _shellWidth = (tmpv[2] - _shellWidth)/(2.0*tmpv[2]);
    
    glUniform3f(_shader->uniformLocation("permutation"), 
                _permutation[0], _permutation[1], _permutation[2]);
    
    glUniform2i(_shader->uniformLocation("clip"), 
                _clip[_permutation[0]], 
                _clip[_permutation[1]]);

  }
  else
  {
    if (_lighting)
    {
      glUniform3fARB(_shader->uniformLocation("dimensions"), _nx, _ny, _nz);
      glUniform1fARB(_shader->uniformLocation("kd"), _kd);
      glUniform1fARB(_shader->uniformLocation("ka"), _ka);
      glUniform1fARB(_shader->uniformLocation("ks"), _ks);
      glUniform1fARB(_shader->uniformLocation("expS"), _expS);
      glUniform3fARB(_shader->uniformLocation("lightDirection"), 
                     _pos[0], _pos[1], _pos[2]);
    }

    float tmpv[3];
    
    permute(_permutation, tmpv, 
            (float)data_roi[0]/(_nx-1),
            (float)data_roi[1]/(_ny-1), 
            (float)data_roi[2]/(_nz-1));
      
    glUniform3fARB(_shader->uniformLocation("tmin"), 
                   tmpv[0], tmpv[1], tmpv[2]);
    
    permute(_permutation, tmpv,
            (float)data_roi[3]/(_nx-1), 
            (float)data_roi[4]/(_ny-1), 
            (float)data_roi[5]/(_nz-1));
    
    glUniform3fARB(_shader->uniformLocation("tmax"), 
                   tmpv[0], tmpv[1], tmpv[2]);
    
    permute(_permutation, tmpv, extents[0], extents[1], extents[2]);
    
    glUniform3fARB(_shader->uniformLocation("dmin"), 
                   tmpv[0], tmpv[1], tmpv[2]);
    
    _shellWidth = tmpv[2];
    
    permute(_permutation, tmpv, extents[3], extents[4], extents[5]);
    
    glUniform3fARB(_shader->uniformLocation("dmax"), 
                   tmpv[0], tmpv[1], tmpv[2]);
    
    assert(tmpv[2]);
    _shellWidth = (tmpv[2] - _shellWidth)/(2.0*tmpv[2]);
    
    glUniform3fARB(_shader->uniformLocation("permutation"), 
                   _permutation[0], _permutation[1], _permutation[2]);
    
    glUniform2iARB(_shader->uniformLocation("clip"), 
                   _clip[_permutation[0]], 
                   _clip[_permutation[1]]);
  } 

  _shader->disable();
}

//----------------------------------------------------------------------------
// Spherical Default Shader
//----------------------------------------------------------------------------
char DVRSpherical::spherical_shader_default[] = 
"uniform sampler1D colormap;"
"uniform sampler3D volumeTexture;"
"uniform vec3 tmin;"
"uniform vec3 tmax;"
"uniform vec3 dmin;"
"uniform vec3 dmax;"
"uniform vec3 permutation;"
"uniform bvec2 clip;"
""
"//-------------------------------------------------------------------------\n"
"// Rearrange the vector into (lon, lat, radius) order \n"
"//-------------------------------------------------------------------------\n"
"vec3 permute(vec3 v)"
"{"
"  vec3 p = v;"
""
"  // \n"
"  // Unfortunately, GLSL 2.0 doesn't seem to allow \n"
"  // a uniform to be used as an index. We'll do it \n"
"  // the long way. \n"
"  // \n"
"  if (permutation.x == 2.0) p.z = v.x;"
"  else if (permutation.x == 1.0) p.y = v.x;"
""
"  if (permutation.y == 2.0) p.z = v.y;"
"  else if (permutation.y == 0.0) p.x = v.y;"
""
"  if (permutation.z == 0.0) p.x = v.z;"
"  else if (permutation.z == 1.0) p.y = v.z;"
""
"  return p;"
"}"
""
"//------------------------------------------------------------------------ \n"
"// Rearrange the vector from (lon, lat, radius) order into the original order"
"//------------------------------------------------------------------------ \n"
"vec3 invPermute(vec3 v)"
"{"
"  vec3 p = v;"
""
"  // \n"
"  // Unfortunately, GLSL 2.0 doesn't seem to allow \n"
"  // a uniform to be used as an index. We'll do it \n"
"  // the long way. \n"
"  // \n"
"  if (permutation.x == 2.0) p.x = v.z;"
"  else if (permutation.x == 1.0) p.x = v.y;"
""
"  if (permutation.y == 2.0) p.y = v.z;"
"  else if (permutation.y == 0.0) p.y = v.x;"
""
"  if (permutation.z == 0.0) p.z = v.x;"
"  else if (permutation.z == 1.0) p.z = v.y;"
""
"  return p;"
"}"
""
"//------------------------------------------------------------------ \n"
"// Fragment shader main"
"//------------------------------------------------------------------ \n"
"void main(void)"
"{"
"  const float pi = 3.14159;"
""
"  vec3 cartesian = permute(gl_TexCoord[0].xyz);"
"  vec3 spherical;"
""
"  cartesian = dmax.z * (cartesian * 2.0 - 1.0);"
""
"  spherical.z = length(cartesian.xyz);"
""
"  if (spherical.z > dmax.z || spherical.z < dmin.z)"
"  {"
"    gl_FragColor = vec4(0,0,0,0);"
"  }"
"  else"
"  {"
"    spherical.x = asin(clamp(cartesian.y / length(cartesian.yz), -1.0, 1.0));"
"    spherical.y = acos(clamp(cartesian.x / spherical.z, -1.0, 1.0));"
""
"    if (cartesian.z >= 0.0)"
"    {"
"      if (spherical.x < 0.0) spherical.x += 2.0*pi;"
"    }"
"    else"
"    {"
"      spherical.x = pi - spherical.x;"
"    }"
""
"    // Spherical coordinates \n"
"    // spherical.y (0, pi) \n"
"    // spherical.x (0, 2pi) \n"
""    
"    spherical.y = (spherical.y / (pi));"
"    spherical.x = (spherical.x / (2.0*pi));"
""
"    // Normalized spherical coordinates \n"
"    // spherical.z (0, 1) \n"
"    // spherical.y (0, 1) \n"
"    // spherical.x (0, 1) \n"
""
"    if ((clip.y && (spherical.y < dmin.y || spherical.y > dmax.y)) ||"
"        (clip.x && (spherical.x < dmin.x || spherical.x > dmax.x)))"
"    {"
"      gl_FragColor = vec4(1,1,1,0);"
"    }"
"    else"
"    {"
"      // Texture coordinates (dmin, dmax) -> (tmin, tmax) \n"
"      spherical = (spherical - dmin) * ((tmax - tmin) / (dmax - dmin))+tmin;"
""
"      vec4 intensity = vec4(texture3D(volumeTexture, invPermute(spherical)));"
"      gl_FragColor = vec4 (texture1D(colormap, intensity.x));"
"    }"
"  }"
"}";

//----------------------------------------------------------------------------
// Spherical Lighting Shader
//----------------------------------------------------------------------------
char DVRSpherical::spherical_shader_lighting[] = 
"#version 110\n"
"uniform sampler1D colormap;"
"uniform sampler3D volumeTexture;"
"uniform vec3 tmin;"
"uniform vec3 tmax;"
"uniform vec3 dmin;"
"uniform vec3 dmax;"
"uniform vec3 permutation;"
"uniform vec3 dimensions;"
"uniform bvec2 clip;"
"uniform vec3 lightDirection;"
"uniform float kd;"
"uniform float ka;"
"uniform float ks;"
"uniform float expS;"
"varying vec3 view;"
""
"//-------------------------------------------------------------------------\n"
"// Rearrange the vector into (lon, lat, radius) order \n"
"//-------------------------------------------------------------------------\n"
"vec3 permute(vec3 v)"
"{"
"  vec3 p = v;"
""
"  // \n"
"  // Unfortunately, GLSL 2.0 doesn't seem to allow \n"
"  // a uniform to be used as an index. We'll do it \n"
"  // the long way. \n"
"  // \n"
"  if (permutation.x == 2.0) p.z = v.x;"
"  else if (permutation.x == 1.0) p.y = v.x;"
""
"  if (permutation.y == 2.0) p.z = v.y;"
"  else if (permutation.y == 0.0) p.x = v.y;"
""
"  if (permutation.z == 0.0) p.x = v.z;"
"  else if (permutation.z == 1.0) p.y = v.z;"
""
"  return p;"
"}"
""
"//-------------------------------------------------------------------------\n"
"// Rearrange the vector from (lon, lat, radius) order into the original order"
"//-------------------------------------------------------------------------\n"
"vec3 invPermute(vec3 v)"
"{"
"  vec3 p = v;"
""
"  // \n"
"  // Unfortunately, GLSL 2.0 doesn't seem to allow"
"  // a uniform to be used as an index. We'll do it"
"  // the long way."
"  // \n"
"  if (permutation.x == 2.0) p.x = v.z;"
"  else if (permutation.x == 1.0) p.x = v.y;"
""
"  if (permutation.y == 2.0) p.y = v.z;"
"  else if (permutation.y == 0.0) p.y = v.x;"
""
"  if (permutation.z == 0.0) p.z = v.x;"
"  else if (permutation.z == 1.0) p.z = v.y;"
""
"  return p;"
"}"
""
""
"//-------------------------------------------------------------------------\n"
"// Cartesian -> Spherical transform"
"//-------------------------------------------------------------------------\n"
"vec3 cart2sph(vec3 cartesian)"
"{"
"  const float pi = 3.14159;"
""
"  vec3 spherical;"
""
"  spherical.z = length(cartesian.xyz);"
"  spherical.x = asin(clamp(cartesian.y / length(cartesian.yz), -1.0, 1.0));"
"  spherical.y = acos(clamp(cartesian.x / spherical.z, -1.0, 1.0));"
""  
"  if (cartesian.z >= 0.0)"
"  {"
"    if (spherical.x < 0.0) spherical.x += 2.0*pi;"
"  }"
"  else"
"  {"
"    spherical.x = pi - spherical.x;"
"  }"
""  
"  // Spherical coordinates \n"
"  // spherical.y (0, pi) \n"
"  // spherical.x (0, 2pi) \n"
""  
"  spherical.y = (spherical.y / (pi));"
"  spherical.x = (spherical.x / (2.0*pi));"
""
"  // Normalized spherical coordinates \n"
"  // spherical.z (0, 1) \n"
"  // spherical.y (0, 1) \n"
"  // spherical.x (0, 1) \n"
"  spherical = (spherical - dmin) * ((tmax - tmin) / (dmax - dmin)) + tmin;"
""
"  return spherical;"
"}"
""
"//--------------------------------------------------------------------- \n"
"// Compute the data gradient"
"//--------------------------------------------------------------------- \n"
"vec3 dataGradient(vec3 coord)"
"{"
"  const float pi = 3.14159;"
""
"  vec3 sgradient;"
"  vec3 gradient;"
""
"  float dx = 0.5/(dimensions.x);"
"  float dy = 0.5/(dimensions.y);"
"  float dz = 0.5/(dimensions.z);"
""  
"  vec3 a0;"
"  vec3 a1;"
""
"  a0.x = texture3D(volumeTexture, coord + vec3(dx,0,0)).x;"
"  a1.x = texture3D(volumeTexture, coord + vec3(-dx,0,0)).x;"
"  a0.y = texture3D(volumeTexture, coord + vec3(0,dy,0)).x;"
"  a1.y = texture3D(volumeTexture, coord + vec3(0,-dy,0)).x;"
"  a0.z = texture3D(volumeTexture, coord + vec3(0,0,dz)).x;"
"  a1.z = texture3D(volumeTexture, coord + vec3(0,0,-dz)).x;"
""
"  // \n"
"  // Compute the spherical gradient \n"
"  // \n"
"  sgradient = permute((a1-a0)/2.0);"
"  //sgradient.x = (sgradient.x / (coord.z * sin(coord.y*pi-pi/2.0)));"
"  //sgradient.y = (sgradient.y / coord.z); \n"
""
"  // \n"
"  // Transform the vector into the cartesian coordinate space \n"
"  // \n"
"  float sinphi = sin(sgradient.x * 2.0*pi-pi);"
"  float cosphi = cos(sgradient.x * 2.0*pi-pi);"
"  float sintheta = sin(sgradient.y * pi - pi/2.0);"
"  float costheta = cos(sgradient.y * pi - pi/2.0);"
""
"  mat3 t = mat3(-sinphi, costheta*cosphi,   sintheta*cosphi,"
"                cosphi,  sintheta*sintheta, costheta*sinphi,"
"                0,       costheta,          -sintheta);"
""
"  gradient = t * sgradient;"
""
"  return gradient;"
"}"
""
"//----------------------------------------------------------------- \n"
"// Fragment shader main \n"
"//----------------------------------------------------------------- \n"
"void main(void)"
"{"
"  const float pi = 3.14159;"
""
"  vec3 cartesian = permute(gl_TexCoord[0].xyz);"
"  vec3 spherical;"
""
"  cartesian = dmax.z * (cartesian * 2.0 - 1.0);"
""
"  spherical.z = length(cartesian.xyz);"
""
"  if (spherical.z > dmax.z || spherical.z < dmin.z)"
"  {"
"    gl_FragColor = vec4(0,0,0,0);"
"  }"
"  else"
"  {"
"    spherical.x = asin(clamp(cartesian.y / length(cartesian.yz), -1.0, 1.0));"
"    spherical.y = acos(clamp(cartesian.x / spherical.z, -1.0, 1.0));"
""
"    if (cartesian.z >= 0.0)"
"    {"
"      if (spherical.x < 0.0) spherical.x += 2.0*pi;"
"    }"
"    else"
"    {"
"      spherical.x = pi - spherical.x;"
"    }"
""
"    // Spherical coordinates \n"
"    // spherical.y (0, pi) \n"
"    // spherical.x (0, 2pi) \n"
""    
"    spherical.y = (spherical.y / (pi));"
"    spherical.x = (spherical.x / (2.0*pi));"
""
"    // Normalized spherical coordinates \n"
"    // spherical.z (0, 1) \n"
"    // spherical.y (0, 1) \n"
"    // spherical.x (0, 1) \n"
""
"    if ((clip.y && (spherical.y < dmin.y || spherical.y > dmax.y)) ||"
"        (clip.x && (spherical.x < dmin.x || spherical.x > dmax.x)))"
"    {"
"      gl_FragColor = vec4(1,1,1,0);"
"    }"
"    else"
"    {"
"      // Texture coordinates (dmin, dmax) -> (tmin, tmax) \n"
"      spherical = (spherical - dmin) * ((tmax - tmin)/(dmax - dmin)) + tmin;"
""
"      vec3 gradient = gl_NormalMatrix * dataGradient(invPermute(spherical));"
""
"      if (length(gradient) <= 0.001)"
"      {"
"        gl_FragColor = vec4(0,0,0,0);"
"      }"
"      else"
"      {"
"        gradient = normalize(gradient);"
""
"        // \n"
"        // TBD -- There is something squirely with the lighting direction \n"
"        // The problem is most likely in the gradient calculation \n"
"        // and this modelview multiplication should go away. \n"
"        // \n"
"        vec3 light = (gl_ModelViewMatrix * vec4(lightDirection,0)).xyz;"
"        vec3 halfv = normalize(light + gradient);"
""      
"        float d  = kd * abs(dot(normalize(light), gradient));"
"        float s = ks * pow(abs(dot(halfv, normalize(view))), expS);"
""        
"       vec4 intensity = vec4(texture3D(volumeTexture,invPermute(spherical)));"
"       gl_FragColor = vec4 (texture1D(colormap, intensity.x)) * (d + s + ka);"
"      }"
"    }"
"  }"
"}";
