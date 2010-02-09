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
#include <QFrame>
#include <qwidget.h>
#include <qgl.h>
#include <qlayout.h>
#include <QResizeEvent>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPaintEvent>
#include "gltwoDwindow.h"
#include "glutil.h"
#include "vizwinmgr.h"
#include "twoDdataeventrouter.h"
#include "twoDimageeventrouter.h"


TwoDFrame::TwoDFrame( QWidget * parent, Qt::WFlags f ) :
	QFrame(parent, f) {
	
	setFocusPolicy(Qt::StrongFocus);
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
	QHBoxLayout* flayout = new QHBoxLayout( this);
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
	if(!glTwoDWindow->isRendering())glTwoDWindow->updateGL();
}