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
#include <iostream>
#include <cassert>
#include <vapor/Base64.h>
#include <vapor/errorcodes.h>
#include <vapor/ParamNode.h>
#include <vapor/ParamsBase.h>

using namespace VAPoR;
using namespace VetsUtil;
const string ParamNode::_typeAttr= "Type";
const string ParamNode::_paramsBaseAttr= "ParamsBase";
const string ParamNode::_paramNodeAttr= "ParamNode";

ParamNode::ParamNode(
	const string &tag, const map <string, string> &attrs, 
	size_t numChildrenHint
) : XmlNode(tag, attrs, numChildrenHint) {

	_paramsBase = 0;
	SetClassName("ParamNode");
}
ParamNode::ParamNode(
	const string &tag, 
	size_t numChildrenHint
	) : XmlNode(tag, numChildrenHint){
	_paramsBase = 0;
	pair<string,string> mapval(_typeAttr,_paramNodeAttr);
	Attrs().insert(mapval);
}

//Clone all children, and have them clone their paramsBases.
//Don't touch this' ParamsBase
ParamNode* ParamNode::deepCopy()
{
	ParamNode* copyNode = ShallowCopy();
	for (int i=0; i<GetNumChildren(); i++) {
		ParamNode *child = GetChild(i);
		
		ParamNode *newchild = child->deepCopy();
		if (child->GetParamsBase()){
			newchild->SetParamsBase(child->GetParamsBase()->deepCopy(newchild));
			newchild->GetParamsBase()->SetRootParamNode(newchild);
		}
		else newchild->SetParamsBase(0);
		copyNode->AddChild(newchild);
	}
	return copyNode;
}
//copy constructor clones itself, calls deepCopy on children
ParamNode::ParamNode(const ParamNode &node) :
	XmlNode()
{
	*this = node;
	this->ClearChildren();
	for (int i=0; i<node.GetNumChildren(); i++) {
		ParamNode *child = node.GetChild(i);
		ParamNode *newchild = child->deepCopy();
		this->AddChild(newchild);
	}
}
ParamNode* ParamNode::ShallowCopy(){
	ParamNode* newNode = new ParamNode(Tag(),Attrs());
	newNode->_longmap = _longmap;
	newNode->_doublemap = _doublemap;
	newNode->_stringmap = _stringmap;
	newNode->_paramsBase = 0;
	return newNode;
}
//node copy constructor clones ParamNode structure, calls buildNode to construct ParamNodes associated with ParamBase 
ParamNode* ParamNode::NodeCopy()
{
	ParamNode* newNode = ShallowCopy();
	for (int i=0; i<GetNumChildren(); i++) {
		ParamNode *child = GetChild(i);
		ParamNode *newChild;
		if (child->GetParamsBase())
			newChild = child->GetParamsBase()->buildNode();
		else newChild = child->NodeCopy();
		newNode->AddChild(newChild);
	}
	return newNode;
}
ParamNode::~ParamNode() {
	if (_paramsBase) 
		delete _paramsBase;
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
	
int ParamNode::SetElementLong(
	const string &tag, long value
) {

	longvec.clear();
	longvec.push_back(value);
	return SetElementLong(tag, longvec);
}
int ParamNode::SetElementDouble(
	const string &tag, double value
) {

	doublevec.clear();
	doublevec.push_back(value);
	return SetElementDouble(tag, doublevec);
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
	
int ParamNode::SetElementStringVec(
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
	int rc = currNode->SetElementStringVec(tag, str);
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

int ParamNode::SetElementStringVec(
	const string &tag, const vector<string> &str
){ 

	int rc = XmlNode::SetElementStringVec(tag, str);
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

int ParamNode::SetElementStringVec(
	const string &tag, const vector<string> &str, string blankSub
){ 
	//Go through the vector, replacing blanks with blankSub.
	//If the string is of length 0, replace the string with blankSub
	vector<string> subString;
	for (int i = 0; i<str.size(); i++){
		string newString = str[i];
		if (newString.size() == 0) newString = blankSub;
		else for (int j = 0; j<str.size(); j++){
			//replace all the blanks with blankSub
			size_t found = newString.find(" ");
			if (found == string::npos) break;
			newString.replace(found, 1, blankSub);
		}
		subString.push_back(newString);
	}
	return SetElementStringVec(tag,subString);
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
		SetErrMsg(VAPOR_ERROR_PARAMS,"Child node named %s already exists", child->Tag().c_str());
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
		SetErrMsg(VAPOR_ERROR_PARAMS,"Child node named %s already exists", tag.c_str());
		return(-1);
	}
	XmlNode::AddChild(child);
	child->Tag() = tag;
	return(0);
}

int ParamNode::AddRegisteredNode(const string& tag, ParamNode* child, ParamsBase* associate) {
	if (HasChild(tag)) {
		SetErrMsg(VAPOR_ERROR_PARAMS,"Child node named %s already exists", child->Tag().c_str());
		return(-1);
	}
	XmlNode::AddChild(child);
	//Make the child and the associate point to each other:
	child->SetParamsBase(associate);
	associate->SetRootParamNode(child);
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
		SetErrMsg(VAPOR_ERROR_PARAMS,"Child node named %s already exists", tag.c_str());
		return(-1);
	}
	XmlNode::AddChild(child);
	//Make the child and the associate point to each other:
	child->SetParamsBase(associate);
	associate->SetRootParamNode(child);
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
int ParamNode::ReplaceNode(const vector<string> &tagpath, ParamNode* newNode){
		ParamNode* pNode = GetNode(tagpath);
		if (!pNode) return -1;
		XmlNode* parent = pNode->GetParent();
		return parent->ReplaceChild(pNode, newNode);
 }
int ParamNode::ReplaceNode(const string &tag, ParamNode* newNode){
		ParamNode* pNode = GetNode(tag);
		if (!pNode) return -1;
		return ReplaceChild(pNode, newNode);
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
const vector<long> &ParamNode::GetElementLong(const vector<string> &tagpath, const vector<long>& defaultVal) {

	//Iterate through tags, finding associated node
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		ParamNode* childNode = currNode->GetNode(tagpath[i]);
		if (!childNode) { //add the node:
			childNode = new ParamNode(tagpath[i]);
			currNode->AddNode(tagpath[i],childNode);
		}
		currNode = childNode;
	}
	string tag = tagpath[tagpath.size()-1];
	map <string, vector<long> >::const_iterator p = currNode->_longmap.find(tag);

	// see if entry for this key (tag) already exists
	//
	if (p == currNode->_longmap.end()) { 
		if (_errOnMissing) {
			SetErrMsg(
				VAPOR_ERROR_PARSING, "Element tagged \"%s\" does not exist", tag.c_str()
			);
		}
		currNode->SetElementLong(tag, defaultVal);
		return(defaultVal);
	}

	return(p->second);
}
const vector<double> &ParamNode::GetElementDouble(const vector<string> &tagpath, const vector<double>& defaultVal) {
	//Iterate through tags, finding associated node
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		ParamNode* childNode = currNode->GetNode(tagpath[i]);
		if (!childNode) { //add the node:
			childNode = new ParamNode(tagpath[i]);
			currNode->AddNode(tagpath[i],childNode);
		}
		currNode = childNode;
	}
	string tag = tagpath[tagpath.size()-1];
	map <string, vector<double> >::const_iterator p = currNode->_doublemap.find(tag);
	
	// see if entry for this key (tag) already exists
	//
	if (p == currNode->_doublemap.end()) { 
		if (_errOnMissing) {
			SetErrMsg(
					  VAPOR_ERROR_PARSING, "Element tagged \"%s\" does not exist", tag.c_str()
					  );
		}
		currNode->SetElementDouble(tag, defaultVal);
		return(defaultVal);
	}
	
	return(p->second);
}
const vector<double> &ParamNode::GetElementDouble(const string &tag, const vector<double>& defaultVal) {
	if (HasElementDouble(tag)) return XmlNode::GetElementDouble(tag);
	SetElementDouble(tag,defaultVal);
	return defaultVal;
}
const vector<long> &ParamNode::GetElementLong(const string &tag, const vector<long>& defaultVal) {
	if (HasElementLong(tag)) return XmlNode::GetElementLong(tag);
	SetElementLong(tag,defaultVal);
	return defaultVal;
}
const string &ParamNode::GetElementString(const string &tag, const string& defaultVal){
	if (!HasElementString(tag)) 
		SetElementString(tag,defaultVal);
	return XmlNode::GetElementString(tag);
}

void ParamNode::GetElementStringVec(const vector<string> &tagpath, vector <string> &vec, const vector<string>& defaultVal)  {
	//Iterate through tags, finding associated node
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		ParamNode* childNode = currNode->GetNode(tagpath[i]);
		if (!childNode) { //add the node:
			childNode = new ParamNode(tagpath[i]);
			currNode->AddNode(tagpath[i],childNode);
		}
		currNode = childNode;
	}
	string tag = tagpath[tagpath.size()-1];
	map <string, string >::const_iterator p = currNode->_stringmap.find(tag);
	
	// see if entry for this key (tag) already exists
	//
	if (p == currNode->_stringmap.end()) { 
		if (_errOnMissing) {
			SetErrMsg(
					  VAPOR_ERROR_PARSING, "Element tagged \"%s\" does not exist", tag.c_str()
					  );
		}
		vec = defaultVal;
		currNode->SetElementStringVec(tag, vec);
		return;
	}
	string s = currNode->GetElementString(tag);
	StrToWordVec(s, vec);
}

void ParamNode::GetElementStringVec(const string &tag, vector <string> &vec, const vector<string>& defaultVal)  {
	ParamNode* currNode = this;
	if (!currNode->HasElementString(tag)){
		vec = defaultVal;
		currNode->SetElementStringVec(tag,defaultVal);
		return;
	}
	string s = currNode->GetElementString(tag);
	StrToWordVec(s, vec);
}
void ParamNode::GetElementStringVec(const string &tag, vector <string> &vec, const string blankSub)  {
	ParamNode* currNode = this;
	if (!currNode->HasElementString(tag)){
		return;
	}
	string s = currNode->GetElementString(tag);
	StrToWordVec(s, vec);
	for (int i = 0; i<vec.size(); i++){
		string newString = vec[i];
		for (int j = 0; j<vec[i].size(); j++){ //prevent infinite looping in case of invalid substitution
			size_t found = newString.find(blankSub);
			if (found == string::npos) break; // not found
			newString.replace(found,blankSub.length()," ");
		}
		vec[i] = newString;
	}

}
const string& ParamNode::GetElementString(const vector<string> &tagpath, const string& defaultVal) {
	//Iterate through tags, finding associated node
	ParamNode* currNode = this;
	for (int i = 0; i< tagpath.size()-1; i++){
		ParamNode* childNode = currNode->GetNode(tagpath[i]);
		if (!childNode) { //add the node:
			childNode = new ParamNode(tagpath[i]);
			currNode->AddNode(tagpath[i],childNode);
		}
		currNode = childNode;
	}
	string tag = tagpath[tagpath.size()-1];
	map <string, string >::const_iterator p = currNode->_stringmap.find(tag);
	
	// see if entry for this key (tag) already exists
	//
	if (p == currNode->_stringmap.end()) { 
		if (_errOnMissing) {
			SetErrMsg(
					  VAPOR_ERROR_PARSING, "Element tagged \"%s\" does not exist", tag.c_str()
					  );
		}
		
		currNode->SetElementString(tag, defaultVal);
		return defaultVal;
	}
	return currNode->GetElementString(tag);
}

