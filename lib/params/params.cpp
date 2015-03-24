//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		params.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the Params, BasicParams, and DummyParams classes.
//		These are  abstract classes for all the tabbed panel params classes.
//		Supports functionality common to all the tabbed panel params.
//
#include "glutil.h"	// Must be included first!!!
#include <vapor/XmlNode.h>
#include <cstring>
#include <string>
#include <vapor/MyBase.h>
#include "assert.h"
#include "params.h"
#include "renderparams.h"
#include "datastatus.h"
#include "vizwinparams.h"
#include <vapor/ParamNode.h>
#include <vapor/DataMgr.h>
#include <vapor/RegularGrid.h>
#include <vapor/errorcodes.h>
#include "Box.h"
#include <iostream>
#include <sstream>
#include <string>


using namespace VAPoR;

#include "command.h"
const string Params::_regionParamsTag = "RegionPanelParameters";
const string Params::_animationParamsTag = "AnimationPanelParameters";
const string Params::_viewpointParamsTag = "ViewpointPanelParameters";
const string Params::_VisualizerNumTag = "VisualizerNum";
const string Params::_VariablesTag = "Variables";
const string Params::_RefinementLevelTag = "RefinementLevel";
const string Params::_CompressionLevelTag = "CompressionLevel";
const string Params::_VariableNamesTag = "VariableNames";
const string Params::_LocalTag = "Local";
const string Params::_IsoControlTag = "IsoControl";
const string Params::_IsoValueTag = "IsoValue";
const string Params::_Variables2DTag = "2DVariables";
const string Params::_Variables3DTag = "3DVariables";
const string Params::_VariableNameTag = "VariableName";


std::map<pair<int,int>,vector<Params*> > Params::paramsInstances;
std::map<pair<int,int>, int> Params::currentParamsInstance;
std::map<int, Params*> Params::defaultParamsInstance;
std::vector<Params*> Params::dummyParamsInstances;



Params::Params(
	XmlNode *parent, const string &name, int winNum
	): ParamsBase(parent,name) {
	SetVizNum(winNum);
	//Avoid command queue use in constructor:
	if(winNum < 0) GetRootNode()->SetElementLong(_LocalTag,0);
	else GetRootNode()->SetElementLong(_LocalTag,1);
	
}


Params::~Params() {
	
}

const std::string Params::paramName(Params::ParamsBaseType type){
	return GetDefaultParams(type)->getShortName();
}




void Params::BailOut(const char *errstr, const char *fname, int lineno)
{
    /* Terminate program after printing an error message.
     * Use via the macros Verify and MemCheck.
     */
    //Error("Error: %s, at %s:%d\n", errstr, fname, lineno);
    //if (coreDumpOnError)
	//abort();
	string errorMessage(errstr);
	errorMessage += "\n in file: ";
	errorMessage += fname;
	errorMessage += " at line ";
	std::stringstream ss;
	ss << lineno;
	errorMessage += ss.str(); 
	SetErrMsg(VAPOR_FATAL,"Fatal error: %s",(const char*)errorMessage.c_str());
	//MessageReporter::fatalMsg(errorMessage);
    exit(-1);
}





Params* Params::GetParamsInstance(int pType, int winnum, int instance){
	if (winnum < 0) return defaultParamsInstance[pType];
	if (instance < 0) instance = (int) currentParamsInstance[make_pair(pType,winnum)];
	assert(instance >= 0);
	map< pair<int,int>,vector<Params*> >::const_iterator it = paramsInstances.find(make_pair(pType,winnum));
	if (it == paramsInstances.end()) return 0;  //return null if no instances exist.
	
	assert(instance < (int) (paramsInstances[make_pair(pType, winnum)].size()));
	
	Params*p = paramsInstances[make_pair(pType, winnum)][instance];
	assert(p->GetInstanceIndex() == instance);
	return p;
}

void Params::RemoveParamsInstance(int pType, int winnum, int instance){
	//Make sure the specified instance exists
	map<pair<int,int>,vector<Params*> >::iterator it = paramsInstances.find(make_pair(pType,winnum));
	if (it == paramsInstances.end()){
		assert(0);
		return;
	}
	vector<Params*>& instVec = paramsInstances[make_pair(pType,winnum)];
	Params* p = instVec.at(instance);
	assert(p->GetInstanceIndex() == instance);
	int currInst = currentParamsInstance[make_pair(pType,winnum)];
	if (currInst > instance) currentParamsInstance[make_pair(pType,winnum)] = currInst - 1;
	instVec.erase(instVec.begin()+instance);
	if (currInst >= (int) instVec.size()) currentParamsInstance[make_pair(pType,winnum)]--;
	delete p;
	if (instVec.size() == 0) {
		paramsInstances.erase(it);
		//Also remove the current instance index:
		map<pair<int,int>,int >::iterator it2 = currentParamsInstance.find(make_pair(pType,winnum));
		if (it2 == currentParamsInstance.end()){
			assert(0);
			return;
		}
		currentParamsInstance.erase(it2);
	}

}
map <int, vector<Params*> >* Params::cloneAllParamsInstances(int winnum){
	map<int, vector<Params*> >* winParamsMap = new map<int, vector<Params*> >;
	for (int i = 1; i<= GetNumParamsClasses(); i++){
		vector<Params*> *paramsVec = new vector<Params*>;
		for (int j = 0; j<GetNumParamsInstances(i,winnum); j++){
			Params* p = GetParamsInstance(i,winnum,j);
			paramsVec->push_back((Params*)(p->deepCopy(0)));
		}
		(*winParamsMap)[i] = *paramsVec;
	}
	return winParamsMap;

			
}
vector <Params*>* Params::cloneAllDefaultParams(){
	vector <Params*>* defaultParams = new vector<Params*>;
	defaultParams->push_back(0); //don't use position 0
	for (int i = 1; i<= GetNumParamsClasses(); i++){
		Params* p = GetDefaultParams(i);
		defaultParams->push_back((Params*)(p->deepCopy(0)));
	}
	return (defaultParams);
}

bool Params::IsRenderingEnabled(int winnum){
	for (int i = 1; i<= GetNumParamsClasses(); i++){
		for (int inst = 0; inst < GetNumParamsInstances(i,winnum); inst++){
			Params* p = GetParamsInstance(i,winnum, inst);
			if (!(p->isRenderParams())) break;
			if (((RenderParams*)p)->IsEnabled()) return true;
		}
	}
	return false;
}
//Default copy constructor.  Don't copy the paramnode
Params::Params(const Params& p) :
	ParamsBase(p)
{

}

Params* Params::CreateDefaultParams(int pType){
	Command::blockCapture();
	Params*p = (Params*)(createDefaultFcnMap[pType])();
	Command::unblockCapture();
	return p;
}

Params* Params::CreateDummyParams(const std::string tag){
	return ((Params*)(new DummyParams(0,tag,0)));
}
DummyParams::DummyParams(XmlNode* parent, const string tag, int winnum) : Params(parent, tag, winnum){
	myTag = tag;
}
void Params::clearDummyParamsInstances(){
	for(int i = 0; i< dummyParamsInstances.size(); i++){
		delete dummyParamsInstances[i];
	}
	dummyParamsInstances.clear();
}

int Params::GetInstanceIndex(){
	int viznum = GetVizNum();
	vector<Params*> instances = GetAllParamsInstances(GetParamsBaseTypeId(), GetVizNum());
	for (int i = 0; i<instances.size(); i++){
		if (this == instances[i]) return i;
	}
	assert(0);
	return -1;
}



int Params::DeleteVisualizer(int viz){

	std::map<pair<int,int>,vector<Params*> >::iterator it = paramsInstances.begin();
	int num = 0;
	while (it != paramsInstances.end()){
		pair<int,int> pr = it->first;
		if (pr.second == viz){ //delete all params associated with the visualizer
			std::map<pair<int,int>,vector<Params*> >::iterator eraseIt = it;
			vector<Params*> pvec = it->second;
			for (int i = 0; i<pvec.size(); i++) {delete pvec[i]; num++;}
			++it;
			paramsInstances.erase(eraseIt);
		} else
		{
			++it;
		}
	}
	return num;
}

 int Params::GetNumParamsInstances(int pType, int winnum){
	std::map<pair<int,int>,vector<Params*> >::iterator it = paramsInstances.find(std::make_pair(pType,winnum));
	if (it == paramsInstances.end()) return 0;
	return paramsInstances[make_pair(pType, winnum)].size();
}
bool Params::isRenderParams() const {
	return ((dynamic_cast<const RenderParams*>(this)) != 0);
}
bool Params::isBasicParams() const {
	return (dynamic_cast<const BasicParams*>(this) != 0);
}