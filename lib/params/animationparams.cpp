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
const string AnimationParams::_startTimestepTag = "StartTimestep";
const string AnimationParams::_endTimestepTag = "EndTimestep";
const string AnimationParams::_currentTimestepTag = "CurrentTimestep";
const string AnimationParams::_minTimestepTag = "MinTimestep";
const string AnimationParams::_maxTimestepTag = "MaxTimestep";
const string AnimationParams::_playDirectionTag = "PlayDirection";

AnimationParams::AnimationParams(XmlNode* parent, int winnum): Params( parent, Params::_animationParamsTag, winnum){
	Command::blockCapture();
	restart();
	Command::unblockCapture();
}
AnimationParams::~AnimationParams(){
	
}


//Reset to initial state
//
void AnimationParams::
restart(){
	
	// set everything to default state:
	setPlayDirection (0);
	setRepeating (false);
	setFrameStepSize(1);
	setStartTimestep(0);
	setEndTimestep (100);
	
	setMaxTimestep(100); 
	setMinTimestep(0);
	
	setCurrentTimestep (0);
	setMaxFrameRate(0.1);
	
}

void AnimationParams::Validate(bool useDefault){
	//Command capturing should be disabled
	assert(!Command::isRecording());
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	setMaxTimestep(dataMgr->GetNumTimeSteps()-1);
	setMinTimestep(0);
	//Narrow the range to the actual data limits:
	//Find the first framenum with data:
	int mints = 0;
	int maxts = dataMgr->GetNumTimeSteps()-1;
	int i;
	for (i = mints; i<= maxts; i++){
		
		if(DataStatus::getInstance()->dataIsPresent(i)) break;

	}
	if(i <= maxts) mints = i;
	//Find the last framenum with data:
	for (i = maxts; i>= mints; i--){
		if(DataStatus::getInstance()->dataIsPresent(i)) break;
	}
	
	if(i >= mints) maxts = i;
	//force start & end to be consistent:
	if (useDefault){
		setStartTimestep(mints);
		setEndTimestep(maxts);
		int bar = getEndTimestep();
		setCurrentTimestep(mints);
		bar = getEndTimestep();
	} else {
		
		if (getStartTimestep() > maxts) setStartTimestep(maxts);
		if (getStartTimestep() < mints) setStartTimestep(mints);
		if (getEndTimestep() > maxts) setEndTimestep(maxts);
		if (getEndTimestep() < mints) setEndTimestep(mints);
		if (getCurrentTimestep() < mints) setCurrentTimestep(mints);
		if (getCurrentTimestep() > maxts) setCurrentTimestep(maxts);
		
	}
	
	// set pause state
	setPlayDirection(0);
	
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
	assert(getPlayDirection() != 0);
	int newFrame = getNextFrame(getPlayDirection());
	if (newFrame == getCurrentTimestep() ) {
		if (isRepeating()) return false;
		setPlayDirection(0);
		return true;
	}
	//See if direction needs to change:

	if (((newFrame-getCurrentTimestep())*getPlayDirection()) > 0) {
		//No change in direction
		setCurrentTimestep(newFrame);
		return false;
	} else {
		setCurrentTimestep(newFrame);
		return true;
	}
}
bool AnimationParams::checkLastFrame(){
	int newFrame = getNextFrame(getPlayDirection());
	if (newFrame == getCurrentTimestep() ) {
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
		int testFrame = getCurrentTimestep() + dir*getFrameStepSize();
		DataStatus* ds = DataStatus::getInstance();
		int firstFrame = getStartTimestep();
		int lastFrame = getEndTimestep();
		
		for (i = 1; i<= (lastFrame - firstFrame + getFrameStepSize())/getFrameStepSize(); i++){
			
			if (testFrame > lastFrame){ 
				if (isRepeating()) testFrame =  firstFrame;
				else testFrame = getCurrentTimestep();
			}
			if (testFrame < firstFrame){
				if (isRepeating()) testFrame = lastFrame;
				else testFrame = getCurrentTimestep();
			}


			if (ds->dataIsPresent(testFrame))
				break;
			testFrame += dir*getFrameStepSize();
		}
		//It's OK, or we looped all the way around:
		if(i > ((lastFrame - firstFrame + getFrameStepSize())/getFrameStepSize()))
			testFrame = getCurrentTimestep();
		
		return testFrame;
		
		
	}
}


