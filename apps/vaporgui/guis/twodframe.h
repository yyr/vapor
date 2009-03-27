//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twodframe.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2008
//
//	Description:	Defines the TwoDFrame class.  This provides
//		a frame in which the gltwoDwindow sits
//		Principally involved in drawing and responding to mouse events.
//		This class is used as a custom dialog for Qt Designer, so that
//		it can be embedded in the twoD tab
//		It's *not* in the Vapor namespace, so that designer can use it.
//
#ifndef TWODFRAME_H
#define TWODFRAME_H
#include <qframe.h>
#include <qwidget.h>
#include "gltwoDwindow.h"
class QLabel;
class QWidget;


namespace VAPoR {
	class GLTwoDWindow;
	class TwoDParams;
}
using namespace VAPoR;
class TwoDFrame : public QFrame {
	Q_OBJECT
public:
	TwoDFrame( QWidget * parent = 0, const char * name = 0, WFlags f = 0 );
	~TwoDFrame();
	void setGLWindow(VAPoR::GLTwoDWindow* w){glTwoDWindow = w;}
	void setTextureSize(float h, float v){
		glTwoDWindow->setTextureSize(h,v);
	}
	
	void setParams(TwoDParams* p, bool isData) {
		twoDParams = p; isDataWindow = isData;
		glTwoDWindow->setWindowType(isData);
	}
	TwoDParams* getParams() {return twoDParams;}
	
	

public slots:
	
	
signals:
	
protected:
	
	//Virtual, Reimplemented here:
	void paintEvent(QPaintEvent* event);
	void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );
	
	void resizeEvent( QResizeEvent * );
	
	

	VAPoR::GLTwoDWindow* glTwoDWindow;
	bool needUpdate;
	bool amDragging;
	
	bool mouseIsDown;
	VAPoR::TwoDParams* twoDParams;
	bool isDataWindow;
	
};


#endif

