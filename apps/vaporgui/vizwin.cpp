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
#include "glutil.h"
#include "glbox.h"
#include "viewpoint.h"
#ifdef VOLUMIZER
#include "volumizerrenderer.h"
#endif
#include "assert.h"

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
	numRenderers = 0;
            
        
    move(location->topLeft());
    resize(location->size());
    //qWarning("Launching window %d", winNum);
    myWinMgr = myMgr;
	mouseDownHere = false;
	newViewerCoords = false;
	regionDirty = true;
	
	
    // actions
   
    //helpContentsAction = new QAction( this, "helpContentsAction" );
    //helpIndexAction = new QAction( this, "helpIndexAction" );
    //helpAboutAction = new QAction( this, "helpAboutAction" );

    

    languageChange();
    
    clearWState( WState_Polished );
	

    // signals and slots connections
   
    //connect( helpIndexAction, SIGNAL( activated() ), this, SLOT( helpIndex() ) );
    //connect( helpContentsAction, SIGNAL( activated() ), this, SLOT( helpContents() ) );
    //connect( helpAboutAction, SIGNAL( activated() ), this, SLOT( helpAbout() ) );
	//connect (homeAction, SIGNAL(activated()), this, SLOT(goHome()));
	//connect (sethomeAction, SIGNAL(activated()), this, SLOT(setHome()));
   
	
	
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
    myGLWindow = new GLWindow(fmt, f, "glbox", this);
	if (!(fmt.directRendering() && fmt.depth() && fmt.rgba() && fmt.alpha() && fmt.doubleBuffer())){
		qWarning("unable to obtain required rendering format");
	}


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
	setValuesFromGui(vpparms->getCurrentViewpoint());

	show();
	
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
}


void VizWin::closeEvent(QCloseEvent* e){
	//Tell the winmgr that we are closing:
    myWinMgr->vizAboutToDisappear(myWindowNum);
	QWidget::closeEvent(e);
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
			myWinMgr->comboChanged(1, myWindowNum);
		}
		else if (isMinimized()) {
			myWinMgr->minimize(myWindowNum);
			myWinMgr->comboChanged(0, myWindowNum);
		}	
		else {
			//went to normal
			myWinMgr->normalize(myWindowNum);
			myWinMgr->comboChanged(2, myWindowNum);
		}
		
}
// React to a user-change in window size/position (or possibly max/min)
// Either the window is minimized, maximized, restored, or just resized.
void VizWin::resizeEvent(QResizeEvent*){
		//qWarning(" Resize Event received in window %d ", myWindowNum);
		if (isReallyMaximized()) {
			//qWarning( " resize due to maximization");
			myWinMgr->maximize(myWindowNum);
			myWinMgr->comboChanged(1, myWindowNum);
		}
		else if (isMinimized()) {
			myWinMgr->minimize(myWindowNum);
			myWinMgr->comboChanged(0, myWindowNum);
		}	
		else {
			//User either resized or restored from min/max
			myWinMgr->normalize(myWindowNum);
			myWinMgr->comboChanged(2, myWindowNum);
		}
}

void VizWin::hideEvent(QHideEvent* ){
	myWinMgr->minimize(myWindowNum);
	myWinMgr->comboChanged(0, myWindowNum);
	
}
/* If the user presses the mouse on the active viz window,
 * We record the position of the click.
 */
void VizWin::
mousePressEvent(QMouseEvent* e){
	
	switch (myWinMgr->selectionMode){
		case Command::navigateMode : 
			myWinMgr->getViewpointParams(myWindowNum)->captureMouseDown();
			myTrackball->MouseOnTrackball(0, e->button(), e->x(), e->y(), width(), height());
			mouseDownHere = true;
			mouseDownPosition = e->pos();
			//Force an update of region params, so low res is shown
			setRegionDirty(true);
			break;
		case Command::regionMode :
			
			break;
		default:
			break;
	}
	
}
/*
 * If the user releases the mouse or moves it (with the left mouse down)
 * then we note the displacement
 */
void VizWin:: 
mouseReleaseEvent(QMouseEvent*e){
	switch (myWinMgr->selectionMode){
		case Command::navigateMode : 
			myWinMgr->getViewpointParams(myWindowNum)->captureMouseUp();
			myTrackball->MouseOnTrackball(2, e->button(), e->x(), e->y(), width(), height());
			mouseDownHere = false;
			
			//Force an update of region params, so low res is shown
			setRegionDirty(true);
			myGLWindow->updateGL();
			break;

		case Command::regionMode :
			break;
		default:
			break;
	}
	
}	
/* 
 * When the mouse is moved, it can affect navigation, contour planes,
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
	if (!mouseDownHere) return;

	
	//Respond based on what activity we are tracking
	//Need to tell the appropriate params about the change,
	//And it should refresh the panel
	QPoint deltaPoint = e->globalPos() - mouseDownPosition;
	switch (myWinMgr->selectionMode){
		case Command::navigateMode : 
			myTrackball->MouseOnTrackball(1, e->button(), e->x(), e->y(), width(), height());
			//Note that the coords have changed:
			newViewerCoords = true;
			break;
		case Command::regionMode :
			myWinMgr->getRegionParams(myWindowNum)->slide(deltaPoint);
			break;
		default:
			break;
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
    qWarning( "VizWin::helpIndex(): Not implemented yet" );
}

void VizWin::helpContents()
{
    qWarning( "VizWin::helpContents(): Not implemented yet" );
}

void VizWin::helpAbout()
{
    qWarning( "VizWin::helpAbout(): Not implemented yet" );
}

//Due to X11 probs need to check again.  Compare this window with the available space.
bool VizWin::isReallyMaximized() {
    if (isMaximized() ) return true;
	QWidget* thisCentralWidget = ((QMainWindow*)myWinMgr->getMainWindow())->centralWidget();
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
	myWinMgr->getViewpointParams(myWindowNum)->navigate(vpos, vdir, upvec);
	newViewerCoords = false;
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
 */
void VizWin::
setValuesFromGui(Viewpoint* vp){
	
	myTrackball->setFromFrame(vp->getCameraPos(), vp->getViewDir(), vp->getUpVec(), vp->hasPerspective());
	//If the perspective was changed, a resize event will be triggered at next redraw:
	
	myGLWindow->setPerspective(vp->hasPerspective());
	
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
	setValuesFromGui(vpparms->getCurrentViewpoint());
}
/*
 * Add a renderer to this visualization window
 * This happens when the renderer is enabled, or if
 * the local/global switch causes a new one to be enabled
 */
void VizWin::
addRenderer(Renderer* ren)
{
	if (numRenderers < MAXNUMRENDERERS){
		//Don't allow more than one renderer in a window (yet)
		assert(numRenderers == 0);
		renderer[numRenderers++] = ren;
		ren->initializeGL();
		myGLWindow->update();
	}
}
/* 
 * Remove renderer of specified classname (only one can exist).
 */
void VizWin::removeRenderer(const char* rendererName){
	int i;
	for (i = 0; i<numRenderers; i++) {		
		if (strcmp(renderer[i]->className(),rendererName)) continue;
		delete renderer[i];
		break;
	}
	//Note that we won't always find one to remove!
	if (i == numRenderers) return;
	numRenderers--;
	for (int j = i; j<numRenderers; j++){
		renderer[j] = renderer[j+1];
	}
	myGLWindow->updateGL();
	
}


    
    

