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
#include "vizwinparams.h"
#include "command.h"

const std::string InstanceParams::_shortName = "Instances";
const string InstanceParams::_instanceParamsTag = "InstanceParams";
const string InstanceParams::_visualizersTag = "Visualizers";
const string InstanceParams::_paramNodeTag = "InstanceParamNode";
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
}
//Reinitialize settings, session has changed

void InstanceParams::
Validate(bool doOverride){
	//Command capturing should be disabled
	assert(!Command::isRecording());
	
	if (doOverride){
		GetRootNode()->DeleteAll();
		vector<long>viznums = VizWinParams::GetVisualizerNums();
		for (int i = 0;i<viznums.size(); i++)
			addVizWin(viznums[i]);
	}
	return;	
	
}
int InstanceParams::AddInstance(const std::string name, int viznum, Params* p){ 
	
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	Command* cmd = Command::CaptureStart(ip, "Add new renderer instance", InstanceParams::UndoRedo);
	
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
	
	Command::CaptureEnd(cmd, ip);
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
	
	Command* cmd = Command::CaptureStart(this, "set current instance",InstanceParams::UndoRedo);
	
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
	if (!rc) Params::SetCurrentParamsInstanceIndex(GetTypeFromTag(name),viznum, instance);
	if (!rc) Command::CaptureEnd(cmd, this);
	else delete cmd;
	return rc;
}
int InstanceParams::RemoveInstance(const std::string name, int viznum, int instance){
	
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	Command* cmd = Command::CaptureStart(ip, "Remove renderer instance",InstanceParams::UndoRedo);
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
	Params::SetCurrentParamsInstanceIndex(GetTypeFromTag(name),viznum, currentInstance);
	Command::CaptureEnd(cmd, ip);
	return 0;
}

int InstanceParams::addVizWin(int viznum){ 
	
	//Need to find the specified visualizer node.   If it already exists, then quit.
	//Then find the renderer child node associated with the name (create if it does not exist)
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
		renderNode->SetElementLong(_numInstancesTag,0);
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
ParamNode* InstanceParams::getChangingParamNode(){
	if(GetRootNode()->HasChild(_paramNodeTag)) return GetRootNode()->GetNode(_paramNodeTag);
	else return 0;
}
//Static method finds the first instance that differs between two InstanceParams.  This is
//The instance that is being added or deleted, or the instance that is changing
//Returns 0 if no instance change is found.  Returns 1 if the changed instance is in p1, 2 if it's in p2
//Returns 3 if the current instance changes but not the number of instances
int InstanceParams::instanceDiff(InstanceParams* p1, InstanceParams* p2, string& tag, int* instance, int* viz){
	//Iterate through child viz nodes
	
	ParamNode* vizNodes1 = p1->GetRootNode()->GetNode(_visualizersTag);
	ParamNode* vizNodes2 = p2->GetRootNode()->GetNode(_visualizersTag);
	for (int i = 0; i< vizNodes1->GetNumChildren(); i++){
		ParamNode* vizNode1 = vizNodes1->GetChild(i);
		string viztag = vizNode1->Tag();
		ParamNode* vizNode2 = vizNodes2->GetNode(viztag);
		int vizNum = atoi(viztag.substr(3,4).c_str()); //remove "Viz" from tag
		//Iterate through child nodes:
		for (int j = 0; j< vizNode1->GetNumChildren(); j++){
			ParamNode* rparamNode1 = vizNode1->GetChild(j);
			ParamNode* rparamNode2 = vizNode2->GetChild(j);
			int numInst1 = rparamNode1->GetElementLong(_numInstancesTag)[0];
			int numInst2 = rparamNode2->GetElementLong(_numInstancesTag)[0];
			int curInst1 = rparamNode1->GetElementLong(_currentInstanceTag)[0];
			int curInst2 = rparamNode2->GetElementLong(_currentInstanceTag)[0];
			
			if (numInst1 != numInst2){
				tag = rparamNode1->Tag();
				*viz = vizNum;
				int rc;
				if (numInst1 < numInst2) {//Add instance, the new instance is the last one
					*instance = numInst2-1;
					rc = 2;
				} else { //Remove instance, the current instance is being removed...
					*instance = rparamNode1->GetElementLong(_currentInstanceTag)[0];
					rc = 1;
				}
				return rc;
			}
			if (curInst1 != curInst2){
				tag = rparamNode1->Tag();
				*viz = vizNum;
				*instance = -1; //Just indicates it's changing
				return 3;
			}
		}
	}
	return 0;
}

void InstanceParams::UndoRedo(bool isUndo, int /*instance*/, Params* beforeP, Params* afterP){
	//This only handles InstanceParams
	assert (beforeP->GetParamsBaseTypeId() == Params::GetTypeFromTag(InstanceParams::_instanceParamsTag));
	assert (afterP->GetParamsBaseTypeId() == Params::GetTypeFromTag(InstanceParams::_instanceParamsTag));
	//Check for an add or remove, by enumerating all the instances 
	InstanceParams* p1 = (InstanceParams*)beforeP;
	InstanceParams* p2 = (InstanceParams*)afterP;
	ParamNode* pnode = p1->getChangingParamNode();
	if(!pnode) pnode = p2->getChangingParamNode();
	//The correct instance to use is NOT the one passed as an argument to this method...
	int instance, viz;
	string tag;
	int changeType = InstanceParams::instanceDiff(p1, p2, tag, &instance, &viz);
	if(changeType == 3) { //It's not an add or remove change; Just need to set the current instance
		//Note that we obtain the viznum from the instanceDiff traversal, not from the GetVizNum method.
		if (isUndo) {
			instance = p1->getCurrentInstance(tag, viz);
		}
		else {
			instance = p2->getCurrentInstance(tag, viz);
		}
		Params::SetCurrentParamsInstanceIndex(Params::GetTypeFromTag(tag), viz, instance);
		return;
	}
	assert(changeType != 0);
	ParamsBase::ParamsBaseType pType = ParamsBase::GetTypeFromTag(tag);
	//ChangeType is 1 if p1 has the changed instance, 2 if p2 has the changed instance.
	//addInstance has changeType 2, removeInstance has changeType 1.
	//OK, make the undo or redo requested:
	//To Undo an addInstance or redo a removeInstance need to remove the specified instance
	if ((isUndo && (changeType == 2))||(!isUndo &&(changeType == 1))){
		Params::RemoveParamsInstance(pType, viz, instance);
		return;
	}
	//To Redo an addInstance, or Undo a removeInstance, need to insert the specified instance.
	if ((!isUndo && (changeType == 2))||(isUndo &&(changeType == 1))){
		Params* p = Params::CreateDefaultParams(pType);
		ParamNode* pn = p->GetRootNode();
		p->SetRootParamNode(pnode);
		pn->SetParamsBase(0);
		delete pn;
		Params::InsertParamsInstance(pType, viz, instance, p);
		return;
	}
	//We should never get here...
	assert(0);
	return;
}
