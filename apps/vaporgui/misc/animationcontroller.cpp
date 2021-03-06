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
#include "mainform.h"
#include "tabmanager.h"
#include <qapplication.h>
#include <qwaitcondition.h>
#include <qthread.h>
#include <qmutex.h>
#include <qdatetime.h>
#include <qregion.h>
#include "animationcontroller.h"
#include "animationparams.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "messagereporter.h"
#include "animationeventrouter.h"
#include "eventrouter.h"

using namespace VAPoR;

//Initialize the singleton to 0:
AnimationController* AnimationController::theAnimationController = 0;

AnimationController::AnimationController(){
	
	myClock = new QTime();
	//Initialize status flags 
	for (int i = 0; i<MAXVIZWINS; i++) {
		statusFlag[i] = inactive;
		startTime[i] = 0;
	}
	mySharedController = new SharedControllerThread();
	myUnsharedController = new UnsharedControllerThread();
}

AnimationController::~AnimationController(){
	
	delete mySharedController;
	delete myUnsharedController;
	delete myClock;
}

//Set up when new data is read, by the Session
void AnimationController::
restart(){
	
	myClock->start();
	numClockRollovers = 0;
	lastClockMillisec = (size_t)myClock->elapsed();
	animationMutex.lock();
	//Initialize the status bits
	for (int viznum = 0; viznum<MAXVIZWINS; viznum++) {
		//begin with inactive/finished status:
		statusFlag[viznum] = finished;
		startTime[viznum] = 0;
		//set the shared bit:
		if (!VizWinMgr::getInstance()->getAnimationParams(viznum)->isLocal())
			setGlobal(viznum);
	}
	animationMutex.unlock();
	mySharedController->restart();
	myUnsharedController->restart();
}

//Renderers call the following methods before and after rendering.
// If beginRendering returns true, the controller is tracking the rendering, so
// endRendering must be called.  Otherwise, the rendering is not being timed,
// and endRendering should not be called.
// This works the same for both shared and unshared animations, but 
// the wakeup at the end is to the specific controller thread
//
bool AnimationController::
beginRendering(int viznum){
	
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
	
	//The controller could change us to "finished" if we were taking too long:
	if (!renderUnstarted(viznum)) {
		animationMutex.unlock();
		return false;
	}
	//OK now change status bits:
	setStartRender(viznum);
	//int frmnum = VizWinMgr::getInstance()->getAnimationParams(viznum)->getCurrentFrameNumber();
	//qWarning("window %d is started on frame %d", viznum, frmnum);
	bool sharing = isShared(viznum);
	animationMutex.unlock();
	//wakeup the controller thread:
	if (sharing) mySharedController->wakeup();
	else myUnsharedController->wakeup();
	return true;
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
	VizWinMgr::getInstance()->getVizWin(viznum)->setAnimationDirty(true);
	QEvent* renderEvent = new QEvent(QEvent::UpdateRequest);
	QApplication::postEvent(VizWinMgr::getInstance()->getVizWin(viznum)->getGLWindow(), renderEvent, INT_MAX);
	QApplication::processEvents();
}
//Activate renderer prior to starting play, in response to user clicking "play" button.
void AnimationController::
startPlay(int viznum) {
	
	animationMutex.lock();
	//Sometimes we get here with an active visualizer.  That should be OK.
	if(isActive(viznum))
		MessageReporter::infoMsg("Animation starting an active visualizer");
	else activate(viznum);
	setFinishRender(viznum);
	//This is where we set the local/global bit, which will determine
	//which thread is in control:
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
	
	if (!isShared(viznum)) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  (isShared(i) ){
			setChangeBit(i);
		}
	}
}


//When the rendering is complete, need to change state, setup for next frame.
//Unshared renderers update themselves, shared renderers are updated by their
//controller thread.
//
void AnimationController::
endRendering(int vizNum){
	
	animationMutex.lock();
	
	// check if this window is animating, if not just leave, maybe update the tab
	if (!isActive(vizNum)) { 
		animationMutex.unlock();
		AnimationParams* aParams = VizWinMgr::getInstance()->getAnimationParams(vizNum);
		if (aParams->isStateChanged()){
			VizWinMgr::getInstance()->getAnimationRouter()->updateTab();
			aParams->setStateChanged(false);
		}
		return;
	}
	//assert(renderStarted(vizNum));
	//Set the flag indicating this rendering is done.
	setFinishRender(vizNum);
	
	bool doWake = isOverdue(vizNum);
	bool shared = isShared(vizNum);
	if (!shared) 
		myUnsharedController->endRendering(vizNum);
	animationMutex.unlock();
	
	AnimationParams* aParams = VizWinMgr::getInstance()->getAnimationParams(vizNum);
	if (aParams->isStateChanged()){
		aParams->setStateChanged(false);
		VizWinMgr::getInstance()->getAnimationRouter()->updateTab();
		if (!aParams->isPlaying()) return;
	}

	//Do call Update dialog here because Update can't be called from the 
	//controller thread (X11 limitation!)
	//See if this is the active visualizer
	if (VizWinMgr::getInstance()->getActiveViz() == vizNum){
	/*  This was needed in Qt3 but causes minor issues in Qt4	
	    Evidently the multithreading in Qt 4 helps processing events
	    while the app is otherwise busy*/
		QCoreApplication* app;
		app = MainForm::getInstance()->getApp();
		bool hasEvents = app->hasPendingEvents();
		if (hasEvents) {
			app->processEvents();
		}
		//Must lock, so multiple threads can't advance the time step
		animationMutex.lock();
		if (aParams->checkLastFrame()){
			animationMutex.unlock();
			//At the end of animation, update the front tab and
			//the animation toolbar.
			TabManager* tmgr = MainForm::getTabManager();
			EventRouter* eRouter = tmgr->getFrontEventRouter();
			eRouter->updateTab();
			//Set the pause button:
			MainForm::getInstance()->setPause();
		} else animationMutex.unlock();

		VizWinMgr::getInstance()->getAnimationRouter()->updateTab();
		MainForm::getInstance()->setCurrentTimestep(VizWinMgr::getActiveAnimationParams()->getCurrentTimestep());
	}
	
	//Do this out of the mutex:
	//Wakeup the controller:
	if (doWake) {
		if (shared) mySharedController->wakeup();
		else myUnsharedController->wakeup();
	}
}

//Replacement for QTime::elapsed(), avoids 24 hour rollover
//
size_t AnimationController::getElapsedMillisec(){
	size_t elapsedClockMillisec = (size_t)myClock->elapsed();
	if (elapsedClockMillisec < lastClockMillisec){
		//Clock has rolled over.  Add 1 to numClockRollovers until we get a positive time change
		while(numClockRollovers*86400000ULL + elapsedClockMillisec < lastClockMillisec) {
			numClockRollovers++;
		}
	}
	lastClockMillisec = elapsedClockMillisec;
	return(86400000ULL*numClockRollovers+elapsedClockMillisec);
}


