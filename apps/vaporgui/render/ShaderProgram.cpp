//-- ShaderProgram.cpp -------------------------------------------------------
// 
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// OGSL Shader program C++ wrapper. 
//
//----------------------------------------------------------------------------

#include "ShaderProgram.h"
#include "glutil.h"
#include <sstream>
#include <iostream>
#include <fstream>

#include <qapplication.h>
#include <qfile.h>
#include <qcstring.h>

using namespace VAPoR;


//
// Static member data initalization
//
std::map<GLenum, int> ShaderProgram::_textureSizes;

//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------
ShaderProgram::ShaderProgram()
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
ShaderProgram::~ShaderProgram()
{
  glDeleteProgram(_program);
}

  
//----------------------------------------------------------------------------
// Load vertex shader source from a file. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadVertexShader(const char *filename)
{
  return loadShader(filename, GL_VERTEX_SHADER);
}


//----------------------------------------------------------------------------
// Load fragment shader source from a file. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadFragmentShader(const char *filename)
{
  return loadShader(filename, GL_FRAGMENT_SHADER);
}


//----------------------------------------------------------------------------
// Load shader source from a file. 
//
// The shader is assumed to live in the executable directory.
//----------------------------------------------------------------------------
bool ShaderProgram::loadShader(const char *filename, GLenum shaderType)
{
  GLuint shader;

  //
  // Read the file (using Qt for platform independence). 
  //
  QString path = qApp->applicationDirPath();
  path += QString("/");
  path += QString(filename);

  QFile file(path);
  
  if (!file.exists() || !file.open(IO_Raw | IO_ReadOnly))
  {
    return false;
  }

  //
  // Create a shader object, and load it's source
  //
  QByteArray buffer = file.readAll();
  const GLchar *sourceBuffer = (const GLchar*)buffer.data();
  shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &sourceBuffer, NULL);
  printOpenGLError();

  //
  // Attach the shader
  //
  glAttachShader(_program, shader);
  printOpenGLError();

  _shaders.push_back(shader);

  return true;
}

//----------------------------------------------------------------------------
// Load vertex shader source from a string. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadVertexSource(const char *source)
{
  return loadShader(source, GL_VERTEX_SHADER);
}


//----------------------------------------------------------------------------
// Load fragment shader source from a file. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadFragmentSource(const char *source)
{
  return loadShader(source, GL_FRAGMENT_SHADER);
}

//----------------------------------------------------------------------------
// Create the shader from the char* source
//----------------------------------------------------------------------------
bool ShaderProgram::loadSource(const char *source, GLenum shaderType)
{
  GLuint shader;

  //
  // Create a shader object, and load it's source
  //
  const GLchar *sourceBuffer = (const GLchar*)source;
  shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &sourceBuffer, NULL);
  printOpenGLError();

  //
  // Attach the shader
  //
  glAttachShader(_program, shader);
  printOpenGLError();

  _shaders.push_back(shader);

  return true;
}

//----------------------------------------------------------------------------
// Create the shader program
//----------------------------------------------------------------------------
void ShaderProgram::create()
{
  _program = glCreateProgram();
  printOpenGLError();
}


//----------------------------------------------------------------------------
// Compile and link the shader program
//----------------------------------------------------------------------------
bool ShaderProgram::compile()
{
  std::list<GLuint>::iterator iter;
  bool compiled = true;

  //
  // Compile all the shaders
  //
  for (iter=_shaders.begin(); iter != _shaders.end(); iter++)
  {
    GLuint shader = *iter;
    GLint infologLength = 0;
    GLint shaderCompiled;

    //
    // Compile a single shader
    //
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);
    printOpenGLError();

    compiled &= shaderCompiled;

    //
    // Print log information
    //
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
    printOpenGLError();

    if (infologLength > 1)
    {
      GLchar *infoLog = new GLchar[infologLength];
      int nWritten  = 0;

      glGetShaderInfoLog(shader, infologLength, &nWritten, infoLog);

      std::cerr << "Shader InfoLog:" << std::endl << infoLog << std::endl;

      delete infoLog;
      infoLog = NULL;
    }
  }

  if (!compiled)
  {
    return false;
  }

  //
  // Link the program
  //
  GLint linked;

  glLinkProgram(_program);
  glGetProgramiv(_program, GL_LINK_STATUS, &linked);
  printOpenGLError();
 
  //
  // Print log information
  //
  GLint infologLength = 0;
  glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &infologLength);
  printOpenGLError();

  if (infologLength > 1)
  {
    GLchar *infoLog = new GLchar[infologLength];
    int nWritten  = 0;

    glGetProgramInfoLog(_program, infologLength, &nWritten, infoLog);
    
    std::cerr << "Program InfoLog:" << std::endl << infoLog << std::endl;

    delete infoLog;
    infoLog = NULL;
  }

  return (linked != 0);
}

//----------------------------------------------------------------------------
// Enable the shader program.
//----------------------------------------------------------------------------
void ShaderProgram::enable()
{
  glUseProgram(_program);
  printOpenGLError();
}


//----------------------------------------------------------------------------
// Disable the shader program.
//----------------------------------------------------------------------------
void ShaderProgram::disable()
{
  glUseProgram(0);
  printOpenGLError();
}


//----------------------------------------------------------------------------
// Find uniform location in the shader. 
//----------------------------------------------------------------------------
GLint ShaderProgram::uniformLocation(const char *uniformName)
{
  GLint location;

  location = glGetUniformLocation(_program, uniformName);

  if (location == -1)
  {
    std::cerr << "uniform \"" << uniformName << "\" not found." << std::endl;
  }

  printOpenGLError();

  return location;
}


//----------------------------------------------------------------------------
// Determine if shader programming is supported. 
//----------------------------------------------------------------------------
bool ShaderProgram::supported()
{
#if defined(GL_ARB_fragment_shader)
  return true;
#else
  return false;
#endif
}


//----------------------------------------------------------------------------
// Determine and return the maximum texture size (in bytes). 
//----------------------------------------------------------------------------
int ShaderProgram::maxTexture(GLenum format)
{
  if (_textureSizes.find(format) == _textureSizes.end())
  {
    int i;

    for (i = 128; i < 130000; i*=2)
    {
      glTexImage3D(GL_PROXY_TEXTURE_3D, 0, format, i, i, i, 0,
                   format, GL_UNSIGNED_BYTE, NULL);

      GLint width;

      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0,
                               GL_TEXTURE_WIDTH, &width);
      if (width == 0)
      {
        i /= 2;
        
        _textureSizes[format] = i;

        return i*i*i;
      }
    }
  }

  return _textureSizes[format];
}

