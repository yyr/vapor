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
namespace VAPoR {
	class ShaderProgram;
	class ShaderMgr
	{
	public:
		ShaderMgr();
		~ShaderMgr();
		ShaderMgr(const char* directory);
		ShaderMgr(std::string directory);
		void setShaderSourceDir(const char* directory);
		bool loadShaders();
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
		bool effectExists(std::string effect);
		void printEffects();
	private:
		bool loadEffectFile(std::string file);
		bool loadVertShader(std::string file, ShaderProgram *prog, std::string fileName);
		bool loadFragShader(std::string file, ShaderProgram *prog, std::string fileName);
		std::vector<std::string> preprocess(const char* source, std::string &processed);
		std::map<std::string, ShaderProgram*> effects;
		std::string sourceDir;
		std::string findEffect(GLuint prog);
		bool loaded;
	};
};
#endif