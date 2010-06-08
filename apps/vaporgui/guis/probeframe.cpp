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
#include <QFrame>
#include <qwidget.h>
#include <qgl.h>
#include <qlayout.h>

#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QHBoxLayout>
#include "glprobewindow.h"
#include "glutil.h"
#include "vizwinmgr.h"
#include "probeeventrouter.h"


ProbeFrame::ProbeFrame( QWidget * parent, Qt::WFlags f ) :
	QFrame(parent, f) {
	
	setFocusPolicy(Qt::StrongFocus);
	 // Create our OpenGL widget.  
	QGLFormat fmt;
	fmt.setAlpha(true);
	fmt.setRgba(true);
	fmt.setDoubleBuffer(true);
	fmt.setDirectRendering(true);
	//QGLContext* ctx = new QGLContext(fmt);
	//CustomContext* ctx = new CustomContext(fmt);
	//ctx->create();
    glProbeWindow = new GLProbeWindow(fmt, this, "glprobewindow", this);
	if (!(fmt.directRendering() && fmt.rgba() && fmt.alpha() && fmt.doubleBuffer())){
		Params::BailOut("Unable to obtain required OpenGL rendering format",__FILE__,__LINE__);	
	}
	QBoxLayout* flayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    flayout->addWidget( glProbeWindow, 0 );
	probeParams = 0;

}
ProbeFrame::~ProbeFrame() {
	
}
	

void ProbeFrame::paintEvent(QPaintEvent* ){
	
	//if (!glProbeWindow){ //should never happen?
		
	//	return;
	//}
	//Defer to the glProbeWindow (do a GL draw)
	if (GLWindow::isRendering()) return;
	glProbeWindow->updateGL();
}

