//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		mousemodeparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2014
//
//	Description:	Implements the MouseModeParams class.
//		This class supports parameters associted with the
//		mouse mode
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif



#include <vector>
#include <string>

#include "params.h"
#include "command.h"

#include <vapor/ParamNode.h>


using namespace VAPoR;
#include "mousemodeparams.h"
#include "vapor/ExtensionClasses.h"
#include "../../apps/vaporgui/images/wheel.xpm"
#include "../../apps/vaporgui/images/cube.xpm"


const std::string MouseModeParams::_shortName = "MouseMode";
const std::string MouseModeParams::_mouseModeTag = "MouseModeTag";
const std::string MouseModeParams::_mouseModeParamsTag = "MouseModeParamsTag";
vector<int> MouseModeParams::manipFromMode;
vector<int> MouseModeParams::paramsFromMode;
map<ParamsBase::ParamsBaseType, int> MouseModeParams::modeFromParams;
vector<string>MouseModeParams::modeName;
map<int,const char* const *> MouseModeParams::modeXPMIcon;

MouseModeParams::MouseModeParams(XmlNode* parent, int winnum): Params(parent, MouseModeParams::_mouseModeParamsTag, winnum){
	restart();
}


MouseModeParams::~MouseModeParams(){
}


//Reset region settings to initial state
void MouseModeParams::
restart(){
	//Set to navigation mode
	SetCurrentMouseMode(navigateMode);
}
//Reinitialize settings, session has changed

void MouseModeParams::
Validate(bool doOverride){
	//Command capturing should be disabled
	assert(!Command::isRecording());
	
	if (doOverride) SetCurrentMouseMode(navigateMode);
	//Otherwise, verify that this is a registered mouse mode
	//else if (GetCurrentMouseMode()
	return;	
	
}

int MouseModeParams::AddMouseMode(const std::string paramsTag, int manipType, const char* name){
	
	ParamsBase::ParamsBaseType pType = ParamsBase::GetTypeFromTag(paramsTag);
	paramsFromMode.push_back(pType);
	manipFromMode.push_back(manipType);
	modeName.push_back(std::string(name));
	int mode = (int)modeName.size()-1;
	//Setup reverse mapping
	modeFromParams[pType] = mode;
	assert(mode == (int)paramsFromMode.size()-1);
	assert(manipFromMode.size() == mode+1);
	return (mode);
}
//Stash the name and icon where the gui can find them
int MouseModeParams::RegisterMouseMode(const std::string paramsTag, int manipType,  const char* name,const char* const xpmIcon[]){
	
	int newMode = AddMouseMode(paramsTag, manipType, name);
	modeXPMIcon[newMode] = xpmIcon;
	return newMode;
}

//! Static method called at startup to register all the built-in mouse modes,
//! by calling RegisterMouseMode() for each built-in mode.
//! Also calls InstallExtensionMouseModes() to register extension modes.
void MouseModeParams::RegisterMouseModes(){
	RegisterMouseMode(Params::_viewpointParamsTag,0,"Navigation", wheel );
	RegisterMouseMode(Params::_regionParamsTag,1, "Region",cube );
	//RegisterMouseMode(Params::_flowParamsTag,1, "Flow rake",rake );
	//RegisterMouseMode(Params::_probeParamsTag,3,"Probe", probe);
	//RegisterMouseMode(Params::_twoDDataParamsTag,2,"2D Data", twoDData);
	//RegisterMouseMode(Params::_twoDImageParamsTag,2, "Image",twoDImage);
	//RegisterMouseMode(IsolineParams::_isolineParamsTag,3,"Isoline", isoline);
	InstallExtensionMouseModes();
}