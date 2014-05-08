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
	//Avoid command queue use in constructor:
	if(winNum < 0) GetRootNode()->SetElementLong(_LocalTag,0);
	else GetRootNode()->SetElementLong(_LocalTag,1);
	SetInstanceIndex(0);
	changeBit = true;
}


Params::~Params() {
	
}

RenderParams::RenderParams(XmlNode *parent, const string &name, int winnum):Params(parent, name, winnum){
	SetLocal(true);
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

bool Params::HasChanged(int viz){
	if (IsLocal()) return changeBit;
	//Find the instance associated with viz:
	assert(viz >= 0);
	Params* localParams = Params::GetParamsInstance(GetParamsBaseTypeId(),viz);
	return localParams->changeBit;
}

void Params::SetChanged(bool val){
	//If it's shared, need to set the change bits for all the
	//different windows that are using 
	if (IsLocal()) {
		changeBit = val;
		return;
	}
	//Find all instances that are sharing this
	std::map<pair<int,int>,vector<Params*> >::iterator it;
	vector<long> viznums = VizWinParams::GetVisualizerNums();
	for (int i = 0; i<viznums.size(); i++){
		int viz = viznums[i];
		it = paramsInstances.find(make_pair(GetParamsBaseTypeId(),viz));
		if (it == paramsInstances.end()) continue;
		Params* p = (it->second)[0]; //get the first (only) instance of the params
		if (p->IsLocal()) continue;
		p->changeBit = val;
	}
	return;
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
	map<pair<int,int>,vector<Params*> >::const_iterator it = paramsInstances.find(make_pair(pType,winnum));
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
	for (int i = instance; i<instVec.size(); i++) instVec[i]->SetInstanceIndex(i);
	delete p;
	if (instVec.size() == 0) 
		paramsInstances.erase(it);
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
Params* Params::CreateDefaultParams(int pType){
	Command::blockCapture();
	Params*p = (Params*)(createDefaultFcnMap[pType])();
	Command::unblockCapture();
	return p;
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
	return GetValueLong(_CompressionLevelTag,defaultLevel);
 }
int RenderParams::SetCompressionLevel(int level){
	 vector<long> valvec(1,(long)level);
	 int rc = SetValueLong(_CompressionLevelTag,"Set compression level",valvec);
	 setAllBypass(false);
	 return rc;
}
int RenderParams::SetRefinementLevel(int level){
		
		int maxref = DataStatus::getInstance()->getNumTransforms();
		if (level < 0 || level > maxref) return -1;
		SetValueLong(_RefinementLevelTag, "Set refinement level",level);
		setAllBypass(false);
		return 0;
}
int RenderParams::GetRefinementLevel(){
		const vector<long>defaultRefinement(1,0);
		return (GetValueLong(_RefinementLevelTag,defaultRefinement));
}
int Params::SetInstanceIndex(int indx){
	//Not a user-level setting; no undo support
	GetRootNode()->SetElementLong(_InstanceTag, indx);
	return 0;
}
int Params::GetInstanceIndex(){
	const vector<long>defaultInstance(1,1);
	return (GetValueLong(_InstanceTag,defaultInstance));
}
//Following calculates box corners in user space.  Does not use
//stretching.
void Params::
calcLocalBoxCorners(double corners[8][3], float extraThickness, int timestep, double rotation, int axis){
	double transformMatrix[12];
	buildLocalCoordTransform(transformMatrix, extraThickness, timestep, rotation, axis);
	double boxCoord[3];
	//Return the corners of the box (in world space)
	//Go counter-clockwise around the back, then around the front
	//X increases fastest, then y then z; 

	//Fatten box slightly, in case it is degenerate.  This will
	//prevent us from getting invalid face normals.

	boxCoord[0] = -1.f;
	boxCoord[1] = -1.f;
	boxCoord[2] = -1.f;
	vtransform(boxCoord, transformMatrix, corners[0]);
	boxCoord[0] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[1]);
	boxCoord[1] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[3]);
	boxCoord[0] = -1.f;
	vtransform(boxCoord, transformMatrix, corners[2]);
	boxCoord[1] = -1.f;
	boxCoord[2] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[4]);
	boxCoord[0] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[5]);
	boxCoord[1] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[7]);
	boxCoord[0] = -1.f;
	vtransform(boxCoord, transformMatrix, corners[6]);
	
}
void Params::
buildLocalCoordTransform(double transformMatrix[12], double extraThickness, int timestep, double rotation, int axis){
	
	double theta, phi, psi;
	if (rotation != 0.) {
		convertThetaPhiPsi(&theta,&phi,&psi, axis, rotation);
	} else {
		vector<double> angles = GetBox()->GetAngles();
		theta = angles[0];
		phi = angles[1];
		psi = angles[2];
	}
	
	double boxSize[3];
	double boxExts[6];
	GetBox()->GetLocalExtents(boxExts, timestep);
	

	for (int i = 0; i< 3; i++) {
		boxExts[i] -= extraThickness;
		boxExts[i+3] += extraThickness;
		boxSize[i] = (boxExts[i+3] - boxExts[i]);
	}
	
	//Get the 3x3 rotation matrix:
	double rotMatrix[9];
	getRotationMatrix(theta*M_PI/180., phi*M_PI/180., psi*M_PI/180., rotMatrix);

	//then scale according to box:
	transformMatrix[0] = 0.5*boxSize[0]*rotMatrix[0];
	transformMatrix[1] = 0.5*boxSize[1]*rotMatrix[1];
	transformMatrix[2] = 0.5*boxSize[2]*rotMatrix[2];
	//2nd row:
	transformMatrix[4] = 0.5*boxSize[0]*rotMatrix[3];
	transformMatrix[5] = 0.5*boxSize[1]*rotMatrix[4];
	transformMatrix[6] = 0.5*boxSize[2]*rotMatrix[5];
	//3rd row:
	transformMatrix[8] = 0.5*boxSize[0]*rotMatrix[6];
	transformMatrix[9] = 0.5*boxSize[1]*rotMatrix[7];
	transformMatrix[10] = 0.5*boxSize[2]*rotMatrix[8];
	//last column, i.e. translation:
	transformMatrix[3] = .5*(boxExts[3]+boxExts[0]);
	transformMatrix[7] = .5*(boxExts[4]+boxExts[1]);
	transformMatrix[11] = .5*(boxExts[5]+boxExts[2]);
	
}
//Determine a new value of theta phi and psi when the probe is rotated around either the
//x-, y-, or z- axis.  axis is 0,1,or 2 1. rotation is in degrees.
//newTheta and newPhi are in degrees, with theta between -180 and 180, phi between 0 and 180
//and newPsi between -180 and 180
void Params::convertThetaPhiPsi(double *newTheta, double* newPhi, double* newPsi, int axis, double rotation){

	//First, get original rotation matrix R0(theta, phi, psi)
	double origMatrix[9], axisRotate[9], newMatrix[9];
	vector<double> angles = GetBox()->GetAngles();
	getRotationMatrix(angles[0]*M_PI/180., angles[1]*M_PI/180., angles[2]*M_PI/180., origMatrix);
	//Second, get rotation matrix R1(axis,rotation)
	getAxisRotation(axis, rotation*M_PI/180., axisRotate);
	//New rotation matrix is R1*R0
	mmult33(axisRotate, origMatrix, newMatrix);
	//Calculate newTheta, newPhi, newPsi from R1*R0 
	getRotAngles(newTheta, newPhi, newPsi, newMatrix);
	//Convert back to degrees:
	(*newTheta) *= (180./M_PI);
	(*newPhi) *= (180./M_PI);
	(*newPsi) *= (180./M_PI);
	return;
}
//Find the smallest stretched extents containing the rotated box
//Similar to above, but using stretched extents
void Params::calcRotatedStretchedBoxExtents(double* bigBoxExtents){
	if(!DataStatus::getInstance()) return;
	//Determine the smallest axis-aligned cube that contains the probe.  This is
	//obtained by mapping all 8 corners into the space.
	//It will not necessarily fit inside the unit cube.
	double corners[8][3];
	calcLocalBoxCorners(corners, 0.f, -1);
	
	double boxMin[3],boxMax[3];
	int crd, cor;
	
	//initialize extents, and variables that will be min,max
	for (crd = 0; crd< 3; crd++){
		boxMin[crd] = 1.e30f;
		boxMax[crd] = -1.e30f;
	}
	
	
	for (cor = 0; cor< 8; cor++){
		//make sure the container includes it:
		for(crd = 0; crd< 3; crd++){
			if (corners[cor][crd]<boxMin[crd]) boxMin[crd] = corners[cor][crd];
			if (corners[cor][crd]>boxMax[crd]) boxMax[crd] = corners[cor][crd];
		}
	}
	//Now convert the min,max back into extents 
	const double* stretch = DataStatus::getInstance()->getStretchFactors();
	
	for (crd = 0; crd<3; crd++){
		bigBoxExtents[crd] = (boxMin[crd]*stretch[crd]);
		bigBoxExtents[crd+3] = (boxMax[crd]*stretch[crd]);
	}
	return;
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
