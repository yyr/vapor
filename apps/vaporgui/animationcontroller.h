//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		animationcontroller.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		January 2005
//
//	Description:	Defines the AnimationController class
//		This class has the methods needed for running an
//		animation. 
//		There is only one of these  It will schedule
//		all rendering in all windows.  It will separately
//		time all the animations in local animation windows, as
//		well as all the renderings that are associated with a shared
//		animation panel.  
//		This class is derived from QThread.
//		The basic operation is as follows (in the AnimationController::run() method):
//		Do Forever {
//			Check all renderings that were scheduled to complete by the current time.
//				Identify which ones are complete.
//			Start any renderings that are due, advancing the counters in the
//				animation panels
//			Determine the next time to start more renderings.
//			Sleep until the next rendering is due, or until woken up, (or 
//				a max wait time??) (whichever comes first).
//		}

//		At the completion of a rendering, the rendering thread checks if it has completed 
//		late and if it is the last thread.  If so it sends a message to wake up the 
//		controller, by calling AnimationController::wakeup()
//
//		Connection to animationParams:  The settings in animationParams
//		are used by the controller in the scheduling.  When animationParams are 
//		changed, in a way that affects
//		animation scheduling, the AnimationParams calls wakeup().
//		If a rendering is paused, each step or change in current frame will result in a
//		new rendering, but not disturb the animation controller.  The animation controller
//		is only notified when the play button is pressed, or when there is a change in
//		animation settings while the play is already pressed.
//
//		Each rendering thread is started by an update() issued by the
//		animation controller or another thread.  If this thread is being timed by
//		the animation controller, it must report a start and an end to its rendering.
//		(The start must be reported because some threads may not ever start to render,
//		if for example they are in windows that are hidden).  The rendering is
//		complete when all the threads that have started before the minimum rendering time
//		have also completed.  Note the rendering is complete after the minimum rendering
//		time elapses, if no thread starts rendering by then.
//		To report its activities, the rendering thread calls 
//		AnimationController::startRendering() and AnimationController::endRendering()
//		
//		To keep track of the various renderers, the AnimationController maintains
//		a status code on each visualizer window.  This includes the following information
//			Is the animation controller for that window global or local?
//			Is the window animated (versus paused or inactive)
//			If the window is animated, has it:
//				not yet started on the current rendering
//				started but not completed the current rendering
//				finished the current rendering
//			Is there a change in the animation settings for this window that has
//				not yet been processed, and will affect the next frame to be rendered?
//			
//		There is a mutex that is locked whenever the status of any renderer is updated.
//		
//		The AnimationController is associated with the session.  Whenever a new session
//		is started, the previous animationcontroller must be destroyed.  When this happens
//		the controller must cancel any renderings in progress (after a reasonable wait).
//		The mechanism for cancelling renderings has not yet been defined.
//
//		At some point it may become possible to perform animations with
//		multiple renderers in the same window.  These renderings will need
//		to operate with identical frame times.
//		

#ifndef ANIMATIONCONTROLLER_H
#define ANIMATIONCONTROLLER_H


#include <qthread.h>
#include <qmutex.h>
#include <cassert>
class QWaitCondition;
class QTime;
#include "vizwinmgr.h"
namespace VAPoR {
class VizWinMgr;
class AnimationController : public QThread {
public:
	//Note this is a singleton class:
	static AnimationController* getInstance(){
		if (!theAnimationController)
			theAnimationController = new AnimationController();
		return theAnimationController;
	}
	void restart();
	~AnimationController();
	//The run method defines the animation cycle.
	//It will run continuously (with intermittent waits)
	//
	void run();
	
	//Renderers call the following two methods:
	bool beginRendering(int vizNum);
	void endRendering(int vizNum);
	//To interrupt the controller's sleep, call the following:
	//Either one renderer needs attention
	void wakeup(int viznum);
	//Or any thread needs attention
	void wakeup();
	
	//These status codes record the status of the individual visualizers
	//A visualizer is active if it exists, if its animation params is not paused
	//and there is a renderer enabled for it; otherwise it is inactive.
	//The animation controller ignores inactive visualizers.
	//The shared bit indicates whether it is using shared or local animation params.
	//
	//An active visualizer is either unstarted, started, or finished.  It starts
	//in the "finished" state, i.e. ready to be animated, when it is made active.
	//Normally a visualizer cycles finished->unstarted->started->finished repeatedly,
	//however it can jump from unstarted to finished if the rendering does not start
	//soon enough.
	//The startTime indicates the time (millisec) that the visualizer was last
	//changed to unstarted.
	//A renderer can be set to "closed" if it fails to change to "started" sufficiently
	//soon after it enters the unstarted state

	enum animationStatus {
		inactive =	0,
		active	 =	1,
		shared	 =	2,
		progressBits = 12,
		unstarted=	4, // Set when initially ask for rendering
		started	 =	8, // Set when rendering begins
		finished =	12, //set when rendering done
		//When the state is changed in the animation params, following bit is set.
		//Completed renderings do not update the animation params
		changed  =	16,
		overdue = 32
	};
	//Gui calls following when a parameter changes that will alter
	//next render frame
	void paramsChanged(int viznum) {
		animationMutex.lock();
		setChangeBit(viznum);
		animationMutex.unlock();
	}
	//Following method called  by the vizwinmgr when gui requests play
	void startPlay(int viznum);
	//When a visualizer is destroyed, need to change its status to "finished"
	//Controller will deactivate it.
	void finishVisualizer(int viznum){
		animationMutex.lock();
		setFinishRender(viznum);
		animationMutex.unlock();
	}
	//When a visualizer is created, need to reset its status to "finished inactive"
	//Controller will activate it the next time "play" is started.
	void initializeVisualizer(int viznum){
		animationMutex.lock();
		statusFlag[viznum] = finished;
		animationMutex.unlock();
	}

		
	
protected:
	static AnimationController* theAnimationController;
	AnimationController();
	//Following function compares current time with the minimum time the rendering
	//should be complete.  Based on the most recent render requested
	int getTimeToFinish(int viznum, int currentTime);
	//Tell a renderer to start rendering:
	void startVisualizer(int viznum, int currentTime);
	//Inline methods to check status:
	bool isActive(int viznum) {return (statusFlag[viznum]&active); }
	bool isShared(int viznum) {return (statusFlag[viznum]&shared); }
	bool renderStarted(int viznum) {return ((statusFlag[viznum] & progressBits) == started);}
	bool renderFinished(int viznum) {return ((statusFlag[viznum] & progressBits) == finished);}
	bool renderUnstarted(int viznum) {return ((statusFlag[viznum] & progressBits) == unstarted);}
	bool isChanged(int viznum) {return (statusFlag[viznum] & changed);}
	bool isOverdue(int viznum) {return (statusFlag[viznum] & overdue);}

	//Methods to set status bits:
	void activate(int viznum) {statusFlag[viznum] = (animationStatus)(statusFlag[viznum]|active);}
	void deActivate(int viznum) {statusFlag[viznum] = (animationStatus)(statusFlag[viznum]&(~active));}
	void setStartRender(int viznum) {
		statusFlag[viznum] = (animationStatus)((statusFlag[viznum]&~progressBits)|started);}
	void setFinishRender(int viznum) {statusFlag[viznum] = (animationStatus)((statusFlag[viznum]&~progressBits)|finished);}
	//When render is requested, clear overdue as well as set set unstarted:
	void setRequestRender(int viznum) {statusFlag[viznum] = 
		(animationStatus)((statusFlag[viznum]&(~progressBits)&(~overdue))|(unstarted));}
	void setChangeBit(int viznum) {statusFlag[viznum] = (animationStatus)(statusFlag[viznum]|changed);}
	void clearChangeBit(int viznum) {statusFlag[viznum] = 
		(animationStatus)(statusFlag[viznum] & ~changed);}
	void setGlobal(int viznum){statusFlag[viznum] = (animationStatus)(statusFlag[viznum]|shared);}
	void setLocal(int viznum) {statusFlag[viznum] = (animationStatus)(statusFlag[viznum]&(~shared));}
	void setOverdue(int viznum) {statusFlag[viznum] = (animationStatus)(statusFlag[viznum]|overdue);}
	
	//Method to be called to set change bits, with mutex already locked:
	void setChangeBitsLocked(int viznum);
	// animationMutex is needed to ensure serial access to
	// the renderingStatus bits
	//
	QMutex animationMutex;
	// Flag to be set if it's time to cancel all current animations:
	bool animationCancelled;
	//Status flags for watching multiple windows.
	//
	animationStatus statusFlag[MAXVIZWINS];
	//Keep track of the starting time for a rendering
	//
	int startTime[MAXVIZWINS];
	
	QWaitCondition* myWaitCondition;
	QTime* myClock;

};

}; //end namespace VAPoR

#endif

