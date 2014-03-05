//-- ShaderProgram.h ---------------------------------------------------------
// 
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// OGSL Shader program C++ wrapper. 
//
//----------------------------------------------------------------------------



#ifndef ShaderProgram_h
#define ShaderProgram_h

#include <vapor/common.h>
#include <GL/glew.h>

#ifdef Darwin
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <map>
#include <QString>
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

namespace VAPoR {
	
	class RENDER_API ShaderProgram
	{
	public:
		
		ShaderProgram();
		virtual ~ShaderProgram();
		
		bool loadVertexShader(const char *filename);
		bool loadFragmentShader(const char *filename);
		bool loadShader(const char *filename, GLenum shaderType);
		bool loadVertexSource(const char *source );
		bool loadFragmentSource(const char *source);
		
		bool loadVertexSource(const char *source, std::string fileName);
		bool loadFragmentSource(const char *source, std::string fileName);
		bool loadSource(const char *source, GLenum shaderType, std::string fileName);
		
		bool create();
		bool compile();
		int enable();
		void disable();
		
		GLint uniformLocation(const char *uniformName);
		
		static bool supported();
		GLuint getProgram();
		void printContents();
	protected:
		
		GLuint      _program;
		std::map<std::string, GLuint> _shaderObjects;
	};
	
};

#endif // ShaderProgram_h
