//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		 *
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
#include <vapor/XmlNode.h>
#include <string>
#include <vapor/MyBase.h>
#include "assert.h"
#include <vapor/ParamNode.h>
#include <vapor/ParamsBase.h>
#include "params.h"
#include "ParamsIso.h"
#include "dvrparams.h"
#include "flowparams.h"
#include "twoDdataparams.h"
#include "twoDimageparams.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"
#include "probeparams.h"
#include "transferfunction.h"
using namespace VAPoR;
std::map<string,int> ParamsBase::classIdFromTagMap;
std::map<int,string> ParamsBase::tagFromClassIdMap;
std::map<int,ParamsBase::BaseCreateFcn> ParamsBase::createDefaultFcnMap;
const std::string ParamsBase::_emptyString;
int ParamsBase::numParamsClasses = 0;
int ParamsBase::numEmbedClasses = 0;
ParamsBase::ParamsBase(
	XmlNode *parent, const string &name
) {
	previousClass = 0;
	assert (!parent || parent->GetNumChildren() == 0);

	map <string, string> attrs;
	_rootParamNode = new ParamNode(name, attrs);
	_currentParamNode = _rootParamNode;
	_rootParamNode->SetParamsBase(this);
	if(parent) parent->AddChild(_rootParamNode);
	_paramsBaseName = name;
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
//Default copy constructor.  Don't copy the paramnode
ParamsBase::ParamsBase(const ParamsBase& pBase) :
	ParsedXml(pBase)
{
	_currentParamNode = _rootParamNode = 0;
	_parseDepth = pBase._parseDepth;
	_paramsBaseName = pBase._paramsBaseName;
}
//Default buildNode clones the ParamNodes but calls buildNode on Registered nodes
ParamNode* ParamsBase::buildNode(){	

	ParamNode* oldNode = GetRootNode();
	ParamNode* newNode = oldNode->ShallowCopy();
	for (int i=0; i<oldNode->GetNumChildren(); i++) {
		ParamNode *child = oldNode->GetChild(i);
		ParamNode *newChild;
		if (child->GetParamsBase())
			newChild = child->GetParamsBase()->buildNode();
		else newChild = child->NodeCopy();
		newNode->AddChild(newChild);
	}
	return newNode;

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

	// Create a child node if this is
	// not the root node, and if it does not have a type attribute.
	//
	if ((StrCmpNoCase(tag, _rootParamNode->Tag()) != 0) && 
		(!(*attrs) || StrCmpNoCase(*attrs, _typeAttr) != 0)) {
		ParamNode* childNode = Push(tag);
		return (childNode != 0);
	}
	//If it has "ParamsBase" attribute, then do similarly, but create an instance of the class associated with the tag.
	
	if (*attrs) {

		if (StrCmpNoCase(*attrs, _typeAttr) != 0) {
			pm->parseError("Invalid attribute : %s", *attrs);
			return(false);
		}

		//See if this is a paramNode or paramsBase 
		string type = *(attrs+1);
		if ( type == ParamNode::_paramNodeAttr){
			ParamNode* childNode = Push(tag);
			return (childNode != 0);
		} else if( type == ParamNode::_paramsBaseAttr){
			//If it has "ParamsBase" attribute, then do similarly, but create an instance of the class associated with the tag.
			ParamsBase* baseNode = CreateDefaultParamsBase(tag);
			//Create a new child node
			map <string, string> childattrs;
			ParamNode *child = new ParamNode(tag, childattrs);
			child->SetParamsBase(baseNode);
			(void) _currentParamNode->AddChild(child);
			_currentParamNode = child;
	
			pm->pushClassStack(baseNode);
			//defer to the base node to do its own parsing:
			baseNode->elementStartHandler(pm, depth, tag, attrs);
			return (true);
		} else {
			attrs++;
			state->data_type = *attrs;
			attrs++;

			state->has_data = 1;
		}
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
//The ParamsBase is attached
ParamNode *ParamsBase::Push(
	string& name,
	ParamsBase* pBase
) {


	//
	// See if child node already exists
	//
	for (size_t i=0; i<_currentParamNode->GetNumChildren(); i++) {
		ParamNode *child = _currentParamNode->GetChild(i);
		if (child->Tag().compare(name) == 0) {
			_currentParamNode = child;
			_currentParamNode->SetParamsBase(pBase);
			return(_currentParamNode);	// We're done
		}
	}
	//Create a new child
	map <string, string> attrs;
	ParamNode *child = new ParamNode(name, attrs);
	child->SetParamsBase(pBase);
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

	//Delete all the existing children?
	_rootParamNode->DeleteAll();
	if (parent){
		// Delete current root node
		int nchildren = parent->GetNumChildren();
		assert(nchildren <= 1);
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
	
//Static methods for class registration
//All ParamsBase classes must provide one line in the following method
void ParamsBase::RegisterParamsBaseClasses(){
	RegisterParamsBaseClass(TransferFunction::_transferFunctionTag, TransferFunction::CreateDefaultInstance, false);
	RegisterParamsBaseClass(MapperFunctionBase::_mapperFunctionTag, MapperFunction::CreateDefaultInstance, false);
	RegisterParamsBaseClass(ParamsIso::_IsoControlTag, IsoControl::CreateDefaultInstance, false);
	RegisterParamsBaseClass(Params::_isoParamsTag, ParamsIso::CreateDefaultInstance, true);
	RegisterParamsBaseClass(Params::_dvrParamsTag, DvrParams::CreateDefaultInstance, true);
	RegisterParamsBaseClass(Params::_flowParamsTag, FlowParams::CreateDefaultInstance, true);
	RegisterParamsBaseClass(Params::_probeParamsTag, ProbeParams::CreateDefaultInstance, true);
	RegisterParamsBaseClass(Params::_twoDDataParamsTag, TwoDDataParams::CreateDefaultInstance, true);
	//For backwards compatibility; the tag changed in vapor 1.5
	ReregisterParamsBaseClass(Params::_twoDParamsTag, Params::_twoDDataParamsTag, true);
	RegisterParamsBaseClass(Params::_twoDImageParamsTag, TwoDImageParams::CreateDefaultInstance, true);
	RegisterParamsBaseClass(Params::_regionParamsTag, RegionParams::CreateDefaultInstance, true);
	RegisterParamsBaseClass(Params::_animationParamsTag, AnimationParams::CreateDefaultInstance, true);
	RegisterParamsBaseClass(Params::_viewpointParamsTag, ViewpointParams::CreateDefaultInstance, true);
}
int ParamsBase::RegisterParamsBaseClass(const string& tag, BaseCreateFcn fcn, bool isParams){
	// See if tag is registered already 
	// If so, error
	map <string, int> :: const_iterator getIdIter;
    getIdIter = classIdFromTagMap.find(tag);
	if (getIdIter != classIdFromTagMap.end()) {
		assert(0);
		return 0;
	}
	//Find range of used indices, but min must be <= 0, max must be >= 0
	int minIndex = 0, maxIndex = 0;
	if (classIdFromTagMap.size() > 0){
		minIndex = 0;
		maxIndex = 0;
		for (getIdIter = classIdFromTagMap.begin(); getIdIter != classIdFromTagMap.end(); getIdIter++){
			if (minIndex > getIdIter->second) minIndex = getIdIter->second;
			if (maxIndex < getIdIter->second) maxIndex = getIdIter->second;
		}
	} 
	int newIndex;
	if (isParams) newIndex = maxIndex+1;
	else newIndex = minIndex -1;
	classIdFromTagMap.insert(pair<string,int>(tag,newIndex));
	tagFromClassIdMap.insert(pair<int,string>(newIndex,tag));
	createDefaultFcnMap.insert(pair<int, BaseCreateFcn>(newIndex, fcn));
	if(isParams) numParamsClasses = newIndex;
	else numEmbedClasses = -newIndex;
	return newIndex;

}
int ParamsBase::ReregisterParamsBaseClass(const string& tag, const string& newtag, bool isParams){
	// See if tag is registered already, or if newtag is not registered: 
	// If so, error
	map <string, int> :: const_iterator getIdIter;
    getIdIter = classIdFromTagMap.find(tag);
	if (getIdIter != classIdFromTagMap.end()) {
		return 0;
	}
	getIdIter = classIdFromTagMap.find(newtag);
	if (getIdIter == classIdFromTagMap.end()) {
		return 0;
	}
	int index = getIdIter->second;
	if ((isParams && index < 0) || (!isParams && index > 0)) return 0;
	
	classIdFromTagMap.insert(pair<string,int>(tag,index));
	
	return index;

}
ParamsBase::ParamsBaseType ParamsBase::GetTypeFromTag(const string&tag){
	map <string,int> :: const_iterator getIdIter;
    getIdIter = classIdFromTagMap.find(tag);
	if (getIdIter == classIdFromTagMap.end()) return 0;
	return (ParamsBaseType)(getIdIter->second);
}
const string& ParamsBase::GetTagFromType(ParamsBaseType t){
	map <int,string> :: const_iterator getTagIter;
    getTagIter = tagFromClassIdMap.find(t);
	if (getTagIter == tagFromClassIdMap.end()) return _emptyString;
	return getTagIter->second;
}
