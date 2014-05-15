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

#include "assert.h"

using namespace VAPoR;

//Statics
vector<UndoRedoHelp*> UndoRedoHelp::undoRedoHelpQueue;

bool UndoRedoInstanceHelp::UndoRedo(Params* beforeP, Params* afterP){
	if (beforeP && afterP) return false; //At least one must be null.
	if (!afterP){ //Need to undo the deletion of beforeP, or redo the creation of beforeP
		assert(beforeP->isRenderParams());
		int instance = beforeP->GetInstanceIndex();
		assert(instance >=0);
		int viz = beforeP->GetVizNum();
		assert(viz >= 0);
		string tag = beforeP->GetName();
		Params::ParamsBaseType t = ParamsBase::GetTypeFromTag(tag);
		Params::InsertParamsInstance(t, viz, instance, beforeP->deepCopy());
	} else { // afterP is non-null. Need to redo the deletion of beforeP or undo the creation of beforeP
		assert(afterP->isRenderParams());
		int instance = afterP->GetInstanceIndex();
		assert(instance >=0);
		int viz = afterP->GetVizNum();
		assert(viz >= 0);
		string tag = afterP->GetName();
		Params::ParamsBaseType t = ParamsBase::GetTypeFromTag(tag);
		Params::RemoveParamsInstance(t, viz, instance);
	}
	return true;
}