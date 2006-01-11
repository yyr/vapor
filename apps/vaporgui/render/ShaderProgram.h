//-- ShaderProgram.h ---------------------------------------------------------
// 
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// OGSL Shader program C++ wrapper. 
//
//----------------------------------------------------------------------------

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <string>
#include <list>
#include <map>

#ifndef ShaderProgram_h
#define ShaderProgram_h

namespace VAPoR {

class ShaderProgram
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

  void create();
  bool compile();
  void enable();
  void disable();

  GLint uniformLocation(const char *uniformName);

  static bool supported();
  static int maxTexture(GLenum format);

protected:

  GLuint      _program;
  std::list<GLuint> _shaders;

  static std::map<GLenum, int> _textureSizes;
};

};

#endif // ShaderProgram_h
