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
#include <vapor/ParamNode.h>


using namespace VAPoR;
const string AnimationParams::_shortName = "Animation";
const string AnimationParams::_repeatAttr = "RepeatPlay";
const string AnimationParams::_maxRateAttr = "MaxFrameRate";
const string AnimationParams::_stepSizeAttr = "FrameStepSize";
const string AnimationParams::_startFrameAttr = "StartFrame";
const string AnimationParams::_endFrameAttr = "EndFrame";
const string AnimationParams::_currentFrameAttr = "CurrentFrame";
const string AnimationParams::_maxWaitAttr = "MaxWait";
float AnimationParams::defaultMaxFPS = 10.f;
float AnimationParams::defaultMaxWait = 6000.f;


AnimationParams::AnimationParams(int winnum): Params( winnum, Params::_animationParamsTag){
	
	restart();
}
AnimationParams::~AnimationParams(){
	for (int i = 0; i<keyframes.size(); i++){
		delete keyframes[i];
	}
	keyframes.clear();
}


//Currently nothing "deep" to copy:
Params* AnimationParams::deepCopy(ParamNode*){
	AnimationParams* newParams = new AnimationParams(*this);
	newParams->keyframes = keyframes;
	for (int i = 0; i<keyframes.size(); i++){
		newParams->keyframes[i] = new Keyframe(*keyframes[i]);
		newParams->keyframes[i]->viewpoint = new Viewpoint(*(keyframes[i]->viewpoint));
	}
	return (Params*)newParams;
}

//Reset to initial state
//
void AnimationParams::
restart(){
	// set everything to default state:
	playDirection = 0;
	repeatPlay = false;
	maxFrameRate = defaultMaxFPS;
	frameStepSize = 1;
	startFrame = 1;
	endFrame = 100;
	maxFrame = 100; 
	minFrame = 1;
	currentFrame = 0;
	maxWait = defaultMaxWait;
	useTimestepSampleList = false;
	timestepList.clear();
	loadedViewpoints.clear();
	loadedTimesteps.clear();
	keyframes.clear();
	useKeyframing=false;
	defaultCameraSpeed = 1.;
	//Insert a default keyframe:
	Viewpoint* vp = new Viewpoint();
	Keyframe* kf = new Keyframe(vp,0., 0, 1);
	keyframes.push_back(kf);
	stateChanged = true;
	
}
void AnimationParams::setDefaultPrefs(){
	defaultMaxFPS = 10.f;
	defaultMaxWait = 6000.f;
}
//Respond to change in Metadata
//
bool AnimationParams::
reinit(bool doOverride){
	//Session* session = Session::getInstance();
	//Make min and max conform to new data:
	//minFrame = (int)DataStatus::getInstance()->getMinTimestep();
	//maxFrame = (int)DataStatus::getInstance()->getMaxTimestep();
	maxFrame = DataStatus::getInstance()->getDataMgr()->GetNumTimeSteps()-1;
	minFrame = 0;
	//Narrow the range to the actual data limits:
	//Find the first framenum with data:
	int mints = minFrame;
	int maxts = maxFrame;
	int i;
	for (i = minFrame; i<= maxFrame; i++){
		
		if(DataStatus::getInstance()->dataIsPresent(i)) break;

	}
	if(i <= maxts) mints = i;
	//Find the last framenum with data:
	for (i = maxts; i>= mints; i--){
		if(DataStatus::getInstance()->dataIsPresent(i)) break;
	}
	
	if(i >= mints) maxts = i;
	//force start & end to be consistent:
	if (doOverride){
		startFrame = mints;
		endFrame = maxts;
		currentFrame = startFrame;
		maxFrameRate = defaultMaxFPS;
		maxWait = defaultMaxWait;
	} else {
		if (startFrame > maxts) startFrame = maxts;
		if (startFrame < mints) startFrame = mints;
		if (endFrame < mints) endFrame = mints;
		if (endFrame > maxts) endFrame = maxts;
		if (currentFrame < startFrame) currentFrame = startFrame;
		if (currentFrame > endFrame) currentFrame = endFrame;
	}
	
	// set pause state
	playDirection = 0;
	//Set the first frame for animation control:
	if (doOverride){
		ViewpointParams* vpParams = (ViewpointParams*)Params::GetParamsInstance(_viewpointParamsTag);
		Viewpoint* startingViewpoint = vpParams->getHomeViewpoint();
		keyframes.clear();
		keyframes.push_back(new Keyframe(startingViewpoint, 0., 0, 1));
	}

	
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
bool AnimationParams::checkLastFrame(){
	int newFrame = getNextFrame(playDirection);
	if (newFrame == currentFrame ) {
		setPlayDirection(0);
		return true;
	}
	else return false;
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
		int i;
		//not using timestep sample list.
		// find next valid frame for which there exists data:
		int testFrame = currentFrame + dir*frameStepSize;
		DataStatus* ds = DataStatus::getInstance();
		for (i = 1; i<= (endFrame - startFrame + frameStepSize)/frameStepSize; i++){
			
			if (testFrame > endFrame){ 
				if (repeatPlay) testFrame =  startFrame;
				else testFrame = currentFrame;
			}
			if (testFrame < startFrame){
				if (repeatPlay) testFrame = endFrame;
				else testFrame = currentFrame;
			}

			if (keyframingEnabled()) break;

			if (ds->dataIsPresent(testFrame)) break;
			testFrame += dir*frameStepSize;
		}
		//It's OK, or we looped all the way around:
		if(i > ((endFrame - startFrame + frameStepSize)/frameStepSize))
			testFrame = currentFrame;
		
		return testFrame;
		
		
	}
}
void AnimationParams::deleteKeyframe(int index){
	delete keyframes[index];
	for (int i = index; i<keyframes.size()-1; i++){
		keyframes[i] = keyframes[i+1];
	}
	keyframes.pop_back();
}
void AnimationParams::insertKeyframe(int index, Keyframe* keyframe){
	if (index >= keyframes.size()-1){
		keyframes.push_back(keyframe);
		return;
	}
	vector<Keyframe*>::iterator it;
	it = keyframes.begin()+index+1;
	keyframes.insert(it,keyframe);
}
bool AnimationParams::
elementStartHandler(ExpatParseMgr* pm, int depth, std::string& tag, const char ** attrs){
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
				if (maxFrameRate <= 0.f) maxFrameRate = 0.001f;
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
		}
		return true;
	}
	pm->skipElement(tag, depth);
	return true;
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
ParamNode* AnimationParams::
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
	oss << (double)maxFrameRate;
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


	ParamNode* animationNode = new ParamNode(_animationParamsTag, attrs, 0);

	//No Children!
	
	return animationNode;
}
