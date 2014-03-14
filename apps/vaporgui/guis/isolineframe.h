//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isolineframe.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2014
//
//	Description:	Defines the IsolineFrame class.  This provides
//		a frame in which the glisolinewindow sits
//		Principally involved in drawing and responding to mouse events.
//		This class is used as a custom dialog for Qt Designer, so that
//		it can be embedded in the isoline tab
//		It's *not* in the Vapor namespace, so that designer can use it.
//
#ifndef ISOLINEFRAME_H
#define ISOLINEFRAME_H
#include <QFrame>
#include <qwidget.h>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QLabel>
#include "glisolinewindow.h"
class QLabel;
class QWidget;


namespace VAPoR {
	class GLIsolineWindow;
	class IsolineParams;
}
using namespace VAPoR;
class IsolineFrame : public QFrame {
	Q_OBJECT
public:
	IsolineFrame( QWidget * parent = 0, Qt::WFlags f = 0 );
	~IsolineFrame();
	void setGLWindow(VAPoR::GLIsolineWindow* w){glIsolineWindow = w;}
	void setImageSize(float h, float v){
		glIsolineWindow->setImageSize(h,v);
	}
	
	void setParams(IsolineParams* p) {isolineParams = p;}
	IsolineParams* getParams() {return isolineParams;}
	
	GLIsolineWindow* getGLWindow(){ return glIsolineWindow;}

	
	

public slots:
	
	
signals:
	
protected:
	
	//Virtual, Reimplemented here:
	void paintEvent(QPaintEvent* event);

	VAPoR::GLIsolineWindow* glIsolineWindow;
	
	VAPoR::IsolineParams* isolineParams;
	
};


#endif //ISOLINEFRAME_H

