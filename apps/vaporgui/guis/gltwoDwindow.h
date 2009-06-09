//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		GLTwoDWindow.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2008
//
//	Description:  Definition of GLTwoDWindow class: 
//	A GLTwoDWindow object is embedded in twoDframe.  
//  It paints a texture in the window.

#ifndef GLTWODWINDOW_H
#define GLTWODWINDOW_H

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <qgl.h>
#include "glwindow.h"
#define CURSOR_COLOR 1.f,1.f,1.f
#define CURSOR_SIZE 0.05f
class TwoDFrame;

namespace VAPoR {



class GLTwoDWindow : public QGLWidget
{

public:

    GLTwoDWindow( const QGLFormat& fmt, QWidget* parent, const char* name, TwoDFrame*  );
    ~GLTwoDWindow();
	
	void setTextureSize(float horiz, float vert);
	void mapPixelToTwoDCoords(int ix, int iy, float* x, float* y);

	void setWindowType(bool isData){isDataWindow = isData;}
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
	
	bool getPixelData(int minx, int miny, int sizex, int sizey,unsigned char* pixData);
	
	//Size of twoD in world coords.
	//This rectange will do its best to fill the twoD space.
	float horizTexSize, vertTexSize;
	float rectLeft, rectTop;
	TwoDFrame* twoDFrame;
	bool isDataWindow;  //true if it's a data window
	bool rendering;

};
};

#endif // GLWINDOW_H
