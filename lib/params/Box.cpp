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
	GetRootNode()->SetElementDouble(_extentsTag,extents);
	GetRootNode()->SetElementLong(_timesTag,times);
	GetRootNode()->SetElementDouble(_anglesTag,angles);
	GetRootNode()->Attrs()[_typeAttr] = ParamNode::_paramsBaseAttr;
}
int Box::GetExtents(double extents[6], int timestep){
	const vector<double>& exts = GetRootNode()->GetElementDouble(Box::_extentsTag);
	const vector<long>& times = GetRootNode()->GetElementLong(Box::_timesTag);
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
int Box::SetExtents(const vector<double>& extents, int timestep){
	const vector<double>& curExts = GetRootNode()->GetElementDouble(_extentsTag);
	//If setting default and there are no nondefault extents, 
	//Or if the time is not in the list,
	//just replace default extents
	int index = 0;
	
	if ((timestep < 0) || (curExts.size()>6)) {
		//Check for specified timestep
		const vector<long>& times = GetRootNode()->GetElementLong(_timesTag);
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

	return GetRootNode()->SetElementDouble(_extentsTag, copyExtents);	
}
int Box::SetExtents(const double extents[6], int timestep){
	vector<double> exts;
	for (int i = 0; i<6; i++) exts.push_back(extents[i]);
	return SetExtents(exts, timestep);
}