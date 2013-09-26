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
#include <qapplication.h>
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


using namespace VAPoR;


const string Params::_regionParamsTag = "RegionPanelParameters";
const string Params::_animationParamsTag = "AnimationPanelParameters";
const string Params::_viewpointParamsTag = "ViewpointPanelParameters";
const string Params::_VisualizerNumTag = "VisualizerNum";
const string Params::_VariablesTag = "Variables";
const string Params::_RefinementLevelTag = "RefinementLevel";
const string Params::_CompressionLevelTag = "CompressionLevel";
const string Params::_VariableNamesTag = "VariableNames";

std::map<pair<int,int>,vector<Params*> > Params::paramsInstances;
std::map<pair<int,int>, int> Params::currentParamsInstance;
std::map<int, Params*> Params::defaultParamsInstance;
std::vector<Params*> Params::dummyParamsInstances;



Params::Params(
	XmlNode *parent, const string &name, int winNum
	): ParamsBase(parent,name) {
	vizNum = winNum;
	if(winNum < 0) local = false; else local = true;
	
}


Params::~Params() {
	
}

RenderParams::RenderParams(XmlNode *parent, const string &name, int winnum):Params(parent, name, winnum){
	local = true;
	enabled = false;
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
	QString errorMessage(errstr);
	errorMessage += "\n in file: ";
	errorMessage += fname;
	errorMessage += " at line ";
	errorMessage += QString::number(lineno);
	SetErrMsg(VAPOR_FATAL,"Fatal error: %s",(const char*)errorMessage.toAscii());
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
		Params* p = GetDefaultParams(pType)->deepCopy();
		p->setVizNum(winnum);
		AppendParamsInstance(pType,winnum,p);
		return p;
	}
	return paramsInstances[make_pair(pType, winnum)][instance];
}

void Params::RemoveParamsInstance(int pType, int winnum, int instance){
	vector<Params*>& instVec = paramsInstances[make_pair(pType,winnum)];
	Params* p = instVec.at(instance);
	int currInst = currentParamsInstance[make_pair(pType,winnum)];
	if (currInst > instance) currentParamsInstance[make_pair(pType,winnum)] = currInst - 1;
	instVec.erase(instVec.begin()+instance);
	if (currInst >= (int) instVec.size()) currentParamsInstance[make_pair(pType,winnum)]--;
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
			if (((RenderParams*)p)->isEnabled()) return true;
		}
	}
	return false;
}
//Default copy constructor.  Don't copy the paramnode
Params::Params(const Params& p) :
	ParamsBase(p)
{
	local = p.local;
	vizNum = p.vizNum;
}
Params* Params::deepCopy(ParamNode* ){
	//Start with default copy  
	Params* newParams = CreateDefaultParams(GetParamsBaseTypeId());
	newParams->local = isLocal();
	newParams->setVizNum(getVizNum());
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
	renParams->enabled = enabled;
	renParams->stopFlag = stopFlag;
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

