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
#include "vapor/Base64.h"
#include "vapor/errorcodes.h"
#include "ParamNode.h"

using namespace VAPoR;
using namespace VetsUtil;


ParamNode::ParamNode(
	const string &tag, const map <string, string> &attrs, 
	size_t numChildrenHint
) : XmlNode(tag, attrs, numChildrenHint) {

	SetClassName("ParamNode");
}

ParamNode::ParamNode(
	ParamNode &pn
) : XmlNode(pn) {

	SetClassName("ParamNode");

	_dirtyLongFlags = pn._dirtyLongFlags;
	_dirtyDoubleFlags = pn._dirtyDoubleFlags;
	_dirtyStringFlags = pn._dirtyStringFlags;
	_dirtyNodeFlags = pn._dirtyNodeFlags;
	
}

ParamNode::~ParamNode() {

}


vector<long> &ParamNode::SetElementLong(
	const string &tag, const vector<long> &values
) {

	// Store element in base class
	//
	vector <long> &vec = XmlNode::SetElementLong(tag, values);

	// see if a dirty flag is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyLongFlags.find(tag);
	if (p != _dirtyLongFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(vec);
}
	
vector<double> &ParamNode::SetElementDouble(
	const string &tag, const vector<double> &values
) {

	vector <double> &vec = XmlNode::SetElementDouble(tag, values);

	// see if a dirty flag watcher is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyDoubleFlags.find(tag);
	if (p != _dirtyDoubleFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(vec);
}
	
string &ParamNode::SetElementString(
	const string &tag, const string &str
){ 

	string &vec = XmlNode::SetElementString(tag, str);

	// see if a dirty flag watcher is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyStringFlags.find(tag);
	if (p != _dirtyStringFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return(vec);
}

void ParamNode::SetFlagDirty(const string &tag){
	// see if a dirty flag watcher is registered for this tag. If not
	// do nothing.
	//
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyNodeFlags.find(tag);
	if (p != _dirtyNodeFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			DirtyFlag *df = dirtyflags[i];
			df->Set();
		}
	}
	return;
}

int ParamNode::AddChild(ParamNode* child) {
	if (HasChild(child->Tag())) {
		SetErrMsg(VAPOR_ERROR_PARAMS,"Child named %d already exists", child->Tag().c_str());
		return(-1);
	}
	XmlNode::AddChild(child);
	return(0);
}

	
void ParamNode::RegisterDirtyFlagLong(
	const string &tag, ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyLongFlags.find(tag);

	if (p != _dirtyLongFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;

		// Make sure flag doesn't already exist;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirtyflags[i] == df) return;
		}
		dirtyflags.push_back(df);
	}
	else {
		vector <DirtyFlag *> dirtyflags;
		dirtyflags.push_back(df);
		_dirtyLongFlags[tag] = dirtyflags;
	}
}

void ParamNode::UnRegisterDirtyFlagLong(
	const string &tag, const ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyLongFlags.find(tag);

	if (p == _dirtyLongFlags.end()) return;


	vector <DirtyFlag *> &dirtyflags = p->second;

	// Find the dirty flag if it exists
	//
	for(int i=0; i<dirtyflags.size(); i++) {
		if (dirtyflags[i] == df) {
			dirtyflags.erase(dirtyflags.begin() + i);
			return;
		}
	}
}

void ParamNode::RegisterDirtyFlagDouble(
	const string &tag, ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyDoubleFlags.find(tag);

	if (p != _dirtyDoubleFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;

		// Make sure flag doesn't already exist;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirtyflags[i] == df) return;
		}
		dirtyflags.push_back(df);
	}
	else {
		vector <DirtyFlag *> dirtyflags;
		dirtyflags.push_back(df);
		_dirtyDoubleFlags[tag] = dirtyflags;
	}
}

void ParamNode::UnRegisterDirtyFlagDouble(
	const string &tag, const ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyDoubleFlags.find(tag);

	if (p == _dirtyDoubleFlags.end()) return;


	vector <DirtyFlag *> &dirtyflags = p->second;

	// Find the dirty flag if it exists
	//
	for(int i=0; i<dirtyflags.size(); i++) {
		if (dirtyflags[i] == df) {
			dirtyflags.erase(dirtyflags.begin() + i);
			return;
		}
	}
}

void ParamNode::RegisterDirtyFlagString(
	const string &tag, ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyStringFlags.find(tag);

	if (p != _dirtyStringFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;

		// Make sure flag doesn't already exist;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirtyflags[i] == df) return;
		}
		dirtyflags.push_back(df);
	}
	else {
		vector <DirtyFlag *> dirtyflags;
		dirtyflags.push_back(df);
		_dirtyStringFlags[tag] = dirtyflags;
	}
}

void ParamNode::UnRegisterDirtyFlagString(
	const string &tag, const ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyStringFlags.find(tag);

	if (p == _dirtyStringFlags.end()) return;


	vector <DirtyFlag *> &dirtyflags = p->second;

	// Find the dirty flag if it exists
	//
	for(int i=0; i<dirtyflags.size(); i++) {
		if (dirtyflags[i] == df) {
			dirtyflags.erase(dirtyflags.begin() + i);
			return;
		}
	}
}

	
void ParamNode::RegisterDirtyFlagNode(
	const string &tag, ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyNodeFlags.find(tag);

	if (p != _dirtyNodeFlags.end()) { 

		vector <DirtyFlag *> &dirtyflags = p->second;

		// Make sure flag doesn't already exist;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirtyflags[i] == df) return;
		}
		dirtyflags.push_back(df);
	}
	else {
		vector <DirtyFlag *> dirtyflags;
		dirtyflags.push_back(df);
		_dirtyNodeFlags[tag] = dirtyflags;
	}
}

void ParamNode::UnRegisterDirtyFlagNode(
	const string &tag, const ParamNode::DirtyFlag *df
) {
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyNodeFlags.find(tag);

	if (p == _dirtyNodeFlags.end()) return;


	vector <DirtyFlag *> &dirtyflags = p->second;

	// Find the dirty flag if it exists
	//
	for(int i=0; i<dirtyflags.size(); i++) {
		if (dirtyflags[i] == df) {
			dirtyflags.erase(dirtyflags.begin() + i);
			return;
		}
	}
}

void ParamNode::SetAllFlags(bool dirty){
	map <string, vector <DirtyFlag *> >::iterator p =  _dirtyStringFlags.begin();
	
	while (p != _dirtyStringFlags.end()){

		vector <DirtyFlag *> &dirtyflags = p->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirty) dirtyflags[i]->Set();
			else dirtyflags[i]->Clear();
		}
		p++;
	}
	map <string, vector <DirtyFlag *> >::iterator q =  _dirtyDoubleFlags.begin();
	
	while (q != _dirtyDoubleFlags.end()){

		vector <DirtyFlag *> &dirtyflags = q->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirty) dirtyflags[i]->Set();
			else dirtyflags[i]->Clear();
		}
		q++;
	}
	map <string, vector <DirtyFlag *> >::iterator r =  _dirtyLongFlags.begin();
	
	while (r != _dirtyLongFlags.end()){

		vector <DirtyFlag *> &dirtyflags = r->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirty) dirtyflags[i]->Set();
			else dirtyflags[i]->Clear();
		}
		r++;
	}

	map <string, vector <DirtyFlag *> >::iterator s =  _dirtyNodeFlags.begin();
	
	while (s != _dirtyNodeFlags.end()){

		vector <DirtyFlag *> &dirtyflags = s->second;
		for(int i=0; i<dirtyflags.size(); i++) {
			if (dirty) dirtyflags[i]->Set();
			else dirtyflags[i]->Clear();
		}
		s++;
	}

}
