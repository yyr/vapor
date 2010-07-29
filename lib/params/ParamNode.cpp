//
//      $Id$
//
//************************************************************************
//								*
//		     Copyright (C)  2006			*
//     University Corporation for Atmospheric Research		*
//		     All Rights Reserved			*
//								*
//************************************************************************/
//
//	File:		ParamNode.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Dec  7 15:08:16 MST 2006
//
//	Description:	
//
#include "vapor/ExpatParseMgr.h"
#include <iostream>
#include <cassert>
#include "vapor/Base64.h"
#include "vapor/errorcodes.h"
#include "ParamNode.h"
#include "vapor/ParamsBase.h"

using namespace VAPoR;
using namespace VetsUtil;
const string ParamNode::_typeAttr= "Type";
const string ParamNode::_paramsBaseAttr= "ParamsBase";

ParamNode::ParamNode(
	const string &tag, const map <string, string> &attrs, 
	size_t numChildrenHint
) : XmlNode(tag, attrs, numChildrenHint) {

	_paramsBase = 0;
	SetClassName("ParamNode");
}

ParamNode* ParamNode::deepCopy()
	
{
	ParamNode* copyNode = new ParamNode(*this);
	//If there's a paramsBase, have it clone itself, but not its rootNode
	if(_paramsBase) copyNode->_paramsBase = _paramsBase->deepCopy(copyNode);
	else copyNode->_paramsBase = 0;
	return copyNode;
}
//copy constructor clones itself, calls deepCopy on children
ParamNode::ParamNode(const ParamNode &node) :
	XmlNode()
{
	*this = node;
	this->DeleteAll();
	for (int i=0; i<node.GetNumChildren(); i++) {
		ParamNode *child = node.GetChild(i);
		ParamNode *newchild = child->deepCopy();
		this->AddChild(newchild);
	}
}
ParamNode::~ParamNode() {

}


int ParamNode::SetElementLong(
	const string &tag, const vector<long> &values
) {

	// Store element in base class
	//
	int rc = XmlNode::SetElementLong(tag, values);

	if(rc) return rc;
	// see if a dirty flag is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);
	if (p != _dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(0);
}
	
int ParamNode::SetElementDouble(
	const string &tag, const vector<double> &values
) {

	int rc = XmlNode::SetElementDouble(tag, values);

	if(rc) return rc;
	// see if a dirty flag watcher is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);
	if (p != _dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(0);
}
	
int ParamNode::SetElementString(
	const string &tag, const string &str
){ 

	int rc = XmlNode::SetElementString(tag, str);
	if(rc) return rc;
	// see if a dirty flag watcher is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);
	if (p != _dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(0);
}
int ParamNode::SetElementLong(
	const vector<string> &tagpath, const vector<long> &values
) {
	//Iterate through tags, finding associated node
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return -1;
	}
	string tag = tagpath[tagpath.size()-1];
	// Store element at the last node:
	//
	int rc = currNode->SetElementLong(tag, values);

	if(rc) return rc;
	// see if a dirty flag is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);
	if (p != _dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(0);
}
	
int ParamNode::SetElementDouble(
	const vector<string> &tagpath, const vector<double> &values
) {

	//Iterate through tags, finding associated node
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return -1;
	}
	string tag = tagpath[tagpath.size()-1];
	// Store element at the last node:
	//
	int rc = currNode->SetElementDouble(tag, values);

	if(rc) return rc;
	// see if a dirty flag watcher is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);
	if (p != _dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(0);
}
	
int ParamNode::SetElementString(
	const vector<string> &tagpath, const vector<string> &str
){ 

	//Iterate through tags, finding associated node
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return -1;
	}
	string tag = tagpath[tagpath.size()-1];
	// Store element at the last node:
	//
	int rc = currNode->SetElementString(tag, str);
	if(rc) return rc;
	// see if a dirty flag watcher is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);
	if (p != _dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(0);
}

int ParamNode::SetElementString(
	const string &tag, const vector<string> &str
){ 

	int rc = XmlNode::SetElementString(tag, str);
	if(rc) return rc;
	// see if a dirty flag watcher is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);
	if (p != _dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(0);
}

int ParamNode::SetFlagDirty(const string &tag){
	// see if a dirty flag watcher is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);
	if (p != _dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
		return 0;
	}
	return -1;
}
int ParamNode::SetFlagDirty(const vector<string> &tagpath){
	//Iterate through tags, finding associated node, stop one short
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return -1;
	}
	string tag = tagpath[tagpath.size()-1];
	map <string, vector <DirtyFlag *> >::iterator p =  currNode->_dirtyFlags.find(tag);
	if (p != currNode->_dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
		return 0;
	}
	return -1;
}

int ParamNode::AddNode(const string& tag, ParamNode* child) {
	if (HasChild(tag)) {
		SetErrMsg(VAPOR_ERROR_PARAMS,"Child node named %d already exists", child->Tag().c_str());
		return(-1);
	}
	XmlNode::AddChild(child);
	child->Tag() = tag;
	return(0);
}
int ParamNode::AddNode(const vector<string>& tagpath, ParamNode* child) {
	//Iterate through tags, finding associated node, stop one short
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return -1;
	}
	string tag = tagpath[tagpath.size()-1];

	if (currNode->HasChild(tag)) {
		SetErrMsg(VAPOR_ERROR_PARAMS,"Child node named %d already exists", tag.c_str());
		return(-1);
	}
	XmlNode::AddChild(child);
	child->Tag() = tag;
	return(0);
}

int ParamNode::AddRegisteredNode(const string& tag, ParamNode* child, ParamsBase* associate) {
	if (HasChild(tag)) {
		SetErrMsg(VAPOR_ERROR_PARAMS,"Child node named %d already exists", child->Tag().c_str());
		return(-1);
	}
	XmlNode::AddChild(child);
	assert( child->GetParamsBase() == associate); 
	//child->SetParamsBase(associate);
	child->Tag() = tag;
	map <string, string> attrs = child->Attrs();
	attrs[_typeAttr] = _paramsBaseAttr;
	return(0);
}
int ParamNode::AddRegisteredNode(const vector<string>& tagpath, ParamNode* child, ParamsBase* associate) {
	//Iterate through tags, finding associated node, stop one short
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return -1;
	}
	string tag = tagpath[tagpath.size()-1];

	if (currNode->HasChild(tag)) {
		SetErrMsg(VAPOR_ERROR_PARAMS,"Child node named %d already exists", tag.c_str());
		return(-1);
	}
	XmlNode::AddChild(child);
	child->SetParamsBase(associate);
	child->Tag() = tag;
	map <string, string> attrs = child->Attrs();
	attrs[_typeAttr] = _paramsBaseAttr;
	return(0);
}
int ParamNode::DeleteNode(const string &tag){
		ParamNode* pNode = GetNode(tag);
		if (!pNode) return -1;
		pNode->DeleteAll();
		delete pNode;
		return 0;
 }
int ParamNode::DeleteNode(const vector<string> &tagpath){
		ParamNode* pNode = GetNode(tagpath);
		if (!pNode) return -1;
		pNode->DeleteAll();
		delete pNode;
		return 0;
 }

ParamNode* ParamNode::GetNode(const vector<string>& tagpath){
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return NULL;
	}
	string tag = tagpath[tagpath.size()-1];
	return currNode->GetNode(tag);

}
	
int ParamNode::RegisterDirtyFlag(
	const string &tag, ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);

	if (p != _dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;

		// Make sure flag doesn't already exist;
		// It's OK to register it twice.
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirtyflags[i] == df) return 0;
		}
		dirtyflags.push_back(df);
	}
	else {
		vector <DirtyFlag *> dirtyflags;
		dirtyflags.push_back(df);
		_dirtyFlags[tag] = dirtyflags;
	}
	return 0;
}

int ParamNode::RegisterDirtyFlag(
	const vector<string> &tagpath, ParamNode::DirtyFlag *df
) {
	//Iterate through tags, finding associated node, stop one short
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return -1;
	}
	string tag = tagpath[tagpath.size()-1];

	map <string, vector <DirtyFlag *> >::iterator p =  currNode->_dirtyFlags.find(tag);

	if (p != currNode->_dirtyFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;

		// Make sure flag doesn't already exist;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirtyflags[i] == df) return 0;
		}
		dirtyflags.push_back(df);
	}
	else {
		vector <DirtyFlag *> dirtyflags;
		dirtyflags.push_back(df);
		currNode->_dirtyFlags[tag] = dirtyflags;
	}
	return 0;
}


int ParamNode::UnRegisterDirtyFlag(
	const string &tag, const ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.find(tag);

	if (p == _dirtyFlags.end()) return -1;


	vector <DirtyFlag *> &dirtyflags = p->second;

	// Find the dirty flag if it exists
	//
	for(int i=0; i<dirtyflags.size(); i++) {
		if (dirtyflags[i] == df) {
			dirtyflags.erase(dirtyflags.begin() + i);
			return 0;
		}
	}
	return -1;
}

int ParamNode::UnRegisterDirtyFlag(
	const vector<string> &tagpath, const ParamNode::DirtyFlag *df
) {
	//Iterate through tags, finding associated node, stop one short
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return -1;
	}
	string tag = tagpath[tagpath.size()-1];
	map <string, vector <DirtyFlag *> >::iterator p =  currNode->_dirtyFlags.find(tag);

	if (p == currNode->_dirtyFlags.end()) return -1;


	vector <DirtyFlag *> &dirtyflags = p->second;

	// Find the dirty flag if it exists
	//
	for(int i=0; i<dirtyflags.size(); i++) {
		if (dirtyflags[i] == df) {
			dirtyflags.erase(dirtyflags.begin() + i);
			return 0;
		}
	}
	return -1;
}

void ParamNode::SetAllFlags(bool dirty){
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyFlags.begin();
	
	while (p != _dirtyFlags.end()){

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirty) dirtyflags[i]->Set();
			else dirtyflags[i]->Clear();
		}
		p++;
	}

}
const vector<long> &ParamNode::GetElementLong(const vector<string> &tagpath) const {

	//Iterate through tags, finding associated node
	const ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = (const ParamNode* const)currNode->GetNode(tagpath[i]);
		if (!currNode) return _emptyLongVec;
	}
	string tag = tagpath[tagpath.size()-1];
	map <string, vector<long> >::const_iterator p = currNode->_longmap.find(tag);

	// see if entry for this key (tag) already exists
	//
	if (p == currNode->_longmap.end()) { 
		if (_errOnMissing) {
			SetErrMsg(
				ERR_TNP, "Element tagged \"%s\" does not exist", tag.c_str()
			);
		}
		return(_emptyLongVec);
	}

	//return(_longmap[tag]);
	return(p->second);
}
const vector<double> &ParamNode::GetElementDouble(const vector<string> &tagpath) const {
	//Iterate through tags, finding associated node
	const ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return _emptyDoubleVec;
	}
	string tag = tagpath[tagpath.size()-1];
	map <string, vector<double> >::const_iterator p = currNode->_doublemap.find(tag);

	// see if entry for this key (tag) already exists
	//
	if (p == currNode->_doublemap.end()) { 
		if (_errOnMissing) {
			SetErrMsg(
				ERR_TNP, "Element tagged \"%s\" does not exist", tag.c_str()
			);
		}
		return(_emptyDoubleVec);
	}

	return(p->second);
}
void ParamNode::GetElementStringVec(const vector<string> &tagpath, vector <string> &vec) const {
	//Iterate through tags, finding associated node
	const ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		currNode = currNode->GetNode(tagpath[i]);
		if (!currNode) return;
	}
	string tag = tagpath[tagpath.size()-1];
	string s = currNode->GetElementString(tag);

	StrToWordVec(s, vec);
}
