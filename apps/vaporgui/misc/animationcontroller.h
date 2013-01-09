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
//		all rendering in all windows. It contains the current status
//		of all animations, and it supervises two rendering QThread classes,
//		SharedControllerThread (managing the shared animations) and
//		UnsharedControllerThread (managing the local, i.e. unshared animations).
//	
//		This class has the responsibility of launching the two threads, and
//		managing the interaction between the animations and the rest of the application.
//		Mainly that interaction is with the AnimationParams class, the Renderer class,
//		and the VizWinMgr class.

//		All the information about the various rendering windows is maintained in
//		the status bits.  The status bits are only read or written when
//		the AnimationMutex is locked.  
//
//		Connection to animationParams:  The settings in animationParams
//		are used by the controller in the scheduling.  When the settings are 
//		changed, in a way that affects
//		animation scheduling, the change bit is set.  This prevents the frame 
//		from being changed at the completion of the current rendering.  In some cases
//		(e.g. a change in local/global setting) the rendering does not continue
//		until the user stops and starts it again
//
//		To report its activities, the rendering thread calls 
//		AnimationController::startRendering() and AnimationController::endRendering()
//		at the start and end of its rendering
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
#include "controllerthread.h"
namespace VAPoR {
class VizWinMgr;

class AnimationController{
public:
	//Note this is a singleton class:
	static AnimationController* getInstance(){
		if (!theAnimationController)
			theAnimationController = new AnimationController();
		return theAnimationController;
	}
	//The thread classes will need intimate access to this class:
	friend class SharedControllerThread;
	friend class UnsharedControllerThread;
	void restart();
	~AnimationController();

	
	//Renderers call the following two methods.  They are dispatched to the appropriate
	//shared or unshared methods
	bool beginRendering(int vizNum);
	void endRendering(int vizNum);

	
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
		overdue = 32,
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
	
	//Status flags for watching multiple windows.
	//
	animationStatus statusFlag[MAXVIZWINS];
	//Keep track of the starting time for a rendering
	//
	int startTime[MAXVIZWINS];
	QTime* myClock;
	SharedControllerThread* mySharedController;
	UnsharedControllerThread* myUnsharedController;
	

};

}; //end namespace VAPoR

#endif

