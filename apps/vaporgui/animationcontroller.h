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
//		This class owns all the data associated with the current
//		animation.  It also has the methods needed for running an
//		animation. 
//		There is one of these for each vizwin.  It can schedule
//		one window's rendering.  This is derived from QThread.
//		Each animationcontroller tracks an actual time and an ideal time.
//      Ideal times are based on the minimum frame time, they indicate
//		what the rendering time would be if each frame were rendered at its
//		minimum frame time, (normalized so that the minimum frame time
//		is the ideal rendering time of the fastest frame of any of the
//		variables in the metadata).
//		At the completion of a rendering, the thread recalculates its
//		ideal and actual frame time.  Then (using a semaphore) it finds the
//		slowest ideal time of any thread.    It then determines how long (W secs) 
//		this thread will need to wait for its ideal time to coincide with the slowest
//		thread's ideal time.  It then schedules the next rendering to be the
//		greater of (slowest thread's actual time + W secs , now + my ideal time).
//		If the animation panel specifies a frame skip amount K, then the above
//		algorithm is applied but advancing the counter by K frames, and making
//		appropriate adjustment of ideal time.
//	
//		Once the rendering is triggered (by calling update), this thread
//		sleeps the ideal time.  If when it wakes up the rendering has not started,
//		this thread acts as if the rendering were completed in the ideal time.
//		If the rendering has started but not completed, it sleeps until the rendering 
//		thread wakes it up.  (Note:  this implies the controller will have a
//		QWaitCondition, that the renderer will call wake() on.
//		When the rendering has completed, the algorithm
//		is repeated.
//
//		Connection to animationParams:  The settings in animationParams
//		are used by the controller in the scheduling.  Whenever the state is 
//		changed, there must be a corresponding change in the controller.  Each
//		controller stops at its next interruption and restarts its animation.
//
//		When the animation params are set to "pause", (or the frame advance rate
//		is set to 0?), the thread sleeps (1 second
//		intervals) until the pause state is changed.  While in pause state
//		the user can change frames, which will trigger renderings but not
//		affect the controller
//
//		At some point it will become possible to perform animations with
//		multiple renderers in the same window.  These renderings will need
//		to operate with identical frame times.
//		

#ifndef ANIMATIONCONTROLLER_H
#define ANIMATIONCONTROLLER_H


#include <qthread.h>
#include <qmutex.h>
#include <cassert>
class QWaitCondition;
namespace VAPoR {

class AnimationController : public QThread {
public:
	AnimationController();
	~AnimationController();
	//The run method defines the animation cycle.
	//It will run continuously (with intermittent waits)
	//
	void run();
	//The setPause method is called when a value is changed that necessitates
	//restarting the animation.  (such as the user clicking pause).
	//It does not return until the animation
	//has been halted.  It can also be used to restart after those values
	//are changed.
	//
	void setPause(bool doPause);

	//The restart method resumes this thread after the values have
	//been set to new values.
	//Set methods get results from the gui
	void setFrameNumber(int frameNum){
		assert(paused);
		nextFrameNumber = frameNum;
	}
	void setFrameRate(int rate){
		assert(paused);
		currentFrameRate = rate;
	}
	void setStartFrame(int start){
		assert(paused);
		startFrame = start;
	}
	void setEndFrame(int end){
		assert(paused);
		endFrame = end;
	}
	void setMinFrameTime(float minTime){
		assert(paused);
		minFrameSeconds = minTime;
	}
	
	void setRepeat (bool repeat){
		assert(paused);
		repeating = repeat;
	}

	//Renderers call the following two methods:
	void beginRendering(int vizNum);
	void endRendering(int vizNum);
	//To interrupt a controller sleep, call the following:
	void wakeup();


protected:
	// advanceFrame is called by run() every time it's necessary
	// to do another frame.  Returns true if it needs to be called again soon
	bool advanceFrame();
	// animationMutex is needed to ensure serial access to
	// the renderingStatus bits
	QMutex animationMutex;
	// What frame number is currently rendering
	int currentFrameNumber;
	//The gui sets nextFrameNumber.
	int nextFrameNumber;
	// frame advance interval (0 halts, negative goes back)
	int currentFrameRate;
	// animation occurs between these frame numbers.
	int startFrame;
	int endFrame;
	//Min number of seconds before new frame is started
	float minFrameSeconds;
	//Pause (vs play) state
	bool paused;
	//Repeating versus stopping at end
	bool repeating;
	//Status flags for watching multiple windows.
	//This thread resets them to false before
	//triggering a window.
	//Each renderer calls back when it starts and when it ends 
	//using the mutex.  The controller calls back after waiting
	//minFrameSeconds.  If not all started renderings have completed,
	//that thread exits.  When the last started renderer is done,
	//it restarts the controller
	bool renderStarted;
	bool renderEnded;
	QWaitCondition* myWaitCondition;

};

}; //end namespace VAPoR

#endif

