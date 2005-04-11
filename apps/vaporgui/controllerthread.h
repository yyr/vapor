//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		controllerthread.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2005
//
//	Description:	Define two QThread classes that control the
//  animation (shared and unshared).
//  These classes are similar but their run() methods operate very differently.
//	The shared controller manages the global renderers as a group, keeping them
//	together, and updating the frame when they all complete a rendering.
//	The unshared controller only starts the renderers, and the renderers themselves
//	finish and update their frame at the rendering completion.  Each thread has
//	its own wait condition and wakeup call for reporting completion of a rendering.

#ifndef CONTROLLERTHREAD_H
#define CONTROLLERTHREAD_H
//Maximum milliseconds to wait for a slow renderer
#define MAX_SLOW_WAIT 5000
#include <qthread.h>
#include <qmutex.h>
#include <cassert>
class QWaitCondition;
class QTime;
#include "vizwinmgr.h"
namespace VAPoR {
class VizWinMgr;
class SharedControllerThread : public QThread {
public:
	SharedControllerThread();
	void restart();
	~SharedControllerThread();
	//The run method defines the animation cycle.
	//It will run continuously (with intermittent waits)
	//
	void run();
	
	//To interrupt the controller's sleep, call the following:
	void wakeup();
	

protected:
	QWaitCondition* myWaitCondition;
	// Flag to be set if it's time to cancel all current animations:
	bool animationCancelled;
	bool controllerActive;

};
class UnsharedControllerThread : public QThread {
public:
	UnsharedControllerThread();
	~UnsharedControllerThread();
	void restart();
	
	//The run method defines the animation cycle.
	//It will run continuously (with intermittent waits)
	//
	void run();
	
	//Unshared renderers call the following method, via calling the
	//corresponding method on animationcontroller
	
	void endRendering(int vizNum);
	//To interrupt the controller's sleep, call the following:
	void wakeup();
	

protected:
	QWaitCondition* myWaitCondition;
	// Flag to be set if it's time to cancel all current animations:
	bool animationCancelled;
	bool controllerActive;

};

}; //end namespace VAPoR

#endif

