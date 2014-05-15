//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		instanceparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2014
//
//	Description:	Implements the InstanceParams class.
//		This class supports parameters associted with Vapor Visualizers
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif



#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "params.h"
#include "command.h"
#include <vapor/ParamNode.h>

using namespace VAPoR;
#include "instanceparams.h"

const std::string InstanceParams::_shortName = "Instances";
const string InstanceParams::_instanceParamsTag = "InstanceParams";
const string InstanceParams::_visualizersTag = "Visualizers";
const string InstanceParams::_paramNodeTag = "ParamNode";
const string InstanceParams::_currentInstanceTag = "CurrentInstance";
const string InstanceParams::_numInstancesTag = "NumInstances";



InstanceParams::InstanceParams(XmlNode* parent, int winnum): Params(parent, InstanceParams::_instanceParamsTag, winnum){
	restart();
}


InstanceParams::~InstanceParams(){
}


//Reset settings to initial state
void InstanceParams::
restart(){
	GetRootNode()->DeleteAll();
	addVizWin(0);
}
//Reinitialize settings, session has changed

void InstanceParams::
Validate(bool doOverride){
	//Command capturing should be disabled
	assert(!Command::isRecording());
	
	if (doOverride){
		GetRootNode()->DeleteAll();
		addVizWin(0);
	}
	return;	
	
}
int InstanceParams::AddInstance(const std::string name, int viznum, Params* p){ 
	
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	Command* cmd = Command::CaptureStart(p, "Add new renderer instance");
	
	//set the params child to p's root.
	ParamNode* paramsNode = p->GetRootNode()->deepCopy();
	if (ip->GetRootNode()->HasChild(_paramNodeTag)) ip->GetRootNode()->ReplaceNode(_paramNodeTag, paramsNode);
	else ip->GetRootNode()->AddNode(_paramNodeTag, paramsNode);
	//Need to find the specified visualizer node, 
	//Then find the renderer child node associated with the name (create if it does not exist)
	//Then increment the number of instances for that tag.
	ParamNode* vizNodes = ip->GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viznum;
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	ParamNode* renderNode = vizNode->GetNode(name);
	int prevNumInstances = renderNode->GetElementLong(_numInstancesTag)[0];
	renderNode->SetElementLong(_numInstancesTag,prevNumInstances+1);
	
	Command::CaptureEnd(cmd, p);
	return 0;
}
int InstanceParams::getCurrentInstance(const std::string name, int viznum){ 
	
	//Need to find the specified visualizer node, 
	//Then find the renderer child node associated with the name (create if it does not exist)
	//Then increment the number of instances for that tag.
	ParamNode* vizNodes = GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viznum;
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	ParamNode* renderNode = vizNode->GetNode(name);
	return (renderNode->GetElementLong(_currentInstanceTag)[0]);

}
int InstanceParams::setCurrentInstance(const std::string name, int viznum, int instance){ 
	
	Command* cmd = Command::CaptureStart(this, "set current instance");
	
	//Need to find the specified visualizer node, 
	//Then find the renderer child node associated with the name (create if it does not exist)
	//Then modify the current instance for that tag.
	//Also remove the ParamNode since it doesn't change for instance setting
	//set the params child to p's root.
	if (GetRootNode()->HasChild(_paramNodeTag)) GetRootNode()->DeleteNode(_paramNodeTag);
	
	ParamNode* vizNodes = GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm <<"Viz"<<viznum;
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	ParamNode* renderNode = vizNode->GetNode(name);
	int rc = renderNode->SetElementLong(_currentInstanceTag, instance);
	if (!rc) Command::CaptureEnd(cmd, this);
	else delete cmd;
	return rc;
}
int InstanceParams::RemoveInstance(const std::string name, int viznum, int instance){
	
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	Command* cmd = Command::CaptureStart(ip, "Remove renderer instance");
	Params* p = Params::GetParamsInstance(name, viznum, instance);
	
	//set the params child to p's root.
	ParamNode* paramsNode = p->GetRootNode()->deepCopy();
	if (ip->GetRootNode()->HasChild(_paramNodeTag)) ip->GetRootNode()->ReplaceNode(_paramNodeTag, paramsNode);
	else ip->GetRootNode()->AddNode(_paramNodeTag, paramsNode);
	//Need to find the specified visualizer node, 
	//Then find the renderer child node associated with the name (create if it does not exist)
	//Then increment the number of instances for that tag.
	ParamNode* vizNodes = ip->GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viznum;
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	ParamNode* renderNode = vizNode->GetNode(name);
	int prevNumInstances = renderNode->GetElementLong(_numInstancesTag)[0];
	int currentInstance = renderNode->GetElementLong(_currentInstanceTag)[0];
	//Move the current instance down unless that makes it negative.
	if (currentInstance >= instance) 
		currentInstance--;
	if (currentInstance < 0) currentInstance = 0;
		
	renderNode->SetElementLong(_numInstancesTag,prevNumInstances-1);
	renderNode->SetElementLong(_currentInstanceTag,currentInstance);

	Command::CaptureEnd(cmd, ip);
	return 0;
}

int InstanceParams::addVizWin(int viznum){ 
	
	//Need to find the specified visualizer node.   If it already exists, then quit.
	//Then find the renderer child node associated with the name (create if it does not exist)
	//Then increment the number of instances for that tag.
	ParamNode* vizNodes = GetRootNode()->GetNode(_visualizersTag);
	if (!vizNodes) {
		vizNodes = new ParamNode(_visualizersTag,1);
		GetRootNode()->AddNode(_visualizersTag,vizNodes);
	}
	ostringstream strm;
	strm << "Viz"<<viznum;
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	if (vizNode) return 0;
	
	vizNode = new ParamNode(strm.str(),10);
	int rc = vizNodes->AddNode(strm.str(), vizNode);
	if (rc) return rc;
	
	//Add a child for each renderer
	for (int t = 1; t< Params::GetNumParamsClasses(); t++){
		Params* p = Params::GetDefaultParams(t);
		if (!p->isRenderParams()) continue;
		string tag = Params::GetTagFromType(t);
		ParamNode* renderNode = new ParamNode(tag);
		renderNode->SetElementLong(_numInstancesTag,1);
		renderNode->SetElementLong(_currentInstanceTag,0);
		int rc = vizNode->AddNode(tag,renderNode);
		if (rc) return rc;
	}
	return 0;
}

int InstanceParams::RemoveVizWin(int viznum){ 
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	//Need to find and remove the specified visualizer node, 
	ParamNode* vizNodes = ip->GetRootNode()->GetNode(_visualizersTag);
	if (!vizNodes) return -1;
	ostringstream strm;
	strm << "Viz"<<viznum;
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	if (!vizNode){
		return -1;
	}
	vizNodes->DeleteNode(strm.str());
	return 0;
}