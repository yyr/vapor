//************************************************************************
//																		*
//		     Copyright (C)  2013										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//	File:		vizwin.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2013
//
//	Description:	Implements the VizWin class
//		This is the QGLWidget that performs OpenGL rendering (using associated Visualizer)
//		Plus supports mouse event reporting
//
#include "glutil.h"	// Must be included first!!!
#include "vizwin.h"
#include <QResizeEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QCloseEvent>
#include "visualizer.h"
#include "manip.h"
#include "vapor/ControlExecutive.h"
#include "tabmanager.h"
#include "viztab.h"
#include "viewpointparams.h"
#include "mousemodeparams.h"
#include "vizwinparams.h"
#include "viewpoint.h"
#include "qdatetime.h"
#include "viewpointeventrouter.h"
#include "trackball.h"
#include "mainform.h"
#include "assert.h"
#include "images/vapor-icon-32.xpm"

using namespace VAPoR;

/*
 *  Constructs a VizWindow as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
VizWin::VizWin( MainForm* parent, const QString& name, Qt::WFlags fl, VizWinMgr* myMgr, QRect* location, int winNum)
    : QGLWidget(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);
    myWindowNum = winNum;
	setWindowIcon(QPixmap(vapor_icon___));
	myVisualizer = ControlExec::GetVisualizer(myWindowNum);
	
	setAutoBufferSwap(false);
	spinTimer = 0;
	myWinMgr = myMgr;
	return;
}
/*
 *  Destroys the object and frees any allocated resources
 */
VizWin::~VizWin()
{
	 if (spinTimer) delete spinTimer;
}
void VizWin::closeEvent(QCloseEvent* e){
	//Catch this in the undo/redo queue
	Command* cmd = Command::CaptureStart(ControlExec::GetCurrentParams(-1, VizWinParams::_vizWinParamsTag),"Delete Visualizer", VizWinMgr::UndoRedo);
	//Tell the winmgr that we are closing:
    myWinMgr->vizAboutToDisappear(myWindowNum);
	QWidget::closeEvent(e);
	Command::CaptureEnd(cmd, ControlExec::GetCurrentParams(-1, VizWinParams::_vizWinParamsTag));
}
/******************************************************
 * React when focus is on window:
 ******************************************************/
void VizWin::
focusInEvent(QFocusEvent* e){
	//Test for hidden here, since a vanishing window can get this event.
	if (e->gotFocus() && !isHidden()){
	
		if (myWinMgr->getActiveViz() != myWindowNum ){
			myWinMgr->setActiveViz(myWindowNum);
		}
	}
}


// React to a user-change in window size/position (or possibly max/min)
// Either the window is minimized, maximized, restored, or just resized.
void VizWin::resizeGL(int width, int height){
	
	ControlExec::ResizeViz(myWindowNum, width, height);
	VizWinParams::SetWindowHeight(myWindowNum, height);
	VizWinParams::SetWindowWidth(myWindowNum, width);
	reallyUpdate();
	return;
}
void VizWin::initializeGL(){
	ControlExec::InitializeViz(myWindowNum, width(),height());
}

/* If the user presses the mouse on the active viz window,
 * We record the position of the click.
 */
void VizWin::
mousePressEvent(QMouseEvent* e){
	float screenCoords[2];
	//if (numRenderers <= 0) return;// Even with no renderers, do mouse mode stuff
	screenCoords[0] = (float)e->x()-3.f;
	//To keep orientation correct in plane, and use
	//OpenGL convention (Y 0 at bottom of window), reverse
	//value of y:
	screenCoords[1] = (float)(height() - e->y()) - 5.f;
	int buttonNum = 0;
	if ((e->buttons() & Qt::LeftButton) &&  (e->buttons() & Qt::RightButton))
		;//do nothing
	else if (e->button()== Qt::LeftButton) buttonNum = 1;
	else if (e->button() == Qt::RightButton) buttonNum = 3;
	else if (e->button() == Qt::MidButton) buttonNum = 2;
	//If ctrl + left button is pressed, only respond in navigation mode
	if((buttonNum == 1) && ((e->modifiers() & (Qt::ControlModifier|Qt::MetaModifier))))
			buttonNum = 0;
	Trackball* myTrackball = myVisualizer->GetTrackball();
	//possibly navigate after other activities
	bool doNavigate = true;
	
	int mode = MouseModeParams::GetCurrentMouseMode();
	if (mode > 0 && buttonNum > 0) {  //Not navigation mode:
		
		int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
		int faceNum;
		double boxExtents[6];
		ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
		ParamsBase::ParamsBaseType t = MouseModeParams::getModeParamType(mode);
		
		string tag = ControlExec::GetTagFromType(t);
		Params* rParams = ControlExec::GetCurrentParams(myWindowNum, tag);
		TranslateStretchManip* manip = myVisualizer->getManip(tag);
		manip->setParams(rParams);
		int manipType = MouseModeParams::getModeManipType(mode);
		if(manipType != 3) rParams->GetBox()->GetStretchedLocalExtents(boxExtents, timestep); //non-rotated manip
		else rParams->GetBox()->calcContainingStretchedBoxExtents(boxExtents,true);//rotated
		int handleNum = manip->mouseIsOverHandle(screenCoords, boxExtents, &faceNum);

		if (handleNum >= 0 && myVisualizer->startHandleSlide(screenCoords, handleNum,rParams)){
			
			//With manip type 2, need to prevent right mouse slide on orthogonal handle
			//With manip type 3, need to use orientation
			bool OK = true;//Flag indicates whether the manip takes the mouse 
			switch (manipType) {
				case 1 : //3d box manip
					break;
				case 2 : //2d box manip, ok if not right mouse on orthog direction
					//Do nothing if grabbing orthog direction with right mouse:
					if (buttonNum == 2 && ((handleNum == (rParams->getOrientation() +3))
						|| (handleNum == (rParams->getOrientation() -2)))){
							OK = false;
					}
					break;
				case 3 :  //Rotate-stretch:  check if it's rotated too far
					if (buttonNum <= 1) break;  //OK if left mouse button
					{
						bool doStretch = true;
						//check if the rotation angle is approx a multiple of 90 degrees:
						int tolerance = 20;
						const vector<double>angles = rParams->GetBox()->GetAngles();
						int thet = (int)(fabs(angles[0])+0.5f);
						int ph = (int)(fabs(angles[1])+ 0.5f);
						int ps = (int)(fabs(angles[2])+ 0.5f);
						//Make sure that these are within tolerance of a multiple of 90
						if (abs(((thet+45)/90)*90 -thet) > tolerance) doStretch = false;
						if (abs(((ps+45)/90)*90 -ps) > tolerance) doStretch = false;
						if (abs(((ph+45)/90)*90 -ph) > tolerance) doStretch = false;
						
						if (!doStretch) {
							/*
							MessageReporter::warningMsg("Manipulator is not axis-aligned.\n%s %s %s",
								"To stretch or shrink,\n",
								"You must use the size\n",
								"(sliders or text) in the tab.");
								*/
							OK = false;
						}
					}
					break;
				default: assert(0); //Invalid manip type
			}//end switch
			if (OK) {
				doNavigate = false;
				double dirVec[3];
				//Find the direction vector of the camera (Local coords)
				myVisualizer->pixelToVector(screenCoords, 
					vParams->getCameraPosLocal(), dirVec);
				//Remember which handle we hit, highlight it, save the intersection point.
				manip->captureMouseDown(handleNum, faceNum, vParams->getCameraPosLocal(), dirVec, buttonNum);
				EventRouter* rep = VizWinMgr::getInstance()->getEventRouter(t);
				rep->captureMouseDown(buttonNum);
				setMouseDown(true);
				
			} //otherwise, fall through to navigation mode
			
		} 
		//Set up for spin animation
	} else if (mode == 0 && buttonNum == 1){ //Navigation mode, prepare for spin
		// doNavigate is true;
		//Create a timer to use to measure how long between mouse moves:
		if (spinTimer) delete spinTimer;
		spinTimer = new QTime();
		spinTimer->start();
		moveCount = 0;
		olderMoveTime = latestMoveTime = 0;
		
	} 
	//Otherwise, either mode > 0 or buttonNum != 1. OK to navigate 

	if (doNavigate){
		ViewpointEventRouter* vep = VizWinMgr::getInstance()->getViewpointRouter();
					vep->captureMouseDown(buttonNum);
		
		myTrackball->MouseOnTrackball(0, buttonNum, e->x(), e->y(), width(), height());
		setMouseDown(true);
		mouseDownPosition = e->pos();
	}
	
}
/*
 * If the user releases the mouse or moves it (with the left mouse down)
 * then we note the displacement
 */
void VizWin:: 
mouseReleaseEvent(QMouseEvent*e){
	//if (numRenderers <= 0) return;//used for mouse mode stuff
	bool doNavigate = false;
	bool navMode = false;
	Trackball* myTrackball = myVisualizer->GetTrackball();
	int buttonNum = 0;
	if ((e->buttons() & Qt::LeftButton) &&  (e->buttons() & Qt::RightButton))
		;//do nothing
	else if (e->button()== Qt::LeftButton) buttonNum = 1;
	else if (e->button() == Qt::RightButton) buttonNum = 3;
	else if (e->button() == Qt::MidButton) buttonNum = 2;
	//If ctrl + left button is pressed, only respond in navigation mode
	if((buttonNum == 1) && ((e->modifiers() & (Qt::ControlModifier|Qt::MetaModifier))))
			buttonNum = 0;
	//TranslateStretchManip* myManip;
	int mode = MouseModeParams::GetCurrentMouseMode();
	if (mode > 0) {
	
		ParamsBase::ParamsBaseType t = MouseModeParams::getModeParamType(mode);
		TranslateStretchManip* myManip = myVisualizer->getManip(Params::GetTagFromType(t));
		//Check if the seed bounds were moved
		if (myManip->draggingHandle() >= 0){
			float screenCoords[2];
			screenCoords[0] = (float)e->x();
			screenCoords[1] = (float)(height() - e->y());
			setMouseDown(false);
			//The manip must move the region, and then tells the params to
			//record end of move
			myManip->mouseRelease(screenCoords);
			VizWinMgr::getInstance()->getEventRouter(t)->captureMouseUp();
			
		} else {//otherwise fall through to navigate mode
			doNavigate = true;
		}
		
	} else {//In true navigation mode
		doNavigate = true;
		navMode = true;
	}
		
	if(doNavigate){
		myTrackball->MouseOnTrackball(2, buttonNum, e->x(), e->y(), width(), height());
		setMouseDown(false);
		//If it's a right mouse being released, must update near/far distances:
		if (e->button() == Qt::RightButton){
//			myWinMgr->resetViews(
//			myWinMgr->getViewpointParams(myWindowNum));
		}
		//Decide whether or not to start a spin animation
		
		//bool doSpin = (Visualizer::spinAnimationEnabled() && navMode && e->button() == Qt::LeftButton && spinTimer &&
		//	!myVisualizer->getActiveAnimationParams()->isPlaying());
		bool doSpin = (navMode && e->button() == Qt::LeftButton && spinTimer);
			//Determine if the motion is sufficient to start a spin animation.
			//Require that some time has elapsed since the last move event, and,
			//to allow users to stop spin by holding mouse steady, make sure that
			//the time from the last move event is no more than 6 times the 
			//difference between the last two move times.
		if (doSpin) {
			int latestTime = spinTimer->elapsed();
		
			if (moveDist > 3 && moveCount > 0 && (latestTime - latestMoveTime)< 6*(latestMoveTime-olderMoveTime)){
//				myVisualizer->startSpin(latestTime/moveCount);
			} else {
				doSpin = false;
			}
		}
		if (!doSpin){
			//terminate current mouse motion
			VizWinMgr::getEventRouter(Params::_viewpointParamsTag)->captureMouseUp();
		}
		
		//Force redisplay of tab
		VizWinMgr::getEventRouter(Params::_viewpointParamsTag)->updateTab();
	}
	
}	
/* 
 * When the mouse is moved, it can affect navigation, 
 * region position, light position, or probe position, depending 
 * on current mode.  The values associated with the window are 
 * changed whether or not the tabbed panel is visible.  
 *  It's important that coordinate changes eventually get recorded in the
 * viewpoint params panel.  This requires some work every time there is
 * mouse navigation.  Changes in the viewpoint params panel will notify
 * the viztab if it is active and change the values there.
 * Conversely, when values are changed in the viztab, the viewpoint
 * values are set in the vizwin class, provided they did not originally 
 * come from the mouse navigation.  Such a change forces a reinitialization
 * of the trackball and the new values will be used at the next rendering.
 * 
 */
void VizWin:: 
mouseMoveEvent(QMouseEvent* e){
	if (!mouseIsDown()) return;
	Trackball* myTrackball = myVisualizer->GetTrackball();
	bool doNavigate = true;
	
	//Respond based on what activity we are tracking
	//Need to tell the appropriate params about the change,
	//And it should refresh the panel
	float mouseCoords[2];
	float projMouseCoords[2];
	mouseCoords[0] = (float) e->x();
	mouseCoords[1] = (float) height()-e->y();
	int mode = MouseModeParams::GetCurrentMouseMode();
	ParamsBase::ParamsBaseType t = MouseModeParams::getModeParamType(mode);
	if (mode > 0){
		
		TranslateStretchManip* manip = myVisualizer->getManip(Params::GetTagFromType(t));
		//bool constrain = manip->getParams()->isDomainConstrained();
		bool constrain = true;
		ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
		int handleNum = manip->draggingHandle();
		//check first to see if we are dragging face
		if (handleNum >= 0){
			if (myVisualizer->projectPointToLine(mouseCoords,projMouseCoords)) {
				double dirVec[3];
				myVisualizer->pixelToVector(projMouseCoords, vParams->getCameraPosLocal(), dirVec);
				//qWarning("Sliding handle %d, direction %f %f %f", handleNum, dirVec[0],dirVec[1],dirVec[2]);
				manip->slideHandle(handleNum, dirVec,constrain);
				doNavigate = false;
			}
		}
	} else if (spinTimer) { //Navigate mode, handle spin animation
		moveCount++;
		if (moveCount > 0){//find distance from last move event...
			moveDist = abs(moveCoords[0]-e->x())+abs(moveCoords[1]-e->y());
		}
		moveCoords[0] = e->x();
		moveCoords[1] = e->y();
		int latestTime = spinTimer->elapsed();
		olderMoveTime = latestMoveTime;
		latestMoveTime = latestTime;
	
	}
	if(doNavigate){
		QPoint deltaPoint = e->globalPos() - mouseDownPosition;
		//Note:  the button is remembered by the trackball here.
		//e->button() is always Qt::NoButton on mouse move events.
		myTrackball->MouseOnTrackball(1, 0, e->x(), e->y(), width(), height());
		//Note that the coords have changed in the trackball:
		myVisualizer->SetTrackballCoordsChanged(true);
		
	}
	paintGL();
	return;
}

	
void VizWin::setFocus(){
    //qWarning("Setting Focus in win %d", myWindowNum);
    //??QMainWindow::setFocus();
	QWidget::setFocus();
}



void VizWin::paintGL(){
	//only paint if necessary
	//Note that makeCurrent is needed when here we have multiple windows.
	makeCurrent();
	int rc = ControlExec::Paint(myWindowNum, false);
	
	if (!rc) swapBuffers();
	return;
	
}
void VizWin::reallyUpdate(){
	makeCurrent();
	ControlExec::Paint(myWindowNum, true);
	swapBuffers();
	return;

}


    
    
