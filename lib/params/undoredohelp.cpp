//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		undoredohelp.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2014
//
//	Description:	Implements UndoRedoHelp class plus a few subclasses
//
#include "undoredohelp.h"
#include "params.h"
#include "vapor/ParamsBase.h"
#include "instanceparams.h"

#include "assert.h"

using namespace VAPoR;

//Statics
vector<UndoRedoHelp*> UndoRedoHelp::undoRedoHelpQueue;

bool UndoRedoInstanceHelp::UndoRedo(bool isUndo, Params* beforeP, Params* afterP){
	//This only handles InstanceParams
	if (beforeP->GetParamsBaseTypeId() != Params::GetTypeFromTag(InstanceParams::_instanceParamsTag)) return false;
	if (afterP->GetParamsBaseTypeId() != Params::GetTypeFromTag(InstanceParams::_instanceParamsTag)) return false;
	//Check for an add or remove, by enumerating all the instances 
	InstanceParams* p1 = (InstanceParams*)beforeP;
	InstanceParams* p2 = (InstanceParams*)afterP;
	ParamNode* pnode = p1->getChangingParamNode();
	if(!pnode) pnode = p2->getChangingParamNode();
	if(!pnode) return true; //It's not an add or remove change
	string tag;
	int instance, viz;
	int changeType = InstanceParams::instanceDiff(p1, p2, tag, &instance, &viz);
	if (changeType == 0 ) return true;
	ParamsBase::ParamsBaseType pType = ParamsBase::GetTypeFromTag(tag);
	//ChangeType is 1 if p1 has the changed instance, 2 if p2 has the changed instance.
	//addInstance has changeType 2, removeInstance has changeType 1.
	//OK, make the undo or redo requested:
	//To Undo an addInstance or redo a removeInstance need to remove the specified instance
	if ((isUndo && (changeType == 2))||(!isUndo &&(changeType == 1))){
		Params::RemoveParamsInstance(pType, viz, instance);
		return true;
	}
	//To Redo an addInstance, or Undo a removeInstance, need to insert the specified instance.
	if ((!isUndo && (changeType == 2))||(isUndo &&(changeType == 1))){
		Params* p = Params::CreateDefaultParams(pType);
		ParamNode* pn = p->GetRootNode();
		p->SetRootParamNode(pnode);
		pn->SetParamsBase(0);
		delete pn;
		Params::InsertParamsInstance(pType, viz, instance, p);
		return true;
	}
	//We should never get here...
	assert(0);
	return false;
}