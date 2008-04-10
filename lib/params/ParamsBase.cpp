//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		ParamsBase.cpp
//
//	Author:		John Clyne with modifications by Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2008
//
//	Description:	Implements the ParamsBase class
//		This is  abstract class for classes that use a ParamNode
//		to support get/set functionality, and to support dirty bits
//		It is a base class for Params, MapperFunction and other classes
//		that retain their state in xml nodes
//
#include "vapor/XmlNode.h"
#include <string>
#include "vapor/MyBase.h"
#include "assert.h"
#include "ParamNode.h"
#include "vapor/ParamsBase.h"

using namespace VAPoR;

ParamsBase::ParamsBase(
	XmlNode *parent, const string &name
) {
	
	
	previousClass = 0;

	assert (!parent || parent->GetNumChildren() == 0);

	map <string, string> attrs;
	_rootParamNode = new ParamNode(name, attrs);
	_currentParamNode = _rootParamNode;
	if(parent) parent->AddChild(_rootParamNode);

	_parseDepth = 0;
}


ParamsBase::~ParamsBase() {
	
	_rootParamNode = NULL;
	_currentParamNode = NULL;
}

void ParamsBase::SetParent(
	XmlNode *parent
) {
	assert (parent->GetNumChildren() == 1);//??
	XmlNode *child = parent->GetChild(0);
	assert(child->getClassName().compare("ParamNode") == 0);//??
	this->_rootParamNode = (ParamNode *) child;
	this->_currentParamNode = _rootParamNode;
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
		restart(); //Required to ensure that all base tags get defined to default
		if (StrCmpNoCase(tag, _rootParamNode->Tag()) != 0) {
			pm->parseError("Invalid root tag");
			return(false);
		}
	}
	_parseDepth++;

	// Create a child node if this is
	// not the root node, and if it does not have a type attribute.
	//
	if ((StrCmpNoCase(tag, _rootParamNode->Tag()) != 0) && 
		(!(*attrs) || StrCmpNoCase(*attrs, _typeAttr) != 0)) {
		ParamNode* childNode = Push(tag);
		return (childNode != 0);
	}
	
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

	//Call pop() if it was a child node
	if (! state->has_data) {
		(void) Pop();
	}

	if (_parseDepth == 0){
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	}
	return true;
	
}

//Default Push() creates a child node, that can then have its own data.
ParamNode *ParamsBase::Push(
	string& name
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

	if (parent){
		// Delete current root node
		int nchildren = parent->GetNumChildren();
		for (int i=0; i<nchildren; i++) {
			parent->DeleteChild((int)0);	// Should only be one childe
		}
	}
	// Create a new root node
	//
	map <string, string> attrs;
	_rootParamNode = new ParamNode(name, attrs);
	_currentParamNode = _rootParamNode;
	if(parent) parent->AddChild(_rootParamNode);

}
void ParamsBase::SetFlagDirty(const string& flag)
{
	_rootParamNode->SetFlagDirty(flag);
}
	
