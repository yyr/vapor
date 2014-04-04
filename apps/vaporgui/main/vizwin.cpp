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
//		Supports mouse event reporting
//
#include "glutil.h"	// Must be included first!!!
#include "vizwin.h"
#include <qdatetime.h>
#include <qvariant.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qaction.h>
#include <qmenubar.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <qnamespace.h>
#include <QHideEvent>
#include <QResizeEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QCloseEvent>
#include "visualizer.h"
#include "vapor/ControlExecutive.h"
#include <qdesktopwidget.h>
#include "tabmanager.h"
#include "viztab.h"
#include "regiontab.h"
#include "viewpointparams.h"
#include "regionparams.h"

#include "viewpoint.h"

#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "trackball.h"
#include "animationeventrouter.h"
#include "mainform.h"
#include "assert.h"
#include <vapor/jpegapi.h>
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
	myVisualizer = ControlExecutive::getInstance()->GetVisualizer(myWindowNum);
	myParent = parent;
	myName = name;
	setAutoBufferSwap(false);
	spinTimer = 0;
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
	//Tell the winmgr that we are closing:
    myWinMgr->vizAboutToDisappear(myWindowNum);
	QWidget::closeEvent(e);
	delete myVisualizer;
}
/******************************************************
 * React when focus is on window:
 ******************************************************/

// React to a user-change in window activation:
void VizWin::windowActivationChange(bool ){
	
}
// React to a user-change in window size/position (or possibly max/min)
// Either the window is minimized, maximized, restored, or just resized.
void VizWin::resizeGL(int width, int height){
	ControlExecutive* ce = ControlExecutive::getInstance();
	ce->ResizeViz(myWindowNum, width, height);
	reallyUpdate();
	return;
}
void VizWin::initializeGL(){
	ControlExecutive* ce = ControlExecutive::getInstance();
	ce->InitializeViz(myWindowNum, width(),height());
	return;
	
}
void VizWin::hideEvent(QHideEvent* ){

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
	
	int mode = Visualizer::getCurrentMouseMode();
	if (mode > 0 && buttonNum > 0) {  //Not navigation mode:
		/*
		int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
		int faceNum;
		float boxExtents[6];
		ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
		ParamsBase::ParamsBaseType t = GLWindow::getModeParamType(mode);
		
		Params* rParams = VizWinMgr::getInstance()->getParams(myWindowNum,t);
		TranslateStretchManip* manip = myVisualizer->getManip(Params::GetTagFromType(t));
		manip->setParams(rParams);
		int manipType = Visualizer::getModeManipType(mode);
		if(manipType != 3) rParams->calcStretchedBoxExtentsInCube(boxExtents, timestep); //non-rotated manip
		else rParams->calcContainingStretchedBoxExtentsInCube(boxExtents,true);//rotated
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
						int thet = (int)(fabs(rParams->getTheta())+0.5f);
						int ph = (int)(fabs(rParams->getPhi())+ 0.5f);
						int ps = (int)(fabs(rParams->getPsi())+ 0.5f);
						//Make sure that these are within tolerance of a multiple of 90
						if (abs(((thet+45)/90)*90 -thet) > tolerance) doStretch = false;
						if (abs(((ps+45)/90)*90 -ps) > tolerance) doStretch = false;
						if (abs(((ph+45)/90)*90 -ph) > tolerance) doStretch = false;
						
						if (!doStretch) {
							MessageReporter::warningMsg("Manipulator is not axis-aligned.\n%s %s %s",
								"To stretch or shrink,\n",
								"You must use the size\n",
								"(sliders or text) in the tab.");
							OK = false;
						}
					}
					break;
				default: assert(0); //Invalid manip type
			}//end switch
			if (OK) {
				doNavigate = false;
				float dirVec[3];
				//Find the direction vector of the camera (Local coords)
				myVisualizer->pixelToVector(screenCoords, 
					vParams->getCameraPosLocal(), dirVec);
				//Remember which handle we hit, highlight it, save the intersection point.
				manip->captureMouseDown(handleNum, faceNum, vParams->getCameraPosLocal(), dirVec, buttonNum);
				EventRouter* rep = VizWinMgr::getInstance()->getEventRouter(t);
				rep->captureMouseDown(buttonNum);
				setMouseDown(true);
				
			} //otherwise, fall through to navigation mode
			
		} */
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
	int mode = Visualizer::getCurrentMouseMode();
	if (mode > 0) {
		/*
		ParamsBase::ParamsBaseType t = Visualizer::getModeParamType(mode);
		myManip = myVisualizer->getManip(Params::GetTagFromType(t));
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
		*/
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
	int buttonNum = 0;
	if ((e->buttons() & Qt::LeftButton) &&  (e->buttons() & Qt::RightButton))
		;//do nothing
	else if (e->button()== Qt::LeftButton) buttonNum = 1;
	else if (e->button() == Qt::RightButton) buttonNum = 3;
	else if (e->button() == Qt::MidButton) buttonNum = 2;
	//If ctrl + left button is pressed, only respond in navigation mode
	if((buttonNum == 1) && ((e->modifiers() & (Qt::ControlModifier|Qt::MetaModifier))))
			buttonNum = 0;
	//Respond based on what activity we are tracking
	//Need to tell the appropriate params about the change,
	//And it should refresh the panel
	float mouseCoords[2];

	mouseCoords[0] = (float) e->x();
	mouseCoords[1] = (float) height()-e->y();
	int mode = Visualizer::getCurrentMouseMode();
//	ParamsBase::ParamsBaseType t = Visualizer::getModeParamType(mode);
	if (mode > 0){
		/*
		TranslateStretchManip* manip = myVisualizer->getManip(Params::GetTagFromType(t));
		bool constrain = manip->getParams()->isDomainConstrained();
		ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
		int handleNum = manip->draggingHandle();
		//check first to see if we are dragging face
		if (handleNum >= 0){
			if (myVisualizer->projectPointToLine(mouseCoords,projMouseCoords)) {
				float dirVec[3];
				myVisualizer->pixelToVector(projMouseCoords, vParams->getCameraPosLocal(), dirVec);
				//qWarning("Sliding handle %d, direction %f %f %f", handleNum, dirVec[0],dirVec[1],dirVec[2]);
				manip->slideHandle(handleNum, dirVec,constrain);
				doNavigate = false;
			}
		}*/
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
		myTrackball->MouseOnTrackball(1, buttonNum, e->x(), e->y(), width(), height());
		//Note that the coords have changed in the trackball:
		myVisualizer->SetTrackballCoordsChanged(true);
		
	}
	paintGL();
	return;
}


/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void VizWin::languageChange()
{
    setWindowTitle(myName);
   
}

void VizWin::helpIndex()
{
    //qWarning( "VizWin::helpIndex(): Not implemented yet" );
}

void VizWin::helpContents()
{
    //qWarning( "VizWin::helpContents(): Not implemented yet" );
}

void VizWin::helpAbout()
{
    //qWarning( "VizWin::helpAbout(): Not implemented yet" );
}

//Due to X11 probs need to check again.  Compare this window with the available space.
bool VizWin::isReallyMaximized() {
    if (isMaximized() ) return true;
	QWidget* thisCentralWidget = (MainForm::getInstance())->centralWidget();
    QSize mySize = frameSize();
    QSize spaceSize = thisCentralWidget->size();
    //qWarning(" space is %d by %d, frame is %d by %d ", spaceSize.width(), spaceSize.height(), mySize.width(), mySize.height());
    if ((mySize.width() > 0.95*spaceSize.width()) && (mySize.height() > 0.95*spaceSize.height()) )return true;
    return false;
        
}
	
void VizWin::setFocus(){
    //qWarning("Setting Focus in win %d", myWindowNum);
    //??QMainWindow::setFocus();
	QWidget::setFocus();
}

/* 
 * Get viewpoint info from GUI
 * Note:  if the current settings are global, this should be using global params
 */
void VizWin::
setValuesFromGui(ViewpointParams* vpparams){
	if (!vpparams->IsLocal()){
		assert(myWinMgr->getViewpointParams(myWindowNum) == vpparams);
	}
	float transCameraPos[3];
	float cubeCoords[3];
	//Must transform from world coords to unit cube coords for trackball.
	//ViewpointParams::localToStretchedCube(vpparams->getCameraPosLocal(), transCameraPos);
	//ViewpointParams::localToStretchedCube(vpparams->getRotationCenterLocal(), cubeCoords);
//	myTrackball->setFromFrame(transCameraPos, vp->getViewDir(), vp->getUpVec(), cubeCoords, vp->hasPerspective());
	
}
/*
 *  Switch Tball when change to local or global viewpoint
 */
void VizWin::
setGlobalViewpoint(bool setGlobal){
	if (!setGlobal){
//		myTrackball = localTrackball;
		globalVP = false;
	} else {
//		myTrackball = myWinMgr->getGlobalTrackball();
		globalVP = true;
	}
//	myVisualizer->myTBall = myTrackball;
	ViewpointParams* vpparms = myWinMgr->getViewpointParams(myWindowNum);
	setValuesFromGui(vpparms);
}


void VizWin::paintGL(){
	ControlExecutive* ce = ControlExecutive::getInstance();
	//only paint if necessary
	int rc = ce->Paint(myWindowNum, false);
	if (!rc) swapBuffers();
	return;
	
}
void VizWin::reallyUpdate(){
	makeCurrent();
	ControlExecutive* ce = ControlExecutive::getInstance();
	ce->Paint(myWindowNum, true);
	swapBuffers();
	return;

}


    
    
