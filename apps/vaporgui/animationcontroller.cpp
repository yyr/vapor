//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		animationcontroller.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		January 2005
//
//	Description:	Implements the AnimationController class
//		


#include <qwaitcondition.h>
#include <qthread.h>
#include <qmutex.h>
#include "animationcontroller.h"
using namespace VAPoR;


AnimationController::AnimationController(): QThread(){
	myWaitCondition = new QWaitCondition();
}

AnimationController::~AnimationController(){
	delete myWaitCondition;
}
	

//The run method continues as long as rendering needs to be performed.
void AnimationController::
run(){
	//Outer loop continues forever
	while (1){
		//Is a new rendering necessary?
		if (!advanceFrame()) {
			//Wait a second, 
			sleep(1);
			//then try again
			continue;
		}

		//Lock things, in case some renderer is stranded!
		animationMutex.lock();
		//Clear out the flags:
		
		animationMutex.unlock();
		//set the parameters for the next rendering
		//tell all the renderers to update
		//sleep for minimum time.
		//see if rendering is done:
		animationMutex.lock();
		//Test if all started renderings have completed:
		
	}
}

//Renderers call the following two methods:
void AnimationController::
beginRendering(int vizNum){
	//Get the mutex:
	animationMutex.lock();
	assert(renderStarted == false &&
		renderEnded == false);
	renderStarted = true;
	animationMutex.unlock();
}


void AnimationController::
endRendering(int vizNum){
	animationMutex.lock();
	assert(renderStarted == true &&
		renderEnded == false);
	renderEnded = true;
	//Check if I was the last one to finish.  If so I must restart the
	//controller thread.
	animationMutex.unlock();
}


// advanceFrame is called by run() every time it's time
// to do another frame.  Contains the frame-advance logic based on internal state.
// Returns true if a frame change is called for, false if
// further user action is needed before the frame will change.
// The resulting value in currentFrameNumber is the one that needs to be
// rendered.
//
bool AnimationController::
advanceFrame(){
	//Check if UI has changed the frame number
	if (currentFrameNumber == nextFrameNumber){
		if (paused || (currentFrameRate == 0)) return false;
		if(!repeating){ //check if we are finishing a one-time-through
			if (currentFrameRate >0 && currentFrameNumber >= endFrame)
				return false;
			if (currentFrameRate < 0 && currentFrameNumber <= startFrame)
				return false;
		}
		currentFrameNumber += currentFrameRate;
		if (currentFrameNumber > endFrame){
			assert(currentFrameRate > 0);
			if (repeating) currentFrameNumber = startFrame;
			else currentFrameNumber = endFrame;
		}
		if (currentFrameNumber < startFrame){
			assert(currentFrameRate < 0);
			if (repeating) currentFrameNumber = endFrame;
			else currentFrameNumber = startFrame;
		}
		nextFrameNumber = currentFrameNumber;
		return true;
	} else { 
		//User has changed the frame number.  We always need to
		//perform a rendering
		currentFrameNumber = nextFrameNumber;
		assert(currentFrameNumber >= startFrame && currentFrameNumber <= endFrame);
		return true;
	}
}
// Call wakeup if the controller has work to do
//
void AnimationController::
wakeup(){
	myWaitCondition->wakeAll();
}
	
	




