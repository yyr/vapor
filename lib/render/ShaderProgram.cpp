//-- ShaderProgram.cpp -------------------------------------------------------
// 
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// OGSL Shader program C++ wrapper. 
//
//----------------------------------------------------------------------------

#include "ShaderProgram.h"
#include "glutil.h"
#include "errorcodes.h"
#include "vapor/MyBase.h"
#include <sstream>
#include <iostream>
#include <fstream>

#include <qapplication.h>
#include <qfile.h>
#include <qcstring.h>

using namespace VAPoR;


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
  std::list<GLuint>::iterator iter;

  for (iter=_shaders.begin(); iter != _shaders.end(); iter++)
  {
    GLuint shader = *iter;

    if (GLEW_VERSION_2_0)
    {
      glDeleteShader(shader);
    }
    else
    {
      glDeleteObjectARB(shader);
    }
  }

  if (GLEW_VERSION_2_0)
  {
    glDeleteProgram(_program);
  }
  else
  {
    glDeleteObjectARB(_program);
  }

  printOpenGLError();
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
  int length = 2896;
  static int count = 0;

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
  file.flush();

  const GLchar *sourceBuffer = (const GLchar*)buffer.data();
  int size = buffer.size();

  if (GLEW_VERSION_2_0)
  {
    shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &sourceBuffer, &size);

    printOpenGLError();

    glAttachShader(_program, shader);

    printOpenGLError();
  }
  else
  {
    shader = glCreateShaderObjectARB(shaderType);
    glShaderSourceARB(shader, 1, &sourceBuffer, &size);

    printOpenGLError();

    glAttachObjectARB(_program, shader);

    printOpenGLError();
  }

  _shaders.push_back(shader);
 
  file.close();

  return true;
}

//----------------------------------------------------------------------------
// Load vertex shader source from a string. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadVertexSource(const char *source)
{
  return loadSource(source, GL_VERTEX_SHADER);
}


//----------------------------------------------------------------------------
// Load fragment shader source from a file. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadFragmentSource(const char *source)
{
  return loadSource(source, GL_FRAGMENT_SHADER);
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
  int length = strlen(source);
  const GLchar *sourceBuffer = (const GLchar*)source;

  if (GLEW_VERSION_2_0)
  {
    shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &sourceBuffer, &length);
    printOpenGLError();

    //
    // Attach the shader
    //
    glAttachShader(_program, shader);
    printOpenGLError();
  }
  else
  {
    shader = glCreateShaderObjectARB(shaderType);
    glShaderSourceARB(shader, 1, &sourceBuffer, &length);
    printOpenGLError();

    //
    // Attach the shader
    //
    glAttachObjectARB(_program, shader);
    printOpenGLError();
  }


  _shaders.push_back(shader);

  return true;
}

//----------------------------------------------------------------------------
// Create the shader program
//----------------------------------------------------------------------------
void ShaderProgram::create()
{
  if (GLEW_VERSION_2_0)
  {
    _program = glCreateProgram();
  }
  else
  {
    _program = glCreateProgramObjectARB();    
  }

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

    if (GLEW_VERSION_2_0)
    {
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

        std::cerr <<  "Shader InfoLog: " << infoLog << std::endl;

        VetsUtil::MyBase::SetErrMsg(VAPOR_WARNING_GL_SHADER_LOG, 
                                    "Shader InfoLog: %s\n", infoLog);
        
        delete infoLog;
        infoLog = NULL;
      }
    }
    else
    {
      //
      // Compile a single shader
      //
      glCompileShaderARB(shader);
      glGetObjectParameterivARB(shader, 
                                GL_OBJECT_COMPILE_STATUS_ARB, 
                                &shaderCompiled);
      printOpenGLError();

      compiled &= shaderCompiled;
      
      //
      // Print log information
      //
      glGetObjectParameterivARB(shader, 
                                GL_OBJECT_INFO_LOG_LENGTH_ARB, 
                                &infologLength);
      printOpenGLError();
      
      if (infologLength > 1)
      {
        GLchar *infoLog = new GLchar[infologLength];
        int nWritten  = 0;
        
        glGetInfoLogARB(shader, infologLength, &nWritten, infoLog);
        
        std::cerr <<  "Shader InfoLog: " << infoLog << std::endl;

        VetsUtil::MyBase::SetErrMsg(VAPOR_WARNING_GL_SHADER_LOG, 
                                   "Shader InfoLog: %s\n", infoLog);
        
        delete infoLog;
        infoLog = NULL;
      }
    }

    if (!compiled)
    {
      return false;
    }
  }

  //
  // Link the program
  //
  GLint linked;

  if (GLEW_VERSION_2_0)
  {
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

      std::cerr <<  "Shader InfoLog: " << infoLog << std::endl;
 
      VetsUtil::MyBase::SetErrMsg(VAPOR_WARNING_GL_SHADER_LOG, 
                                  "Shader InfoLog: %s\n", infoLog);

      delete infoLog;
      infoLog = NULL;
    }
  }
  else
  {
    glLinkProgramARB(_program);
    glGetObjectParameterivARB(_program, GL_OBJECT_LINK_STATUS_ARB, &linked);
    printOpenGLError();

    //
    // Print log information
    //
    GLint infologLength = 0;
    glGetObjectParameterivARB(_program, 
                              GL_OBJECT_INFO_LOG_LENGTH_ARB, &infologLength);
    printOpenGLError();
    
    if (infologLength > 1)
    {
      GLchar *infoLog = new GLchar[infologLength];
      int nWritten  = 0;
      
      glGetInfoLogARB(_program, infologLength, &nWritten, infoLog);
      
      std::cerr <<  "Shader InfoLog: " << infoLog << std::endl;

      VetsUtil::MyBase::SetErrMsg(VAPOR_WARNING_GL_SHADER_LOG, 
                                  "Shader InfoLog: %s\n", infoLog);

      delete infoLog;
      infoLog = NULL;
    }
  }
  
  return (linked != 0);
}

//----------------------------------------------------------------------------
// Enable the shader program.
//----------------------------------------------------------------------------
void ShaderProgram::enable()
{
  if (GLEW_VERSION_2_0)
  {
    glUseProgram(_program);
  }
  else
  {
    glUseProgramObjectARB(_program);
  }

  printOpenGLError();
}


//----------------------------------------------------------------------------
// Disable the shader program.
//----------------------------------------------------------------------------
void ShaderProgram::disable()
{
  if (GLEW_VERSION_2_0)
  {
    glUseProgram(0);
  }
  else
  {
    glUseProgramObjectARB(0);
  }

  printOpenGLError();
}


//----------------------------------------------------------------------------
// Find uniform location in the shader. 
//----------------------------------------------------------------------------
GLint ShaderProgram::uniformLocation(const char *uniformName)
{
  GLint location;

  if (GLEW_VERSION_2_0)
  {
    location = glGetUniformLocation(_program, uniformName);
  }
  else
  {
    location = glGetUniformLocationARB(_program, uniformName);
  }

  if (location == -1)
  {
    std::cerr << "Unknown uniform " << uniformName << std::endl;

    VetsUtil::MyBase::SetErrMsg(VAPOR_ERROR_GL_UNKNOWN_UNIFORM, 
                                "uniform \"%s\" not found.\n", uniformName);
  }

  printOpenGLError();

  return location;
}


//----------------------------------------------------------------------------
// Determine if shader programming is supported. 
//----------------------------------------------------------------------------
bool ShaderProgram::supported()
{
  return (GLEW_VERSION_2_0 || GLEW_ARB_shader_objects);
}



