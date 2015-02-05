//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		regionparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Implements the RegionParams class.
//		This class supports parameters associted with the
//		region panel
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include "regionparams.h"
#include <vapor/errorcodes.h>

#include "datastatus.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "command.h"
#include <vapor/DataMgr.h>
#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>


using namespace VAPoR;

const string RegionParams::_shortName = "Region";

RegionParams::RegionParams(XmlNode* parent, int winnum): Params(parent, Params::_regionParamsTag, winnum){
	Command::blockCapture();
	restart();
	Command::unblockCapture();
}


RegionParams::~RegionParams(){
	clearRegionsMap();
}


//Reset region settings to initial state
void RegionParams::
restart(){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds || !ds->getDataMgr()) ds = 0;
	
	
	Box* myBox = new Box();
	vector<string>path;
	path.push_back(Box::_boxTag);
	SetParamsBase(path, myBox);
	
	vector<double> regexts;
	for (int i = 0; i<3; i++) regexts.push_back(0.);
	for (int i = 0; i<3; i++) regexts.push_back(1.);
	GetBox()->SetLocalExtents(regexts,this);
	
}
//Reinitialize region settings, session has changed
//Need to force the regionMin, regionMax to be OK.
void RegionParams::
Validate(int type){
	//Command capturing should be disabled
	assert(!Command::isRecording());
	bool doOverride = (type == 0);
	int i;
	
	const double* extents = DataStatus::getInstance()->getLocalExtents();
	
	double regionExtents[6];
	vector<double> exts(3,0.0);
	if (doOverride) {
		for (i = 0; i< 3; i++) {
			exts.push_back( extents[i+3]-extents[i]);
		}
		clearRegionsMap();
		GetBox()->SetLocalExtents(exts,this);
		GetBox()->Trim(this);
	} else {
		//ensure all the local time-extents to be valid
		const vector<long>& times = GetBox()->GetTimes();
		for (int timenum = 0; timenum< times.size(); timenum++){
			int currTime = times[timenum];
			GetBox()->GetLocalExtents(regionExtents,currTime);
			
			//force them to fit in current volume 
			for (i = 0; i< 3; i++) {
				
				if (regionExtents[i] > extents[i+3]-extents[i])
					regionExtents[i] = extents[i+3]-extents[i];
				if (regionExtents[i] < 0.)
					regionExtents[i] = 0.;
				if (regionExtents[i+3] > extents[i+3]-extents[i])
					regionExtents[i+3] = extents[i+3]-extents[i];
				if (regionExtents[i+3] < 0.)
					regionExtents[i+3] = 0.;
				if (regionExtents[i] > regionExtents[i+3]) 
					regionExtents[i+3] = regionExtents[i];
			}
			exts.clear();
			for (int j = 0; j< 6; j++) exts.push_back(regionExtents[j]);
			GetBox()->SetLocalExtents(exts,this,currTime);
		}	
	}
	
	return;	
	
}

int RegionParams::SetLocalRegionMin(int coord, double minval, int timestep){
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	int rc = 0;
	
	double exts[6];
	GetBox()->GetLocalExtents(exts, timestep);
	exts[coord] = minval;
	return GetBox()->SetLocalExtents(exts, this,timestep);
}
int RegionParams::SetLocalRegionMax(int coord, double maxval, int timestep){
	double exts[6];
	GetBox()->GetLocalExtents(exts, timestep);
	exts[coord+3] = maxval;
	return GetBox()->SetLocalExtents(exts, this,timestep);

}

void RegionParams::clearRegionsMap(){
	GetBox()->Trim(this);
}
bool RegionParams::insertTime(int timestep){
	if (timestep<0) return false;
	const vector<long>& times = GetTimes();
	int index = 0;
	for (int i = 1; i<times.size(); i++){
		if (times[i] == timestep){
			index = i;
			break;
		}
	}
	if (index != 0) return false;
	Command* cmd = Command::CaptureStart(this, "Insert time in region list");
	const vector<double>& extents = GetAllExtents();
	vector<long> copyTimes = vector<long>(times);
	vector<double>copyExts = vector<double>(extents);
	copyTimes.push_back(timestep);
	//Set the new extents to default extents:
	for (int i = 0; i<6; i++) copyExts.push_back(extents[i]);
	GetBox()->SetValueLong(Box::_timesTag, "",copyTimes, this);
	GetBox()->SetValueDouble(Box::_extentsTag,"", copyExts, this);
	Validate(2);
	Command::CaptureEnd(cmd,this);
	return true;
}
bool RegionParams::removeTime(int timestep){
	if (timestep<0) return false;
	const vector<long>& times = GetTimes();
	int index = 0;
	for (int i = 1; i<times.size(); i++){
		if (times[i] == timestep){
			index = i;
			break;
		}
	}
	if (index == 0) return false;
	const vector<double>& extents = GetAllExtents();
	vector<long> copyTimes = vector<long>(times);
	vector<double>copyExts = vector<double>(extents);
	vector<long>::iterator itlong = copyTimes.begin()+index;
	vector<double>::iterator itdbl = copyExts.begin()+6*index;
	copyTimes.erase(itlong);
	copyExts.erase(itdbl,itdbl+5);
	return true;
}
