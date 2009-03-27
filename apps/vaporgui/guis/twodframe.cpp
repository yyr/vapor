//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDframe.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2008
//
//	Description:	Implements the TwoDFrame class.  This provides
//		a frame in which the twoD texture is drawn
//		Principally involved in drawing and responding to mouse events.
//
#include "twodframe.h"
#include "twoDparams.h"
#include <qframe.h>
#include <qwidget.h>
#include <qgl.h>
#include <qlayout.h>
#include "gltwoDwindow.h"
#include "glutil.h"
#include "vizwinmgr.h"
#include "twoDdataeventrouter.h"
#include "twoDimageeventrouter.h"


TwoDFrame::TwoDFrame( QWidget * parent, const char * name, WFlags f ) :
	QFrame(parent, name, f) {
	
	setFocusPolicy(QWidget::StrongFocus);
	 // Create our OpenGL widget.  
	QGLFormat fmt;
	fmt.setAlpha(true);
	fmt.setRgba(true);
	fmt.setDoubleBuffer(true);
	fmt.setDirectRendering(true);
    glTwoDWindow = new GLTwoDWindow(fmt, this, "gltwoDwindow", this);
	if (!(fmt.directRendering() && fmt.rgba() && fmt.alpha() && fmt.doubleBuffer())){
		Params::BailOut("Unable to obtain required OpenGL rendering format",__FILE__,__LINE__);	
	}
	QHBoxLayout* flayout = new QHBoxLayout( this, 2, 2, "flayout");
    flayout->addWidget( glTwoDWindow, 1 );
	twoDParams = 0;
	isDataWindow = true;

}
TwoDFrame::~TwoDFrame() {
	
}
	

void TwoDFrame::paintEvent(QPaintEvent* ){
	
	//if (!glTwoDWindow){ //should never happen?
		
	//	return;
	//}
	//Defer to the glTwoDWindow (do a GL draw)
	if (GLWindow::isRendering()) return;
	glTwoDWindow->updateGL();
}

void TwoDFrame::mousePressEvent( QMouseEvent * e){
	float x,y;
	if (!twoDParams) return;
	if (isDataWindow){
		TwoDDataEventRouter* per = VizWinMgr::getInstance()->getTwoDDataRouter();
		glTwoDWindow->mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
		per->guiStartCursorMove();
	}
	else {
		TwoDImageEventRouter* per = VizWinMgr::getInstance()->getTwoDImageRouter();
		glTwoDWindow->mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
		per->guiStartCursorMove();
	}
	twoDParams->setCursorCoords(x,y);
	update();
	
}




void TwoDFrame::mouseReleaseEvent( QMouseEvent *e ){
	if (!glTwoDWindow) return;
	if (!twoDParams) return;
	float x,y;
	if (isDataWindow){
		TwoDDataEventRouter* per = VizWinMgr::getInstance()->getTwoDDataRouter();
		glTwoDWindow->mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
		twoDParams->setCursorCoords(x,y);
		per->guiEndCursorMove();
	}
	else {
		TwoDImageEventRouter* per = VizWinMgr::getInstance()->getTwoDImageRouter();
		glTwoDWindow->mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
		twoDParams->setCursorCoords(x,y);
		per->guiEndCursorMove();
	}
	update();
}
	
//When the mouse moves, display its new coordinates.  Move the "grabbed" 
//control point, or zoom/pan the display
//
void TwoDFrame::mouseMoveEvent( QMouseEvent * e){
	if (!glTwoDWindow) return;
	if (!twoDParams) return;
	float x,y;
	glTwoDWindow->mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
	twoDParams->setCursorCoords(x,y);
	update();
	
}
void TwoDFrame::resizeEvent( QResizeEvent *  ){
	needUpdate = true;
}
