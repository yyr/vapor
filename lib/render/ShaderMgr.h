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
#include <QApplication>
#include <QFile>
#include <vector>
#include <QString>
#include <QDir>
#include <QTextStream>
#include <QIODevice>
#include <map>
#include <list>
#include <iostream>

namespace VAPoR {
	class ShaderProgram;
class ShaderMgr
	{
	public:
		ShaderMgr();
		~ShaderMgr();
		ShaderMgr(const char* directory);
		ShaderMgr(QString directory);
		void setShaderSourceDir(const char* directory);
		bool loadShaders();
		bool enableEffect(QString effect);
		bool disableEffect();
		bool uploadEffectData(QString effect, QString variable, int value);
		bool uploadEffectData(QString effect, QString variable, int value1, int value2);
		bool uploadEffectData(QString effect, QString variable, int value1, int value2, int value3);
		bool uploadEffectData(QString effect, QString variable, int value1, int value2, int value3, int value4);
		bool uploadEffectData(QString effect, QString variable, float value);
		bool uploadEffectData(QString effect, QString variable, float value1, float value2);
		bool uploadEffectData(QString effect, QString variable, float value1, float value2, float value3);
		bool uploadEffectData(QString effect, QString variable, float value1, float value2, float value3, float value4);
		bool effectExists(QString effect);
		void printEffects();
	private:
		bool loadEffectFile(QString file);
		bool loadVertShader(QString file, ShaderProgram *prog, QString fileName);
		bool loadFragShader(QString file, ShaderProgram *prog, QString fileName);
		QStringList preprocess(const char* source, std::string &processed);
		std::map<QString, ShaderProgram*> effects;
		QString sourceDir;
		QString findEffect(GLuint prog);
	};
};
#endif