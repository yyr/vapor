//************************************************************************
//																		*
//		     Copyright (C)  2014									*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isolineframe.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Implements the IsolineFrame class.  This provides
//		a frame in which the isoline texture is drawn
//		Principally involved in drawing and responding to mouse events.
//
#include "isolineframe.h"
#include "isolineparams.h"
#include <QFrame>
#include <qwidget.h>
#include <qgl.h>
#include <qlayout.h>

#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QHBoxLayout>
#include "glisolinewindow.h"
#include "vizwinmgr.h"
#include "isolineeventrouter.h"


IsolineFrame::IsolineFrame( QWidget * parent, Qt::WFlags f ) :
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
    glIsolineWindow = new GLIsolineWindow(fmt, this, "glisolinewindow", this);
	if (!(fmt.directRendering() && fmt.rgba() && fmt.alpha() && fmt.doubleBuffer())){
		Params::BailOut("Unable to obtain required OpenGL rendering format",__FILE__,__LINE__);	
	}
	QBoxLayout* flayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    flayout->addWidget( glIsolineWindow, 0 );
	isolineParams = 0;

}
IsolineFrame::~IsolineFrame() {
	
}
	

void IsolineFrame::paintEvent(QPaintEvent* ){
	
	glIsolineWindow->updateGL();
}

