/*
 *  ShaderMgr.cpp
 *  
 *
 *  Created by Yannick Polius on 6/6/11.
 *  Copyright 2011 Yannick Polius All rights reserved.
 *
 */

#include "glutil.h"	// Must be included first!!!
#include "ShaderProgram.h"

#include "ShaderMgr.h"

#include <vapor/errorcodes.h>
#include <sstream>
using namespace VAPoR;
using namespace VetsUtil;
ShaderMgr::ShaderMgr()
{
	loaded = false;
	glsl_version = 120; // default
}


ShaderMgr::ShaderMgr(const char* directory)
{
	SetDiagMsg("ShaderMgr::ShaderMgr(%s)", directory);
	loaded = false;
	sourceDir = std::string(directory);
	glsl_version = 120;
}

ShaderMgr::ShaderMgr(std::string directory, QGLWidget *owner)
{
	SetDiagMsg("ShaderMgr::ShaderMgr(%s)", directory.c_str());
	loaded = false;
	sourceDir = directory;
	parent = owner;
	glsl_version = 120;
}

ShaderMgr::~ShaderMgr()
{
#ifdef DEBUG
	std::cout << "ShaderMgr::~ShaderMgr()" << std::endl;
#endif
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
#ifdef DEBUG
	std::cout << "ShaderMgr::loadShaders() " << std::endl;
#endif	
	if (!loaded){
		QDir mainDir = QDir(QString(sourceDir.c_str()));
		mainDir.setFilter( QDir::Files );
		QStringList entries = mainDir.entryList();
		QStringList efcFiles = entries.filter(".efc");
		
		if (efcFiles.size() == 0) {
			SetErrMsg(VAPOR_ERROR_GL_SHADER, "No .efc files found in directory: %s",sourceDir.c_str());
			return false;
		}
		for (int i = 0; i < efcFiles.size(); i++) {
			if(loadEffectFile(efcFiles.at(i).toStdString()) == false){	
				SetErrMsg(VAPOR_ERROR_GL_SHADER, 
											"EFC file \"%s\" failed to load\n", efcFiles.at(i).toStdString().c_str());	
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
#ifdef DEBUG
	std::cout << "ShaderMgr::reloadShaders() " << std::endl;
#endif	
	//Check to see if any shaders are currently running	
	GLint current;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current);
	parent->makeCurrent();
	if (current != 0) {
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "shader is in use, cannot delete anything!"
		);
		return false;
	}
	
	//Delete old shaders
	for (std::map< std::string, ShaderProgram*>::const_iterator iter = effects.begin();
		 iter != effects.end(); iter++ )
	{		
		delete iter->second;
	}
	//Clear the map
	effects.clear();
	baseEffects.clear();
	//Reload shaders
	loaded = false;
	return loadShaders();	
}
//----------------------------------------------------------------------------
// Enable an effect for rendering
//----------------------------------------------------------------------------
bool ShaderMgr::enableEffect(std::string effect)
{
	SetDiagMsg("ShaderMgr::enableEffect(%s)", effect.c_str());
#ifdef DEBUG
	std::cout << "ShaderMgr::enableEffect(" << effect << ")" << std::endl;
#endif	
	if (effectExists(effect)) {
		if(effects[effect]->enable() == 0)
			return true;
		else {
			return false;
		}
		
	}
	else {
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect %s does not exist", effect.c_str()
		);
		return false;
	}
	return true;
}
//----------------------------------------------------------------------------
// Disable the currently loaded effect
//----------------------------------------------------------------------------			
bool ShaderMgr::disableEffect()
{
	SetDiagMsg("ShaderMgr::disableEffect()");
#ifdef DEBUG
	std::cout << "ShaderMgr::disableEffect()" << std::endl;
#endif	
	if (GLEW_VERSION_2_0)
	{
		glUseProgram(0);
	}
	else
	{
		glUseProgramObjectARB(0);
	}
	printOpenGLError();

	return true;
}
void ShaderMgr::setGLSLVersion(int version)
{
	glsl_version = version;
}
//----------------------------------------------------------------------------
// Convert the define string into OGL define directives
//----------------------------------------------------------------------------
std::string ShaderMgr::convertDefines(std::string defines)
{
#ifdef DEBUG
	std::cout << "ShaderMgr::convertDefines()" << std::endl;
#endif	
	int colonIndex = defines.find(";");
	int pos = 0;
	stringstream ss;
	ss << glsl_version;
	std::string result = "#version " + ss.str() + "\n";
	if(colonIndex < 0) return result;
	//last character must be a semi colon
	if (defines[defines.length() -1] != ';') {
		return result;
	}
	while (pos < defines.length() -1 ) {
		//substring the first section up to semi colon
		std::string token = defines.substr(pos, colonIndex - pos);
		std::string definition = "#define " + token + "\n";
		result += definition;
		definition = "";
		pos = colonIndex + 1;
		colonIndex = defines.find(";", pos);
	}
	return result;
}
//----------------------------------------------------------------------------
// Instantiates a shader program based on the defines
// example define: "PI 3.14;TAU 2.1872;MU"
//----------------------------------------------------------------------------
bool ShaderMgr::defineEffect(std::string baseName, std::string defines, std::string instanceName)
{
	SetDiagMsg(
		"ShaderMgr::defineEffect(%s,%s,%s)",
		baseName.c_str(), defines.c_str(), instanceName.c_str()
	);

#ifdef DEBUG		
	std::cout << "ShaderMgr::defineEffect(baseName, defines, intanceName) : " << baseName << " " << defines << " " << instanceName << std::endl;
#endif
	if (baseEffects.count(baseName) < 0) {
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Base effect \"%s\" is not loaded", 
			baseName.c_str()
		);
		return false;
	}
	else {
		//check if another effect is already using the instanceName
		if(effects.count(instanceName) > 0) {
			SetErrMsg(
				VAPOR_ERROR_GL_SHADER, "Effect \"%s\" already in use", 
				instanceName.c_str()
			);
			return false;
		}
		effects[instanceName] = new ShaderProgram();
		effects[instanceName]->create();
		//Convert defines
		std::string OGLdefines = convertDefines(defines);
		//iterate through vertex shader code
		for (int i = 0; i < baseEffects[baseName][0].size(); i++) {
			//add definition tokens to everything
			std::string defined = OGLdefines + baseEffects[baseName][0][i];			
			effects[instanceName]->loadVertexSource((const GLchar*)defined.c_str(), baseEffects[baseName][2][i]);
		}	
		//iterate through fragment shader code
		for (int i = 0; i < baseEffects[baseName][1].size(); i++) {
			//add definition tokens to all source code
			std::string defined = OGLdefines + baseEffects[baseName][1][i];
			effects[instanceName]->loadFragmentSource((const GLchar*)defined.c_str(), baseEffects[baseName][3][i]);
		}
			
	}
	
	if (effects[instanceName]->compile()){
#ifdef DEBUG
		std::cout << "effect " << baseName << " defined " << "name: " << instanceName << std::endl;
#endif
		return true;
	}
	else {
#ifdef DEBUG
		std::cout << "effect " << baseName << " failed compilation " << "name: " << instanceName << std::endl;
#endif
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect \"%s::%s\" failed to compile", 
			baseName.c_str(), instanceName.c_str()
		);
		return false;
	}

}
bool ShaderMgr::undefEffect(std::string instanceName)
{
#ifdef DEBUG		
	std::cout << "ShaderMgr::undefEffect(instanceName) : " << instanceName << std::endl;
#endif
	if (effects.count(instanceName) < 0) {
	  //effect has not been defined
		return false;
	}
	else {
	  delete effects[instanceName];
	  effects.erase(instanceName);
	}	
	return true;
}

//----------------------------------------------------------------------------
// Handles parsing an effect file and constructing the shaderprogram
//----------------------------------------------------------------------------
bool ShaderMgr::loadEffectFile(std::string effect)
{
	SetDiagMsg("ShaderMgr::loadEffectFile(%s)", effect.c_str());
	
#ifdef DEBUG		
	std::cout << "ShaderMgr::loadEffectFile(" << effect << ")" << std::endl;
#endif
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
	
	//--------------------------------------
	// Prepare to parse main shaders
	// Unlike normal GLSL this will load shaders
	// with basic preprocessor support
	// 
	// Currently supported macros:
	//
	// #include
	//	included files must reside in the sourcedir/includes directory
	// #defines are supported programmatically, with the defineEffect function
	//--------------------------------------
	if (!line2.empty()) {
		if(loadVertShader(sourceDir + "/main/" + line2 + ".vgl", line2 + ".vgl", line1 ) == false){
			return false;
		}
	}
	if (!line3.empty()) {
		if(loadFragShader(sourceDir + "/main/" + line3 + ".fgl", line3 + ".fgl", line1) == false){
			return false;
		}
	}
	return true;
	
}
//----------------------------------------------------------------------------
// Loads a vertex shader, both main and included
//----------------------------------------------------------------------------
bool ShaderMgr::loadVertShader(std::string path, std::string fileName, std::string effectName)
{
	SetDiagMsg(
		"ShaderMgr::loadVertShader(%s,,%s)",
		path.c_str(), fileName.c_str()
	);
#ifdef DEBUG		
	std::cout << "ShaderMgr::loadVertShader(" << path << "," << fileName << ")" << std::endl;
#endif	
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
		if(loadVertShader(sourceDir + "/includes/" + includes[i], includes[i], effectName) == false)
			return false;
	}	
	//Save source code
	baseEffects[effectName][0].push_back(processed);
	//Save the vert shader name for later use
	baseEffects[effectName][2].push_back(fileName);
	return true;
}
//----------------------------------------------------------------------------
// Loads a fragment shader, both main and included
//----------------------------------------------------------------------------
bool ShaderMgr::loadFragShader(std::string path, std::string fileName, std::string effectName)
{
	SetDiagMsg(
		"ShaderMgr::loadFragShader(%s,,%s)",
		path.c_str(), fileName.c_str()
	);
	
#ifdef DEBUG		
	std::cout << "ShaderMgr::loadFragShader(" << path << "," << fileName << ")" << std::endl;
#endif	
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
	
	const char *sourceBuffer = (const char*)code.c_str();
	std::string processed = "";
	std::vector<std::string> includes = preprocess(sourceBuffer, processed);
	for (int i = 0; i <  includes.size(); i++){		
		if(loadFragShader(sourceDir + "/includes/" + includes[i], includes[i], effectName) == false)
			return false;
	}
	//Save source code
	baseEffects[effectName][1].push_back(processed);
	//Save the frag shader name for later use
	baseEffects[effectName][3].push_back(fileName);
	
	return true;
	
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
	std::cout << "ShaderMgr::uploadEffectDatai(effect,variable,value) : " << effect << " " << variable << " " << value << endl;
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
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect \"%s\" has not been defined", 
			effect.c_str()
		);
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
	std::cout << "ShaderMgr::uploadEffectDatai(effect,variable,value) : " << effect << " " << variable << " " << value1 << " " << value2 << endl;
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
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect \"%s\" has not been defined", 
			effect.c_str()
		);
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
	std::cout << "ShaderMgr::uploadEffectDatai(effect,variable,value) : " << effect << " " << variable << " " << value1 << " " << value2 << " " << value3 << endl;
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
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect \"%s\" has not been defined", 
			effect.c_str()
		);
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
	std::cout << "ShaderMgr::uploadEffectDatai(effect,variable,value) : " << effect << " " << variable << " " << value1 << " " << value2 << " " << value3 << " " << value4 << endl;
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
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect \"%s\" has not been defined", 
			effect.c_str()
		);
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
	std::cout << "ShaderMgr::uploadEffectDataf(effect,variable,value) : " << effect << " " << variable << " " << value <<endl;
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
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect \"%s\" has not been defined", 
			effect.c_str()
		);
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
	std::cout << "ShaderMgr::uploadEffectDataf(effect,variable,value) : " << effect << " " << variable << " " << value1 << " " << value2 << endl;
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
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect \"%s\" has not been defined", 
			effect.c_str()
		);
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
	std::cout << "ShaderMgr::uploadEffectDataf(effect,variable,value) : " << effect << " " << variable << " " << value1 << " " << value2 << " " << value3 << endl;
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
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect \"%s\" has not been defined", 
			effect.c_str()
		);
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
	std::cout << "ShaderMgr::uploadEffectDataf(effect,variable,value) : " << effect << " " << variable << " " << value1 << " " << value2 << " " << value3 << " " << value4 <<endl;
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
		SetErrMsg(
			VAPOR_ERROR_GL_SHADER, "Effect \"%s\" has not been defined", 
			effect.c_str()
		);
	}
	
}

//----------------------------------------------------------------------------
// Prints out the effect name, followed by shader info
//----------------------------------------------------------------------------
void ShaderMgr::printEffects(){
	std::cout << "Loaded Effects: " << std::endl;
	for (std::map< std::string, ShaderProgram*>::const_iterator iter = effects.begin();
		 iter != effects.end(); ++iter ){
		std::cout << '\t' << iter->first << '\t' << iter->second << '\t' << iter->second->getProgram() << '\n';
		iter->second->printContents();
	}
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

std::string ShaderMgr::GLVendor(){
  const GLubyte* vendor =  glGetString(GL_VENDOR);
  std::string vendorString = vendor ? (const char*) vendor : "";
  return vendorString;
}

std::string ShaderMgr::GLRenderer(){
  const GLubyte* renderer = glGetString(GL_RENDERER);
  std::string rendererString = renderer ? (const char*)renderer : ""; 
  return rendererString;
}

std::string ShaderMgr::GLVersion(){
  const GLubyte* version =  glGetString(GL_VERSION);
  std::string versionString = version ? (const char*) version : "";
  return versionString;
}

std::string ShaderMgr::GLShaderVersion(){
  const GLubyte* glsl =  glGetString(GL_SHADING_LANGUAGE_VERSION);
  std::string glslString = glsl ? (const char*) glsl : "";
  return glslString;
}

std::string ShaderMgr::GLExtensions(){
  const GLubyte* extensions =  glGetString(GL_EXTENSIONS);
  std::string extensionString = extensions ? (const char*) extensions : "";
  return extensionString;
}

bool ShaderMgr::supportsExtension(std::string extension){
  std::string extensions = GLExtensions();
  if (extensions.find(extension)!=std::string::npos)
    return true;
  else 
    return false;
}

bool ShaderMgr::supportsFeatures(std::string features){
  if (glewIsSupported(features.c_str())) return true;
  else return false;
}

GLint* ShaderMgr::getUniformValuei(std::string effect, std::string variable, int count){
#ifdef DEBUG
  std::cout << "ShaderMgr::getUniformValuei() - " << "effect: " << effect << std::endl;
#endif
	
  if (effectExists(effect)){
    GLint location = effects[effect]->uniformLocation(variable.c_str());
    GLint * result = (GLint*)malloc(sizeof(GLint)*count);
    glGetUniformiv(effects[effect]->getProgram(), location, result);
    return result;
  }
  else
    return NULL;
}

GLfloat* ShaderMgr::getUniformValuef(std::string effect, std::string variable, int count){
#ifdef DEBUG
  std::cout << "ShaderMgr::getUniformValuef() - " << "effect: " << effect << std::endl;
#endif
	
  if (effectExists(effect)){
    GLint location = effects[effect]->uniformLocation(variable.c_str());
    GLfloat * result = (GLfloat*)malloc(sizeof(GLfloat)*count);
    glGetUniformfv(effects[effect]->getProgram(), location, result);
    return result;
  }
  else 
    return NULL;
}

int ShaderMgr::maxTexUnits(bool fixed){
  if(fixed){
    GLint result;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &result);
    return (int)(result);
  }
  else{
    GLint result;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &result);
    return (int)(result);
  }
}

bool ShaderMgr::checkFramebufferStatus()
{
  // check FBO status
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
      std::cout << "Framebuffer complete." << std::endl;
      return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      std::cout << "[ERROR] Framebuffer incomplete: Attachment is NOT complete." << std::endl;
      return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      std::cout << "[ERROR] Framebuffer incomplete: No image is attached to FBO." << std::endl;
      return false;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      std::cout << "[ERROR] Framebuffer incomplete: Draw buffer." << std::endl;
      return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      std::cout << "[ERROR] Framebuffer incomplete: Read buffer." << std::endl;
      return false;

    case GL_FRAMEBUFFER_UNSUPPORTED:
      std::cout << "[ERROR] Framebuffer incomplete: Unsupported by FBO implementation." << std::endl;
      return false;

    default:
      std::cout << "[ERROR] Framebuffer incomplete: Unknown error." << std::endl;
      return false;
    }
}
