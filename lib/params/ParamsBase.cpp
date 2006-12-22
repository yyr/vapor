//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2006	                        *
//          University Corporation for Atmospheric Research             *
//                      All Rights Reserved                             *
//                                                                      *
//***********************************************************************
//
//	File:		ParamsBase.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Tue Dec 5 14:23:51 MST 2006
//
//	Description:	
//
//

#include "ParamsBase.h"
#include "assert.h"

using namespace VetsUtil;
using namespace VAPoR;

ParamsBase::ParamsBase(
	XmlNode *parent, const string &name
) {
	assert (parent->GetNumChildren() == 0);

	map <string, string> attrs;
	_rootParamNode = new ParamNode(name, attrs);
	_currentParamNode = _rootParamNode;
	parent->AddChild(_rootParamNode);

	_parseDepth = 0;
}


ParamsBase::~ParamsBase() {

	_rootParamNode = NULL;
	_currentParamNode = NULL;
}

void ParamsBase::SetParent(
	XmlNode *parent
) {
	assert (parent->GetNumChildren() == 1);

	XmlNode *child = parent->GetChild(0);
	assert(child->GetClassName().compare("ParamNode") == 0);
	_rootParamNode = (ParamNode *) child;
	_currentParamNode = _rootParamNode;
}

bool ParamsBase::elementStartHandler(
        ExpatParseMgr* pm, int depth, string& tag, const char ** attrs
) {
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	state->user_defined = 0;

	if (_parseDepth == 0) {
		// If this is the root of the parameter tree clear the
		// child nodes. N.B. can't use 'depth' argument because
		// we might don't know the ancestory of the parameter 
		// tree root node.
		Clear();

		if (StrCmpNoCase(tag, _rootParamNode->Tag()) != 0) {
			pm->parseError("Invalid root tag");
			return(false);
		}
	}
	_parseDepth++;

	if (*attrs) {
		if (StrCmpNoCase(*attrs, _typeAttr) != 0) {
			pm->parseError("Invalid attribute : %s", *attrs);
			return(false);
		}
		attrs++;

		state->data_type = *attrs;
		attrs++;

		state->has_data = 1;
	}

    if (*attrs) {
        pm->parseError("Too many attributes");
        return(false);
    }

	// Create a new child node if not a data node and if this is
	// not the root node
	//
	if (! state->has_data && (StrCmpNoCase(tag, _rootParamNode->Tag()) != 0)) {
		(void) Push(tag);
	}

	return (true);
}

bool ParamsBase::elementEndHandler(ExpatParseMgr* pm, int depth, string& tag) {

	ExpatStackElement *state = pm->getStateStackTop();

	_parseDepth--;


	if (StrCmpNoCase(state->data_type, _longType) == 0) {
		const vector <long> &lvec = pm->getLongData();
		_currentParamNode->SetElementLong(tag, lvec);
	}

	if (StrCmpNoCase(state->data_type, _doubleType) == 0) {
		const vector <double> &dvec = pm->getDoubleData();
		_currentParamNode->SetElementDouble(tag, dvec);
	}

	if (StrCmpNoCase(state->data_type, _stringType) == 0) {
		const string &strdata = pm->getStringData();
		_currentParamNode->SetElementString(tag, strdata);
	}


	if (! state->has_data) {
		(void) Pop();
	}

	
	return (true);
}

ParamNode *ParamsBase::Push(
	const string &name
) {


	//
	// See if child node already exists
	//
	for (size_t i=0; i<_currentParamNode->GetNumChildren(); i++) {
		ParamNode *child = _currentParamNode->GetChild(i);
		if (child->Tag().compare(name) == 0) {
			_currentParamNode = child;
			return(_currentParamNode);	// We're done
		}
	}

	map <string, string> attrs;
	ParamNode *child = new ParamNode(name, attrs);
	(void) _currentParamNode->AddChild(child);
	_currentParamNode = child;
	return(_currentParamNode);
}

ParamNode *ParamsBase::Pop() {
	if (_currentParamNode->GetParent()) {
		_currentParamNode = (ParamNode *) _currentParamNode->GetParent();
	}
	return(_currentParamNode);
}



const map <string, string> &ParamsBase::GetAttributes() {
	return(_currentParamNode->Attrs());
}

void ParamsBase::Clear() {
	// Save the parent and the root node's name
	//
	XmlNode *parent = _rootParamNode->GetParent();
	string name = _rootParamNode->Tag();

	// Delete current root node
	int nchildren = parent->GetNumChildren();
	for (int i=0; i<nchildren; i++) {
		parent->DeleteChild(0);	// Should only be one childe
	}

	// Create a new root node
	//
	map <string, string> attrs;
	_rootParamNode = new ParamNode(name, attrs);
	_currentParamNode = _rootParamNode;
	parent->AddChild(_rootParamNode);

}
 

