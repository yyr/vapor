//************************************************************************
//									*
//		     Copyright (C)  2004				*
//     University Corporation for Atmospheric Research			*
//		     All Rights Reserved				*
//									*
//************************************************************************/
//	File:		vizwin.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:	Implements the VizWin class
//		This is the widget that contains the visualizers
//		Supports mouse event reporting
//
#include "GL/glew.h"
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
#include "glwindow.h"
#include "vizwinmgr.h"
#include <qdesktopwidget.h>
#include "tabmanager.h"
#include "viztab.h"
#include "regiontab.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "messagereporter.h"
#include "glutil.h"
#include "viewpoint.h"
#include "manip.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"

#include "animationeventrouter.h"
#include "floweventrouter.h"
#include "flowrenderer.h"
#include "VolumeRenderer.h"
#include "mainform.h"
#include "assert.h"
#include "session.h"
#include <vapor/jpegapi.h>
#include "images/vapor-icon-32.xpm"

using namespace VAPoR;

/*
 *  Constructs a VizWindow as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
VizWin::VizWin( MainForm* parent, const QString& name, Qt::WFlags fl, VizWinMgr* myMgr, QRect* location, int winNum)
    : QWidget( (QWidget*)parent, fl )
{
	setAttribute(Qt::WA_DeleteOnClose);
	MessageReporter::infoMsg("VizWin::VizWin() begin");
	myName = name;
    myParent = parent;
    myWindowNum = winNum;
	spinTimer = 0;
	elapsedTime = 0;
	moveCount = 0;
	moveCoords[0] = moveCoords[1] = 0;
	moveDist = 0;
    move(location->topLeft());
    resize(location->size());
    myWinMgr = myMgr;
	mouseDownHere = false;
    languageChange();
	setWindowIcon(QPixmap(vapor_icon___));
    // Create our OpenGL widget.  
	QGLFormat fmt;
	fmt.setAlpha(true);
	fmt.setRgba(true);
	fmt.setDepth(true);
	fmt.setDoubleBuffer(true);
	fmt.setDirectRendering(true);

    	myGLWindow = new GLWindow(fmt, this ,myWindowNum);
	if (!(fmt.directRendering() && fmt.depth() && fmt.rgba() && fmt.alpha() && fmt.doubleBuffer())){
		Params::BailOut("Unable to obtain required OpenGL rendering format",__FILE__,__LINE__);	
	}
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++)
		myGLWindow->setActiveParams(Params::GetCurrentParamsInstance(i,myWindowNum),i);
	
	myGLWindow->setPreRenderCB(preRenderSetup);
	myGLWindow->setPostRenderCB(endRender);

	QHBoxLayout* flayout = new QHBoxLayout(this);
	flayout->setContentsMargins(2,2,2,2);
	flayout->addWidget(myGLWindow);

	//Get viewpoint from viewpointParams
	ViewpointParams* vpparms = myWinMgr->getViewpointParams(myWindowNum);
	//Attach the trackball:
	localTrackball = new Trackball();
	if (vpparms->isLocal()){
		myTrackball = localTrackball;
		globalVP = false;
	} else {
		myTrackball = myWinMgr->getGlobalTrackball();
		globalVP = true;
	}
	myGLWindow->myTBall = myTrackball;
	setValuesFromGui(vpparms);
	
	MessageReporter::infoMsg("VizWin::VizWin() end");
}
/*
 *  Destroys the object and frees any allocated resources
 */
VizWin::~VizWin()
{
	
     if (localTrackball) delete localTrackball;
	 //The renderers are deleted in the glwindow destructor:
	 if (spinTimer) delete spinTimer;
}
void VizWin::closeEvent(QCloseEvent* e){
	//Tell the winmgr that we are closing:
    myWinMgr->vizAboutToDisappear(myWindowNum);
	QWidget::closeEvent(e);
	delete myGLWindow;
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

// React to a user-change in window activation:
void VizWin::windowActivationChange(bool ){
	
		//qWarning(" Activation Event %d received in window %d ", value, myWindowNum);
		
		//We may also be going out of minimized state:
		if (isReallyMaximized()) {
			//qWarning( " resize due to maximization");
			
			myWinMgr->maximize(myWindowNum);
			
		}
		else if (isMinimized()) {
			myWinMgr->minimize(myWindowNum);
			
		}	
		else {
			//went to normal
			myWinMgr->normalize(myWindowNum);
			
		}
		
}
// React to a user-change in window size/position (or possibly max/min)
// Either the window is minimized, maximized, restored, or just resized.
void VizWin::resizeEvent(QResizeEvent*){
		//qWarning(" Resize Event received in window %d ", myWindowNum);
		if (isReallyMaximized()) {
			//qWarning( " resize due to maximization");
			myWinMgr->maximize(myWindowNum);
			
		}
		else if (isMinimized()) {
			myWinMgr->minimize(myWindowNum);
			
		}	
		else {
			//User either resized or restored from min/max
			myWinMgr->normalize(myWindowNum);
			
		}
}

void VizWin::hideEvent(QHideEvent* ){
	myWinMgr->minimize(myWindowNum);
	
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
	else if (e->button() == Qt::RightButton) buttonNum = 2;
	//If ctrl + left button is pressed, only respond in navigation mode
	if((buttonNum == 1) && ((e->modifiers() & (Qt::ControlModifier|Qt::MetaModifier))))
			buttonNum = 0;

	//possibly navigate after other activities
	bool doNavigate = true;
	endSpin();
	int mode = GLWindow::getCurrentMouseMode();
	if (mode > 0 && buttonNum > 0) {  //Not navigation mode:
		
		int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
		int faceNum;
		float boxExtents[6];
		ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
		ParamsBase::ParamsBaseType t = GLWindow::getModeParamType(mode);
		
		Params* rParams = VizWinMgr::getInstance()->getParams(myWindowNum,t);
		TranslateStretchManip* manip = myGLWindow->getManip(Params::GetTagFromType(t));
		manip->setParams(rParams);
		int manipType = GLWindow::getModeManipType(mode);
		if(manipType != 3) rParams->calcStretchedBoxExtentsInCube(boxExtents, timestep);
		else rParams->calcContainingStretchedBoxExtentsInCube(boxExtents);
		int handleNum = manip->mouseIsOverHandle(screenCoords, boxExtents, &faceNum);

		if (handleNum >= 0 && myGLWindow->startHandleSlide(screenCoords, handleNum,rParams)){
			
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
				myGLWindow->pixelToVector(screenCoords, 
					vParams->getCameraPosLocal(), dirVec);
				//Remember which handle we hit, highlight it, save the intersection point.
				manip->captureMouseDown(handleNum, faceNum, vParams->getCameraPosLocal(), dirVec, buttonNum);
				EventRouter* rep = VizWinMgr::getInstance()->getEventRouter(t);
				rep->captureMouseDown();
				setMouseDown(true);
				myGLWindow->update();
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
					vep->captureMouseDown();
		Qt::MouseButton btn = e->button();
		//Left button + ctrl = mid button:
		if ((e->buttons() & Qt::LeftButton) &&  (e->buttons() & Qt::RightButton)) btn = Qt::MidButton;
		else if(btn == Qt::LeftButton && ((e->modifiers() & (Qt::ControlModifier|Qt::MetaModifier))))
			btn = Qt::MidButton;
		myTrackball->MouseOnTrackball(0, btn, e->x(), e->y(), width(), height());
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
	TranslateStretchManip* myManip;
	int mode = GLWindow::getCurrentMouseMode();
	if (mode > 0) {
		ParamsBase::ParamsBaseType t = GLWindow::getModeParamType(mode);
		myManip = myGLWindow->getManip(Params::GetTagFromType(t));
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
		myTrackball->MouseOnTrackball(2, e->button(), e->x(), e->y(), width(), height());
		setMouseDown(false);
		//If it's a right mouse being released, must update near/far distances:
		if (e->button() == Qt::RightButton){
			myWinMgr->resetViews(
				myWinMgr->getViewpointParams(myWindowNum));
		}
		//Decide whether or not to start a spin animation
		bool doSpin = (GLWindow::spinAnimationEnabled() && navMode && e->button() == Qt::LeftButton && spinTimer &&
			!getGLWindow()->getActiveAnimationParams()->isPlaying());
		
			//Determine if the motion is sufficient to start a spin animation.
			//Require that some time has elapsed since the last move event, and,
			//to allow users to stop spin by holding mouse steady, make sure that
			//the time from the last move event is no more than 6 times the 
			//difference between the last two move times.
		if (doSpin) {
			int latestTime = spinTimer->elapsed();
		
			if (moveDist > 3 && moveCount > 0 && (latestTime - latestMoveTime)< 6*(latestMoveTime-olderMoveTime)){
				myGLWindow->startSpin(latestTime/moveCount);
			} else {
				doSpin = false;
			}
		}
		if (!doSpin){
			//terminate current mouse motion
			VizWinMgr::getEventRouter(Params::_viewpointParamsTag)->captureMouseUp();
		}
		//Done with timer:
		if(spinTimer) delete spinTimer;
		spinTimer = 0;
		
		//Force rerender, so correct resolution is shown
		//setRegionNavigating(true);
		myGLWindow->update();
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

	bool doNavigate = true;
	//Respond based on what activity we are tracking
	//Need to tell the appropriate params about the change,
	//And it should refresh the panel
	float mouseCoords[2];
	float projMouseCoords[2];
	mouseCoords[0] = (float) e->x();
	mouseCoords[1] = (float) height()-e->y();
	int mode = GLWindow::getCurrentMouseMode();
	ParamsBase::ParamsBaseType t = GLWindow::getModeParamType(mode);
	if (mode > 0){
		TranslateStretchManip* manip = myGLWindow->getManip(Params::GetTagFromType(t));
		bool constrain = manip->getParams()->isDomainConstrained();
		ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
		int handleNum = manip->draggingHandle();
		//check first to see if we are dragging face
		if (handleNum >= 0){
			if (myGLWindow->projectPointToLine(mouseCoords,projMouseCoords)) {
				float dirVec[3];
				myGLWindow->pixelToVector(projMouseCoords, vParams->getCameraPosLocal(), dirVec);
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
		myTrackball->MouseOnTrackball(1, e->button(), e->x(), e->y(), width(), height());
		//Note that the coords have changed:
		myGLWindow->setViewerCoordsChanged(true);
		setDirtyBit(ProjMatrixBit, true);
	}
	myGLWindow->update();
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
 * Following method is called when the coords have changed in visualizer.  Need to reset the params and
 * update the gui
 */

void VizWin::
changeCoords(float *vpos, float* vdir, float* upvec) {
	float worldPos[3];
	ViewpointParams::localFromStretchedCube(vpos,worldPos);
	myWinMgr->getViewpointRouter()->navigate(myWinMgr->getViewpointParams(myWindowNum),worldPos, vdir, upvec);
	
	myGLWindow->setViewerCoordsChanged(false);
	//If this window is using global VP, must tell all other global windows to update:
	if (globalVP){
		for (int i = 0; i< MAXVIZWINS; i++){
			if (i == myWindowNum) continue;
			VizWin* viz = myWinMgr->getVizWin(i);
			if (viz && viz->globalVP) viz->updateGL();
		}
	}
}
/* 
 * Get viewpoint info from GUI
 * Note:  if the current settings are global, this should be using global params
 */
void VizWin::
setValuesFromGui(ViewpointParams* vpparams){
	if (!vpparams->isLocal()){
		assert(myWinMgr->getViewpointParams(myWindowNum) == vpparams);
	}
	Viewpoint* vp = vpparams->getCurrentViewpoint();
	float transCameraPos[3];
	float cubeCoords[3];
	//Must transform from world coords to unit cube coords for trackball.
	ViewpointParams::localToStretchedCube(vpparams->getCameraPosLocal(), transCameraPos);
	ViewpointParams::localToStretchedCube(vpparams->getRotationCenterLocal(), cubeCoords);
	myTrackball->setFromFrame(transCameraPos, vp->getViewDir(), vp->getUpVec(), cubeCoords, vp->hasPerspective());
	
	//If the perspective was changed, a resize event will be triggered at next redraw:
	
	myGLWindow->setPerspective(vp->hasPerspective());
	//Set dirty bit.
	setDirtyBit(ProjMatrixBit,true);
	//Force a redraw
	myGLWindow->update();
}
/*
 *  Switch Tball when change to local or global viewpoint
 */
void VizWin::
setGlobalViewpoint(bool setGlobal){
	if (!setGlobal){
		myTrackball = localTrackball;
		globalVP = false;
	} else {
		myTrackball = myWinMgr->getGlobalTrackball();
		globalVP = true;
	}
	myGLWindow->myTBall = myTrackball;
	ViewpointParams* vpparms = myWinMgr->getViewpointParams(myWindowNum);
	setValuesFromGui(vpparms);
}


/*
 * Obtain current view frame from gl model matrix
 */
void VizWin::
changeViewerFrame(){
	GLfloat m[16], minv[16];
	GLdouble modelViewMtx[16];
	//Get the frame from GL:
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	//Also, save the modelview matrix for picking purposes:
	glGetDoublev(GL_MODELVIEW_MATRIX, modelViewMtx);
	//save the modelViewMatrix in the viewpoint params (it may be shared!)
	VizWinMgr::getInstance()->getViewpointParams(myWindowNum)->
		setModelViewMatrix(modelViewMtx);

	//Invert it:
	int rc = minvert(m, minv);
	if(!rc) assert(rc);
	vscale(minv+8, -1.f);
	if (!myGLWindow->getPerspective()){
		//Note:  This is a hack.  Putting off the time when we correctly implement
		//Ortho coords to actually send perspective viewer to infinity.
		//get the scale out of the (1st 3 elements of) matrix:
		//
		float scale = vlength(m);
		float trans;
		if (scale < 5.f) trans = 1.-5./scale;
		else trans = scale - 5.f;
		minv[14] = -trans;
	}
	changeCoords(minv+12, minv+8, minv+4);
	
}
//Terminate the current spin, and complete the VizEventRouter command that
//started when the mouse was pressed...
void VizWin::endSpin(){
	if (!myGLWindow) return;
	if (!myGLWindow->stopSpin()) return;
	VizWinMgr::getInstance()->getViewpointRouter()->endSpin();
	myGLWindow->update();
}


    
    
