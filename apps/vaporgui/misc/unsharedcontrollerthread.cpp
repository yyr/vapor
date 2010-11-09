//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		unsharedcontrollerthread.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2005
//
//	Description:	Implements the UnsharedControllerThread class
//			Functionality of that class is described in controllerthread.h
//		


#include <qwaitcondition.h>

#include <qmutex.h>
#include <qdatetime.h>
#include "animationcontroller.h"
#include "animationeventrouter.h"
#include "controllerthread.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "messagereporter.h"
#include "dvrparams.h"
#include "glutil.h"
#include "tabmanager.h"
#include "eventrouter.h"
#include "mainform.h"
#include <qthread.h>
using namespace VAPoR;

UnsharedControllerThread::UnsharedControllerThread() : QThread(){
	myWaitCondition = new QWaitCondition();
	controllerActive = false;
}

UnsharedControllerThread::~UnsharedControllerThread(){
	animationCancelled = true;
	myWaitCondition->wakeAll();
	//Wait up to 20 seconds for renderings to complete:
	if (!wait(20000)){
		//qWarning("terminating thread");
		terminate();
		if (!wait(MAX_SLOW_WAIT)) 
			Params::BailOut("Excessive wait for animation thread termination",__FILE__,__LINE__);
	}
	delete myWaitCondition;
}

//Set up when new data is read, by the Session
void UnsharedControllerThread::
restart(){
	int i;
	//If animation in progress, terminate it
	animationCancelled = true;
	//Wakeup the controller thread.  It should return after rendering is complete
	myWaitCondition->wakeAll();
	
	//Wait until controller thread completes:
	for (i = 0; i<100; i++){
		if (!controllerActive) break;
		//qWarning("waiting until controller threat completes");
		wait(MAX_SLOW_WAIT);
	}
	if( i>= 100) Params::BailOut("Excessive wait for animation thread completion",__FILE__,__LINE__);
	//restart the controller thread (calls run()) after it returns.
	animationCancelled = false;
	start();
	//start(QThread::IdlePriority);
}

// The run method continues as long as rendering needs to be performed.
// It will return if animationCancelled is set to true,
// after all unshared renderings are done.
//
void UnsharedControllerThread::
run(){
	VizWinMgr* myVizWinMgr = VizWinMgr::getInstance();
	AnimationController* myAnimationController = AnimationController::getInstance();
	int viznum;
	controllerActive = true;
	
	//Outer loop continues until animationCancelled is set
	while (1){
		if (animationCancelled) break;
		int currentTime = myAnimationController->myClock->elapsed();

		int timeToRecheck = 1000;
		int numActive = 0;
		myAnimationController->animationMutex.lock();
		
		//Check the time for unfinished unshared renderers.  Note if they affect
		//the wakeup time, or if they are overdue.
		for (viznum = 0; viznum<MAXVIZWINS; viznum++){
			if (myAnimationController->isActive(viznum) &&!myAnimationController->isShared(viznum)){
				numActive++;
				if (!myAnimationController->renderFinished(viznum)){
					int timeToFinish = myAnimationController->getTimeToFinish(viznum, currentTime);
					if (timeToFinish <= 0) myAnimationController->setOverdue(viznum);
					else if (timeToFinish < timeToRecheck) timeToRecheck = timeToFinish;
				}
			}
		}
		//Go try again if nothing is running:
		if( numActive == 0) {
			
			//qWarning("Waiting for an unshared renderer to start");
			myWaitCondition->wait(&myAnimationController->animationMutex,IDLE_WAIT);
			myAnimationController->animationMutex.unlock();
			continue;
		}

		//Then loop over all active, finished unshared visualizers.  Decide
		//Whether to restart them or to deactivate them, or to just wait longer
		
		for (viznum = 0; viznum<MAXVIZWINS; viznum++){
			if (myAnimationController->isActive(viznum)&&!myAnimationController->isShared(viznum)){

				if (myAnimationController->renderFinished(viznum)){
				
					//Either deactivate, or Call startVisualizer(viznum).
					//If a visualizer is local, finished, and late
					if ((!myVizWinMgr->getVizWin(viznum)) || 
						(!myVizWinMgr->getAnimationParams(viznum)->isPlaying()) ||
						!Params::IsRenderingEnabled(viznum)) {
						//deactivate stopped animations
						//qWarning("Deactivating stopped animation in viz %d",viznum);
						myAnimationController->deActivate(viznum);
					} else { //start it if it's ready to go
						int timeToFinish = myAnimationController->getTimeToFinish(viznum, currentTime);
						if (timeToFinish <= 0) {
							int renderTime = myVizWinMgr->getAnimationParams(viznum)->getMinTimeToRender();
							myAnimationController->startVisualizer(viznum, currentTime);
							if (timeToRecheck > renderTime) timeToRecheck = renderTime;
							myAnimationController->setRequestRender(viznum);
							//qWarning("Requesting render in %d",viznum);
						} else if (timeToFinish < timeToRecheck){
							//If we don't restart, remember when to recheck:
							timeToRecheck = timeToFinish;
							//qWarning("remembering when to recheck viz %d",viznum);
						} else {
							//qWarning("time to finish > time to recheck");
						}
					}
				//Following needed only for Darwin OSX 10.5 bug, it's not needed in 10.6
				} else if (!myAnimationController->renderStarted(viznum)){
					
					myAnimationController->startVisualizer(viznum, currentTime);
					myAnimationController->setRequestRender(viznum);
					//qWarning("Re-Requesting render in %d",viznum);
				} 
				//End of Darwin fix
			}
		}//loop over viznum
		
		
		//qWarning("Wait at end loop for %d",timeToRecheck);
		myWaitCondition->wait(&myAnimationController->animationMutex,timeToRecheck);
		myAnimationController->animationMutex.unlock();
		
		//Refresh the front tab:
		TabManager* tmgr = MainForm::getInstance()->getTabManager();
		EventRouter* eRouter = tmgr->getFrontEventRouter();
		eRouter->updateTab();
		
	}//end while(1) loop

	//Only exit loop if someone set animationCancelled; wait for completion, then return
	bool allDone;
	int tries;
	for (tries = 0; tries < 100; tries++){
		//See if any renderings are still in progress; make sure
		//to cancel unstarted renderings:
		allDone = true;
		myAnimationController->animationMutex.lock();
		for (viznum = 0; viznum<MAXVIZWINS; viznum++) {
			if (!myAnimationController->isActive(viznum)) continue;
			if (myAnimationController->isShared(viznum)) continue;
			if (myAnimationController->renderUnstarted(viznum)) myAnimationController->setFinishRender(viznum);
			if (myAnimationController->renderStarted(viznum) && !myAnimationController->renderFinished(viznum)) {
				allDone = false;
				break;
			}
		}
		
		if (allDone) {
			myAnimationController->animationMutex.unlock();
			controllerActive = false;
			break;
		}
		//wait; may be woken if someone finishes, or status changes.
		///
		//qWarning("waiting for someone else to finish or status to change");
		myWaitCondition->wait(&myAnimationController->animationMutex,IDLE_WAIT);
		myAnimationController->animationMutex.unlock();
	}
	
	//Assert that all renderers completed in 100*MAX_SLOW_WAIT seconds
	if(tries>=100) Params::BailOut("Excessive animation wait", __FILE__,__LINE__);
	return;

}



//When the rendering is complete, need to change state, setup for next frame.
//This is called by animationcontroller, with mutex already locked:
//
void UnsharedControllerThread::
endRendering(int vizNum){
	VizWinMgr* myVizWinMgr = VizWinMgr::getInstance();
	AnimationController* myAnimationController = AnimationController::getInstance();
	//Unshared renderers have the responsibility of doing the 
	//frame updating.
	//Check change bit.  If set don't update the currentFrame.  
	if (myAnimationController->isChanged(vizNum)){
		myAnimationController->clearChangeBit(vizNum);
	} else {
		//Note if the change bit needs to be set:
		bool setChange = myVizWinMgr->getAnimationParams(vizNum)->advanceFrame();
		if (setChange) {
			myAnimationController->setChangeBitsLocked(vizNum);
			//if (!myVizWinMgr->getAnimationParams(vizNum)->isRepeating()){
			//	myVizWinMgr->getAnimationRouter()->guiSetPlay(0);
			//}
		}
	}
}

// Call wakeup if the controller has work to do
//
void UnsharedControllerThread::
wakeup(){
	myWaitCondition->wakeAll();
}



	




