//************************************************************************
//																		*
//		     Copyright (C)  2011										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		Box.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2011
//
//	Description:	Implements the Box class.
//		Used to control extents and orientation of 2D and 3D data regions.
//		Supports time-varying extents.
//

#include <vapor/XmlNode.h>
#include <cstring>
#include <string>
#include <vector>
#include <vapor/MyBase.h>
#include "command.h"
#include "datastatus.h"
#include "assert.h"
#include "Box.h"
using namespace VAPoR;

const std::string Box::_boxTag = "Box";
const std::string Box::_anglesTag = "Angles";
const std::string Box::_extentsTag = "Extents";
const std::string Box::_timesTag = "Times";

Box::Box(): ParamsBase(0, Box::_boxTag) {
	//Initialize with default box:
	vector<double> extents;
	for (int i=0; i<3; i++) extents.push_back(0.0);
	for (int i=0; i<3; i++) extents.push_back(1.0);
	vector<long> times;
	times.push_back(-1);
	vector<double> angles;
	angles.push_back(0.);
	angles.push_back(0.);
	angles.push_back(0.);
	//Provide initial values in XML node
	SetValueDouble(_extentsTag,"",extents,0);
	SetValueLong(_timesTag,"",times,0);
	SetValueDouble(_anglesTag,"",angles,0);
}

ParamsBase* Box::deepCopy(ParamNode* newRoot) {
	Box* base = new Box(*this);
	base->SetRootParamNode(newRoot);
	if(newRoot) newRoot->SetParamsBase(base);
	return base;
}
int Box::GetLocalExtents(double extents[6], int timestep){
	const vector<double> defaultExtents(6,0.);
	const vector<long> defaultTimes(1,0);
	const vector<double> exts = GetValueDoubleVec(Box::_extentsTag,defaultExtents);
	const vector<long> times = GetValueLongVec(Box::_timesTag,defaultTimes);
	//If there are times, look for a match.  The first time should be -1
	for (int i = 1; i<times.size(); i++){
		if (times[i] != timestep) continue;
		//Matching time found:
		if (exts.size()< 6*(i+1)) return -1; //error
		//Copy over the desired extents
		for (int j = 0;j<6; j++){
			extents[j] = exts[6*i+j];
		}
		return 0;
	}
	//No match: return first one
	for (int j = 0; j<6; j++){extents[j] = exts[j];}
	return 0;
}
int Box::GetLocalExtents(float extents[6], int timestep){
	double exts[6];
	int rc = GetLocalExtents(exts, timestep);
	if (rc) return rc;
	for (int i = 0; i<6; i++) extents[i] = exts[i];
	return 0;
}
int Box::GetStretchedLocalExtents(double extents[6], int timestep){
	double exts[6];
	int rc = GetLocalExtents(exts, timestep);
	if (rc) return rc;
	const double *stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<6; i++) extents[i] = exts[i]*stretch[i%3];
	return 0;
}
int Box::SetLocalExtents(const vector<double>& extents, Params*p, int timestep){
	const vector<double> defaultExtents(6,0.);
	const vector<long> defaultTimes(1,0);
	const vector<double> curExts = GetValueDoubleVec(_extentsTag,defaultExtents);
	//If setting default and there are no nondefault extents, 
	//Or if the time is not in the list,
	//just replace default extents
	int index = 0;
	
	if ((timestep < 0) || (curExts.size()>6)) {
		//Check for specified timestep
		const vector<long> times = GetValueLongVec(_timesTag,defaultTimes);
		for (int i = 1; i<times.size(); i++){
			if (times[i] == timestep) {
				index = i;
				break;
			}
		}
	}
	// index will point to extents to replace.  Need to duplicate current extents:
	vector<double> copyExtents = vector<double>(curExts);
	
	for (int i = 0; i<6; i++)
		copyExtents[i+index*6] = extents[i];

	return SetValueDouble(_extentsTag, "Set box extents",copyExtents,p);	
}
int Box::SetLocalExtents(const double extents[6],Params*p, int timestep){
	vector<double> exts;
	for (int i = 0; i<6; i++) exts.push_back(extents[i]);
	return SetLocalExtents(exts, p, timestep);
}
int Box::SetLocalExtents(const float extents[6], Params*p, int timestep){
	vector<double> exts;
	for (int i = 0; i<6; i++) exts.push_back((double)extents[i]);
	return SetLocalExtents(exts, p, timestep);
}
int Box::SetStretchedLocalExtents(const double extents[6], Params*p, int timestep){
	vector<double> exts;
	const double* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<6; i++) exts.push_back((double)(extents[i]/stretch[i%3]));
	return SetLocalExtents(exts, p, timestep);
}
int Box::GetUserExtents(double extents[6], size_t timestep){
	int rc = GetLocalExtents(extents, (int)timestep);
	if (rc) return rc;
	if (!DataStatus::getInstance()->getDataMgr()) return -1;
	const vector<double>tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
	//Time-varying extents are just used to get an offset that varies in time.
	for (int i = 0; i<6; i++){
		extents[i] += tvExts[i%3];
	}
	return 0;
}
	
int Box::GetUserExtents(float extents[6], size_t timestep){
	int rc = GetLocalExtents(extents, (int)timestep);
	if (rc) return rc;
	if (!DataStatus::getInstance()->getDataMgr()) return -1;
	const vector<double>tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
	//Time-varying extents are just used to get an offset that varies in time.
	for (int i = 0; i<6; i++){
		extents[i] += tvExts[i%3];
	}
	return 0;
}
void Box::Trim(Params* p,int numTimes){
		if (numTimes > GetTimes().size()) return;
		//Not a user-accessible change, do not put in command queue.
		vector<long> times = GetTimes();
		times.resize(numTimes);
		SetValueLong(Box::_timesTag,"",times, p);
		vector<double> exts; 
		vector<double>defaultExts(6,0.);
		exts = GetValueDoubleVec(Box::_extentsTag,defaultExts);
		SetValueDouble(_extentsTag, "", exts,p);
}