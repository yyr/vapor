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
    : QGLWidget()
{
	return;
	setAttribute(Qt::WA_DeleteOnClose);
	
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
    
	myVisualizer = ControlExecutive::getInstance()->GetVisualizer(myWindowNum);
	
	myVisualizer->setPreRenderCB(preRenderSetup);
	myVisualizer->setPostRenderCB(endRender);

	
	//Get viewpoint from viewpointParams
	ViewpointParams* vpparms = myWinMgr->getViewpointParams(myWindowNum);
	//Attach the trackball:
//	localTrackball = new Trackball();
	if (vpparms->isLocal()){
//		myTrackball = localTrackball;
		globalVP = false;
	} else {
//		myTrackball = myWinMgr->getGlobalTrackball();
		globalVP = true;
	}
//	myVisualizer->myTBall = myTrackball;
	setValuesFromGui(vpparms);
	
}
/*
 *  Destroys the object and frees any allocated resources
 */
VizWin::~VizWin()
{
	
  //   if (localTrackball) delete localTrackball;
	 //The renderers are deleted in the Visualizer destructor:
//	 if (spinTimer) delete spinTimer;
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
/*
void VizWin::
focusInEvent(QFocusEvent* e){
	//Test for hidden here, since a vanishing window can get this event.
	if (e->gotFocus() && !isHidden()){
		if (myWinMgr->getActiveViz() != myWindowNum ){
			myWinMgr->setActiveViz(myWindowNum);
		}
	}
}
*/
// React to a user-change in window activation:
void VizWin::windowActivationChange(bool ){
	
		
		
		
}
// React to a user-change in window size/position (or possibly max/min)
// Either the window is minimized, maximized, restored, or just resized.
void VizWin::resizeGL(int width, int height){
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width, height);
    gluPerspective(60, (float)width / (float)height, 0.1f, 512.f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //updated = true;
	return;
	ControlExecutive* ce = ControlExecutive::getInstance();
	ce->ResizeViz(myWindowNum, width, height);
}
void VizWin::initializeGL(){
	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    //setProjection(w, h);
	//ControlExecutive* ce = ControlExecutive::getInstance();
	//ce->InitializeViz(myWindowNum, width(),height());
}
void VizWin::hideEvent(QHideEvent* ){

}
/* If the user presses the mouse on the active viz window,
 * We record the position of the click.
 */
void VizWin::
mousePressEvent(QMouseEvent* e){
}
/*
 * If the user releases the mouse or moves it (with the left mouse down)
 * then we note the displacement
 */
void VizWin:: 
mouseReleaseEvent(QMouseEvent*e){
	
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
//	ViewpointParams::localFromStretchedCube(vpos,worldPos);
	myWinMgr->getViewpointRouter()->navigate(myWinMgr->getViewpointParams(myWindowNum),worldPos, vdir, upvec);
	
	//If this window is using global VP, must tell all other global windows to update:
	if (globalVP){
		for (int i = 0; i< myWinMgr->getNumVisualizers(); i++){
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
	
	changeCoords(minv+12, minv+8, minv+4);
	
}
void VizWin::updateGL(){
	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	float* X = new float[3];
    X[0] = 1.f;
    X[1] = 0.f;
    X[2] = 0.f;
    float* x = new float[3];
    x[0] = 0.f;
    x[1] = 1.f;
    x[2] = 1.f;
    float* Y = new float[3];
    Y[0] = 0.f;
    Y[1] = 1.f;
    Y[2] = 0.f;
    float* y = new float[3];
    y[0] = 1.f;
    y[1] = 0.f;
    y[2] = 1.f;
    float* Z = new float[3];
    Z[0] = 0.f;
    Z[1] = 0.f;
    Z[2] = 1.f;
    float* z = new float[3];
    z[0] = 1.f;
    z[1] = 1.f;
    z[2] = 0.f;
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0.f,0.f,-5.f);
	 glBegin(GL_QUADS);		// Draw The Cube Using quads
        glColor3f(Y[0],Y[1],Y[2]);
        glVertex3f( 1.0f, 1.0f,-1.0f);	// Top Right Of The Quad (Top)
        glVertex3f(-1.0f, 1.0f,-1.0f);	// Top Left Of The Quad (Top)
        glVertex3f(-1.0f, 1.0f, 1.0f);	// Bottom Left Of The Quad (Top)
        glVertex3f( 1.0f, 1.0f, 1.0f);	// Bottom Right Of The Quad (Top)
      glColor3f(y[0],y[1],y[2]);
        glVertex3f( 1.0f,-1.0f, 1.0f);	// Top Right Of The Quad (Bottom)
        glVertex3f(-1.0f,-1.0f, 1.0f);	// Top Left Of The Quad (Bottom)
        glVertex3f(-1.0f,-1.0f,-1.0f);	// Bottom Left Of The Quad (Bottom)
        glVertex3f( 1.0f,-1.0f,-1.0f);	// Bottom Right Of The Quad (Bottom)
      glColor3f(Z[0],Z[1],Z[2]);
        glVertex3f( 1.0f, 1.0f, 1.0f);	// Top Right Of The Quad (Front)
        glVertex3f(-1.0f, 1.0f, 1.0f);	// Top Left Of The Quad (Front)
        glVertex3f(-1.0f,-1.0f, 1.0f);	// Bottom Left Of The Quad (Front)
        glVertex3f( 1.0f,-1.0f, 1.0f);	// Bottom Right Of The Quad (Front)
      glColor3f(z[0],z[1],z[2]);
        glVertex3f( 1.0f,-1.0f,-1.0f);	// Top Right Of The Quad (Back)
        glVertex3f(-1.0f,-1.0f,-1.0f);	// Top Left Of The Quad (Back)
        glVertex3f(-1.0f, 1.0f,-1.0f);	// Bottom Left Of The Quad (Back)
        glVertex3f( 1.0f, 1.0f,-1.0f);	// Bottom Right Of The Quad (Back)
      glColor3f(x[0],x[1],x[2]);
        glVertex3f(-1.0f, 1.0f, 1.0f);	// Top Right Of The Quad (Left)
        glVertex3f(-1.0f, 1.0f,-1.0f);	// Top Left Of The Quad (Left)
        glVertex3f(-1.0f,-1.0f,-1.0f);	// Bottom Left Of The Quad (Left)
        glVertex3f(-1.0f,-1.0f, 1.0f);	// Bottom Right Of The Quad (Left)
      glColor3f(X[0],X[1],X[2]);
        glVertex3f( 1.0f, 1.0f,-1.0f);	// Top Right Of The Quad (Right)
        glVertex3f( 1.0f, 1.0f, 1.0f);	// Top Left Of The Quad (Right)
        glVertex3f( 1.0f,-1.0f, 1.0f);	// Bottom Left Of The Quad (Right)
        glVertex3f( 1.0f,-1.0f,-1.0f);	// Bottom Right Of The Quad (Right)
    glEnd();			// End Drawing The Cube
	glPopMatrix();
//	ControlExecutive* ce = ControlExecutive::getInstance();
//	ce->Paint(myWindowNum, true);
}


    
    
