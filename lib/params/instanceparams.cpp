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
#include "renderparams.h"
#include "command.h"
#include <vapor/ParamNode.h>

using namespace VAPoR;
#include "instanceparams.h"
#include "vizwinparams.h"
#include "command.h"

const std::string InstanceParams::_shortName = "Instances";
const string InstanceParams::_instanceParamsTag = "InstanceParams";
const string InstanceParams::_visualizersTag = "Visualizers";
const string InstanceParams::_renderParamsNodeTag = "RendererParamsNode";
const string InstanceParams::_instanceInfoTag = "InstanceInfo";
const string InstanceParams::_selectionInfoTag = "SelectionInfo";




InstanceParams::InstanceParams(XmlNode* parent, int winnum): BasicParams(parent, InstanceParams::_instanceParamsTag){
	restart();
}


InstanceParams::~InstanceParams(){
}


//Reset settings to initial state
void InstanceParams::
restart(){
	GetRootNode()->DeleteAll();
	ParamNode* rpnode = new ParamNode(_renderParamsNodeTag);
	ParamNode* vnode = new ParamNode(_visualizersTag);
	GetRootNode()->AddChild(vnode);
	GetRootNode()->AddChild(rpnode);

}
//Reinitialize settings, session has changed

void InstanceParams::
Validate(int type){
	//Command capturing should be disabled
	assert(!Command::isRecording());
	bool doOverride = (type == 0);
	if (doOverride){
		GetRootNode()->GetNode(_visualizersTag)->DeleteAll();
		vector<long>viznums = VizWinParams::GetVisualizerNums();
		for (int i = 0;i<viznums.size(); i++)
			addVizWin(viznums[i]);
	}
	return;	
	
}
int InstanceParams::AddInstance(const std::string rendererName, int viznum, RenderParams* p){ 
	
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	
	Command* cmd = Command::CaptureStart(ip, "Add new renderer instance", InstanceParams::UndoRedo);
	
	//Prepare the params...
	p->SetRendererName(rendererName);
	p->SetVizNum(viznum);
	p->SetEnabled(false);
	
	//Need to find the specified visualizer node, 
	//Then create the renderer child node associated with the name (it should not exist)
	//This node's index becomes the selected index, and the instance becomes the current index.
	ParamNode* vizNodes = ip->GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viznum;
	//Obtain the specific visualizer node
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	//Remember the current instance of the new selected renderer type:
	int numInstances = Params::GetNumParamsInstances(p->GetName(), viznum);
	int prevCurrentInstance = -1;
	if (numInstances > 0)
		prevCurrentInstance = Params::GetCurrentParamsInstanceIndex(p->GetName(), viznum);
	//Create a new renderer node
	ParamNode* renderNode = new ParamNode(rendererName);
	int rc = vizNode->AddNode(rendererName, renderNode);
	assert (rc == 0);
	int index = vizNode->GetNumChildren()-1;
	
	Params::AppendParamsInstance(p->GetName(),viznum,p);
	//set the renderparams node to a clone of p's root.
	ParamNode* renderParamsNode = p->GetRootNode()->deepCopy();
	
	ip->setRenderParamsNode(renderParamsNode);
	
	vector<long> selectionInfo;
	//When an instance is added, it is always the selected instance:
	selectionInfo.push_back(index);
	//Remember the previous instance index of the same type as the newly selected instance.
	//This information would be lost because the newly selected instance will become current.
	selectionInfo.push_back(prevCurrentInstance);
	vizNode->SetElementLong(_selectionInfoTag,selectionInfo);
	int instance = Params::GetNumParamsInstances(p->GetName(),viznum)-1;
	assert(instance >= 0);
	//Create a vector of instance information
	vector<long> instInfo;
	instInfo.push_back((long) p->GetParamsBaseTypeId());
	instInfo.push_back((long) instance);
	//When an instance is added, it is always set to be current:
	Params::SetCurrentParamsInstanceIndex(p->GetParamsBaseTypeId(),viznum,instance);
	renderNode->SetElementLong(_instanceInfoTag,instInfo);

	Command::CaptureEnd(cmd, ip);
	return 0;
}

int InstanceParams::RemoveSelectedInstance(int viznum){
	
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	Command* cmd = Command::CaptureStart(ip, "Remove renderer instance",InstanceParams::UndoRedo);

	//Determine what is the currently selected instance:
	RenderParams* p = GetSelectedRenderParams(viznum);
	
	int selectedType = p->GetParamsBaseTypeId();
	if (!p) return -1;  //Error!
	
	//set the params child to p's root.
	ParamNode* renParamsNode = p->GetRootNode()->deepCopy();
	ip->setRenderParamsNode(renParamsNode);
	
	//Need to find the specified visualizer node, 
	//Then find the renderer child node associated with the name (it should already exist)
	
	ParamNode* vizNodes = ip->GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viznum;
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	
	//Change the selected child index as appropriate:
	vector<long> selectionInfo = vizNode->GetElementLong(_selectionInfoTag);
	int currentSelectedChild = selectionInfo[0];
	//Modify the selected instance.  Either use the same child index, or subtract one if not possible.
	if (selectionInfo[0] >= vizNode->GetNumChildren()-1){
		selectionInfo[0]--;
	}
	//Save the current instance of the selected renderer type, which is the instance being removed:
	int prevCurrentInstance = Params::GetCurrentParamsInstanceIndex(p->GetName(), viznum);
	selectionInfo[1] = prevCurrentInstance;
	
	vizNode->SetElementLong(_selectionInfoTag, selectionInfo);
		
	//Remove the selected child node:
	vizNode->DeleteChild(currentSelectedChild);
	//Also remove the params instance.  This will reset the current instance too.
	Params::RemoveParamsInstance(selectedType, viznum, prevCurrentInstance);
	
	//Now make all the instances of the removed type consistent with Params instance tables
	//i.e. consecutively numbered.
	renumberInstances(viznum,selectedType);
	//If the newly selected instance exists, make it the current instance
	if (selectionInfo[0]>=0){
		vector<long> instInfo = vizNode->GetChild(selectionInfo[0])->GetElementLong(_instanceInfoTag);
		Params::SetCurrentParamsInstanceIndex(instInfo[0], viznum, instInfo[1]);
	}
	
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
	vector<long> selectInfo;
	selectInfo.push_back(-1);
	selectInfo.push_back(0);
	vizNode->SetElementLong(_selectionInfoTag,selectInfo);
	return rc;
}

int InstanceParams::RemoveVizWin(int viznum){ 
	
	//Need to find and remove the specified visualizer node, 
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
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


void InstanceParams::UndoRedo(bool isUndo, int /*instance*/, Params* beforeP, Params* afterP){
	//This only handles InstanceParams add, remove, or select.
	int viznum = 0;
	int type = 0;
	//change will be -1 for removal, +1 for add, 0 for selection change
	int change = changeType((InstanceParams*)beforeP, (InstanceParams*)afterP, &viznum, &type);
	ParamNode* pnode = 0;
	//Get the renderParamsNode of the added or removed params
	if (change != 0) pnode = ((InstanceParams*)afterP)->getRenderParamsNode();

	if((change < 0 && isUndo) || (change > 0 && !isUndo)){
		//Undo a remove or redo an add:
		//Make a Params from the added or deleted pnode:
		RenderParams* p = (RenderParams*)Params::CreateDefaultParams(type);
		ParamNode* pn = p->GetRootNode();
		p->SetRootParamNode(pnode);
		pn->SetParamsBase(0);
		delete pn; //Delete it, as it's been replaced
		
		if(!isUndo){  //Redo add
			//For redoing an add, just add again and make it current
			Params::AppendParamsInstance(type,viznum,p);
			Params::SetCurrentParamsInstanceIndex(type,viznum,Params::GetNumParamsInstances(type,viznum)-1);
		} else {
			//if undoing remove, insert and make current the removed one at position that was previously current.
			ParamNode* viznode = afterP->GetRootNode()->GetNode(_visualizersTag)->GetChild(viznum);
			int instanceNum = viznode->GetElementLong(_selectionInfoTag)[1];
			RenderParams::InsertParamsInstance(type, viznum, instanceNum, p);
			Params::SetCurrentParamsInstanceIndex(type, viznum,instanceNum);
			//The previous instance params should already have this inserted at the right place and selected:
			//check it out:
			viznode = beforeP->GetRootNode()->GetNode(_visualizersTag)->GetChild(viznum);
			int prevSelected = viznode->GetElementLong(_selectionInfoTag)[0];
			ParamNode* renNode = viznode->GetChild(prevSelected);
			assert(renNode->GetElementLong(_instanceInfoTag)[1] == instanceNum);
			renumberInstances(viznum,type);
		}
		return;
	} else if (change!= 0) { //undoing add, redoing remove
		if (isUndo){
			//For undo Add, need to remove selected instance and to make current the previously selected instance of that type
			Params::RemoveParamsInstance(type,viznum,Params::GetNumParamsInstances(type,viznum)-1);
			ParamNode* viznode = afterP->GetRootNode()->GetNode(_visualizersTag)->GetChild(viznum);
			int instanceNum = viznode->GetElementLong(_selectionInfoTag)[1];
			if (instanceNum >= 0) Params::SetCurrentParamsInstanceIndex(type, viznum,instanceNum);
		} else {
			//For redo Remove, just remove the prev selected instance again,
			//and make current the next or previous instance
			ParamNode* viznode = beforeP->GetRootNode()->GetNode(_visualizersTag)->GetChild(viznum);
			ParamNode* renNode = viznode->GetChild(viznode->GetElementLong(_selectionInfoTag)[0]);
			int instanceNum = renNode->GetElementLong(_instanceInfoTag)[1];
			Params::RemoveParamsInstance(type,viznum, instanceNum);
			int newSelected = instanceNum;
			if (instanceNum >= Params::GetNumParamsInstances(type,viznum)) instanceNum--;
			Params::SetCurrentParamsInstanceIndex(type,viznum,instanceNum);
		}
		return;
	} else { //undo or redo selection change:
		if (isUndo){
			// need to make current the previously selected instance of the current type
			ParamNode* viznode = afterP->GetRootNode()->GetNode(_visualizersTag)->GetChild(viznum);
			int instanceNum = viznode->GetElementLong(_selectionInfoTag)[1];
			Params::SetCurrentParamsInstanceIndex(type,viznum,instanceNum);
		} else {
			// need to make current the newly selected instance
			ParamNode* viznode = afterP->GetRootNode()->GetNode(_visualizersTag)->GetChild(viznum);
			ParamNode* renNode = viznode->GetChild(viznode->GetElementLong(_selectionInfoTag)[0]);
			int instanceNum = renNode->GetElementLong(_instanceInfoTag)[1];
			Params::SetCurrentParamsInstanceIndex(type,viznum,instanceNum);
		}
		return;
	}
	assert(0); //never get here!
	return;
}
//Selecting a renderer instance has the side-effect of setting the current instance for the renderer type.
//If we undo this operation, need to remember the previous current instance setting
int InstanceParams::setSelectedIndex(int viz, int index){
	
	Command* cmd = Command::CaptureStart(this, "Select a Renderer instance", InstanceParams::UndoRedo);
	//Determine the RenderParams instance that is being selected:
	
	//Need to find the specified visualizer node, 
	//Then create the renderer child node associated with the name (it should not exist)
	//This node's index becomes the selected index, and the instance becomes the current index.
	ParamNode* vizNodes = GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viz;
	//Obtain the specific visualizer node
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	//Find the params type that is being selected:
	int newSelectedType = vizNode->GetChild(index)->GetElementLong(_instanceInfoTag)[0];
	int newSelectedInstance = vizNode->GetChild(index)->GetElementLong(_instanceInfoTag)[1];
	int oldCurrentInstanceIndex = Params::GetCurrentParamsInstanceIndex(newSelectedType,viz);
	
	vector<long> selectionInfo;
	selectionInfo.push_back(index);
	selectionInfo.push_back(oldCurrentInstanceIndex);
	vizNode->SetElementLong(_selectionInfoTag,selectionInfo);
	//Now make this instance current
	Params::SetCurrentParamsInstanceIndex(newSelectedType,viz,newSelectedInstance);
	//Remove any ParamNode
	removeRenderParamsNode();

	Command::CaptureEnd(cmd, this);
	return 0;

}
int InstanceParams::getSelectedIndex(int viz){
	
	//Need to find the specified visualizer node, 
	ParamNode* vizNodes = GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viz;
	//Obtain the specific visualizer node
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	return (vizNode->GetElementLong(_selectionInfoTag)[0]);
}
RenderParams* InstanceParams::GetSelectedRenderParams(int viz){
	
	//Need to find the specified visualizer node, 
	//Then create the renderer child node associated with the name (it should not exist)
	//This node's index becomes the selected index, and the instance becomes the current index.
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	ParamNode* vizNodes = ip->GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viz;
	//Obtain the specific visualizer node
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	int childIndex = vizNode->GetElementLong(_selectionInfoTag)[0];
	ParamNode* renderNode = vizNode->GetChild(childIndex);
	int instanceIndex = renderNode->GetElementLong(_instanceInfoTag)[1];
	int instanceType = renderNode->GetElementLong(_instanceInfoTag)[0];
	return (RenderParams*)Params::GetParamsInstance(instanceType, viz, instanceIndex);
}
int InstanceParams::changeType(InstanceParams* p1, InstanceParams* p2, int* viz, int* type){
	//Determine which viz changed: Look at number of instances for each viz.  If it increases, then it's an add
	//with that viz.  Decreases, it's a remove.  Stays the same, then need to see which viz node has a change in
	//selection, that's the one that changed selection.
	//Returns +1 if add, -1 if remove, 0 if change selection.
	ParamNode* vizNodes1 = p1->GetRootNode()->GetNode(_visualizersTag);
	ParamNode* vizNodes2 = p2->GetRootNode()->GetNode(_visualizersTag);
	//There should be the same number of visualizers before and after the change:
	assert (vizNodes1->GetNumChildren() == vizNodes2->GetNumChildren());
	//Go through the visualizers, look for any change in number of instances:
	for (int i = 0; i< vizNodes1->GetNumChildren(); i++){
		ParamNode* vNode1 = vizNodes1->GetChild(i);
		ParamNode* vNode2 = vizNodes2->GetChild(i);
		//Determine viznum;
		string viztag = vNode1->Tag();
		int vizNum = atoi(viztag.substr(3,4).c_str()); //remove "Viz" from tag
		//When we return, *viz will contain the changed viz index:
		*viz = vizNum;
		
		if (vNode1->GetNumChildren() < vNode2->GetNumChildren()){
			//Find the added renderer type
			ParamNode* renNode = vNode2->GetChild(vNode2->GetNumChildren() - 1);
			*type = renNode->GetElementLong(_instanceInfoTag)[0];
			return 1;
		} else if (vNode1->GetNumChildren() > vNode2->GetNumChildren()){
			//Find the removed renderer type
			int childIndex = vNode1->GetElementLong(_selectionInfoTag)[0];
			*type = vNode1->GetChild(childIndex)->GetElementLong(_instanceInfoTag)[0];
			return -1;
		} else {  //Check if selection changed
			if (vNode1->GetElementLong(_selectionInfoTag)[0] != vNode2->GetElementLong(_selectionInfoTag)[0]){
				//Find the selected renderer type
				int childIndex = vNode2->GetElementLong(_selectionInfoTag)[0];
				*type = vNode2->GetChild(childIndex)->GetElementLong(_instanceInfoTag)[0];
				return 0;
			}
		}
	}
	assert(0); //Error: not add, remove, or select
	return -100;
}
//After a RenderParams instance is removed, or after an instance is inserted (but not added)
//we need to adjust the instance indices in this class to match those in the params class,
//Note that the RenderParams instance nodes should already be matched up, we just need to
//fix the InstanceInfo of the instances to match the Params instance information
//Return 0 if we performed the adjustment as needed
int InstanceParams::renumberInstances(int viznum,int changedType){
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	ParamNode* vizNodes = ip->GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viznum;
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	//Go through all the child nodes.  All the instance indices of this type should be consecutively increasing
	int nextInstIndex = 0;
	for (int i = 0; i< vizNode->GetNumChildren(); i++){
		ParamNode* renNode = vizNode->GetChild(i);
		vector<long> instanceInfo = renNode->GetElementLong(_instanceInfoTag);
		//Only change instances for the type that was inserted or removed:
		if (instanceInfo[0] != changedType) continue;
		instanceInfo[1] = nextInstIndex++;

		//Check we are correct:
		Params* p = Params::GetParamsInstance(instanceInfo[0], viznum, instanceInfo[1]);
		assert(p->GetInstanceIndex() == instanceInfo[1]);
		assert(p->GetParamsBaseTypeId() == instanceInfo[0]);

		renNode->SetElementLong(_instanceInfoTag,instanceInfo);
	}
	return 0;
}
int InstanceParams::GetSelectedInstance(int viz, int* type, int* instance){
	
	ParamNode* vizNode = getVizNode(viz);
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	int childIndex = ip->getSelectedIndex(viz);
	if (childIndex < 0) {
		*type = 0;
		*instance = -1;
		return -1;
	}
	ParamNode* renNode = vizNode->GetChild(childIndex);
	vector<long> instanceInfo = renNode->GetElementLong(_instanceInfoTag);
	*type = instanceInfo[0];
	*instance = instanceInfo[1];
	return 0;
}

RenderParams* InstanceParams::GetRenderParamsInstance(int viz, int renIndex){
	
	ParamNode* viznode = getVizNode(viz);
	ParamNode* renNode = viznode->GetChild(renIndex);
	vector<long> instanceInfo = renNode->GetElementLong(_instanceInfoTag);
	return (RenderParams*)Params::GetParamsInstance(instanceInfo[0],viz,instanceInfo[1]);
}
	
int InstanceParams::GetNumInstances(int viz){
	
	ParamNode* viznode = getVizNode(viz);
	return viznode->GetNumChildren();
}
ParamNode* InstanceParams::getVizNode(int viz){
	InstanceParams* ip = (InstanceParams*)Params::GetParamsInstance(_instanceParamsTag);
	ParamNode* vizNodes = ip->GetRootNode()->GetNode(_visualizersTag);
	ostringstream strm;
	strm << "Viz"<<viz;
	ParamNode* vizNode = vizNodes->GetNode(strm.str());
	return vizNode;
}
ParamNode* InstanceParams::getRenderParamsNode(){
	
	if (GetRootNode()->GetNode(_renderParamsNodeTag)->HasChild(0))
		return GetRootNode()->GetNode(_renderParamsNodeTag)->GetChild(0);
	else return 0;
}
void InstanceParams::setRenderParamsNode(ParamNode* pnode){
	
	ParamNode* pn = getRenderParamsNode();
	if (pn) {
		GetRootNode()->GetNode(_renderParamsNodeTag)->ReplaceChild(pn, pnode);
	} else {
		GetRootNode()->GetNode(_renderParamsNodeTag)->AddChild(pnode);
	}
}