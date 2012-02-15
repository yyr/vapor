//-- ShaderProgram.cpp -------------------------------------------------------
// 
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// OGSL Shader program C++ wrapper. 
// 
// Changelog:
// 06/30/2011 - Yannick Polius
// Modified to accomodate the new VAPOR shader architecture
//
//----------------------------------------------------------------------------

#include "ShaderProgram.h"
#include "glutil.h"
#include <vapor/errorcodes.h>
#include <vapor/MyBase.h>
#include <sstream>
#include <iostream>
#include <fstream>

#include <qapplication.h>
#include <qfile.h>

using namespace VAPoR;


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------
ShaderProgram::ShaderProgram()
{
	_program = 0;
	_shaderObjects.clear();
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
ShaderProgram::~ShaderProgram()
{	
	printOpenGLError();
	for (std::map< std::string, GLuint>::const_iterator iter = _shaderObjects.begin();
		 iter != _shaderObjects.end(); iter++ ){
		
		GLuint shader = iter->second;
#ifdef DEBUG
		std::cout << "Attempting to delete shader obj: " << shader << " prog: " << _program << std::endl;
#endif		
		glDetachShader(_program, shader);
#ifdef DEBUG		
		if (printOpenGLError() != 0 )
			std::cout << "Delete " << shader << " FAILED" << std::endl;
#endif		
		if (GLEW_VERSION_2_0)
		{
			glDeleteShader(shader);
		}
		else
		{
			glDeleteObjectARB(shader);
		}
		
	}
	
	printOpenGLError();
	if (GLEW_VERSION_2_0)
	{
		glDeleteProgram(_program);
	}
	else
	{
		glDeleteObjectARB(_program);
	}
	_shaderObjects.clear();
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
	//  int length = 2896;
	// static int count = 0;
	
	GLuint shader;
	
	//
	// Read the file (using Qt for platform independence). 
	//
	QString path = qApp->applicationDirPath();
	path += QString("/");
	path += QString(filename);
	
	QFile file(path);
	
	if (!file.exists() || !file.open(QIODevice::Unbuffered | QIODevice::ReadOnly))
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
		
		if (printOpenGLError() != 0) return(false);
		
		glAttachShader(_program, shader);
		
		if (printOpenGLError() != 0) return(false);
	}
	else
	{
		shader = glCreateShaderObjectARB(shaderType);
		glShaderSourceARB(shader, 1, &sourceBuffer, &size);
		
		if (printOpenGLError() != 0) return(false);
		
		glAttachObjectARB(_program, shader);
		
		if (printOpenGLError() != 0) return(false);
	}
	
	_shaderObjects[std::string(filename)] = shader;
#ifdef DEBUG
	std::cout << "creating shader obj: " << shader << " prog: " << _program << std::endl;
#endif		
	
	file.close();
	
	return true;
}

//----------------------------------------------------------------------------
// Load vertex shader source from a string. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadVertexSource(const char *source, std::string fileName)
{
	return loadSource(source, GL_VERTEX_SHADER, fileName);
}
//----------------------------------------------------------------------------
// Load vertex shader source from a string. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadVertexSource(const char *source)
{
	return loadSource(source, GL_VERTEX_SHADER, "");
}
//----------------------------------------------------------------------------
// Load fragment shader source from a file. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadFragmentSource(const char *source)
{
	return loadSource(source, GL_FRAGMENT_SHADER, "");
}
//----------------------------------------------------------------------------
// Load fragment shader source from a file. 
//----------------------------------------------------------------------------
bool ShaderProgram::loadFragmentSource(const char *source, std::string fileName)
{
	return loadSource(source, GL_FRAGMENT_SHADER, fileName);
}


//----------------------------------------------------------------------------
// Create the shader from the char* source
//----------------------------------------------------------------------------
bool ShaderProgram::loadSource(const char *source, GLenum shaderType, std::string fileName)
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
		if (printOpenGLError() != 0) return(false);
		
		//
		// Attach the shader
		//
		glAttachShader(_program, shader);
		if (printOpenGLError() != 0) return(false);
	}
	else
	{
		shader = glCreateShaderObjectARB(shaderType);
		glShaderSourceARB(shader, 1, &sourceBuffer, &length);
		if (printOpenGLError() != 0) return(false);
		
		//
		// Attach the shader
		//
		glAttachObjectARB(_program, shader);
		if (printOpenGLError() != 0) return(false);
	}
	_shaderObjects[fileName] = shader;
//#ifdef DEBUG
	std::cout << "Creating shader obj: " << shader << " prog: " << _program << "name: " << fileName << std::endl;
//#endif	
	return true;
}

//----------------------------------------------------------------------------
// Create the shader program
//----------------------------------------------------------------------------
bool ShaderProgram::create()
{
	if (GLEW_VERSION_2_0)
	{
		_program = glCreateProgram();
	}
	else
	{
		_program = glCreateProgramObjectARB();    
	}
	
	if (printOpenGLError() != 0) return(false);
	return(true);
}


//----------------------------------------------------------------------------
// Compile and link the shader program
//----------------------------------------------------------------------------
bool ShaderProgram::compile()
{
	//
	// Compile all the shaders
	//
	
	bool compileSuccess = true;
	std::string shaderErrors ("");
	for (std::map< std::string, GLuint>::const_iterator iter = _shaderObjects.begin();
		 iter != _shaderObjects.end(); ++iter ){
		
		GLuint shader = iter->second;
		GLint infologLength = 0;
		GLint shaderCompiled;
		
		if (GLEW_VERSION_2_0)
		{
			//
			// Compile a single shader
			//
			glCompileShader(shader);
			glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);
			if (printOpenGLError() != 0) return(false);
			
			
			//
			// Print log information
			//
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
			if (printOpenGLError() != 0) return(false);
			
			if (shaderCompiled != GL_TRUE && infologLength > 1)
			{
				GLchar *infoLog = new GLchar[infologLength];
				int nWritten  = 0;
				
				glGetShaderInfoLog(shader, infologLength, &nWritten, infoLog);
				
				std::string output = iter->first + ":\t\n " + (char*)infoLog;
				
				shaderErrors += output;
				
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
			if (printOpenGLError() != 0) return(false);
			
			//
			// Print log information
			//
			glGetObjectParameterivARB(shader, 
									  GL_OBJECT_INFO_LOG_LENGTH_ARB, 
									  &infologLength);
			if (printOpenGLError() != 0) return(false);
			
			if (shaderCompiled != GL_TRUE && infologLength > 1)
			{
				GLchar *infoLog = new GLchar[infologLength];
				int nWritten  = 0;
				
				glGetInfoLogARB(shader, infologLength, &nWritten, infoLog);
				
				std::string output = iter->first + ":\t\n " + (char*)infoLog;
				
				shaderErrors += output;
				delete infoLog;
				infoLog = NULL;
			}
		}
		
		//if (!compiled)
		if (shaderCompiled != GL_TRUE)
		{
			compileSuccess = false;
		}
	}
	
	if (!compileSuccess) {
		//print out errrors and exit
		VetsUtil::MyBase::SetErrMsg(VAPOR_WARNING_GL_SHADER_LOG, "Shader InfoLog:\n%s", (GLchar*)shaderErrors.c_str());
		return false;
	}
	
	//
	// Link the program
	//
	GLint linked;
	
	if (GLEW_VERSION_2_0)
	{
		glLinkProgram(_program);
		glGetProgramiv(_program, GL_LINK_STATUS, &linked);
		if (printOpenGLError() != 0) return(false);
		
		//
		// Print log information
		//
		GLint infologLength = 0;
		glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &infologLength);
		if (printOpenGLError() != 0) return(false);
		
		if (linked != GL_TRUE && infologLength > 1)
		{
			GLchar *infoLog = new GLchar[infologLength];
			int nWritten  = 0;
			
			glGetProgramInfoLog(_program, infologLength, &nWritten, infoLog);
			
			VetsUtil::MyBase::SetErrMsg(VAPOR_WARNING_GL_SHADER_LOG, 
										"ShaderProgram Link Error: %s\n", infoLog);
			
			delete infoLog;
			infoLog = NULL;
		}
	}
	else
	{
		glLinkProgramARB(_program);
		glGetObjectParameterivARB(_program, GL_OBJECT_LINK_STATUS_ARB, &linked);
		if (printOpenGLError() != 0) return(false);
		
		//
		// Print log information
		//
		GLint infologLength = 0;
		glGetObjectParameterivARB(_program, 
								  GL_OBJECT_INFO_LOG_LENGTH_ARB, &infologLength);
		if (printOpenGLError() != 0) return(false);
		
		if (linked != GL_TRUE && infologLength > 1)
		{
			GLchar *infoLog = new GLchar[infologLength];
			int nWritten  = 0;
			
			glGetInfoLogARB(_program, infologLength, &nWritten, infoLog);
			
			VetsUtil::MyBase::SetErrMsg(VAPOR_WARNING_GL_SHADER_LOG, 
										"ShaderProgram Link Error: %s\n", infoLog);
			
			delete infoLog;
			infoLog = NULL;
		}
	}
	
	return (linked != 0);
}

//----------------------------------------------------------------------------
// Enable the shader program.
//----------------------------------------------------------------------------
int ShaderProgram::enable()
{
	if (GLEW_VERSION_2_0)
	{
		glUseProgram(_program);
	}
	else
	{
		glUseProgramObjectARB(_program);
	}
	
	if (printOpenGLError() != 0) return(-1);
	return(0);
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

GLuint ShaderProgram::getProgram()
{
	return _program;
}

void ShaderProgram::printContents()
{
	std::cout << "Program " << (int)_program << " Shader Objects: \n\t";
	for (std::map< std::string, GLuint>::const_iterator iter = _shaderObjects.begin();
		 iter != _shaderObjects.end(); ++iter ){ 
		std::cout << iter->first << "\n\t";
	}
	std::cout << std::endl;
}

