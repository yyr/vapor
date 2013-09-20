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

#include "glutil.h"	// Must be included first!!!
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
const string AnimationParams::_repeatTag = "RepeatPlay";
const string AnimationParams::_maxRateTag = "MaxFrameRate";
const string AnimationParams::_stepSizeTag = "FrameStepSize";
const string AnimationParams::_startFrameTag = "StartFrame";
const string AnimationParams::_endFrameTag = "EndFrame";

const string AnimationParams::_currentFrameTag = "CurrentFrame";
float AnimationParams::defaultMaxFPS = 10.f;

AnimationParams::AnimationParams(int winnum): Params( winnum, Params::_animationParamsTag){
	restart();
}
AnimationParams::~AnimationParams(){
	
}


//Currently nothing "deep" to copy:
Params* AnimationParams::deepCopy(ParamNode*){
	AnimationParams* newParams = new AnimationParams(*this);
	
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
	
	maxTimestep = 100; 
	minTimestep = 1;
	
	currentTimestep = 0;
	
	
	stateChanged = true;
	
}
void AnimationParams::setDefaultPrefs(){
	defaultMaxFPS = 10.f;
}
//Respond to change in Metadata
//
bool AnimationParams::
reinit(bool doOverride){
	
	
	maxTimestep = DataStatus::getInstance()->getDataMgr()->GetNumTimeSteps()-1;
	minTimestep = 0;
	//Narrow the range to the actual data limits:
	//Find the first framenum with data:
	int mints = minTimestep;
	int maxts = maxTimestep;
	int i;
	for (i = minTimestep; i<= maxTimestep; i++){
		
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
		
		currentTimestep = startFrame;
		currentInterpolatedFrame = 0;
		maxFrameRate = defaultMaxFPS;
		
	} else {
		
		if (startFrame > maxts) startFrame = maxts;
		if (startFrame < mints) startFrame = mints;
		if (endFrame < mints) endFrame = mints;
		if (endFrame > maxts) endFrame = maxts;
		
		if (currentTimestep < mints) currentTimestep = mints;
		if (currentTimestep > maxts) currentTimestep = maxts;
		
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
	if (newFrame == getCurrentFrameNumber() ) {
		if (repeatPlay) return false;
		setPlayDirection(0);
		return true;
	}
	//See if direction needs to change:

	if (((newFrame-getCurrentFrameNumber())*playDirection) > 0) {
		//No change in direction
		setCurrentFrameNumber(newFrame);
		return false;
	} else {
		setCurrentFrameNumber(newFrame);
		return true;
	}
}
bool AnimationParams::checkLastFrame(){
	int newFrame = getNextFrame(playDirection);
	if (newFrame == getCurrentFrameNumber() ) {
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
	{
		int i;
		//not using timestep sample list.
		// find next valid frame for which there exists data:
		int testFrame = getCurrentFrameNumber() + dir*frameStepSize;
		DataStatus* ds = DataStatus::getInstance();
		int firstFrame = startFrame;
		int lastFrame = endFrame;
		
		for (i = 1; i<= (lastFrame - firstFrame + frameStepSize)/frameStepSize; i++){
			
			if (testFrame > lastFrame){ 
				if (repeatPlay) testFrame =  firstFrame;
				else testFrame = getCurrentFrameNumber();
			}
			if (testFrame < firstFrame){
				if (repeatPlay) testFrame = lastFrame;
				else testFrame = getCurrentFrameNumber();
			}


			if (ds->dataIsPresent(testFrame))
				break;
			testFrame += dir*frameStepSize;
		}
		//It's OK, or we looped all the way around:
		if(i > ((lastFrame - firstFrame + frameStepSize)/frameStepSize))
			testFrame = getCurrentFrameNumber();
		
		return testFrame;
		
		
	}
}


