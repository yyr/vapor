//-- ShaderProgram.h ---------------------------------------------------------
// 
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// OGSL Shader program C++ wrapper. 
//
//----------------------------------------------------------------------------



#ifndef ShaderProgram_h
#define ShaderProgram_h

#include "common.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <string>
#include <list>

namespace VAPoR {

class RENDER_API ShaderProgram
{
 public:

  ShaderProgram();
  virtual ~ShaderProgram();
  
  bool loadVertexShader(const char *filename);
  bool loadFragmentShader(const char *filename);
  bool loadShader(const char *filename, GLenum shaderType);

  bool loadVertexSource(const char *source);
  bool loadFragmentSource(const char *source);
  bool loadSource(const char *source, GLenum shaderType);

  bool create();
  bool compile();
  int enable();
  void disable();

  GLint uniformLocation(const char *uniformName);

  static bool supported();

protected:

  GLuint      _program;
  std::list<GLuint> _shaders;
};

};

#endif // ShaderProgram_h
