//
//      $Id$
//
//************************************************************************
//								*
//		     Copyright (C)  2004			*
//     University Corporation for Atmospheric Research		*
//		     All Rights Reserved			*
//								*
//************************************************************************/
//
//	File:		XmlNode.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Sep 30 14:06:17 MDT 2004
//
//	Description:	
//
#include <iostream>
#include <cassert>
#include "vapor/XmlNode.h"

using namespace VAPoR;
using namespace VetsUtil;


XmlNode::XmlNode(
	const string &tag, const map <string, string> &attrs, 
	size_t numChildrenHint
) {
	_objInitialized = 0;

	SetClassName("XmlNode");

	_tag = tag;
	_attrmap = attrs;

	_emptyLongVec.clear();
	_emptyDoubleVec.clear();
	_emptyString.clear();

	if (numChildrenHint) _children.reserve(numChildrenHint);

	_objInitialized = 1;
}

XmlNode::~XmlNode() {

	if (! _objInitialized) return;

	map <string, vector<long>*>::iterator plong;
	map <string, vector<double>*>::iterator pdouble;
	int	i;

	for (plong = _longmap.begin(); plong != _longmap.end(); plong++) {
		vector<long> *vptr = plong->second;
		delete vptr;
	}

	for (pdouble = _doublemap.begin(); pdouble != _doublemap.end(); pdouble++) {
		vector<double> *vptr = pdouble->second;
		delete vptr;
	}

	for (i=0; i<(int)_children.size(); i++) {
		delete _children[i];
	}

	_objInitialized = 0;
}


int XmlNode::SetElementLong(const string &tag, const vector<long> &values) {

	vector<long> *vptr;

	map <string, vector<long>*>::iterator p = _longmap.find(tag);
	int	i;

	// see if entry for this key (tag) already exists
	//
	if (p != _longmap.end()) { 
		vptr = p->second;
		vptr->resize(values.size());
	}
	else {
		vptr = new vector<long>(values.size());
		_longmap[tag] = vptr;
	}

	vector<long> &v = *vptr;

	for(i=0; i<(int)values.size(); i++) {
		v[i] = values[i];
	}

	return(0);
}
	
const vector<long> &XmlNode::GetElementLong(const string &tag) const {

	map <string, vector<long>*>::const_iterator p = _longmap.find(tag);

	// see if entry for this key (tag) already exists
	//
	if (p == _longmap.end()) { 
		SetErrMsg(ERR_TNP, "Element tagged \"%s\" does not exist", tag.c_str());
		return(_emptyLongVec);
	}

	const vector<long> *vptr = p->second;
	return(*vptr);
}

int XmlNode::HasElementLong(const string &tag) const {
	map <string, vector<long>*>::const_iterator p = _longmap.find(tag);
	return(p != _longmap.end());
}

int XmlNode::SetElementDouble(const string &tag, const vector<double> &values) {

	vector<double> *vptr;

	map <string, vector<double>*>::iterator p = _doublemap.find(tag);
	int	i;

	// see if entry for this key (tag) already exists
	//
	if (p != _doublemap.end()) { 
		vptr = p->second;
		vptr->resize(values.size());
	}
	else {
		vptr = new vector<double>(values.size());
		_doublemap[tag] = vptr;
	}

	vector<double> &v = *vptr;

	for(i=0; i<(int)values.size(); i++) {
		v[i] = values[i];
	}

	return(0);
}
	
const vector<double> &XmlNode::GetElementDouble(const string &tag) const {

	map <string, vector<double>*>::const_iterator p = _doublemap.find(tag);

	// see if entry for this key (tag) already exists
	//
	if (p == _doublemap.end()) { 
		SetErrMsg(ERR_TNP, "Element tagged \"%s\" does not exist", tag.c_str());
		return(_emptyDoubleVec);
	}

	vector<double> *vptr = p->second;

	return(*vptr);
}

int XmlNode::HasElementDouble(const string &tag) const {
	map <string, vector<double>*>::const_iterator p = _doublemap.find(tag);
	return(p != _doublemap.end());
}

int XmlNode::SetElementString(const string &tag, const string &str) {


	map <string, string>::iterator p = _stringmap.find(tag);
	string s = str;
	StrRmWhiteSpace(s);

	// see if entry for this key (tag) already exists
	//
	if (p != _stringmap.end()) { 
		p->second = str;
	}
	else {
		_stringmap[tag] = str;
	}

	return(0);
}
	
const string &XmlNode::GetElementString(const string &tag) const {

	map <string, string>::const_iterator p = _stringmap.find(tag);

	// see if entry for this key (tag) already exists
	//
	if (p == _stringmap.end()) { 
		SetErrMsg(ERR_TNP, "Element tagged \"%s\" does not exist", tag.c_str());
		return(_emptyString);
	}


	return(p->second);
}

int XmlNode::HasElementString(const string &tag) const {
	map <string, string>::const_iterator p = _stringmap.find(tag);
	return(p != _stringmap.end());
}

XmlNode	*XmlNode::NewChild(
	const string &tag, const map <string, string> &attrs, 
	size_t numChildrenHint
) {

	XmlNode	*node = new XmlNode(tag, attrs, numChildrenHint);
	// need to check for error
	assert(node != NULL);

//	_children[_children.size()] = node;	// Doesn't change size!!!! 
	_children.push_back(node);
	return(node);
}

void	XmlNode::AddChild( XmlNode* child)
{
	_children.push_back(child);
	return;
}


int	XmlNode::DeleteChild(size_t index) {
	if (index >= _children.size()) return(-1);

	XmlNode	*node = _children[index];
	delete node;
	_children.erase(_children.begin()+index);
	return(0);
}

XmlNode	*XmlNode::GetChild(size_t index) {

	if (index >= _children.size()) {
		// need to report error message
		SetErrMsg("Invalid child id : %d", index);
		return(NULL);
	}

	return(_children[index]);
}

int XmlNode::HasChild(size_t index) {

	return(index < _children.size());
	
}

XmlNode	*XmlNode::GetChild(const string &tag) {

	XmlNode *child;

	for (size_t i = 0; i<_children.size(); i++) {
		if (! (child = GetChild(i))) return(NULL);
	
		if (StrCmpNoCase(child->_tag, tag) == 0) return(child);
	}

	SetErrMsg("Invalid child tag : %s", tag.c_str());
	return(NULL);
}

int XmlNode::HasChild(const string &tag) {

	XmlNode *child;

	for (size_t i = 0; i<_children.size(); i++) {
		if (! HasChild(i)) return(0);
		assert ((child = GetChild(i)) != NULL);
	
		if (StrCmpNoCase(child->_tag, tag) == 0) return(1);
	}
	return(0);
}



ostream&
XmlNode::streamOut(ostream&os, const XmlNode& node) {
	os << node;
	return os;
}

namespace VAPoR {
std::ostream& operator<<(ostream& os, const VAPoR::XmlNode& node) {

	map <string, vector<long>*>::const_iterator plong;
	map <string, vector<double>*>::const_iterator pdouble;
	map <string, string>::const_iterator pstring;
	map <string, string>::const_iterator pattr;

	int	i;

	plong = node._longmap.begin();
	pdouble = node._doublemap.begin();
	pstring = node._stringmap.begin();
	pattr = node._attrmap.begin();

	os << "<" << node._tag;

	for(; pattr != node._attrmap.end(); pattr++) {
		const string &name = pattr->first;
		const string &value = pattr->second;

		os << " " << name << "=\"" << value << "\" ";

	}
	os << ">" << endl;

	for(; plong != node._longmap.end(); plong++) {
		const string &tag = plong->first;

		os << "<" << tag << " Type=\"Long\">" << endl << "  ";

		vector<long> *vptr = plong->second;
		vector<long> &v = *vptr;

		for(i=0; i<(int)v.size(); i++) {
			os << v[i] << " ";
		}
		os << endl;

		os << "</" << tag << ">" << endl;
	}

	for(; pdouble != node._doublemap.end(); pdouble++) {
		const string &tag = pdouble->first;

		os << "<" << tag << " Type=\"Double\">" << endl << "  ";

		vector<double> *vptr = pdouble->second;
		vector<double> &v = *vptr;

		for(i=0; i<(int)v.size(); i++) {
			os << v[i] << " ";
		}
		os << endl;

		os << "</" << tag << ">" << endl;
	}

	for(; pstring != node._stringmap.end(); pstring++) {
		const string &tag = pstring->first;

		os << "<" << tag << " Type=\"String\">" << endl << "  ";

		const string &v = pstring->second;

		os << v;

		os << endl;

		os << "</" << tag << ">" << endl;
	}

	if (node._children.size()) {

		for (int i = 0; i<(int)node._children.size(); i++) {
			XmlNode *childptr;

			childptr = node._children[i];
			os << *childptr;
		}

	}

	os << "</" << node._tag << ">" << endl;

	return (os);
}
};

