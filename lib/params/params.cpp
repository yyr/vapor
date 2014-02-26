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
//	Description:	Implements the Params and RenderParams classes.
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
#include "datastatus.h"
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
const string RenderParams::_EnabledTag = "Enabled";
const string Params::_InstanceTag = "Instance";

std::map<pair<int,int>,vector<Params*> > Params::paramsInstances;
std::map<pair<int,int>, int> Params::currentParamsInstance;
std::map<int, Params*> Params::defaultParamsInstance;
std::vector<Params*> Params::dummyParamsInstances;



Params::Params(
	XmlNode *parent, const string &name, int winNum
	): ParamsBase(parent,name) {
	SetVizNum(winNum);
	if(winNum < 0) SetLocal(false); else SetLocal(true);
	SetInstanceIndex(0);
	SetValidationMode(CHECK_AND_FIX);
}


Params::~Params() {
	
}

RenderParams::RenderParams(XmlNode *parent, const string &name, int winnum):Params(parent, name, winnum){
	SetLocal(true);
}
const std::string& Params::paramName(Params::ParamsBaseType type){
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

//Following methods adapted from ParamsBase.cpp
void RenderParams::initializeBypassFlags(){
	bypassFlags.clear();
	int numTimesteps = DataStatus::getInstance()->getNumTimesteps();
	bypassFlags.resize(numTimesteps, 0);
}
void RenderParams::setAllBypass(bool val){ 
	//set all bypass flags to either 0 or 2
	//when set to 0, indicates "never bypass"
	//when set to 2, indicates "always bypass"
	int ival = val ? 2 : 0;
	for (int i = 0; i<bypassFlags.size(); i++)
		bypassFlags[i] = ival;
}



Params* Params::GetParamsInstance(int pType, int winnum, int instance){
	if (winnum < 0) return defaultParamsInstance[pType];
	if (instance < 0) instance = currentParamsInstance[make_pair(pType,winnum)];
	if (instance >= paramsInstances[make_pair(pType, winnum)].size()) {
		instance = paramsInstances[make_pair(pType, winnum)].size();
		Params* p = GetDefaultParams(pType)->deepCopy();
		p->SetVizNum(winnum);
		AppendParamsInstance(pType,winnum,p);
		assert(p->GetInstanceIndex() == instance);
		return p;
	}
	Params*p = paramsInstances[make_pair(pType, winnum)][instance];
	assert(p->GetInstanceIndex() == instance);
	return p;
}

void Params::RemoveParamsInstance(int pType, int winnum, int instance){
	vector<Params*>& instVec = paramsInstances[make_pair(pType,winnum)];
	Params* p = instVec.at(instance);
	assert(p->GetInstanceIndex() == instance);
	int currInst = currentParamsInstance[make_pair(pType,winnum)];
	if (currInst > instance) currentParamsInstance[make_pair(pType,winnum)] = currInst - 1;
	instVec.erase(instVec.begin()+instance);
	if (currInst >= (int) instVec.size()) currentParamsInstance[make_pair(pType,winnum)]--;
	for (int i = instance; i<instVec.size(); i++) instVec[i]->SetInstanceIndex(i);
	delete p;
}
map <int, vector<Params*> >* Params::cloneAllParamsInstances(int winnum){
	map<int, vector<Params*> >* winParamsMap = new map<int, vector<Params*> >;
	for (int i = 1; i<= GetNumParamsClasses(); i++){
		vector<Params*> *paramsVec = new vector<Params*>;
		for (int j = 0; j<GetNumParamsInstances(i,winnum); j++){
			Params* p = GetParamsInstance(i,winnum,j);
			paramsVec->push_back(p->deepCopy(0));
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
		defaultParams->push_back(p->deepCopy(0));
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
Params* Params::deepCopy(ParamNode* ){
	//Start with default copy  
	Params* newParams = CreateDefaultParams(GetParamsBaseTypeId());
	
	// Need to clone the xmlnode; 
	ParamNode* rootNode = GetRootNode();
	if (rootNode) {
		newParams->SetRootParamNode(rootNode->deepCopy());
		newParams->GetRootNode()->SetParamsBase(newParams);
	}
	
	newParams->setCurrentParamNode(newParams->GetRootNode());
	return newParams;
}
Params* RenderParams::deepCopy(ParamNode* nd){
	//Start with default copy  
	Params* newParams = Params::deepCopy(nd);
	
	RenderParams* renParams = dynamic_cast<RenderParams*>(newParams);
	
	renParams->bypassFlags = bypassFlags;
	return renParams;
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

int RenderParams::GetCompressionLevel(){
	const vector<long> defaultLevel(1,2);
	vector<long> valvec = GetRootNode()->GetElementLong(_CompressionLevelTag,defaultLevel);
	return (int)valvec[0];
 }
int RenderParams::SetCompressionLevel(int level){
	 Command* cmd = 0;
	 int maxlod = DataStatus::getInstance()->getNumLODs();
	 if (level < 0 || level >maxlod ) return -1;
	 if (Command::isRecording())
		cmd = Command::captureStart(this,"Set compression level");
	 vector<long> valvec(1,(long)level);
	 GetRootNode()->SetElementLong(_CompressionLevelTag,valvec);
	 setAllBypass(false);
	 return 0;
}
int RenderParams::SetRefinementLevel(int level){
		Command* cmd = 0;
		int maxref = DataStatus::getInstance()->getNumTransforms();
		if (level < 0 || level > maxref) return -1;
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Set refinement level");
		GetRootNode()->SetElementLong(_RefinementLevelTag, level);
		setAllBypass(false);
		if(cmd)Command::captureEnd(cmd,this);
		return 0;
}
int RenderParams::GetRefinementLevel(){
		const vector<long>defaultRefinement(1,0);
		return (GetRootNode()->GetElementLong(_RefinementLevelTag,defaultRefinement)[0]);
}
int Params::SetInstanceIndex(int indx){
	GetRootNode()->SetElementLong(_InstanceTag, indx);
	return 0;
}
int Params::GetInstanceIndex(){
	const vector<long>defaultInstance(1,1);
	return (GetRootNode()->GetElementLong(_InstanceTag,defaultInstance)[0]);
}