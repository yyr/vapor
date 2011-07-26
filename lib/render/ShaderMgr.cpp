/*
 *  ShaderMgr.cpp
 *  
 *
 *  Created by Yannick Polius on 6/6/11.
 *  Copyright 2011 Yannick Polius All rights reserved.
 *
 */

#include "ShaderProgram.h"

#include "ShaderMgr.h"

#include <vapor/errorcodes.h>
#include <vapor/MyBase.h>
#include "glutil.h"
using namespace VAPoR;
using namespace VetsUtil;
ShaderMgr::ShaderMgr()
{
	loaded = false;
}


ShaderMgr::ShaderMgr(const char* directory)
{
	SetDiagMsg("ShaderMgr::ShaderMgr(%s)", directory);

	loaded = false;
	sourceDir = std::string(directory);
}

ShaderMgr::ShaderMgr(std::string directory)
{
	SetDiagMsg("ShaderMgr::ShaderMgr(%s)", directory.c_str());
	loaded = false;
	sourceDir = directory;
}

ShaderMgr::~ShaderMgr()
{
	for (std::map< std::string, ShaderProgram*>::const_iterator iter = effects.begin();
		 iter != effects.end(); ++iter )
		delete iter->second;
}
void ShaderMgr::setShaderSourceDir(const char* directory)
{
	sourceDir = std::string(directory);
}

//----------------------------------------------------------------------------
// Look through the source directory for the effect files and proceed to load them
//
// Effect files point to the main vertex and fragment shader files for a single effect
// The file hierarchy is :
// -> Effect file source dir 
//    *.efc - files to describe effects (bundled fragment + vertex shader)
//	  -> main - contains the main shader programs (must contain main function)
//		 *.fgl - main fragment shader file
//		 *.vgl - main vertex shader file
//    -> includes - contains the shared code fragments (functions, not full shader programs)
//		 *.hgl - shader fragment code include file
//----------------------------------------------------------------------------

bool ShaderMgr::loadShaders()
{
	SetDiagMsg("ShaderMgr::loadShaders()");
	
	if (!loaded){
		QDir mainDir = QDir(QString(sourceDir.c_str()));
		mainDir.setFilter( QDir::Files );
		QStringList entries = mainDir.entryList();
		QStringList efcFiles = entries.filter(".efc");
		
		if (efcFiles.size() == 0) {
			SetErrMsg(VAPOR_ERROR_GL_SHADER, "No .efc files found");
			return false;
		}
		for (int i = 0; i < efcFiles.size(); i++) {
			if(loadEffectFile(efcFiles.at(i).toStdString()) == false){	
				SetErrMsg(VAPOR_ERROR_GL_SHADER, 
											"EFC file \"%s\" failed to load\n", efcFiles.at(i).toStdString().c_str());	
#ifdef DEBUG
				std::cout << "ShaderMgr::loadShaders() - " << efcFiles.at(i).toStdString() << " failed to load" << std::endl;
#endif
				return false;
			}
		}
		loaded = true;
		return true;
	}
	return false;
}
//----------------------------------------------------------------------------
// Reload shaders from the shaders directory
//----------------------------------------------------------------------------

bool ShaderMgr::reloadShaders()
{
	loaded = false;
	//Delete old shaders
	for (std::map< std::string, ShaderProgram*>::const_iterator iter = effects.begin();
		 iter != effects.end(); ++iter )
		delete iter->second;
	//Clear the map
	effects.clear();
	//Reload shaders
	return loadShaders();	
}
//----------------------------------------------------------------------------
// Enable an effect for rendering
//----------------------------------------------------------------------------
bool ShaderMgr::enableEffect(std::string effect)
{
	SetDiagMsg("ShaderMgr::enableEffect(%s)", effect.c_str());

	if (effectExists(effect)) {
		if(effects[effect]->enable() == 0)
			return true;
		else {
			return false;
		}
		
	}
	else {
		return false;
	}
	return true;
}
//----------------------------------------------------------------------------
// Disable the currently loaded effect
//----------------------------------------------------------------------------			
bool ShaderMgr::disableEffect()
{
	if (GLEW_VERSION_2_0)
	{
		glUseProgram(0);
	}
	else
	{
		glUseProgramObjectARB(0);
	}
	return true;
	
	printOpenGLError();
}
//----------------------------------------------------------------------------
// Handles parsing an effect file and constructing the shaderprogram
//----------------------------------------------------------------------------
bool ShaderMgr::loadEffectFile(std::string effect)
{
	SetDiagMsg("ShaderMgr::loadEffectFile(%s)", effect.c_str());

	//this is an effect file, open to read info
	std::string path = sourceDir + "/" + effect;
	
	ifstream file (path.c_str());
	
	std::string line1, line2, line3;
	if (file.is_open())
	{		
		getline(file, line1);
		getline(file, line2);
		getline(file, line3);				
		
		file.close();
	}	
	else {
		SetErrMsg(VAPOR_ERROR_GL_SHADER, 
									"EFC file \"%s\" failed to load\n", path.c_str());	
#ifdef DEBUG
		std::cout << "ShaderMgr::loadEffectFile - " << "path " + path << " does not exist" << std::endl;
#endif
		return false;
	}
	
	
	effects[line1] = new ShaderProgram();
	effects[line1]->create();
	//--------------------------------------
	// Prepare to parse main shaders
	// Unlike normal GLSL this will load shaders
	// with basic preprocessor support
	// 
	// Currently supported macros:
	//
	// #include
	//	included files must reside in the sourcedir/includes directory
	//--------------------------------------
	if (!line2.empty()) {
		if(loadVertShader(sourceDir + "/main/" + line2 + ".vgl", effects[line1], line2 + ".vgl") == false){
			return false;
		}
	}
	if (!line3.empty()) {
		if(loadFragShader(sourceDir + "/main/" + line3 + ".fgl", effects[line1], line3 + ".fgl") == false){
			return false;
		}
	}
	if (effects[line1]->compile())
		return true;
	else {
		return false;
	}
	
}
//----------------------------------------------------------------------------
// Loads a vertex shader, both main and included
//----------------------------------------------------------------------------
bool ShaderMgr::loadVertShader(std::string path, ShaderProgram *prog, std::string fileName)
{
	SetDiagMsg(
		"ShaderMgr::loadVertShader(%s,,%s)",
		path.c_str(), fileName.c_str()
	);
	//Vertex Shader loading
	ifstream file (path.c_str());
	std::string code;
	std::string line;
	if (file.is_open())
	{
		while ( !file.eof() )
		{
			getline (file,line);
			line += '\n';
			code += line;
		}
		file.close();
	}	
	else {
		SetErrMsg(VAPOR_ERROR_GL_SHADER, 
									"vertex shader file \"%s\" failed to load\n", path.c_str());
#ifdef DEBUG
		std::cout << "ShaderMgr::loadVertShader - " << "path " + path << " does not exist" << std::endl;
#endif
		return false;
	}
	
	
	const char *sourceBuffer = (const char*)code.c_str();;
	std::string processed = "";
	std::vector<std::string> includes = preprocess(sourceBuffer, processed);
	for (int i = 0; i <  includes.size(); i++){		
		if(loadVertShader(sourceDir + "/includes/" + includes[i], prog, includes[i]) == false)
			return false;
	}
	//finished loading includes, load main source
	if(prog->loadVertexSource((const GLchar*)processed.c_str(), fileName)){
		return true;
	}
	else {
		return false;
	}
	
}
//----------------------------------------------------------------------------
// Loads a fragment shader, both main and included
//----------------------------------------------------------------------------
bool ShaderMgr::loadFragShader(std::string path, ShaderProgram *prog, std::string fileName)
{
	SetDiagMsg(
		"ShaderMgr::loadFragShader(%s,,%s)",
		path.c_str(), fileName.c_str()
	);

	//Fragment Shader loading
	ifstream file (path.c_str());
	std::string code;
	std::string line;
	if (file.is_open())
	{
		while ( !file.eof() )
		{
			getline (file,line);
			line += '\n';
			code += line;
		}
		file.close();
	}	
	else {
		SetErrMsg(VAPOR_ERROR_GL_SHADER, 
									"fragment shader file \"%s\" failed to load\n", path.c_str());
#ifdef DEBUG
		std::cout << "ShaderMgr::loadFragShader - " << "path " + path << " does not exist" << std::endl;
#endif
		return false;
	}
	
#ifdef DEBUG
	std::cout << "ShaderMgr::loadFragShader - " << "path " + path << std::endl;
#endif
	
	const char *sourceBuffer = (const char*)code.c_str();
	std::string processed = "";
	std::vector<std::string> includes = preprocess(sourceBuffer, processed);
	for (int i = 0; i <  includes.size(); i++){		
		if(loadFragShader(sourceDir + "/includes/" + includes[i], prog, includes[i]) == false)
			return false;
	}
	//finished loading includes, load main source
	if(prog->loadFragmentSource((const GLchar*)processed.c_str(), fileName)){
		return true;
	}
	else {
		return false;
	}
	
}


//----------------------------------------------------------------------------
// Process the source string to remove preprocessor macros
// A clean version of the source code is returned in the processed handle
//----------------------------------------------------------------------------

std::vector<std::string> ShaderMgr::preprocess(const char* source, std::string &processed)
{
	std::string tmpProcess ("");
	std::string code (source);
	std::string line;
	std::vector<std::string> includes;
	for (int i = 0 ; i < code.length(); i++) {
		if (code[i] == '\n') {
			//check line for preprocessor macros
			int index = line.find("#include");
			if (index > -1) {
				//add the included file to the list of includes
				QString include (line.substr(index + 8).c_str());
				include = include.simplified();
				includes.push_back(include.toStdString());
#ifdef DEBUG
				std::cout << "ShaderMgr::preprocess - " << "include: " << include.toStdString() << " line " << line << std::endl;
#endif
				line = "";
				
			}
			else {
				line += '\n';
				//normal line, just append to processed
				tmpProcess += line;
				line = "";
			}
		}
		else {
			line += code[i];
		}
	}
	processed = tmpProcess;
	return includes;
}

//----------------------------------------------------------------------------
// Uploads data to the selected effect's uniform
//----------------------------------------------------------------------------
bool ShaderMgr::uploadEffectData(std::string effect, std::string variable, int value){
	//Check to see if program is currently loaded
	GLint current;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current);
#ifdef DEBUG
	std::cout << "ShaderMgr::uploadEffectData1i() - " << "effect: " << effect << std::endl;
#endif
	
	if (effectExists(effect)){
		if (current == effects[effect]->getProgram()) {
			//dont enable, just upload data
			if (GLEW_VERSION_2_0) {
				glUniform1i(effects[effect]->uniformLocation(variable.c_str()), value);
			}
			else {
				glUniform1iARB(effects[effect]->uniformLocation(variable.c_str()), value);
			}					
			printOpenGLError();
		}
		else {
			//enable the program to upload to temporarily, then re-enable the previous program
			if(effects[effect]->enable() > 0)
				return false;
			if (GLEW_VERSION_2_0) {
				glUniform1i(effects[effect]->uniformLocation(variable.c_str()), value);
			}
			else {
				glUniform1iARB(effects[effect]->uniformLocation(variable.c_str()), value);
			}	
			glUseProgram(current);
			
			printOpenGLError();
		}
		return true;
	}
	else {
		return false;
	}
	
}
//----------------------------------------------------------------------------
// Uploads data to the selected effect's uniform
//----------------------------------------------------------------------------
bool ShaderMgr::uploadEffectData(std::string effect, std::string variable, int value1, int value2){
	//Check to see if program is currently loaded
	GLint current;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current);	
#ifdef DEBUG
	std::cout << "ShaderMgr::uploadEffectData2i() - " << "effect: " << effect << std::endl;
#endif
	
	printOpenGLError();
	if (effectExists(effect)){
		printOpenGLError();
		if (current == effects[effect]->getProgram()) {
			//dont enable, just upload data
			if (GLEW_VERSION_2_0) {
				printOpenGLError();
				glUniform2i(effects[effect]->uniformLocation(variable.c_str()), value1, value2);					
			}
			else {
				glUniform2iARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2);
			}					
			printOpenGLError();
		}
		else {
			//enable the program to upload to temporarily, then re-enable the previous program
			if(effects[effect]->enable() != 0)
				return false;
			if (GLEW_VERSION_2_0) {
				glUniform2i(effects[effect]->uniformLocation(variable.c_str()), value1, value2);
			}
			else {
				glUniform2iARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2);
			}	
			glUseProgram(current);
			
			printOpenGLError();
		}
		return true;
	}
	else {
		return false;
	}
	
}
//----------------------------------------------------------------------------
// Uploads data to the selected effect's uniform
//----------------------------------------------------------------------------
bool ShaderMgr::uploadEffectData(std::string effect, std::string variable, int value1, int value2, int value3){
	//Check to see if program is currently loaded
	GLint current;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current);
#ifdef DEBUG
	std::cout << "ShaderMgr::uploadEffectData3i() - " << "effect: " << effect << std::endl;
#endif
	
	if (effectExists(effect)){
		if (current == effects[effect]->getProgram()) {
			//dont enable, just upload data
			if (GLEW_VERSION_2_0) {
				glUniform3i(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3);
			}
			else {
				glUniform3iARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3);
			}					
			printOpenGLError();			
		}
		else {
			//enable the program to upload to temporarily, then re-enable the previous program
			if(effects[effect]->enable() != 0)
				return false;
			if (GLEW_VERSION_2_0) {
				glUniform3i(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3);
			}
			else {
				glUniform3iARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3);
			}	
			glUseProgram(current);
			
			printOpenGLError();
		}
		return true;
	}
	else {
		return false;
	}
	
}
//----------------------------------------------------------------------------
// Uploads data to the selected effect's uniform
//----------------------------------------------------------------------------
bool ShaderMgr::uploadEffectData(std::string effect, std::string variable, int value1, int value2, int value3, int value4){
	//Check to see if program is currently loaded
	GLint current;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current);	
#ifdef DEBUG
	std::cout << "ShaderMgr::uploadEffectData4i() - " << "effect: " << effect << std::endl;
#endif
	
	if (effectExists(effect)){
		if (current == effects[effect]->getProgram()) {
			//dont enable, just upload data
			if (GLEW_VERSION_2_0) {
				glUniform4i(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3, value4);
			}
			else {
				glUniform4iARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3, value4);
			}		
			
			
			printOpenGLError();
		}
		else {
			//enable the program to upload to temporarily, then re-enable the previous program
			if(effects[effect]->enable() != 0)
				return false;
			if (GLEW_VERSION_2_0) {
				glUniform4i(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3, value4);
			}
			else {
				glUniform4iARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3, value4);
			}	
			glUseProgram(current);
			
			printOpenGLError();
		}
		return true;
	}
	else {
		return false;
	}
	
}
//----------------------------------------------------------------------------
// Uploads data to the selected effect's uniform
//----------------------------------------------------------------------------
bool ShaderMgr::uploadEffectData(std::string effect, std::string variable, float value){
	//Check to see if program is currently loaded
	GLint current;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current);	
#ifdef DEBUG
	std::cout << "ShaderMgr::uploadEffectData1f() - " << "effect: " << effect << std::endl;
#endif
	
	if (effectExists(effect)){
		if (current == effects[effect]->getProgram()) {
			//dont enable, just upload data
			if (GLEW_VERSION_2_0) {
				glUniform1f(effects[effect]->uniformLocation(variable.c_str()), value);
			}
			else {
				glUniform1fARB(effects[effect]->uniformLocation(variable.c_str()), value);
			}					
			printOpenGLError();
		}
		else {
			//enable the program to upload to temporarily, then re-enable the previous program
			if(effects[effect]->enable() != 0)
				return false;
			if (GLEW_VERSION_2_0) {
				glUniform1f(effects[effect]->uniformLocation(variable.c_str()), value);
			}
			else {
				glUniform1fARB(effects[effect]->uniformLocation(variable.c_str()), value);
			}	
			glUseProgram(current);
			
			printOpenGLError();
		}
		return true;
	}
	else {
		return false;
	}
	
}
//----------------------------------------------------------------------------
// Uploads data to the selected effect's uniform
//----------------------------------------------------------------------------
bool ShaderMgr::uploadEffectData(std::string effect, std::string variable, float value1, float value2){
	//Check to see if program is currently loaded
	GLint current;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current);	
#ifdef DEBUG
	std::cout << "ShaderMgr::uploadEffectData2f() - " << "effect: " << effect << std::endl;
#endif
	
	if (effectExists(effect)){
		if (current == effects[effect]->getProgram()) {
			//dont enable, just upload data
			if (GLEW_VERSION_2_0) {
				glUniform2f(effects[effect]->uniformLocation(variable.c_str()), value1, value2);
			}
			else {
				glUniform2fARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2);
			}		
			
			printOpenGLError();
		}
		else {
			//enable the program to upload to temporarily, then re-enable the previous program
			if(effects[effect]->enable() != 0)
				return false;
			if (GLEW_VERSION_2_0) {
				glUniform2f(effects[effect]->uniformLocation(variable.c_str()), value1, value2);
			}
			else {
				glUniform2fARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2);
			}	
			glUseProgram(current);
			
			printOpenGLError();
		}	
		return true;
	}
	else {
		return false;
	}
	
}
//----------------------------------------------------------------------------
// Uploads data to the selected effect's uniform
//----------------------------------------------------------------------------
bool ShaderMgr::uploadEffectData(std::string effect, std::string variable, float value1, float value2, float value3){
	//Check to see if program is currently loaded
	GLint current;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current);	
#ifdef DEBUG
	std::cout << "ShaderMgr::uploadEffectData3f() - " << "effect: " << effect << std::endl;
#endif
	
	if (effectExists(effect)){
		if (current == effects[effect]->getProgram()) {
			//dont enable, just upload data
			if (GLEW_VERSION_2_0) {
				glUniform3f(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3);
			}
			else {
				glUniform3fARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3);
			}		
			
			printOpenGLError();
			
		}
		else {
			//enable the program to upload to temporarily, then re-enable the previous program
			if(effects[effect]->enable() != 0 )
				return false;
			if (GLEW_VERSION_2_0) {
				glUniform3f(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3);
			}
			else {
				glUniform3fARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3);
			}	
			glUseProgram(current);
			
			printOpenGLError();
		}	
		return true;
	}
	else {
		return false;
	}
	
}
//----------------------------------------------------------------------------
// Uploads data to the selected effect's uniform
//----------------------------------------------------------------------------
bool ShaderMgr::uploadEffectData(std::string effect, std::string variable, float value1, float value2, float value3, float value4){
	//Check to see if program is currently loaded
	GLint current;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current);	
#ifdef DEBUG
	std::cout << "ShaderMgr::uploadEffectData4f() - " << "effect: " << effect << std::endl;
#endif
	
	if (effectExists(effect)){
		if (current == effects[effect]->getProgram()) {
			//dont enable, just upload data
			if (GLEW_VERSION_2_0) {
				glUniform4f(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3, value4);
			}
			else {
				glUniform4fARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3, value4);
			}		
			
			printOpenGLError();
			
		}
		else {
			//enable the program to upload to temporarily, then re-enable the previous program
			if(effects[effect]->enable() != 0)
				return false;
			if (GLEW_VERSION_2_0) {
				glUniform4f(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3, value4);
			}
			else {
				glUniform4fARB(effects[effect]->uniformLocation(variable.c_str()), value1, value2, value3, value4);
			}	
			glUseProgram(current);
			
			printOpenGLError();
		}	
		return true;
	}
	else {
		return false;
	}
	
}

//----------------------------------------------------------------------------
// Prints out the effect name, followed by shader info
//----------------------------------------------------------------------------
void ShaderMgr::printEffects(){
	std::cout << "Loaded Effects: " << std::endl;
	for (std::map< std::string, ShaderProgram*>::const_iterator iter = effects.begin();
		 iter != effects.end(); ++iter )
		std::cout << '\t' << iter->first << '\t' << iter->second << '\t' << iter->second->getProgram() << '\n';
}
//----------------------------------------------------------------------------
// Returns an effect name, given the shader program it is supposed to control
//----------------------------------------------------------------------------
std::string ShaderMgr::findEffect(GLuint prog){
	for (std::map< std::string, ShaderProgram*>::const_iterator iter = effects.begin();
		 iter != effects.end(); ++iter ){
		if (iter->second->getProgram() == prog)
			return iter->first;
	}
	return std::string ("");	
}
//----------------------------------------------------------------------------
// Check to see if an effect exists, must be done due to std::map autocreating 
// an object that does not exist when checked with []
//----------------------------------------------------------------------------
bool ShaderMgr::effectExists(std::string effect){	
	if (effects.count(effect) > 0) {
		return true;
	}
	else {
		return false;
	}
}
