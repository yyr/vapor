//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		sharedcontrollerthread.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2005
//
//	Description:	Implements the SharedControllerThread class
//			Functionality of that class is described in controllerthread.h
//	
//		


#include <qwaitcondition.h>
#include <qthread.h>
#include <qmutex.h>
#include <qdatetime.h>
#include <qregion.h>
#include <qmessagebox.h>
#include "animationcontroller.h"
#include "controllerthread.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "messagereporter.h"
#include "glutil.h"
#include "dvrparams.h"
#include "animationeventrouter.h"
using namespace VAPoR;

SharedControllerThread::SharedControllerThread() : QThread(){
	myWaitCondition = new QWaitCondition();
	controllerActive = false;
}

SharedControllerThread::~SharedControllerThread(){
	animationCancelled = true;
	myWaitCondition->wakeAll();
	//Wait up to 20 seconds for renderings to complete:
	if (!wait(20000)){
		//qWarning("terminating thread");
		terminate();
		if (!wait(20000)) 
			Params::BailOut("Excessive wait for animation thread termination",__FILE__,__LINE__);
	}
	//qWarning("deleting wait condition");
	delete myWaitCondition;
}

//Set up when new data is read, by the Session
void SharedControllerThread::
restart(){
	int i;
	//If animation in progress, terminate it
	animationCancelled = true;
	//Wakeup the controller thread.  It should return after rendering is complete
	myWaitCondition->wakeAll();
	//Wait until controller thread completes:
	for (i = 0; i<100; i++){
		if (!controllerActive) break;
		wait(20000);
	}
	if( i>= 100) Params::BailOut("Excessive wait for animation thread completion",__FILE__,__LINE__);
	//restart the controller thread (calls run()) after it returns.
	animationCancelled = false;
	start(QThread::IdlePriority);
}




// Call wakeup if the controller has work to do
//
void SharedControllerThread::
wakeup(){
	myWaitCondition->wakeAll();
}

// This version of run() is just for managing global renderers!
// The run method continues as long as rendering needs to be performed.
// It will return if animationCancelled is set to true,
// after all started renderings are done.
//
void SharedControllerThread::
run(){
	VizWinMgr* myVizWinMgr = VizWinMgr::getInstance();
	AnimationController* myAnimationController = AnimationController::getInstance();
	int viznum;
	controllerActive = true;

	
	//Keep track of the most recent number of visualizers that are not
	//getting updated.  Wait at most a second before identifying a renderer as "sleeping"
	//
	int numSleeping = 0;
	//Outer loop continues until animationCancelled is set
	//
	while (1){
		if (animationCancelled) break;
		int currentTime = myAnimationController->myClock->elapsed();
		int frameWaitTime = (int)(1000.f*myAnimationController->getMaxSharedWait() + 0.5f);
		//Loop over active, global visualizers:
		int minSharedTimeToFinish = 1000000;
		int numActive= 0;
		int numNotHidden = 0;
		
		myAnimationController->animationMutex.lock();
		
		for (viznum = 0; viznum < MAXVIZWINS; viznum++){
			//Do a census of global visualizers
			//also Determine min number of millisecs to desired completion time.
			//count how many are not hidden
			if (myAnimationController->isActive(viznum) && myAnimationController->isShared(viznum)){
				numActive++;
				VizWin* myWin = myVizWinMgr->getVizWin(viznum);
				if (!myWin || (!myWin->isHidden() && !myWin->isMinimized()&& myWin->isVisible()))
					numNotHidden++;
				int timeToFinish = myAnimationController->getTimeToFinish(viznum, currentTime);
				if (timeToFinish < minSharedTimeToFinish) minSharedTimeToFinish = timeToFinish;
				//qWarning("window %d finishtime %d minfinishtime %d", viznum, timeToFinish, minSharedTimeToFinish);
			}
		}
		//qWarning(" %d windows are non hidden, %d are active",numNotHidden, numActive);
		//If no shared renderers are active, wait 1 second and retry:
		if( numNotHidden == 0) {
			myAnimationController->animationMutex.unlock();
			//qWarning("Waiting for an active renderer to start");
// qt4 mustfix			myWaitCondition->wait(IDLE_WAIT);
			numSleeping = 0;
			continue;
		}
		//If the time to finish is positive, then wait:
		//If finish time is positive, need to come back later to recheck:
		int finishTime = minSharedTimeToFinish;
		while (finishTime > 0) {
			myAnimationController->animationMutex.unlock();
			
// QT4 mustfix			myWaitCondition->wait(finishTime);
			int timeSinceStart = myAnimationController->myClock->elapsed()-currentTime;
			finishTime = minSharedTimeToFinish - timeSinceStart;
			myAnimationController->animationMutex.lock();
		}
		// Now we have waited long enough. 
		// Wait for at all the unhidden ones to have started.  
		// Wait one second at a time as long as we continue to get
		// more renderers started or finished
		int tries;
		int maxStarted = 0;
		int maxFinished = 0;
		int missingViz = -1;
		
		for (tries = 0; tries< MAXVIZWINS; tries++){
			int numStarted = 0;
			int numFinished = 0;
			//This flag will be reset to true if we are making progress
			//i.e. if widgets are continuing to start and finish rendering.
			for (viznum = 0; viznum < MAXVIZWINS; viznum++){
				if (myAnimationController->isActive(viznum)&&myAnimationController->isShared(viznum)){
					if (myAnimationController->renderStarted(viznum)){
						numStarted++;
						if (numStarted > maxStarted){
							maxStarted = numStarted;
						}
						//mark every started renderer as being overdue,
						//so it will notify controller if/when it finishes
						myAnimationController->setOverdue(viznum);
					}
					else if (myAnimationController->renderFinished(viznum)){
						numFinished++;
						if (numFinished > maxFinished) {
							maxFinished = numFinished;
						}
					}
					else missingViz = viznum;
				}
			}
			if((numStarted+numFinished+numSleeping) >= numNotHidden) {
				//Update (increase) the numSleeping count
				//
				numSleeping = numNotHidden - numFinished - numStarted;
				break;
			}
			//If we are waiting more than a second for someone to start,
			//update the number of "sleeping" renderers
			//Don't do this more than MAXVIZWINS times
			assert(tries <MAXVIZWINS-1);
			currentTime = myAnimationController->myClock->elapsed();
			//Is the missing visualizer more than a second late?
			//Don't wait longer!
			if(myAnimationController->getTimeToFinish(missingViz, currentTime)< 
					-frameWaitTime){
				//qWarning(" visualizer %d is more than max shared wait late", missingViz);
				numSleeping = numNotHidden - numFinished - numStarted;
				break;
			}
			myAnimationController->animationMutex.unlock();
			//qWarning("Waiting because started %d < %d ", numStarted, numNotHidden);
// QT4 mustfix			myWaitCondition->wait(frameWaitTime);
			myAnimationController->animationMutex.lock();
		}
		
		//Now enough have started, don't allow any more to start:
		int numOverdue = 0;
		for (viznum = 0; viznum < MAXVIZWINS; viznum++){
			if (myAnimationController->isActive(viznum)&&myAnimationController->isShared(viznum)){
				if (myAnimationController->renderUnstarted(viznum)) myAnimationController->setFinishRender(viznum);
				else if (myAnimationController->renderStarted(viznum)) {
						numOverdue++;
						//Set overdue bit so this renderer will know to wake controller
						myAnimationController->setOverdue(viznum);
				
				}
			}
		}
		
		//Now wait for the completion of all overdue renderings.
		//Give it up to 60 1-second waits
		if (numOverdue > 0){
			
			for (tries = 0; tries< 61; tries++){
				myAnimationController->animationMutex.unlock();
// QT4 mustfix				myWaitCondition->wait(MAX_THREAD_WAIT);
				//qWarning("Waiting for completion of overdue renderings");
				myAnimationController->animationMutex.lock();
				numOverdue = 0;
				for (viznum = 0; viznum < MAXVIZWINS; viznum++){
					if (myAnimationController->isActive(viznum)&&myAnimationController->isShared(viznum)&&myAnimationController->renderStarted(viznum)){
						numOverdue++;
						//Set overdue bit so this renderer will know to wake controller
						myAnimationController->setOverdue(viznum);
					}
				}
				if (numOverdue == 0) break;

				if (tries > 59) {
					//Stop the animation, don't give a warning message,
					//That would interfere with other message boxes
					//QMessageBox::warning(0, "Animation Blocked", 
					//	"Animation stopped, because rendering was delayed for 10 seconds",
					//	QMessageBox::Ok, QMessageBox::NoButton);
					//Turn off the animation in overdue windows:
					for (viznum = 0; viznum < MAXVIZWINS; viznum++){
						if (myAnimationController->isActive(viznum)&&myAnimationController->isShared(viznum)&&myAnimationController->renderStarted(viznum))
							myAnimationController->deActivate(viznum);
							//Don't set render finished, it could still finish!
					}			

				}
			}
		}
		
		//When we get here the renderings have all completed.  
		//update, and
		//Advance the frame counter, and then restart the rendering.
		//If the changed bit was set, then don't advance the frame counter,
		//but do restart the rendering.  If we are changing to "pause" then
		//don't restart the rendering.
		//If the change is from global to local, then set the localglobalchange bit
		//and turn off the shared bit.
		//If the change is from local to global and the localglobalchange bit is set,
		//then set the global bit.
		int timeToRecheck = 1000;
		currentTime = myAnimationController->myClock->elapsed();
		bool frameAdvanced = false;
		for (viznum = 0; viznum<MAXVIZWINS; viznum++){
			if (myAnimationController->isActive(viznum)&&myAnimationController->isShared(viznum)){
				assert(myAnimationController->renderFinished(viznum));
				//has the animation stopped, or was the renderer disabled?
				if ((!myVizWinMgr->getVizWin(viznum)) ||
						(!myVizWinMgr->getAnimationParams(viznum)->isPlaying()) ||
						((!myVizWinMgr->getDvrParams(viznum)->isEnabled())&&
						(!myVizWinMgr->getIsoParams(viznum)->isEnabled())&&
						(!myVizWinMgr->getTwoDDataParams(viznum)->isEnabled())&&
						(!myVizWinMgr->getTwoDImageParams(viznum)->isEnabled())&&
						(!myVizWinMgr->getProbeParams(viznum)->isEnabled())&&
						(!myVizWinMgr->getFlowParams(viznum)->isEnabled()))){
					
					myAnimationController->deActivate(viznum);
				} else if (myAnimationController->isChanged(viznum)) {
					//If change bit was set, don't update the frame counter but
					//do restart the rendering:
					//This is where we will deal with local/global change!!
					
					int renderTime = myVizWinMgr->getAnimationParams(viznum)->getMinTimeToRender();
					myAnimationController->startVisualizer(viznum, currentTime);
					if (timeToRecheck > renderTime) timeToRecheck = renderTime;
					myAnimationController->setRequestRender(viznum);
					myAnimationController->clearChangeBit(viznum);
				} else {  //OK changed bit was not set.  Attempt to advance frame.
					bool setChange = false;
					if (!frameAdvanced){
						setChange = myVizWinMgr->getAnimationParams(viznum)->advanceFrame();
						frameAdvanced = true;
					}
					//OHOH: Do we need to set the change bits for all the renderers???
					if (setChange) {
						
						myAnimationController->setChangeBitsLocked(viznum);
						
					}
					else {//start the next render
						
						int renderTime = myVizWinMgr->getAnimationParams(viznum)->getMinTimeToRender();
						myAnimationController->startVisualizer(viznum, currentTime);
						if (timeToRecheck > renderTime) timeToRecheck = renderTime;
						myAnimationController->setRequestRender(viznum);
						//int frmnum = myVizWinMgr->getAnimationParams(viznum)->getCurrentFrameNumber();
						//qWarning(" requested render of frame %d in window %d", frmnum, viznum);
					}
				}
			}
		}
		//OK, renderers are restarted.
		//Can wait before checking them:
		
		if (timeToRecheck > 10){
			myAnimationController->animationMutex.unlock();
// QT4 mustfix			myWaitCondition->wait(timeToRecheck);
			
		}  else {//Just unlock, go back to the start
			myAnimationController->animationMutex.unlock();
		}
		
		//End of while(1) loop
	}
	//qWarning(" animation cancelled!");
	//Only exit loop if someone set animationCancelled; wait for completion, then return
	bool allDone;
	int tries;
	
	for (tries = 0; tries < 60; tries++){
		//See if any renderings are still in progress; make sure
		//to cancel unstarted renderings:
		allDone = true;
		
		myAnimationController->animationMutex.lock();
		for (viznum = 0; viznum<MAXVIZWINS; viznum++) {
			if (!myAnimationController->isActive(viznum)) continue;
			if (!myAnimationController->isShared(viznum)) continue;
			if (myAnimationController->renderUnstarted(viznum)) myAnimationController->setFinishRender(viznum);
			if (myAnimationController->renderStarted(viznum) && !myAnimationController->renderFinished(viznum)) {
				allDone = false;
				break;
			}
		}
		myAnimationController->animationMutex.unlock();
		if (allDone) {
			controllerActive = false;
			break;
		}
		//wait for a bit; may be woken if someone finishes, or status changes.
		//qWarning("Waiting for completion of started renderings");
// QT4 mustfix		myWaitCondition->wait(IDLE_WAIT);
	}
	
	//Assert that all renderers completed in 60 seconds
	if(tries>= 60) Params::BailOut("Excessive wait for shared animation to finish",__FILE__,__LINE__);
	return;

}
//Basic steps in run loop:
// 1.  Read clock 
// 2.  Identify available renderers.  For each one, set start time to clock time, and
// start rendering.
// 3.  Wait WAIT_A for each expected renderer to show up and get started. 
// 3.  Mark unstarted renderers as finished.
// 4.  Wait min render time (if necessary)
// 5.  Wait WAIT_A for last started renderer to finish
// 6.  Mark unfinished renderers as 
