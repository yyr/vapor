//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		animationparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		January 2005
//
//	Description:	Implements the AnimationParams class
//		This is derived from the Params class
//		It contains all the parameters required for animation

//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>


#include "datastatus.h" 
#include "params.h"
#include "animationparams.h"
#include "vapor/XmlNode.h"

using namespace VAPoR;
const string AnimationParams::_repeatAttr = "RepeatPlay";
const string AnimationParams::_maxRateAttr = "MaxFrameRate";
const string AnimationParams::_stepSizeAttr = "FrameStepSize";
const string AnimationParams::_startFrameAttr = "StartFrame";
const string AnimationParams::_endFrameAttr = "EndFrame";
const string AnimationParams::_currentFrameAttr = "CurrentFrame";
const string AnimationParams::_maxWaitAttr = "MaxWait";

AnimationParams::AnimationParams(int winnum): Params( winnum){
	thisParamType = AnimationParamsType;
	
	restart();
}
AnimationParams::~AnimationParams(){}


//Currently nothing "deep" to copy:
Params* AnimationParams::deepCopy(){
	return (Params*)new AnimationParams(*this);
}

//Reset to initial state
//
void AnimationParams::
restart(){
	// set everything to default state:
	playDirection = 0;
	repeatPlay = false;
	maxFrameRate = 10; 
	frameStepSize = 1;
	startFrame = 1;
	endFrame = 100;
	maxFrame = 100; 
	minFrame = 1;
	currentFrame = 0;
	maxWait = 60.f;
	useTimestepSampleList = false;
	timestepList.clear();
	stateChanged = true;
	
}
//Respond to change in Metadata
//
bool AnimationParams::
reinit(bool doOverride){
	//Session* session = Session::getInstance();
	//Make min and max conform to new data:
	minFrame = (int)DataStatus::getInstance()->getMinTimestep();
	maxFrame = (int)DataStatus::getInstance()->getMaxTimestep();
	//Narrow the range to the actual data limits:
	int numvars = DataStatus::getInstance()->getNumSessionVariables();
	//Find the first framenum with data:
	int i;
	for (i = minFrame; i<= maxFrame; i++){
		int varnum;
		for (varnum = 0; varnum<numvars; varnum++){
			if(DataStatus::getInstance()->dataIsPresent(varnum, i)) break;
		}
		if (varnum < numvars) break;
	}
	minFrame = i;
	//Find the last framenum with data:
	for (i = maxFrame; i>= minFrame; i--){
		int varnum;
		for (varnum = 0; varnum<numvars; varnum++){
			if(DataStatus::getInstance()->dataIsPresent(varnum, i)) break;
		}
		if (varnum < numvars) break;
	}
	
	maxFrame = i;
	//force start & end to be consistent:
	if (doOverride){
		startFrame = minFrame;
		endFrame = maxFrame;
		currentFrame = startFrame;
	} else {
		if (startFrame > maxFrame) startFrame = maxFrame;
		if (startFrame < minFrame) startFrame = minFrame;
		if (endFrame < minFrame) endFrame = minFrame;
		if (endFrame > maxFrame) endFrame = maxFrame;
		if (currentFrame < startFrame) currentFrame = startFrame;
		if (currentFrame > endFrame) currentFrame = endFrame;
	}
	
	// set pause state
	playDirection = 0;

	
	return true;
}

//Following method called when a frame has been rendered, to update the currentFrame, in
//preparation for next rendering.
//It should not be called if the UI has changed the frame number; in that case
//setDirty should be called, and AnimationController::paramsChanged() should be
//called, to force any ongoing animation to not increment the frame number
//This returns true if the caller needs to set the change bit, because 
//we have changed to "pause" status
bool AnimationParams::
advanceFrame(){
	assert(playDirection);
	int newFrame = getNextFrame(playDirection);
	if (newFrame == currentFrame ) {
		if (repeatPlay) return false;
		setPlayDirection(0);
		return true;
	}
	//See if direction needs to change:

	if (((newFrame-currentFrame)*playDirection) > 0) {
		//No change in direction
		currentFrame = newFrame;
		return false;
	} else {
		currentFrame = newFrame;
		return true;
	}
}
//Determine the next frame.  dir must be 1 or -1
//Returns currentTimestep if no change.
//If value returned is in opposite direction from dir, then there needs to
//be a direction change
int AnimationParams::
getNextFrame(int dir){
	//If we are using timestep list, find the current frame in that list:
	if(usingTimestepList() && timestepList.size() > 0){
		int prevIndex = -1; 
		int nextIndex = -1;
		int firstValidFrame = -1;
		int lastValidFrame = -1;
		
		for (int i = 0; i< timestepList.size(); i++){
			if (timestepList[i] >= startFrame && timestepList[i] <= endFrame){
				if (firstValidFrame < 0) firstValidFrame = timestepList[i];
				lastValidFrame = timestepList[i];
			} else continue;  //Ignore any timesteps outside the valid range.
			if (timestepList[i] < currentFrame) prevIndex = i; //before
			else if (timestepList[i] == currentFrame) continue;//at
			else if (nextIndex < 0) nextIndex = i;//beyond
		}
		if (lastValidFrame < 0) {
			//no valid frames
			return currentFrame;
		}
		//is currentFrame at or past the end/beginning?
		if (dir > 0 && (nextIndex < 0) ){ 
			if (repeatPlay)  return firstValidFrame;
			else return currentFrame;
		} 
		if (dir < 0 && prevIndex < 0)	{
			if (repeatPlay) return lastValidFrame;
			else return currentFrame;
		}
		//If we got this far, must be inside the valid range.
		if (dir > 0) return(timestepList[nextIndex]); 
		else return(timestepList[prevIndex]);
				
	} else {
		//not using timestep sample list.
		// find next valid frame for which there exists data:
		int testFrame = currentFrame + dir*frameStepSize;
		DataStatus* ds = DataStatus::getInstance();
		for (int i = 1; i<= (endFrame - startFrame + frameStepSize)/frameStepSize; i++){
			
			if (testFrame > endFrame){ 
				if (repeatPlay) testFrame =  startFrame;
				else testFrame = currentFrame;
			}
			if (testFrame < startFrame){
				if (repeatPlay) testFrame = endFrame;
				else testFrame = currentFrame;
			}
			if (ds->dataIsPresent(testFrame)) break;
			testFrame += dir*frameStepSize;
		}
		//It's OK, or we looped all the way around:
		if(i > ((endFrame - startFrame + frameStepSize)/frameStepSize))
			testFrame = currentFrame;
		
		return testFrame;
		
		
	}
}

bool AnimationParams::
elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& tag, const char ** attrs){
	if (StrCmpNoCase(tag, _animationParamsTag) == 0) {
		//If it's a Animation params tag, save 7 attributes (2 are from Params class)
		//Do this by repeatedly pulling off the attribute name and value
		useTimestepSampleList = false;
		timestepList.clear();
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _vizNumAttr) == 0) {
				ist >> vizNum;
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				if (value == "true") setLocal(true); else setLocal(false);
			}
			else if (StrCmpNoCase(attribName, _repeatAttr) == 0) {
				if (value == "true") repeatPlay = true; else repeatPlay = false;
			}
			else if (StrCmpNoCase(attribName, _stepSizeAttr) == 0) {
				ist >> frameStepSize;
			}
			else if (StrCmpNoCase(attribName, _maxRateAttr) == 0) {
				ist >> maxFrameRate;
			}
			else if (StrCmpNoCase(attribName, _maxWaitAttr) == 0) {
				ist >> maxWait;
			}
			else if (StrCmpNoCase(attribName, _startFrameAttr) == 0) {
				ist >> startFrame;
				if (startFrame < minFrame) minFrame = startFrame;
			}
			else if (StrCmpNoCase(attribName, _endFrameAttr) == 0) {
				ist >> endFrame;
				if (endFrame > maxFrame) maxFrame = endFrame;
			}
			else if (StrCmpNoCase(attribName, _currentFrameAttr) == 0) {
				ist >> currentFrame;
			}
			else if (StrCmpNoCase(attribName, _useTimestepSampleListAttr) == 0){
				if (value == "true") useTimestepSampleList = true; else useTimestepSampleList = false;
			}
			else if (StrCmpNoCase(attribName, _timestepSampleListAttr) == 0){
				//The first value is the list size, followed by the entries in the list:
				int listLength, tstep;
				ist >> listLength;
				for (int i = 0; i<listLength; i++){
					ist >> tstep;
					timestepList.push_back(tstep);
				}
			}
			else return false;
		}
		return true;
	}
	return false;  //Not an animationParams tag
}
bool AnimationParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	//Check only for the animation params tag
	if (StrCmpNoCase(tag, _animationParamsTag) == 0) {
		//If this is animation params, need to
		//pop the parse stack.  
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} 
	else {
		pm->parseError("Unrecognized end tag in AnimationParams %s",tag.c_str());
		return false;  //Could there be other end tags that we ignore??
	}
	return true;  
}
XmlNode* AnimationParams::
buildNode(){
		//Construct the animation node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	if (local)
		oss << "true";
	else 
		oss << "false";
	attrs[_localAttr] = oss.str();

	oss.str(empty);
	oss << (long)maxFrameRate;
	attrs[_maxRateAttr] = oss.str();

	oss.str(empty);
	oss << (long)maxWait;
	attrs[_maxWaitAttr] = oss.str();

	oss.str(empty);
	oss << (long)frameStepSize;
	attrs[_stepSizeAttr] = oss.str();

	oss.str(empty);
	oss << (long)startFrame;
	attrs[_startFrameAttr] = oss.str();

	oss.str(empty);
	oss << (long)endFrame;
	attrs[_endFrameAttr] = oss.str();

	oss.str(empty);
	oss << (long)currentFrame;
	attrs[_currentFrameAttr] = oss.str();

	oss.str(empty);
	if (repeatPlay)
		oss << "true";
	else 
		oss << "false";
	attrs[_repeatAttr] = oss.str();

	oss.str(empty);
	if (useTimestepSampleList){
		oss << "true";
	} else {
		oss << "false";
	}
	attrs[_useTimestepSampleListAttr] =  oss.str();

	if (timestepList.size()> 0){
		oss.str(empty);
		//First entry is the size of the list, followed by the values in the list
		oss << (long) timestepList.size();
		for (int i = 0; i<timestepList.size(); i++){
			oss<<" "<<(long)timestepList[i];
		}
		attrs[_timestepSampleListAttr] = oss.str();
	}


	XmlNode* animationNode = new XmlNode(_animationParamsTag, attrs, 0);

	//No Children!
	
	return animationNode;
}
