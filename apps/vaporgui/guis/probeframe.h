//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		probeframe.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Defines the ProbeFrame class.  This provides
//		a frame in which the glprobewindow sits
//		Principally involved in drawing and responding to mouse events.
//		This class is used as a custom dialog for Qt Designer, so that
//		it can be embedded in the probe tab
//		It's *not* in the Vapor namespace, so that designer can use it.
//
#ifndef PROBEFRAME_H
#define PROBEFRAME_H
#include <q3frame.h>
#include <qwidget.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QLabel>
#include "glprobewindow.h"
class QLabel;
class QWidget;


namespace VAPoR {
	class GLProbeWindow;
	class ProbeParams;
}
using namespace VAPoR;
class ProbeFrame : public Q3Frame {
	Q_OBJECT
public:
	ProbeFrame( QWidget * parent = 0, const char * name = 0, Qt::WFlags f = 0 );
	~ProbeFrame();
	void setGLWindow(VAPoR::GLProbeWindow* w){glProbeWindow = w;}
	void setTextureSize(float h, float v){
		glProbeWindow->setTextureSize(h,v);
	}
	void setAnimatingTexture(bool val){
		glProbeWindow->setAnimatingTexture(val);
	}
	void advanceAnimatingFrame(){
		glProbeWindow->advanceAnimatingFrame();
	}
	void setParams(ProbeParams* p) {probeParams = p;}
	ProbeParams* getParams() {return probeParams;}
	void setCaptureName(QString& name){glProbeWindow->setCaptureName(name);}
	void setCapturing(bool doCapture){glProbeWindow->setCapturing(doCapture);}
	void setCaptureNum(int num) {glProbeWindow->setCaptureNum(num);}
	GLProbeWindow* getGLWindow(){ return glProbeWindow;}

	
	

public slots:
	
	
signals:
	
protected:
	
	//Virtual, Reimplemented here:
	void paintEvent(QPaintEvent* event);
	void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );
	
	void resizeEvent( QResizeEvent * );
	
	

	VAPoR::GLProbeWindow* glProbeWindow;
	bool needUpdate;
	bool amDragging;
	
	bool mouseIsDown;
	VAPoR::ProbeParams* probeParams;
	
};


#endif

