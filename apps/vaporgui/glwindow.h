//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glwindow.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Definition of GLWindow class: 
//	A GLWindow object is embedded in each visualizer window.  It performs basic
//	 navigation and resize, and defers drawing to the viz window's list of 
//	 registered renderers.

#ifndef GLWINDOW_H
#define GLWINDOW_H

#include <qgl.h>
#include "trackball.h"


namespace VAPoR {

class VizWin;

class GLWindow : public QGLWidget
{

public:

    GLWindow( const QGLFormat& fmt, QWidget* parent, const char* name, VizWin*  );
    ~GLWindow();
	Trackball* myTBall;
	//Method that gets the coord frame from GL, 
	//causes an update of the viewpoint params
	//
	void	changeViewerFrame();
	Trackball* getTBall() {return myTBall;}
	void setPerspective(bool isPersp) {perspective = isPersp;}
	bool getPerspective() {return perspective;}
	void setMaxSize(float wsize) {maxDim = wsize;}
	void setCenter(float cntr[3]) { wCenter[0] = cntr[0]; wCenter[1] = cntr[1]; wCenter[2] = cntr[2];}
	

protected:

	//These methods cannot be overridden, but the initialize and paint methods
	//call the corresponding Renderer methods.
	//
    void		initializeGL();
    void		paintGL();
    void		resizeGL( int w, int h );
	
	VizWin* myVizWin;
	float	wCenter[3]; //World center coords
	float	maxDim;		//Max of x, y, z size in world coords
	bool perspective;	//perspective vs parallel coords;
	bool oldPerspective;

};
};

#endif // GLWINDOW_H
