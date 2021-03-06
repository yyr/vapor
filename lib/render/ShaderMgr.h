/*
 *  ShaderMgr.h
 *  
 *
 *  Created by Yannick Polius on 6/6/11.
 *  Copyright 2011 Yannick Polius. All rights reserved.
 *
 */

#ifndef ShaderMgr_h
#define ShaderMgr_h
#include <vector>
#include <QString>
#include <QDir>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <vapor/MyBase.h>
#include <QGLWidget>
using namespace VetsUtil;
namespace VAPoR {
	class ShaderProgram;
	class RENDER_API ShaderMgr : public MyBase {
	public:
		ShaderMgr();
		~ShaderMgr();
		ShaderMgr(const char* directory);
		ShaderMgr(std::string directory, QGLWidget *owner);
		void setShaderSourceDir(const char* directory);
		std::string GLVendor();
		std::string GLRenderer();
		std::string GLVersion();
		std::string GLShaderVersion();
		std::string GLExtensions();
		bool supportsExtension(std::string extension);
		bool supportsFeatures(std::string features);
		void setGLSLVersion(int version);
		bool loadShaders();
		bool reloadShaders();
		bool enableEffect(std::string effect);
		bool disableEffect();
		bool uploadEffectData(std::string effect, std::string variable, int value);
		bool uploadEffectData(std::string effect, std::string variable, int value1, int value2);
		bool uploadEffectData(std::string effect, std::string variable, int value1, int value2, int value3);
		bool uploadEffectData(std::string effect, std::string variable, int value1, int value2, int value3, int value4);
		bool uploadEffectData(std::string effect, std::string variable, float value);
		bool uploadEffectData(std::string effect, std::string variable, float value1, float value2);
		bool uploadEffectData(std::string effect, std::string variable, float value1, float value2, float value3);
		bool uploadEffectData(std::string effect, std::string variable, float value1, float value2, float value3, float value4);
		bool defineEffect(std::string baseName, std::string defines, std::string instanceName);
		GLint* getUniformValuei(std::string effect, std::string variable, int count);
		GLfloat* getUniformValuef(std::string effect, std::string variable, int count);
		bool undefEffect(std::string instanceName);
		bool effectExists(std::string effect);
		void printEffects();
		bool checkFramebufferStatus();
		int maxTexUnits(bool fixed);	    
	private:
		bool loadEffectFile(std::string file);
		std::string convertDefines(std::string defines);
		bool loadVertShader(std::string path, std::string fileName, std::string effectName);
		bool loadFragShader(std::string path, std::string fileName, std::string effectName);
		std::vector<std::string> preprocess(const char* source, std::string &processed);
		std::map<std::string, ShaderProgram*> effects;
		std::map<std::string, std::map<int, std::vector<std::string> > > baseEffects;
		std::string sourceDir;
		std::string findEffect(GLuint prog);
		bool loaded;
		int glsl_version;
		QGLWidget *parent;
	};
};
#endif
