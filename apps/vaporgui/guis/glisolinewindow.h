//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		GLIsolineWindow.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Definition of GLIsolineWindow class: 
//	A GLIsolineWindow object is embedded in isolineframe.  
//  It paints a texture in the window.

#ifndef GLISOLINEWINDOW_H
#define GLISOLINEWINDOW_H

#include <GL/glew.h>
#ifdef Darwin
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <qgl.h>

#include <QPaintEvent>
#include "glwindow.h"
#define ISOL_CURSOR_COLOR 1.f,1.f,1.f
#define ISOL_CURSOR_SIZE 0.05f
class IsolineFrame;


namespace VAPoR {

class GLIsolineWindow : public QGLWidget
{

public:

    GLIsolineWindow( QGLFormat& fmt , QWidget* parent, const char* name, IsolineFrame*  );
    ~GLIsolineWindow();
	
	void setImageSize(float horiz, float vert);
	void mapPixelToIsolineCoords(int ix, int iy, float* x, float* y);
	
	bool isRendering() {return rendering;}


protected:

	
	//These methods cannot be overridden
	//
    void		initializeGL();
    void		paintGL();
    void		resizeGL( int w, int h );
	//Virtual, Reimplemented here:
	void paintEvent(QPaintEvent* event){
		if (!GLWindow::isRendering()) QGLWidget::paintEvent(event);
	}
	void performRendering(int timestep, const std::map<pair<int,int>,vector<float*> >&  lineCache);
	void mousePressEvent( QMouseEvent * );
	void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );

	void doFrameCapture();

	//Size of isoline in world coords.
	//This rectange will do its best to fill the isoline space.
	float horizImgSize, vertImgSize;
	float rectLeft, rectTop;
	IsolineFrame* isolineFrame;

	bool rendering;
	
private: 
	int _winWidth;
	int _winHeight;
	void _resizeGL();

};
};

#endif // GLISOLINEWINDOW_H
