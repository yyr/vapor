//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
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
#include "vizwin.h"

#include <qvariant.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qaction.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qtoolbar.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qmainwindow.h>

#include <qpushbutton.h>
#include <qslider.h>
#include <qmessagebox.h>
#include <qframe.h>
#include <qapplication.h>
#include <qkeycode.h>

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
#include "probeeventrouter.h"
#include "animationeventrouter.h"
#include "floweventrouter.h"
#include "flowrenderer.h"
#include "VolumeRenderer.h"
#include "mainform.h"
#include "assert.h"
#include "session.h"
#include "vaporinternal/jpegapi.h"

using namespace VAPoR;

/*
 *  Constructs a VizWindow as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
VizWin::VizWin( QWorkspace* parent, const char* name, WFlags fl, VizWinMgr* myMgr, QRect* location, int winNum)
    : QMainWindow( (QWidget*)parent, name, fl )
{
	
    if ( !name )
		setName( "VizWin" );
    myParent = parent;
    myWindowNum = winNum;
	
	
        
    move(location->topLeft());
    resize(location->size());
    //qWarning("Launching window %d", winNum);
    myWinMgr = myMgr;
	mouseDownHere = false;
	
    languageChange();
    clearWState( WState_Polished );
	
	/*  Following copied from QT OpenGL Box example:*/
	// Create a nice frame to put around the OpenGL widget
	
    QFrame* f = new QFrame( this, "frame" );
    f->setFrameStyle( QFrame::Sunken | QFrame::Panel );
    f->setLineWidth( 2 );

    // Create our OpenGL widget.  
	QGLFormat fmt;
	fmt.setAlpha(true);
	fmt.setRgba(true);
	fmt.setDepth(true);
	fmt.setDoubleBuffer(true);
	fmt.setDirectRendering(true);
    myGLWindow = new GLWindow(fmt, f, "glwindow",myWindowNum);
	if (!(fmt.directRendering() && fmt.depth() && fmt.rgba() && fmt.alpha() && fmt.doubleBuffer())){
		Params::BailOut("Unable to obtain required OpenGL rendering format",__FILE__,__LINE__);	
	}
	myGLWindow->setViewpointParams(myWinMgr->getViewpointParams(myWindowNum));
	myGLWindow->setRegionParams(myWinMgr->getRegionParams(myWindowNum));
	myGLWindow->setAnimationParams(myWinMgr->getAnimationParams(myWindowNum));
	myGLWindow->setDvrParams(myWinMgr->getDvrParams(myWindowNum));
	myGLWindow->setFlowParams(myWinMgr->getFlowParams(myWindowNum));
	myGLWindow->setProbeParams(myWinMgr->getProbeParams(myWindowNum));
	myGLWindow->setPreRenderCB(preRenderSetup);
	myGLWindow->setPostRenderCB(endRender);

    QHBoxLayout* flayout = new QHBoxLayout( f, 2, 2, "flayout");
    flayout->addWidget( myGLWindow, 1 );

    QHBoxLayout* hlayout = new QHBoxLayout( this, 2, 2, "hlayout");
   
    
    hlayout->addWidget( f, 1 );

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
	
	
	
	
}


/*
 *  Destroys the object and frees any allocated resources
 */
VizWin::~VizWin()
{
    // no need to delete child widgets, Qt does it all for us
	//Do need to notify parent that we are disappearing
    //qWarning("starting viz win destructor %d", myWindowNum);
    //myWinMgr->vizAboutToDisappear(myWindowNum);
     if (localTrackball) delete localTrackball;
	 //The renderers are deleted in the glwindow destructor:
	 //delete myGLWindow;
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
		
//		if (!value && isActiveWindow()&& (myWinMgr->getActiveViz() != myWindowNum)){
//			myWinMgr->setActiveViz(myWindowNum);
//		}
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
	if (e->button()== Qt::LeftButton) buttonNum = 1;
	else if (e->button() == Qt::RightButton) buttonNum = 2;
	//possibly navigate after other activities
	bool doNavigate = false;
	
	switch (GLWindow::getCurrentMouseMode()){
		//In region mode,first check for clicks on selected region
		case GLWindow::regionMode :
			if (buttonNum > 0){
				
				int faceNum;
				float boxExtents[6];
				ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
				RegionParams* rParams = myWinMgr->getRegionParams(myWindowNum);
				TranslateStretchManip* regionManip = myGLWindow->getRegionManip();
				regionManip->setParams(rParams);
				rParams->calcBoxExtentsInCube(boxExtents);
				int handleNum = regionManip->mouseIsOverHandle(screenCoords, boxExtents, &faceNum);
				if (handleNum >= 0) {
					float dirVec[3];
					//Find the direction vector of the camera (World coords)
					myGLWindow->pixelToVector(e->x(), height()-e->y(), 
						vParams->getCameraPos(), dirVec);
					//Remember which handle we hit, highlight it, save the intersection point.
					regionManip->captureMouseDown(handleNum, faceNum, vParams->getCameraPos(), dirVec, buttonNum);
					RegionEventRouter* rep = VizWinMgr::getInstance()->getRegionRouter();
					rep->captureMouseDown();
					setMouseDown(true);
					break;
				}
			
			}
			//Otherwise, fall through to navigate mode:
			doNavigate = true;
			break;
		//Rakemode is like regionmode
		case GLWindow::rakeMode :
			if (buttonNum > 0){
				
				int faceNum;
				float boxExtents[6];
				ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
				FlowParams* fParams = myWinMgr->getFlowParams(myWindowNum);
				TranslateStretchManip* flowManip = myGLWindow->getFlowManip();
				flowManip->setParams(fParams);
				fParams->calcBoxExtentsInCube(boxExtents);
				int handleNum = flowManip->mouseIsOverHandle(screenCoords, boxExtents, &faceNum);
				if (handleNum >= 0) {
					float dirVec[3];
					//Find the direction vector of the camera (World coords)
					myGLWindow->pixelToVector(e->x(), height()-e->y(), 
						vParams->getCameraPos(), dirVec);
					//Remember which handle we hit, highlight it, save the intersection point.
					flowManip->captureMouseDown(handleNum, faceNum, vParams->getCameraPos(), dirVec, buttonNum);
					VizWinMgr::getInstance()->getFlowRouter()->captureMouseDown();
					setMouseDown(true);
					break;
				}
			
			}
			//Otherwise, fall through to navigate mode:
			doNavigate = true;
			break;

		case GLWindow::probeMode :
			if (buttonNum > 0)
			{
				int faceNum;
				float boxExtents[6];
				ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
				ProbeParams* pParams = myWinMgr->getProbeParams(myWindowNum);
				
				TranslateRotateManip* probeManip = myGLWindow->getProbeManip();
				probeManip->setParams(pParams);
				//pParams->calcBoxExtentsInCube(boxExtents);
				pParams->calcContainingBoxExtentsInCube(boxExtents);
				int handleNum = probeManip->mouseIsOverHandle(screenCoords, boxExtents, &faceNum);
				if (handleNum >= 0) {
					//Stretching doesn't work well if rotation is not a multiple of 90 deg.
					if (buttonNum > 1) {
						//check if the rotation angle is approx a multiple of 90 degrees:
						int tolerance = 20;
						int thet = (int)(fabs(pParams->getTheta())+0.5f);
						int ph = (int)(fabs(pParams->getPhi())+ 0.5f);
						//Make sure that these are within tolerance of a multiple of 90
						if (abs(((thet+45)/90)*90 -thet) > tolerance) break;
						
						if (abs(((ph+45)/90)*90 -ph) > tolerance) break;
						
					}

					//if (buttonNum > 1 &&(pParams->getTheta()!= 0.f || pParams->getPhi() != 0.f)) break;
					float dirVec[3];
					//Find the direction vector of the camera (World coords)
					myGLWindow->pixelToVector(e->x(), height()-e->y(), 
						vParams->getCameraPos(), dirVec);
					//Remember which handle we hit, highlight it, save the intersection point.
					probeManip->captureMouseDown(handleNum, faceNum, vParams->getCameraPos(), dirVec, buttonNum);
					ProbeEventRouter* per = VizWinMgr::getInstance()->getProbeRouter();
					per->captureMouseDown();
					setMouseDown(true);
					break;
				}
			}
			//Otherwise, fall through to navigate mode:
			doNavigate = true;
			break;
		default:
			
		case GLWindow::navigateMode : 
			doNavigate = true;
			break;
			
		
	}
	if (doNavigate){
		ViewpointEventRouter* vep = VizWinMgr::getInstance()->getViewpointRouter();
					vep->captureMouseDown();
		
		myTrackball->MouseOnTrackball(0, e->button(), e->x(), e->y(), width(), height());
		setMouseDown(true);
		mouseDownPosition = e->pos();
		//Force an update of region params, so low res is shown
		setDirtyBit(Params::ViewpointParamsType,NavigatingBit,true);
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
	TranslateStretchManip* myManip;
	switch (GLWindow::getCurrentMouseMode()){
		
		case GLWindow::regionMode :
			myManip = myGLWindow->getRegionManip();
			//Check if the seed bounds were moved
			if (myManip->draggingHandle() >= 0){
				float screenCoords[2];
				screenCoords[0] = (float)e->x();
				screenCoords[1] = (float)(height() - e->y());
				setMouseDown(false);
				//The manip must move the region, and then tells the params to
				//record end of move
				myManip->mouseRelease(screenCoords);
				VizWinMgr::getInstance()->getRegionRouter()->captureMouseUp();
				break;
			} //otherwise fall through to navigate mode
			doNavigate = true;
			break;
		case GLWindow::rakeMode :
			myManip = myGLWindow->getFlowManip();
			//Check if the seed bounds were moved
			if (myManip->draggingHandle() >= 0){
				float screenCoords[2];
				screenCoords[0] = (float)e->x();
				screenCoords[1] = (float)(height() - e->y());
				setMouseDown(false);
				//The manip must move the rake, and then tell the params to
				//record end of move??
				myManip->mouseRelease(screenCoords);
				VizWinMgr::getInstance()->getFlowRouter()->captureMouseUp();
				break;
			} //otherwise fall through to navigate mode
			doNavigate = true;
			break;
		case GLWindow::probeMode :
			myManip = myGLWindow->getProbeManip();
			if (myManip->draggingHandle() >= 0){
				float screenCoords[2];
				screenCoords[0] = (float)e->x();
				screenCoords[1] = (float)(height() - e->y());
				setMouseDown(false);
				//The manip must move the probe, and then tell the params to
				//record end of move.
				myManip->mouseRelease(screenCoords);
				VizWinMgr::getInstance()->getProbeRouter()->captureMouseUp();
				break;
			} //otherwise fall through to navigate mode
			doNavigate = true;
			break;
		default:
		case GLWindow::navigateMode : 
			doNavigate = true;
			break;

		
	}
	if(doNavigate){
		VizWinMgr::getEventRouter(Params::ViewpointParamsType)->captureMouseUp();
		myTrackball->MouseOnTrackball(2, e->button(), e->x(), e->y(), width(), height());
		setMouseDown(false);
		//If it's a right mouse being released, must update near/far distances:
		if (e->button() == Qt::RightButton){
			myWinMgr->resetViews(myWinMgr->getRegionParams(myWindowNum),
				myWinMgr->getViewpointParams(myWindowNum));
		}
		//Force an update of region params, so low res is shown
		setRegionDirty(true);
		myGLWindow->updateGL();
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

	bool doNavigate = false;
	//Respond based on what activity we are tracking
	//Need to tell the appropriate params about the change,
	//And it should refresh the panel
	
	switch (GLWindow::getCurrentMouseMode()){
		case GLWindow::regionMode :
			{
				TranslateStretchManip* myRegionManip = myGLWindow->getRegionManip();
				ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
				int handleNum = myRegionManip->draggingHandle();
				//In probe mode, check first to see if we are dragging face
				if (handleNum >= 0){
					float dirVec[3];
					myGLWindow->pixelToVector(e->x(), height()-e->y(), 
						vParams->getCameraPos(), dirVec);
					
					myRegionManip->slideHandle(handleNum, dirVec);
					myGLWindow->updateGL();
					break;
				}
			}
			//Fall through to navigate if not dragging face
			doNavigate = true;
			break;
		case GLWindow::rakeMode :
			{
				TranslateStretchManip* myFlowManip = myGLWindow->getFlowManip();
				ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
				int handleNum = myFlowManip->draggingHandle();
				//In probe mode, check first to see if we are dragging face
				if (handleNum >= 0){
					float dirVec[3];
					myGLWindow->pixelToVector(e->x(), height()-e->y(), 
						vParams->getCameraPos(), dirVec);
					//Don't Convert dirvec from world to cube coords
					//ViewpointParams::worldToCube(dirVec,dirVec);
					//qWarning("Sliding handle %d, direction %f %f %f", handleNum, dirVec[0],dirVec[1],dirVec[2]);
					myFlowManip->slideHandle(handleNum, dirVec);
					myGLWindow->updateGL();
					break;
				}
			}
			//Fall through to navigate if not dragging face
			doNavigate = true;
			break;
		case GLWindow::probeMode :
			{
				TranslateRotateManip* myProbeManip = myGLWindow->getProbeManip();
				ViewpointParams* vParams = myWinMgr->getViewpointParams(myWindowNum);
				int handleNum = myProbeManip->draggingHandle();
				//In probe mode, check first to see if we are dragging face
				if (handleNum >= 0){
					float dirVec[3];
					myGLWindow->pixelToVector(e->x(), height()-e->y(), 
						vParams->getCameraPos(), dirVec);
					//Don't Convert dirvec from world to cube coords
					//ViewpointParams::worldToCube(dirVec,dirVec);
					//qWarning("Sliding handle %d, direction %f %f %f", handleNum, dirVec[0],dirVec[1],dirVec[2]);
					myProbeManip->slideHandle(handleNum, dirVec);
					myGLWindow->updateGL();
					break;
				}
			}
			//Fall through to navigate if not dragging face
			doNavigate = true;
			break;
		default:
		case GLWindow::navigateMode : 
			doNavigate = true;
			break;
		
	}
	if(doNavigate){
		QPoint deltaPoint = e->globalPos() - mouseDownPosition;
		myTrackball->MouseOnTrackball(1, e->button(), e->x(), e->y(), width(), height());
		//Note that the coords have changed:
		myGLWindow->setViewerCoordsChanged(true);
	}
	myGLWindow->updateGL();
	return;
}



/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void VizWin::languageChange()
{
    setCaption( tr( name() ) );
   
    //helpContentsAction->setText( tr( "Contents" ) );
    //helpContentsAction->setMenuText( tr( "&Contents..." ) );
    //helpContentsAction->setAccel( QString::null );
    //helpIndexAction->setText( tr( "Index" ) );
    //helpIndexAction->setMenuText( tr( "&Index..." ) );
    //helpIndexAction->setAccel( QString::null );
    //helpAboutAction->setText( tr( "About" ) );
    //helpAboutAction->setMenuText( tr( "&About" ) );
    //helpAboutAction->setAccel( QString::null );
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
	QWidget* thisCentralWidget = ((QMainWindow*)MainForm::getInstance())->centralWidget();
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
	ViewpointParams::worldFromCube(vpos,worldPos);
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
	ViewpointParams::worldToCube(vp->getCameraPos(), transCameraPos);
	ViewpointParams::worldToCube(vpparams->getRotationCenter(), cubeCoords);
	myTrackball->setFromFrame(transCameraPos, vp->getViewDir(), vp->getUpVec(), cubeCoords, vp->hasPerspective());
	
	//If the perspective was changed, a resize event will be triggered at next redraw:
	
	myGLWindow->setPerspective(vp->hasPerspective());
	//Set dirty bit.
	//?setViewerCoordsChanged(true);  Probably not right to comment this out!!! AN, 2/21/06
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

//This determines if the specified point is over one of the faces of the regioncube.
//ScreenCoords are as in OpenGL:  bottom of window is 0
//
int VizWin::
pointOverCube(RegionParams* rParams, float screenCoords[2]){
	//First get the cube corners into an array of floats
	
	float corners[24];
	float mincrd[3];
	float maxcrd[3];
	for (int i = 0; i<3; i++){
		mincrd[i] = rParams->getRegionMin(i);
		maxcrd[i] = rParams->getRegionMax(i);
	}
	//Specify corners in counterclockwise order 
	//(appearing from front, i.e. pos z axis) 
	//Back first:
	corners[0+3*0] = mincrd[0];
	corners[1+3*0] = mincrd[1];
	corners[2+3*0] = mincrd[2];
	corners[0+3*1] = maxcrd[0];
	corners[1+3*1] = mincrd[1];
	corners[2+3*1] = mincrd[2];
	corners[0+3*2] = maxcrd[0];
	corners[1+3*2] = maxcrd[1];
	corners[2+3*2] = mincrd[2];
	corners[0+3*3] = mincrd[0];
	corners[1+3*3] = maxcrd[1];
	corners[2+3*3] = mincrd[2];
	//Front (large z)
	corners[0+3*4] = mincrd[0];
	corners[1+3*4] = mincrd[1];
	corners[2+3*4] = maxcrd[2];
	corners[0+3*5] = maxcrd[0];
	corners[1+3*5] = mincrd[1];
	corners[2+3*5] = maxcrd[2];
	corners[0+3*6] = maxcrd[0];
	corners[1+3*6] = maxcrd[1];
	corners[2+3*6] = maxcrd[2];
	corners[0+3*7] = mincrd[0];
	corners[1+3*7] = maxcrd[1];
	corners[2+3*7] = maxcrd[2];
	


	//Then transform them in-place to cube coords.
	for (int j = 0; j<8; j++)
		ViewpointParams::worldToCube(corners+j*3, corners+j*3);
	//Finally, for each face, test the point:
	
	//Back (as viewed from the positive z-axis):
	if (myGLWindow->pointIsOnQuad(corners+0,corners+9,corners+6,corners+3,
		screenCoords)) return 0;
	//Front
	if (myGLWindow->pointIsOnQuad(corners+12,corners+15,corners+18,corners+21,
		screenCoords)) return 1;
	//Bottom
	if (myGLWindow->pointIsOnQuad(corners+0,corners+3,corners+15,corners+12,
		screenCoords)) return 2;
	//Top
	if (myGLWindow->pointIsOnQuad(corners+6,corners+9,corners+21,corners+18,
		screenCoords)) return 3;
	//Left:
	if (myGLWindow->pointIsOnQuad(corners+9,corners+0,corners+12,corners+21,
		screenCoords)) return 4;
	//Right:
	if (myGLWindow->pointIsOnQuad(corners+3,corners+6,corners+18,corners+15,
		screenCoords)) return 5;
	
	return -1;
}
//This determines if the specified point is over one of the faces of the regioncube.
//ScreenCoords are as in OpenGL:  bottom of window is 0.  Same as code above, except
//for doing seed region, not subregion
//
int VizWin::
pointOverCube(FlowParams* fParams, float screenCoords[2]){
	//First get the cube corners into an array of floats
	
	float corners[24];
	float mincrd[3];
	float maxcrd[3];
	for (int i = 0; i<3; i++){
		mincrd[i] = fParams->getSeedRegionMin(i);
		maxcrd[i] = fParams->getSeedRegionMax(i);
	}
	//Specify corners in counterclockwise order 
	//(appearing from front, i.e. pos z axis) 
	//Back first:
	corners[0+3*0] = mincrd[0];
	corners[1+3*0] = mincrd[1];
	corners[2+3*0] = mincrd[2];
	corners[0+3*1] = maxcrd[0];
	corners[1+3*1] = mincrd[1];
	corners[2+3*1] = mincrd[2];
	corners[0+3*2] = maxcrd[0];
	corners[1+3*2] = maxcrd[1];
	corners[2+3*2] = mincrd[2];
	corners[0+3*3] = mincrd[0];
	corners[1+3*3] = maxcrd[1];
	corners[2+3*3] = mincrd[2];
	//Front (large z)
	corners[0+3*4] = mincrd[0];
	corners[1+3*4] = mincrd[1];
	corners[2+3*4] = maxcrd[2];
	corners[0+3*5] = maxcrd[0];
	corners[1+3*5] = mincrd[1];
	corners[2+3*5] = maxcrd[2];
	corners[0+3*6] = maxcrd[0];
	corners[1+3*6] = maxcrd[1];
	corners[2+3*6] = maxcrd[2];
	corners[0+3*7] = mincrd[0];
	corners[1+3*7] = maxcrd[1];
	corners[2+3*7] = maxcrd[2];
	


	//Then transform them in-place to cube coords.
	for (int j = 0; j<8; j++)
		ViewpointParams::worldToCube(corners+j*3, corners+j*3);
	//Finally, for each face, test the point:
	
	//Back (as viewed from the positive z-axis):
	if (myGLWindow->pointIsOnQuad(corners+0,corners+9,corners+6,corners+3,
		screenCoords)) return 0;
	//Front
	if (myGLWindow->pointIsOnQuad(corners+12,corners+15,corners+18,corners+21,
		screenCoords)) return 1;
	//Bottom
	if (myGLWindow->pointIsOnQuad(corners+0,corners+3,corners+15,corners+12,
		screenCoords)) return 2;
	//Top
	if (myGLWindow->pointIsOnQuad(corners+6,corners+9,corners+21,corners+18,
		screenCoords)) return 3;
	//Left:
	if (myGLWindow->pointIsOnQuad(corners+9,corners+0,corners+12,corners+21,
		screenCoords)) return 4;
	//Right:
	if (myGLWindow->pointIsOnQuad(corners+3,corners+6,corners+18,corners+15,
		screenCoords)) return 5;
	
	return -1;
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
	minvert(m, minv);
	vscale(minv+8, -1.f);
	if (!myGLWindow->getPerspective()){
		//Note:  This is a bad hack.  Putting off the time when I correctly implement
		//Ortho coords to actually send perspective viewer to infinity.
		//get the scale out of the (1st 3 elements of ) matrix:
		//
		float scale = vlength(m);
		float trans;
		if (scale < 5.f) trans = 1.-5./scale;
		else trans = scale - 5.f;
		minv[14] = -trans;
	}
	changeCoords(minv+12, minv+8, minv+4);
	
}

/*
void VizWin::
doFrameCapture(){
	if (capturing == 0) return;
	//Create a string consisting of captureName, followed by nnnn (framenum), 
	//followed by .jpg
	QString filename;
	if (capturing == 2){
		filename = captureName;
		filename += (QString("%1").arg(captureNum)).rightJustify(4,'0');
		filename +=  ".jpg";
	} //Otherwise we are just capturing one frame:
	else filename = captureName;
	if (!filename.endsWith(".jpg")) filename += ".jpg";
	//Now open the jpeg file:
	FILE* jpegFile = fopen(filename.ascii(), "wb");
	if (!jpegFile) {
		MessageReporter::errorMsg("Image Capture Error: Error opening output Jpeg file: %s",filename.ascii());
		capturing = 0;
		return;
	}
	//Get the image buffer 
	unsigned char* buf = new unsigned char[3*myGLWindow->width()*myGLWindow->height()];
	//Use openGL to fill the buffer:
	if(!myGLWindow->getPixelData(buf)){
		//Error!
		MessageReporter::errorMsg("%s","Image Capture Error; error obtaining GL data");
		capturing = 0;
		delete buf;
		return;
	}
	//Now call the Jpeg library to compress and write the file
	//
	int quality = Session::getInstance()->getJpegQuality();
	int rc = write_JPEG_file(jpegFile, myGLWindow->width(), myGLWindow->height(), buf, quality);
	if (rc){
		//Error!
		MessageReporter::errorMsg("Image Capture Error; Error writing jpeg file %s",
			filename.ascii());
		capturing = 0;
		delete buf;
		return;
	}
	//If just capturing single frame, turn it off, otherwise advance frame number
	if(capturing > 1) captureNum++;
	else capturing = 0;
	delete buf;
}

float VizWin::getPixelSize(){
	float temp[3];
	//Window height is subtended by viewing angle (45 degrees),
	//at viewer distance (dist from camera to view center)
	ViewpointParams* vpParams = VizWinMgr::getInstance()->getViewpointParams(myWindowNum);
	vsub(vpParams->getRotationCenter(),vpParams->getCameraPos(),temp);
	float distToScene = vlength(temp);
	//tan(45 deg *0.5) is ratio between half-height and dist to scene
	float halfHeight = tan(M_PI*0.125)* distToScene;
	return (2.f*halfHeight/(float)height());

}
*/
    
    
