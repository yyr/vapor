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
#include <qdatetime.h>
#include "animationcontroller.h"
#include "vizwinmgr.h"
#include "vizwin.h"
using namespace VAPoR;

//Initialize the singleton to 0:
AnimationController* AnimationController::theAnimationController = 0;

AnimationController::AnimationController(): QThread(){
	myWaitCondition = new QWaitCondition();
	myClock = new QTime();
	controllerActive = false;
	animationCancelled = true;
	//Initialize status flags 
	for (int i = 0; i<MAXVIZWINS; i++) {
		statusFlag[i] = inactive;
		startTime[i] = 0;
	}
}

AnimationController::~AnimationController(){
	animationCancelled = true;
	myWaitCondition->wakeAll();
	//Wait up to 20 seconds for renderings to complete:
	if (!wait(20000)){
		terminate();
		if (!wait(1000)) assert(0);//wait 1 sec more for terminate to succeed
	}
	delete myWaitCondition;
}

//Set up when new data is read, by the Session
void AnimationController::
restart(){
	int i;
	//If animation in progress, terminate it
	animationCancelled = true;
	//Wakeup the controller thread.  It should return after rendering is complete
	myWaitCondition->wakeAll();
	//Wait until controller thread completes:
	for (i = 0; i<1000; i++){
		if (!controllerActive) break;
		wait(100);
	}
	assert(i<1000);
	//restart the controller thread (calls run()) after it returns.
	animationCancelled = false;
	myClock->start();
	start(QThread::IdlePriority);
}

// The run method continues as long as rendering needs to be performed.
// It will return if animationCancelled is set to true,
// after all started renderings are done.
//
void AnimationController::
run(){
	VizWinMgr* myVizWinMgr = VizWinMgr::getInstance();
	int viznum;
	controllerActive = true;
	//First, initialize the status bits:
	
	animationMutex.lock();
	for (viznum = 0; viznum<MAXVIZWINS; viznum++) {
		//begin with inactive/finished status:
		statusFlag[viznum] = finished;
		//set the shared bit:
		if (!VizWinMgr::getInstance()->getAnimationParams(viznum)->isLocal())
			setGlobal(viznum);
	}

	animationMutex.unlock();
	//Outer loop continues until animationCancelled is set
	while (1){
		if (animationCancelled) break;
		int currentTime = myClock->elapsed();

		//Loop over active, shared visualizers:
		int numStarted = 0;
		int numUnstarted = 0;
		int numRendering = 0;
		int minSharedTimeToFinish = 1000000;
		int timeToRecheck = 30;
		
		animationMutex.lock();
		for (viznum = 0; viznum < MAXVIZWINS; viznum++){
			
			//Find how many have started, how many have not
			//Determine min number of millisecs to desired completion time.  If this
			//value is <=0, they are late (late = true), otherwise save this no. of millisecs
			//Note whether or not any started, shared visualizers have not finished.
			if (isActive(viznum)&&isShared(viznum)){
				int timeToFinish = getTimeToFinish(viznum, currentTime);
				if (timeToFinish < minSharedTimeToFinish) minSharedTimeToFinish = timeToFinish;
				if (renderStarted(viznum)) {
					numStarted++;
					if (!renderFinished(viznum)) numRendering++;
				}
				if (renderUnstarted(viznum))numUnstarted++;
			}
		}
		//bool late = (minSharedTimeToFinish <= 0);	
		//If finish time is positive, need to come back later to recheck:
		if (minSharedTimeToFinish > 0) timeToRecheck = minSharedTimeToFinish;
		//If (late), set overdue bit on active rendering windows.  
		//This will cause them to wake up the controller when they finish.
		// If, in addition,
		// some have started and some have not, change all the unstarted to finished.
		//
		if (minSharedTimeToFinish <= 0){
			for (viznum = 0; viznum < MAXVIZWINS; viznum++){
				if (isActive(viznum)&&isShared(viznum)){
					if (renderStarted(viznum)) setOverdue(viznum);
					if (renderUnstarted(viznum)&&(numRendering >0)&&(numUnstarted>0))
						setFinishRender(viznum);
				}
			}
		}
		//Check the time for unfinished unshared renderers.  Note if they affect
		//the wakeup time, or if they are overdue.
		for (viznum = 0; viznum<MAXVIZWINS; viznum++){
			if (isActive(viznum)&&!isShared(viznum) &&!renderFinished(viznum)){
				int timeToFinish = getTimeToFinish(viznum, currentTime);
				if (timeToFinish <= 0) setOverdue(viznum);
				else if (timeToFinish < timeToRecheck) timeToRecheck = timeToFinish;
			
			}
		}

		//Then loop over all active, finished visualizers, shared or not.  Decide
		//Whether to restart them or to deactivate them, or to just wait longer
		
		for (viznum = 0; viznum<MAXVIZWINS; viznum++){
			if (isActive(viznum)&&renderFinished(viznum)){
				//If a visualizer is shared, and if
				//all started, shared visualizers have finished, and if they are late, then
				//Either deactivate, or Call startVisualizer(viznum).
				//Do nothing otherwise -- eventually the shared renderers
				// will finish and be due to render again
				if (isShared(viznum) && numRendering == 0 && (minSharedTimeToFinish <= 0)){
					if ((!myVizWinMgr->getVizWin(viznum)) ||
						(!myVizWinMgr->getAnimationParams(viznum)->isPlaying()) ||
						(!myVizWinMgr->getDvrParams(viznum)->isEnabled())){
						deActivate(viznum);
					} else {//restart them.
						int renderTime = myVizWinMgr->getAnimationParams(viznum)->getMinTimeToRender();
						startVisualizer(viznum, currentTime);
						if (timeToRecheck > renderTime) timeToRecheck = renderTime;
						setRequestRender(viznum);
						
					}
				} else if (!isShared(viznum)) {
					//If a visualizer is local, finished, and late, do the same:
					if ((!myVizWinMgr->getVizWin(viznum)) ||
						(!myVizWinMgr->getAnimationParams(viznum)->isPlaying()) ||
						(!myVizWinMgr->getDvrParams(viznum)->isEnabled())){
						//deactivate stopped animations
						deActivate(viznum);
					} else { //start it if it's ready to go
						int timeToFinish = getTimeToFinish(viznum, currentTime);
						if (timeToFinish <= 0) {
							int renderTime = myVizWinMgr->getAnimationParams(viznum)->getMinTimeToRender();
							startVisualizer(viznum, currentTime);
							if (timeToRecheck > renderTime) timeToRecheck = renderTime;
							setRequestRender(viznum);
						} else if (timeToFinish < timeToRecheck){
							//If we don't restart, remember when to recheck:
							timeToRecheck = timeToFinish;
						}
					}
				}// !isShared(viznum)
			}//isActive(viznum)
		}//loop over viznum
		
		animationMutex.unlock();
		//msleep(300);
		myWaitCondition->wait(timeToRecheck);
	}//end while(1) loop
	//Only exit loop if someone set animationCancelled; wait for completion, then return
	bool allDone;
	int tries;
	for (tries = 0; tries < 100; tries++){
		//See if any renderings are still in progress; make sure
		//to cancel unstarted renderings:
		allDone = true;
		
		animationMutex.lock();
		for (viznum = 0; viznum<MAXVIZWINS; viznum++) {
			if (!isActive(viznum)) continue;
			if (renderUnstarted(viznum)) setFinishRender(viznum);
			if (renderStarted(viznum) && !renderFinished(viznum)) {
				allDone = false;
				break;
			}
		}
		
		animationMutex.unlock();
		if (allDone) {
			controllerActive = false;
			break;
		}
		//wait for a second; may be woken if someone finishes, or status changes.
		myWaitCondition->wait(1000);
	}
	//Assert that all renderers completed in 60 seconds
	assert(tries < 60);
	return;

}

//Renderers call the following methods before and after rendering.
// If beginRendering returns true, the controller is tracking the rendering, so
// endRendering must be called.  Otherwise, the rendering is not being timed,
// and endRendering should not be called.
//
bool AnimationController::
beginRendering(int viznum){
	if (animationCancelled) return false;
	//Get the mutex:
	
	animationMutex.lock();
	// check if this window is animating, if not just go ahead and render individual frames
	if (!isActive(viznum)) { 
		
		animationMutex.unlock();
		return true;
	}
	//We had better be in the "unstarted" state. or "finished" state.
	//If the controller put us into
	//"finished" state then don't start rendering this frame.
	assert(!renderStarted(viznum));  // not in the "started" state
	if (!renderUnstarted(viznum)) {
		
		animationMutex.unlock();
		return false;
	}
	//OK now change status bits:
	setStartRender(viznum);
	
	animationMutex.unlock();
	return true;
}

//When the rendering is complete, need to change state, setup for next frame.
//
void AnimationController::
endRendering(int vizNum){
	VizWinMgr* myVizWinMgr = VizWinMgr::getInstance();
	animationMutex.lock();
	
	// check if this window is animating, if not just leave
	if (!isActive(vizNum)) { 
		
		animationMutex.unlock();
		return;
	}
	assert(renderStarted(vizNum));
	//Set the flag indicating this rendering is done.
	setFinishRender(vizNum);
	
	//Check if I was the last one to finish.  (this is always true for unshared renderers)
	//If so I advance the frame, and maybe wake up the
	//controller thread.  (check the overdue bit)
	
	bool lastToFinish = true;
	if (isShared(vizNum)) {
		//This is the last one to finish if there are no shared unfinished renderings
		//
		for (int i = 0; i<MAXVIZWINS; i++){
			if (isActive(i) && isShared(i) && renderStarted(i)){
				lastToFinish = false;
				break;
			}
		}
	}
	if (lastToFinish){  //possibly update frame:
		//Check change bit.  If set don't update the currentFrame.  Also change
		//the shared bit if it has changed in the gui.
		if (isChanged(vizNum)){
			clearChangeBit(vizNum);
			//Need to update local/global status if change bit was set.
			if (myVizWinMgr->getAnimationParams(vizNum)->isLocal())
				setLocal(vizNum);
			else setGlobal(vizNum);
		} else {
			//Note if the change bit needs to be set:
			bool setChange = myVizWinMgr->getAnimationParams(vizNum)->advanceFrame();
			if (setChange) setChangeBitsLocked(vizNum);
		}
	}
	bool doWake = (lastToFinish && isOverdue(vizNum));
	
	animationMutex.unlock();
	
	//msleep(300);
	//Do this out of the mutex:
	if (doWake) wakeup();
}

// Call wakeup if the controller has work to do
//
void AnimationController::
wakeup(){
	myWaitCondition->wakeAll();
}
//Determine how much time is left to minimum completion,
//based on the most recent render request.
//
int AnimationController::
getTimeToFinish(int viznum, int currentTime){
	return(startTime[viznum] + VizWinMgr::getInstance()->getAnimationParams(viznum)->getMinTimeToRender()- currentTime);
}
//Tell a renderer to start at the next rendering:
//
void AnimationController::
startVisualizer(int viznum, int currentTime){
	startTime[viznum] = currentTime;
	VizWinMgr::getInstance()->getVizWin(viznum)->setRegionDirty(true);
	VizWinMgr::getInstance()->getVizWin(viznum)->updateGL();
}
//Activate renderer prior to starting play
void AnimationController::
startPlay(int viznum) {
	
	animationMutex.lock();
	assert(!isActive(viznum));
	activate(viznum);
	setFinishRender(viznum);
	if (VizWinMgr::getInstance()->getAnimationParams(viznum)->isLocal()) setLocal(viznum);
	else setGlobal(viznum);
	
	animationMutex.unlock();
}
//Set the change bits for all visualizers that are sharing the animation params
//of the specified visualizer.  Calling routine should have already locked the mutex.
void AnimationController::
setChangeBitsLocked(int viznum){
	setChangeBit(viznum);
	//If another viz is using these animation params, set their change bit, too
	AnimationParams* aParams = VizWinMgr::getInstance()->getAnimationParams(viznum);
	if (aParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  (VizWinMgr::getInstance()->getVizWin(i) && (i != viznum)  &&
				!VizWinMgr::getInstance()->getAnimationParams(viznum)->isLocal() )
		{
			setChangeBit(i);
		}
	}
}


	




