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

#include "Matrix3d.h"
#include "Point3d.h"
#include "shaders.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRSpherical::DVRSpherical(DataType_T type, int nthreads) :
  DVRShader(type, nthreads)
{
  _shaders[DEFAULT] = NULL;
  _shaders[LIGHT]   = NULL;
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

  glDeleteTextures(1, &_texid);
  glDeleteTextures(1, &_cmapid);
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
  _shaders[DEFAULT] = new ShaderProgram();
  _shaders[DEFAULT]->create();

  if (Params::searchCmdLine("--spherical"))
  {
    _shaders[DEFAULT]->loadFragmentShader(Params::parseCmdLine("--spherical"));
  }
  else
  {
    _shaders[DEFAULT]->loadFragmentSource(spherical_shader_default);
    //_shaders[DEFAULT]->loadFragmentShader("spherical.frag");
  }

  if (!_shaders[DEFAULT]->compile())
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
  _shaders[LIGHT] = new ShaderProgram();
  _shaders[LIGHT]->create();
  
  if (Params::searchCmdLine("--lightingFS"))
  {
    _shaders[LIGHT]->loadFragmentShader(Params::parseCmdLine("--lightingFS"));
  }
  else
  {
    _shaders[LIGHT]->loadFragmentSource(fragment_shader_lighting);
  }

  if (!_shaders[LIGHT]->compile())
  {
    return -1;
  }

  _shaders[LIGHT]->enable();

  if (GLEW_VERSION_2_0)
  {
    glUniform1i(_shaders[LIGHT]->uniformLocation("colormap"), 1);
    glUniform1i(_shaders[LIGHT]->uniformLocation("volumeTexture"), 0);
    glUniform1f(_shaders[LIGHT]->uniformLocation("kd"), 0.8);
    glUniform1f(_shaders[LIGHT]->uniformLocation("ka"), 0.2);
    glUniform1f(_shaders[LIGHT]->uniformLocation("ks"), 0.5);
    glUniform1f(_shaders[LIGHT]->uniformLocation("expS"), 20);
    glUniform3f(_shaders[LIGHT]->uniformLocation("lightPosition"), 0,0,1);
  }
  else
  {
    glUniform1iARB(_shaders[LIGHT]->uniformLocation("colormap"), 1);
    glUniform1iARB(_shaders[LIGHT]->uniformLocation("volumeTexture"), 0);
    glUniform1fARB(_shaders[LIGHT]->uniformLocation("kd"), 0.8);
    glUniform1fARB(_shaders[LIGHT]->uniformLocation("ka"), 0.2);
    glUniform1fARB(_shaders[LIGHT]->uniformLocation("ks"), 0.5);
    glUniform1fARB(_shaders[LIGHT]->uniformLocation("expS"), 20);
    glUniform3fARB(_shaders[LIGHT]->uniformLocation("lightPosition"), 0,0,1);
  }

  _shaders[LIGHT]->disable();

  //
  // Set the current shader
  //
  _shader = _shaders[DEFAULT];

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRSpherical::SetRegion(void *data,
                         int nx, int ny, int nz,
                         const int data_roi[6],
                         const float extents[6],
                         const int data_box[6],
                         int level) 
{ 
  //
  // Set the texture data
  //
  if (_nx != nx || _ny != ny || _nz != nz)
  {
    _data = data;

    if (_shaders[LIGHT])
    {
      _shaders[LIGHT]->enable();

      // TBD -- should the shader get the power of 2 dimensions?

      if (GLEW_VERSION_2_0)
      {
        glUniform3f(_shaders[LIGHT]->uniformLocation("dimensions"), 
                    nx, ny, nz);
      }
      else
      {
        glUniform3fARB(_shaders[LIGHT]->uniformLocation("dimensions"), 
                       nx, ny, nz);
      }

      _shaders[LIGHT]->disable();
    }
  }

  //
  // Set the geometry extents
  //
  _data = data;

  if (_lastRegion.update(nx, ny, nz, data_roi, data_box, extents))
  {
    if (_nx != nx || _ny != ny || _nz != nz)
    {
      _nx = _bx = nextPowerOf2(nx); 
      _ny = _by = nextPowerOf2(ny); 
      _nz = _bz = nextPowerOf2(nz);
    }
    
    //
    // Hard-code the volume extents to the unit cube 
    //
    _vmin.x = 0.0; //extents[0];
    _vmin.y = 0.0; //extents[1];
    _vmin.z = 0.0; //extents[2];
    _vmax.x = 1.0; //extents[3];
    _vmax.y = 1.0; //extents[4];
    _vmax.z = 1.0; //extents[5];
    
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
      _bricks.push_back(new TextureBrick());
      
      _bricks[0]->volumeMin(0, 0, 0);
      _bricks[0]->volumeMax(1, 1, 1);

      _bricks[0]->textureMin(0, 0, 0);
      _bricks[0]->textureMax(1, 1, 1);

      _shader->enable();

      //
      // Provide the shader the texture extents and the data extents
      // 
      if (GLEW_VERSION_2_0)
      {
        glUniform3f(_shader->uniformLocation("tmin"), 
                    (float)data_roi[0]/(_nx-1),
                    (float)data_roi[1]/(_ny-1), 
                    (float)data_roi[2]/(_nz-1));
        glUniform3f(_shader->uniformLocation("tmax"), 
                    (float)data_roi[3]/(_nx-1), 
                    (float)data_roi[4]/(_ny-1), 
                    (float)data_roi[5]/(_nz-1));

        // Kludge -- hard coding radial extents. The radial extents
        // probably need to set in the vdf file. 
        //
        glUniform3f(_shader->uniformLocation("dmin"), 
                    (float)0.71,
                    (float)extents[1],
                    (float)extents[2]);
        glUniform3f(_shader->uniformLocation("dmax"), 
                    (float).96,
                    (float)extents[4],
                    (float)extents[5]);

      }
      else
      {
        glUniform3fARB(_shader->uniformLocation("tmin"),
                       (float)data_roi[0]/(_nx-1),
                       (float)data_roi[1]/(_ny-1), 
                       (float)data_roi[2]/(_nz-1));
        glUniform3fARB(_shader->uniformLocation("tmax"),
                       (float)data_roi[3]/(_nx-1), 
                       (float)data_roi[4]/(_ny-1), 
                       (float)data_roi[5]/(_nz-1));

        // Kludge -- hard coding radial extents. The radial extents
        // probably need to set in the vdf file. 
        //
        glUniform3fARB(_shader->uniformLocation("dmin"), 
                       (float)0.71,
                       (float)extents[1],
                       (float)extents[2]);
        glUniform3fARB(_shader->uniformLocation("dmax"), 
                       (float)0.96,
                       (float)extents[4],
                       (float)extents[5]);

      }

      _shader->disable();
      
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
// Proof-of-concept Shader
//----------------------------------------------------------------------------
char DVRSpherical::spherical_shader_default[] = 
"uniform sampler1D colormap;\n"
"uniform sampler3D volumeTexture;\n"
"uniform vec3 tmin;\n"
"uniform vec3 tmax;\n"
"uniform vec3 dmin;\n"
"uniform vec3 dmax;\n"
""
"//------------------------------------------------------------------\n"
"// Fragment shader main\n"
"//------------------------------------------------------------------\n"
"void main(void)\n"
"{\n"
"  const float pi = 3.14159;\n"
"  vec3 cartesian = vec3 (gl_TexCoord[0].xyz);\n"
"  vec3 spherical;\n"
""
"  cartesian = cartesian * 2.0 - 1.0;\n"
""
"  spherical.x = length(cartesian.xyz);\n"
""
"  if (spherical.x > dmax.x || spherical.x < dmin.x)\n"
"  {\n"
"    gl_FragColor = vec4(0,0,0,0);\n"
"  }\n"
"  else\n"
"  {\n"
"    // TBD protect divide by 0?\n"
""
"    spherical.y = acos(clamp(cartesian.z / spherical.x, -1.0, 1.0));\n"
""
"    if (cartesian.x >= 0.0)\n"
"    {\n"
"      spherical.z = asin(clamp(cartesian.y / length(cartesian.xy), -1.0, 1.0));\n"
""
"      if (spherical.z < 0.0) spherical.z += 2.0*pi;\n"
"    }\n"
"    else\n"
"    {\n"
"      spherical.z = pi - asin(clamp(cartesian.y / length(cartesian.xy), -1.0, 1.0));\n"
"    }\n"
""
"    // Spherical coordinates\n"
"    // spherical.x (?, ?)\n"
"    // spherical.y (0, pi)\n"
"    // spherical.z (0, 2pi)\n"
"    \n"
"    spherical.x = ((spherical.x - dmin.x) / (dmax.x - dmin.x));\n"
"    spherical.y = (spherical.y / (pi));\n"
"    spherical.z = (spherical.z / (2.0*pi));\n"
""
"    // Normalized spherical coordinates\n"
"    // spherical.x (?, ?)\n"
"    // spherical.y (0, 1)\n"
"    // spherical.z (0, 1)\n"
""
"    if (spherical.y < dmin.y || spherical.y > dmax.y ||\n"
"        spherical.z < dmin.z || spherical.z > dmax.z)\n"
"    {\n"
"      gl_FragColor = vec4(1,1,1,0);\n"
"    }\n"
"    else\n"
"    {\n"
"      // Texture coordinates (dmin, dmax) -> (tmin, tmax)\n"
"      spherical = (spherical - dmin) * ((tmax - tmin) / (dmax - dmin)) + tmin;\n"
""
"      vec4 intensity = vec4 (texture3D(volumeTexture, spherical));\n"
"      gl_FragColor = vec4 (texture1D(colormap, intensity.x));\n"
"    }\n"
"  }\n"
"}\n";
