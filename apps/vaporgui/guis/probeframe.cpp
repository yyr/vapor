//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		probeframe.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Implements the ProbeFrame class.  This provides
//		a frame in which the probe texture is drawn
//		Principally involved in drawing and responding to mouse events.
//
#include "probeframe.h"
#include "probeparams.h"
#include <qframe.h>
#include <qwidget.h>
#include <qgl.h>
#include <qlayout.h>
#include "glprobewindow.h"
#include "glutil.h"
#include "vizwinmgr.h"
#include "probeeventrouter.h"


ProbeFrame::ProbeFrame( QWidget * parent, const char * name, WFlags f ) :
	QFrame(parent, name, f) {
	
	setFocusPolicy(QWidget::StrongFocus);
	 // Create our OpenGL widget.  
	QGLFormat fmt;
	fmt.setAlpha(true);
	fmt.setRgba(true);
	fmt.setDoubleBuffer(true);
	fmt.setDirectRendering(true);
    glProbeWindow = new GLProbeWindow(fmt, this, "glprobewindow", this);
	if (!(fmt.directRendering() && fmt.rgba() && fmt.alpha() && fmt.doubleBuffer())){
		Params::BailOut("Unable to obtain required OpenGL rendering format",__FILE__,__LINE__);	
	}
	QHBoxLayout* flayout = new QHBoxLayout( this, 2, 2, "flayout");
    flayout->addWidget( glProbeWindow, 1 );
	probeParams = 0;

}
ProbeFrame::~ProbeFrame() {
	
}
	

void ProbeFrame::paintEvent(QPaintEvent* ){
	
	//if (!glProbeWindow){ //should never happen?
		
	//	return;
	//}
	//Defer to the glProbeWindow (do a GL draw)
	glProbeWindow->updateGL();
}

void ProbeFrame::mousePressEvent( QMouseEvent * e){
	ProbeEventRouter* per = VizWinMgr::getInstance()->getProbeRouter();
	if (!probeParams) return;
	float x,y;
	glProbeWindow->mapPixelToProbeCoords(e->x(),e->y(), &x, &y);
	per->guiStartCursorMove();
	probeParams->setCursorCoords(x,y);
	update();
	
}




void ProbeFrame::mouseReleaseEvent( QMouseEvent *e ){
	if (!glProbeWindow) return;
	if (!probeParams) return;
	ProbeEventRouter* per = VizWinMgr::getInstance()->getProbeRouter();
	float x,y;
	glProbeWindow->mapPixelToProbeCoords(e->x(),e->y(), &x, &y);
	probeParams->setCursorCoords(x,y);
	per->guiEndCursorMove();
	update();
}
	
//When the mouse moves, display its new coordinates.  Move the "grabbed" 
//control point, or zoom/pan the display
//
void ProbeFrame::mouseMoveEvent( QMouseEvent * e){
	if (!glProbeWindow) return;
	if (!probeParams) return;
	float x,y;
	glProbeWindow->mapPixelToProbeCoords(e->x(),e->y(), &x, &y);
	probeParams->setCursorCoords(x,y);
	update();
	
}
void ProbeFrame::resizeEvent( QResizeEvent *  ){
	needUpdate = true;
}